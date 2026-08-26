#include "ota/OtaClient.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>

namespace joshua::client {

namespace {

// swupdate is launched with an argument vector so no shell is ever involved.
constexpr const char* kSwupdateBin = "/usr/bin/swupdate";

// Allowed source prefixes. HTTPS only for remote; local images must be an
// absolute path under one of these approved roots.
constexpr const char* kHttpsScheme = "https://";
constexpr const char* kApprovedLocalRoot = "/var/tmp/ota/";

bool is_ascii_safe(const std::string& s) {
    return std::none_of(s.begin(), s.end(), [](char c) {
        return static_cast<unsigned char>(c) < 0x20 || c == '\x7f';
    });
}

}  // namespace

OtaClient::OtaClient(OtaStatusCb cb) : cb_(std::move(cb)) {}

OtaClient::~OtaClient() {
    if (monitor_.joinable()) {
        monitor_.join();
    }
}

bool OtaClient::running() const { return running_.load(); }

bool OtaClient::valid_source(const std::string& source, std::string& arg,
                             std::string& err) {
    if (source.empty()) {
        err = "empty upgrade source";
        return false;
    }
    if (!is_ascii_safe(source)) {
        err = "upgrade source contains control characters";
        return false;
    }
    if (source.rfind(kHttpsScheme, 0) == 0) {
        arg = source;
        return true;
    }
    // Local file: must be an absolute path under the approved staging root and
    // must actually exist.
    if (source[0] != '/' ||
        source.rfind(kApprovedLocalRoot, 0) != 0) {
        err = "local upgrade path must be under " + std::string(kApprovedLocalRoot);
        return false;
    }
    if (access(source.c_str(), R_OK) != 0) {
        err = "local upgrade file not readable: " + source;
        return false;
    }
    arg = source;
    return true;
}

pid_t OtaClient::launch_swupdate(const char* const argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        spdlog::error("ota: fork failed: {}", std::strerror(errno));
        return 0;
    }
    if (pid == 0) {
        // Child: exec with the validated argument vector. No shell is
        // involved, so the source value can never alter shell parsing.
        execv(argv[0], const_cast<char* const*>(argv));
        _exit(127);  // exec failed
    }
    return pid;
}

bool OtaClient::trigger(const std::string& image_source) {
    std::string arg, err;
    if (!valid_source(image_source, arg, err)) {
        spdlog::error("ota: rejected source: {}", err);
        if (cb_) cb_("error", "rejected upgrade source: " + err);
        return false;
    }

    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (running_.load()) {
            spdlog::warn("ota: already running, ignoring trigger");
            return false;
        }
        running_.store(true);
    }

    spdlog::info("ota: triggering upgrade from {}", arg);

    const std::string url_flag = "-u";
    const char* argv[] = {kSwupdateBin, url_flag.c_str(), arg.c_str(), nullptr};
    pid_t child = launch_swupdate(argv);
    if (child == 0) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            running_.store(false);
        }
        if (cb_) cb_("error", "swupdate launch failed");
        return false;
    }

    if (cb_) cb_("started", "swupdate triggered for " + image_source);

    // Monitor the child until exit, then report the outcome. The worker owns
    // the lifecycle; `trigger` returns immediately so callers learn the real
    // result through the callback rather than a premature "started".
    auto self = this;
    monitor_ = std::thread([self, child]() {
        int status = 0;
        auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::hours(24);
        // Wait for the child, but bound the wait so a wedged swupdate cannot
        // hold the running flag forever.
        while (true) {
            pid_t r = waitpid(child, &status, WNOHANG);
            if (r == child) break;
            if (r < 0 && errno == EINTR) continue;
            if (r < 0) {
                spdlog::error("ota: waitpid failed: {}", std::strerror(errno));
                self->on_finished("error", "failed to track swupdate process");
                return;
            }
            if (std::chrono::steady_clock::now() > deadline) {
                ::kill(child, SIGTERM);
                self->on_finished("timeout",
                                  "update timed out and was terminated");
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            self->on_finished("complete", "swupdate finished successfully");
        } else {
            self->on_finished("error", "swupdate exited with failure");
        }
    });
    return true;
}

void OtaClient::on_finished(const std::string& status,
                             const std::string& message) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        running_.store(false);
    }
    spdlog::info("ota: {}", message);
    if (cb_) cb_(status, message);
}

bool OtaClient::rollback() {
    if (running_.load()) {
        spdlog::warn("ota: cannot rollback while an update is running");
        if (cb_) cb_("error", "cannot rollback during an active update");
        return false;
    }
    spdlog::info("ota: requesting rollback");
    const char* argv[] = {"/usr/sbin/fw_setenv", "upgrade_available", "0",
                          nullptr};
    pid_t pid = launch_swupdate(argv);
    if (pid == 0) {
        if (cb_) cb_("error", "rollback launch failed");
        return false;
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        spdlog::error("ota: rollback waitpid failed: {}", std::strerror(errno));
        if (cb_) cb_("error", "rollback failed");
        return false;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        if (cb_) cb_("error", "rollback failed");
        return false;
    }
    // Reboot is a separate explicit step; calling reboot from here would
    // detach the process tree. Signal the caller to reboot rather than shell
    // out to a destructive command.
    if (cb_) cb_("rollback", "rollback initiated; reboot required to apply");
    return true;
}

} // namespace joshua::client
