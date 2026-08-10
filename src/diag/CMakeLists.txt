add_library(joshua_diag STATIC
    DiagSocket.cc
)

target_include_directories(joshua_diag PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(joshua_diag PRIVATE
    spdlog::spdlog
)
