#include "common/common.hpp"
#include "common/compression_helper.hpp"
#include "distplusplus.pb.h"
#include "server.hpp"

#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <climits>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <memory>
#include <string>
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
    const std::uint64_t coreCount = std::thread::hardware_concurrency();
    if (coreCount == 0) {
        BOOST_LOG_TRIVIAL(fatal) << "failed to detect number of cores";
        exit(1);
    }
    boost::program_options::options_description desc;
    // clang-format off
    desc.add_options()
        ("compression",
         boost::program_options::value<std::string>()->default_value(DISTPLUSPLUS_DEFAULT_COMPRESSION_STR),
         "compression algorithm to use, must be one of [NONE, zstd]")
        ("compression-level",
         boost::program_options::value<std::int64_t>()->default_value(1),
         "compression level, specific values depend on used algorithm. Accepts positive and negative values")
        ("help",
         "show this help")
        ("jobs",
         boost::program_options::value<std::uint64_t>()->default_value(coreCount),
         "number of maximum jobs being processed")
        ("listen-address",
         boost::program_options::value<std::string>()->default_value("127.0.0.1:3633"),
         "listen adddress")
        ("log-level",
         boost::program_options::value<std::string>(),
         "log level")
    ;
    // clang-format on
    boost::program_options::variables_map varMap;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), varMap);
    boost::program_options::notify(varMap);

    if (varMap.count("help") != 0) {
        std::cout << desc << std::endl;
        exit(0);
    }

    // set up log level before processing other options
    if (varMap.count("log-level") != 0) {
        const std::string logLevel = varMap["log-level"].as<std::string>();
        distplusplus::common::initBoostLogging(logLevel);
    } else {
        distplusplus::common::initBoostLogging();
    }

    const distplusplus::CompressionType compressionType = [&varMap] {
        const std::string userCompressionType = varMap["compression"].as<std::string>();
        if (userCompressionType == "NONE") {
            return distplusplus::CompressionType::NONE;
        }
        if (userCompressionType == "zstd") {
            return distplusplus::CompressionType::zstd;
        }
        BOOST_LOG_TRIVIAL(error) << "provided compression algorithm " << userCompressionType
                                 << " not in [NONE, zstd]";
        exit(1);
    }();

    const std::int64_t compressionLevel = varMap["compression-level"].as<std::int64_t>();

    std::string listenAddr = varMap["listen-address"].as<std::string>();
    char *listenAddrEnv = getenv("DISTPLUSPLUS_LISTEN_ADDRESS");
    if (listenAddrEnv != nullptr) {
        listenAddr = listenAddrEnv;
    }
    BOOST_LOG_TRIVIAL(info) << "listening on " << listenAddr;

    const std::uint64_t maxJobs = varMap["jobs"].as<std::uint64_t>();
    if (maxJobs == 0) {
        BOOST_LOG_TRIVIAL(error) << "--jobs must be greater than 0";
        exit(1);
    }

    const distplusplus::common::CompressorFactory compressorFactory(compressionType, compressionLevel);
    Server service(maxJobs, compressorFactory);
    grpc::ServerBuilder builder;
    builder.SetMaxReceiveMessageSize(INT_MAX);
    builder.SetMaxSendMessageSize(INT_MAX);
    builder.SetDefaultCompressionAlgorithm(grpc_compression_algorithm::GRPC_COMPRESS_NONE);
    builder.AddListeningPort(listenAddr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::jthread signalReaperThread(&signalReaper, server.get());

    server->Wait();
    signalReaperThread.request_stop();
    signalReaperThread.join();
    return 0;
}
