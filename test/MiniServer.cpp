#include "common/argsvec.hpp"
#include "common/common.hpp"
#include "distplusplus.grpc.pb.h"
#include "distplusplus.pb.h"

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
        const auto iter = std::find(uuidList.begin(), uuidList.end(), uuid);
        if (iter != uuidList.end()) {
            return {grpc::StatusCode::FAILED_PRECONDITION, "uuid " + uuid + " not in reservation list"};
        }
        uuidList.erase(iter);

        const std::string &compiler = request->compiler();
        if (std::filesystem::path(compiler).stem() != compiler) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "compiler " + compiler + " is not a basename"};
        }
        const boost::filesystem::path compilerPath = boost::process::search_path(request->compiler());

        distplusplus::common::Tempfile inputFile(request->inputfile().name(), request->inputfile().content());
        distplusplus::common::Tempfile outputFile(
            std::string(std::filesystem::path(request->inputfile().name()).stem()) + std::string(".o"));

        ArgsVec argsVec(request->argument().begin(), request->argument().end());
        argsVec.emplace_back("-o");
        argsVec.emplace_back(outputFile.c_str());
        argsVec.emplace_back(inputFile.c_str());

        distplusplus::common::ProcessHelper compilerProcess(compilerPath, argsVec);
        answer->set_returncode(compilerProcess.returnCode());
        answer->set_stdout(compilerProcess.get_stdout());
        answer->set_stderr(compilerProcess.get_stderr());
        answer->mutable_outputfile()->set_name(outputFile);
        std::ifstream fileStream(outputFile);
        std::stringstream fileContent;
        fileContent << fileStream.rdbuf();
        answer->mutable_outputfile()->set_content(fileContent.str());
        return grpc::Status::OK;
    }
};
} // namespace

static volatile std::sig_atomic_t signalFlag =
    0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

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
