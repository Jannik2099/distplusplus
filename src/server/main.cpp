#include "common/common.hpp"
#include "common/compression_helper.hpp"
#include "config.hpp"
#include "distplusplus.pb.h"
#include "server.hpp"

#include <boost/log/trivial.hpp>
#include <chrono>
#include <climits>
#include <csignal>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <memory>
#include <span>
#include <thread>

using namespace distplusplus::server;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static volatile std::sig_atomic_t signalFlag = 0;

static void signalHandler(int signal) {
    signalFlag = 1;
    std::signal(signal, signalHandler);
}

static void signalReaper(const std::stop_token &token, grpc::Server *server) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    while (signalFlag == 0) {
        if (token.stop_requested()) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    server->Shutdown();
}

int main(int argc, char *argv[]) {
    const Config config = getConfig(std::span(argv, argc));

    const distplusplus::common::CompressorFactory compressorFactory(config.compressionType,
                                                                    config.compressionLevel);
    Server service(config.maxJobs, config.reservationTimeout, compressorFactory);
    grpc::ServerBuilder builder;
    builder.SetMaxReceiveMessageSize(INT_MAX);
    builder.SetMaxSendMessageSize(INT_MAX);
    builder.SetDefaultCompressionAlgorithm(grpc_compression_algorithm::GRPC_COMPRESS_NONE);
    builder.AddListeningPort(config.listenAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::jthread signalReaperThread(&signalReaper, server.get());

    server->Wait();
    signalReaperThread.request_stop();
    signalReaperThread.join();
    return 0;
}
