add_library(joshua_stream STATIC StreamClient.cc
) target_include_directories(joshua_stream PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(joshua_stream PRIVATE joshua_protos ${JOSHUA_GRPC_LIBRARIES} spdlog::spdlog
)
