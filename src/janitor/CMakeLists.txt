add_library(joshua_janitor STATIC Janitor.cc
) target_include_directories(joshua_janitor PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(joshua_janitor PRIVATE joshua_protos spdlog::spdlog
)
