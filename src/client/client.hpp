#pragma once

#include "common/argsvec.hpp"
#include "config.hpp"
#include "distplusplus.grpc.pb.h"
#include "distplusplus.pb.h"

#include <filesystem>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <lmdb++.h>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace distplusplus::client {

struct Server {
    uint32_t currentLoad;
    std::string server;
};

} // namespace distplusplus::client

template <> struct ::std::less<distplusplus::client::Server> {
    constexpr bool operator()(const distplusplus::client::Server &lhs,
                              const distplusplus::client::Server &rhs) const;
};

namespace distplusplus::client {

class Client final {
public:
    Client() = delete;
    Client(const Client &client) = delete;
    Client &operator=(const Client &client) = delete;
    Client(Client &&client) noexcept;
    Client &operator=(Client &&client) noexcept;
    ~Client() = default;

private:
    explicit Client(std::string compilerName);

    class QueryHelper;
    friend QueryHelper;

    using ArgsSpan = distplusplus::common::ArgsSpan;

    // std::unique_ptr<CompilationServer::Stub> stub;
    std::string uuid;
    std::string compilerName;
    std::multiset<Server> servers;

    grpc::Status reserve(CompilationServer::Stub &stub);

    class QueryHelper final {
    private:
        Client *client;
        std::filesystem::path dbPath = config.stateDir().string() + "/queryDB/";
        lmdb::env env = lmdb::env::create();

        void updateServer(lmdb::txn &wtxn, lmdb::dbi &dbi, const std::string &server,
                          const distplusplus::QueryAnswer &queryAnswer);
        static void removeServer(lmdb::txn &wtxn, lmdb::dbi &dbi, const std::string &server);

    public:
        explicit QueryHelper(Client *client);
        QueryHelper() = delete;
        QueryHelper(const QueryHelper &queryHelper) = delete;
        QueryHelper(QueryHelper &&queryHelper) = default;
        QueryHelper &operator=(const QueryHelper &queryHelper) = delete;
        QueryHelper &operator=(QueryHelper &&queryHelper) = default;
        ~QueryHelper() = default;

        void init();
        std::string getCandidate();

        friend Client::Client(Client &&) noexcept;
        friend Client &Client::operator=(Client &&) noexcept;
    };
    QueryHelper queryHelper;

    friend Client ClientFactory(const std::string &compilerName);

public:
    CompileAnswer send(ArgsSpan args, const std::string &fileName, std::string_view fileContent,
                       const std::string &cwd);
};

Client ClientFactory(const std::string &compilerName);

} // namespace distplusplus::client
