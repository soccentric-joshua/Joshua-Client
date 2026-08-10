add_library(joshua_hal_service STATIC HalServiceImpl.cc
) target_include_directories(joshua_hal_service PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/../Joshua-HAL/src
) target_link_libraries(joshua_hal_service PRIVATE joshua_registry joshua_hal_bridge joshua_protos ${JOSHUA_GRPC_LIBRARIES} protobuf::libprotobuf spdlog::spdlog
)
