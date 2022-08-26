#include "common/argsvec.hpp"
#include "common/common.hpp"
#include "common/compression_helper.hpp"
#include "common/process_helper.hpp"
#include "common/tempfile.hpp"
#include "distplusplus.grpc.pb.h"
#include "distplusplus.pb.h"

#include <algorithm>
#include <boost/process.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <climits>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using distplusplus::common::ArgsVec;

namespace {
class MiniServer final : public distplusplus::CompilationServer::Service {
private:
    std::list<std::string> uuidList;

public:
    grpc::Status Query(grpc::ServerContext * /*context*/, const distplusplus::ServerQuery *query,
                       distplusplus::QueryAnswer *answer) final {
        const std::string &compiler = query->compiler();
        if (std::filesystem::path(compiler).stem() != compiler) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "compiler " + compiler + " is not a basename"};
        }
        // TODO: do the rest
        answer->set_currentload(50);
        answer->set_maxjobs(100);
        return grpc::Status::OK;
    }

    grpc::Status Reserve(grpc::ServerContext * /*context*/, const distplusplus::Reservation * /*reservation*/,
                         distplusplus::ReservationAnswer *answer) final {
        const std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
        uuidList.push_back(uuid);
        answer->set_uuid(uuid);
        answer->set_success(true);
        return grpc::Status::OK;
    }
    grpc::Status Distribute(grpc::ServerContext * /*context*/, const distplusplus::CompileRequest *request,
                            distplusplus::CompileAnswer *answer) final {
        const std::string &uuid = request->uuid();
        const auto iter = std::ranges::find(uuidList, uuid);
        if (iter != uuidList.end()) {
            return {grpc::StatusCode::FAILED_PRECONDITION, "uuid " + uuid + " not in reservation list"};
        }
        uuidList.erase(iter);

        const std::string &compiler = request->compiler();
        if (std::filesystem::path(compiler).stem() != compiler) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "compiler " + compiler + " is not a basename"};
        }
        const boost::filesystem::path compilerPath = boost::process::search_path(request->compiler());

        const distplusplus::common::Decompressor decompressor(request->inputfile().compressiontype(),
                                                              request->inputfile().content());
        distplusplus::common::Tempfile inputFile(request->inputfile().name(), "", decompressor.data());
        distplusplus::common::Tempfile outputFile(
            std::string(std::filesystem::path(request->inputfile().name()).stem()) + std::string(".o"));

        ArgsVec argsVec(request->argument().begin(), request->argument().end());
        argsVec.emplace_back("-o");
        argsVec.emplace_back(outputFile.c_str());
        argsVec.emplace_back(inputFile.c_str());

        const char *compressionEnv = getenv("DISTPLUSPLUS_COMPRESS");
        distplusplus::CompressionType compressionType = DISTPLUSPLUS_DEFAULT_COMPRESSION_FULL;
        if (compressionEnv != nullptr) {
            const std::string compressionString = compressionEnv;
            if (compressionString == "NONE") {
                compressionType = distplusplus::CompressionType::NONE;
            } else if (compressionString == "zstd") {
                compressionType = distplusplus::CompressionType::zstd;
            } else {
                std::cerr << "unrecognized compression type " << compressionString << std::endl;
                exit(1);
            }
        }

        distplusplus::common::ProcessHelper compilerProcess(compilerPath, argsVec);
        answer->set_returncode(compilerProcess.returnCode());
        answer->set_stdout(compilerProcess.get_stdout());
        answer->set_stderr(compilerProcess.get_stderr());
        answer->mutable_outputfile()->set_name(outputFile);
        std::ifstream fileStream(outputFile);
        std::stringstream fileContent;
        fileContent << fileStream.rdbuf();
        const distplusplus::common::Compressor compressor(compressionType, 1, fileContent.str());
        answer->mutable_outputfile()->set_compressiontype(compressor.compressionType());
        answer->mutable_outputfile()->set_content(compressor.data().data(), compressor.data().size());
        return grpc::Status::OK;
    }
};
} // namespace

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

int main() {
    char *listenAddr = getenv("DISTPLUSPLUS_LISTEN_ADDRESS");
    if (listenAddr == nullptr) {
        std::cout << "no DISTPLUSPLUS_LISTEN_ADDRESS specified" << std::endl;
        return 1;
    }

    MiniServer service;
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
