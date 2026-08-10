add_library(joshua_ota STATIC OtaClient.cc
) target_include_directories(joshua_ota PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(joshua_ota PRIVATE joshua_protos ${JOSHUA_GRPC_LIBRARIES} spdlog::spdlog
)
