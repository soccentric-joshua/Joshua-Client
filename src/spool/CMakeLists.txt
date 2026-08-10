add_library(joshua_spool STATIC Spool.cc
) target_include_directories(joshua_spool PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(joshua_spool PRIVATE joshua_protos SQLite::SQLite3 spdlog::spdlog
)
