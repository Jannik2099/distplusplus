add_custom_target(gen-protos
    COMMAND
        ${protoc} --grpc_out ${CMAKE_CURRENT_BINARY_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} --plugin=protoc-gen-grpc=${grpc_cpp_plugin} ${CMAKE_CURRENT_SOURCE_DIR}/distplusplus.proto
    COMMAND
        ${protoc} --cpp_out ${CMAKE_CURRENT_BINARY_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/distplusplus.proto
    DEPENDS
        protoc_ready
    BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.grpc.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.grpc.pb.h
)
add_library(protos-protobuf OBJECT ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.pb.cc)
add_library(distplusplus::protobuf ALIAS protos-protobuf)
target_link_libraries(protos-protobuf PkgConfig::protobuf)
obj_propagate_targets(protos-protobuf)

add_library(protos-grpc OBJECT ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.grpc.pb.cc)
add_library(distplusplus::grpc ALIAS protos-grpc)
target_link_libraries(protos-grpc PkgConfig::grpc++)
obj_propagate_targets(protos-grpc)
