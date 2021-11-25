#include "common/common.hpp"
#include "server.hpp"

#include <boost/log/trivial.hpp>
#include <chrono>
#include <climits>
#include <csignal>
#include <cstdlib>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <memory>
#include <string>
#include <thread>

using namespace distplusplus::server;

static volatile std::sig_atomic_t signalFlag = 0; // NOLINT cppcoreguidelines-avoid-non-const-global-variables

static void signalHandler(int signal) {
	signalFlag = 1;
	std::signal(signal, signalHandler);
}

static void signalReaper(const std::stop_token &token, grpc::Server *server) {
	while (signalFlag == 0) {
		if (token.stop_requested()) {
			return;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	server->Shutdown();
}

int main() {
	distplusplus::common::initBoostLogging();
	char *listenAddr = getenv("DISTPLUSPLUS_LISTEN_ADDRESS");
	if (listenAddr == nullptr) {
		BOOST_LOG_TRIVIAL(error) << "no listen address set - see DISTPLUSPLUS_LISTEN_ADDRESS";
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

	std::jthread signalReaperThread(&signalReaper, server.get());
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);

	server->Wait();
	signalReaperThread.request_stop();
	signalReaperThread.join();
	return 0;
}
