add_library(joshua_hal_bridge STATIC
    ResultSerializer.cc
)

target_include_directories(joshua_hal_bridge PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/../Joshua-HAL/src
)

target_link_libraries(joshua_hal_bridge PRIVATE
    joshua_protos
    spdlog::spdlog
)
