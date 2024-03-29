add_custom_target(gen-protos
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.grpc.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.grpc.pb.h
)
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.grpc.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.grpc.pb.h
    COMMAND
        ${protoc} --plugin=protoc-gen-grpc=${grpc_cpp_plugin} -I${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${CMAKE_CURRENT_BINARY_DIR} --grpc_out=${CMAKE_CURRENT_BINARY_DIR} distplusplus.proto
    DEPENDS
        protoc_ready
    MAIN_DEPENDENCY
        distplusplus.proto
)
add_library(protos-protobuf OBJECT ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.pb.cc)
add_library(distplusplus::protobuf ALIAS protos-protobuf)
target_link_libraries(protos-protobuf PkgConfig::protobuf)
obj_propagate_targets(protos-protobuf)

add_library(protos-grpc OBJECT ${CMAKE_CURRENT_BINARY_DIR}/distplusplus.grpc.pb.cc)
add_library(distplusplus::grpc ALIAS protos-grpc)
target_link_libraries(protos-grpc PkgConfig::grpc++)
obj_propagate_targets(protos-grpc)
