set(PROTODIR "${CMAKE_CURRENT_SOURCE_DIR}")
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

message(STATUS "regenerating protobuf files")

execute_process(COMMAND rm -f distplusplus.grpc.pb.cc distplusplus.grpc.pb.h distplusplus.pb.cc distplusplus.pb.h)

execute_process(
	COMMAND protoc
	--grpc_out ${PROTODIR} --proto_path "${PROTODIR}" --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${PROTODIR}/distplusplus.proto)
execute_process(
	COMMAND protoc
	--cpp_out ${PROTODIR} --proto_path "${PROTODIR}" ${PROTODIR}/distplusplus.proto)
add_library(protobuf OBJECT "${PROTODIR}/distplusplus.pb.cc")
add_library(distplusplus::protobuf ALIAS protobuf)
target_link_libraries(protobuf PkgConfig::protobuf)
obj_propagate_targets(protobuf)

add_library(grpc OBJECT "${PROTODIR}/distplusplus.grpc.pb.cc")
add_library(distplusplus::grpc ALIAS grpc)
target_link_libraries(grpc PkgConfig::grpc)
obj_propagate_targets(grpc)
