add_library(joshua_log STATIC LogCollector.cc
) target_include_directories(joshua_log PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(joshua_log PRIVATE joshua_protos spdlog::spdlog
)
