#pragma once
// ota/OtaClient.h — triggers swupdate-based OTA upgrades on receipt of
// an OTA push from the server.
//
// The server sends the OTA image URL via a REST call to the target's
// swupdate socket.  This module wraps that invocation.
//
// Security model (spec: no shell interpretation, validated source, one
// active update tracked until process exit):
//   * The upgrade source must be an HTTPS URL or an approved local file path
//     (validated by an allowlist) — never an unvalidated arbitrary string.
//   * swupdate is launched with an argument vector (execvp), never through a
//     shell, so a crafted value cannot alter shell parsing.
//   * One active update is tracked by PID until the process exits; completion,
//     timeout, and cancellation are surfaced through the status callback.

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace joshua::client {

using OtaStatusCb = std::function<void(const std::string& status,
                                       const std::string& message)>;

class OtaClient {
public:
    explicit OtaClient(OtaStatusCb cb);

    // Trigger an swupdate from an HTTPS URL or an approved local file path.
    // Non-blocking: validates the source, spawns swupdate, and tracks the
    // child on a worker thread. Returns false if an update is already running
    // or the source is rejected.
    bool trigger(const std::string& image_source);

    // Request a rollback to the inactive slot.
    bool rollback();

    // True while an update child process is still running.
    bool running() const;

    ~OtaClient();

private:
    // Validate that `source` is an allowed upgrade source. Returns an
    // allowlisted absolute path + label if accepted, or a message otherwise.
    static bool valid_source(const std::string& source, std::string& arg,
                             std::string& err);

    // Launch an update child process; returns its pid or 0 on failure.
    pid_t launch_swupdate(const char* const argv[]);

    // Report completion from the monitor thread: clears the running flag and
    // invokes the callback.
    void on_finished(const std::string& status, const std::string& message);

    OtaStatusCb cb_;
    std::mutex mtx_;
    std::atomic<bool> running_{false};
    std::thread monitor_;
};

} // namespace joshua::client
