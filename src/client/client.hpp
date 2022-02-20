#pragma once

#include "common/argsvec.hpp"
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
#include <string_view>
#include <vector>

using namespace distplusplus;

namespace distplusplus::client {

class Client final {
private:
    using ArgsVecSpan = distplusplus::common::ArgsVecSpan;

    std::unique_ptr<CompilationServer::Stub> stub;
    std::string uuid;
    grpc::Status reserve();
    QueryAnswer query(const ServerQuery &serverQuery);

public:
    CompileAnswer send(const std::string &compilerName, ArgsVecSpan args, const std::string &fileName,
                       std::string_view fileContent, const std::string &cwd);
    Client() = default;
};

} // namespace distplusplus::client
