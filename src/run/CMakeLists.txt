add_library(joshua_run STATIC
    RunOrchestrator.cc
)

target_include_directories(joshua_run PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/../Joshua-HAL/src
)

target_link_libraries(joshua_run PRIVATE
    joshua_hal_bridge
    joshua_protos
    spdlog::spdlog
)
