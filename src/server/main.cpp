#include <boost/log/trivial.hpp>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <limits.h>
#include <memory>
#include <stdlib.h>
#include <string>

#include "common/common.hpp"
#include "server.hpp"

using namespace distplusplus::server;

int main() {
	distplusplus::common::initBoostLogging();
	char *listenAddr = getenv("DISTPLUSPLUS_LISTEN_ADDRESS");
	if (listenAddr == nullptr) {
		BOOST_LOG_TRIVIAL(fatal) << "no listen address set - see DISTPLUSPLUS_LISTEN_ADDRESS";
		exit(1);
	}
	BOOST_LOG_TRIVIAL(info) << "listening on " << listenAddr;
	Server service;
	grpc::ServerBuilder builder;
	builder.SetMaxReceiveMessageSize(INT_MAX);
	builder.SetMaxSendMessageSize(INT_MAX);
	builder.SetDefaultCompressionAlgorithm(grpc_compression_algorithm::GRPC_COMPRESS_NONE);
	builder.AddListeningPort(listenAddr, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	server->Wait();
	return 0;
}
