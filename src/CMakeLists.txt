# ---------------------------------------------------------------------------
# src/CMakeLists.txt
# Adds each module as a static library then defines the main executable.
# Subdirectories are ordered leaf-first so inter-target references resolve.
# ---------------------------------------------------------------------------

add_subdirectory(config)
add_subdirectory(diag)
add_subdirectory(spool)
add_subdirectory(stream)
add_subdirectory(hal_bridge)
add_subdirectory(registry)
add_subdirectory(ota)
add_subdirectory(log)
add_subdirectory(janitor)
add_subdirectory(run)
add_subdirectory(hal_service)

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------

add_executable(joshua-client
    main.cpp
    app.cpp
)

target_include_directories(joshua-client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../Joshua-HAL/src
)

target_link_libraries(joshua-client PRIVATE
    joshua_config
    joshua_diag
    joshua_hal_bridge
    joshua_hal_service
    joshua_janitor
    joshua_log
    joshua_ota
    joshua_registry
    joshua_run
    joshua_spool
    joshua_stream
    joshua_protos
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    yaml-cpp::yaml-cpp
    SQLite::SQLite3
    ${JOSHUA_GRPC_LIBRARIES}
    protobuf::libprotobuf
)

target_compile_definitions(joshua-client PRIVATE
    JOSHUA_CLIENT_VERSION="${PROJECT_VERSION}"
)

set_compiler_warnings(joshua-client)
