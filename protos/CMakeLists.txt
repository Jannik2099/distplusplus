set(PROTODIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_target(gen-protos
	COMMAND
		${protoc} --grpc_out ${PROTODIR} --proto_path "${PROTODIR}" --plugin=protoc-gen-grpc=${grpc_cpp_plugin} ${PROTODIR}/distplusplus.proto
	COMMAND
		${protoc} --cpp_out ${PROTODIR} --proto_path "${PROTODIR}" ${PROTODIR}/distplusplus.proto
	DEPENDS
		protoc_ready
	BYPRODUCTS
		${PROTODIR}/distplusplus.pb.cc
		${PROTODIR}/distplusplus.pb.h
		${PROTODIR}/distplusplus.grpc.pb.cc
		${PROTODIR}/distplusplus.grpc.pb.h
)
add_library(protos-protobuf OBJECT "${PROTODIR}/distplusplus.pb.cc")
add_library(distplusplus::protobuf ALIAS protos-protobuf)
target_link_libraries(protos-protobuf PkgConfig::protobuf)
obj_propagate_targets(protos-protobuf)

add_library(protos-grpc OBJECT "${PROTODIR}/distplusplus.grpc.pb.cc")
add_library(distplusplus::grpc ALIAS protos-grpc)
target_link_libraries(protos-grpc PkgConfig::grpc++)
obj_propagate_targets(protos-grpc)
