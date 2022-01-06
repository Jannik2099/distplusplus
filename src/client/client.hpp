#pragma once

#include "distplusplus.grpc.pb.h"
#include "distplusplus.pb.h"

#include <filesystem>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace distplusplus;

namespace distplusplus::client {

class Client final {
private:
    std::unique_ptr<CompilationServer::Stub> stub;
    std::string uuid;
    grpc::Status reserve();
    QueryAnswer query(const ServerQuery &serverQuery);

public:
    CompileAnswer send(const std::string &compilerName, const std::vector<std::string> &args,
                       const std::string &fileName, const std::string &fileContent, const std::string &cwd);
    Client() = default;
};

} // namespace distplusplus::client
