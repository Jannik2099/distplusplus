#include "client.hpp"

#include "common/common.hpp"
#include "config.hpp"
#include "fallback.hpp"

#include <grpcpp/client_context.h>
#include <memory>
#include <string>

#ifndef NDEBUG
#include <unordered_set>
#endif
#include <boost/log/trivial.hpp>
#include <chrono>
#include <climits>
#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/support/status_code_enum.h>
#include <lmdb++.h>
#include <thread>
#include <utility>

#define XSTRINGIFY(s) STRINGIFY(s) // NOLINT
#define STRINGIFY(s) #s            // NOLINT

// TODO: split QueryHelper stuff into seperate impl file?

using namespace distplusplus;

constexpr bool ::std::less<distplusplus::client::Server>::operator()(
    const distplusplus::client::Server &lhs, const distplusplus::client::Server &rhs) const {
    return lhs.currentLoad < rhs.currentLoad;
}

namespace distplusplus::client {

Client::QueryHelper::QueryHelper(Client *client)
    : client(client), dbPath(config.stateDir().string() + "/queryDB/"), env(lmdb::env::create()) {}

// NOLINTNEXTLINE
void Client::QueryHelper::init() {
    env.set_mapsize(1024UL * 1024UL);
    /* databases:
        misc
        servers
    entries in servers look like:
        $host_name : $host
        $host_age : timestamp of last query
        $host_compressionTypeSupported : compressionTypeSupported
        $host_supportedCompilers : compiler1/compiler2/...
        $host_currentLoad : currentLoad
        $host_maxJobs : maxJobs

        where $host is the server IP, socket path or what else as provided by the user.
    */
    env.set_max_dbs(2);

    std::filesystem::create_directories(dbPath);
    env.open(dbPath.c_str(), 0, 0664);
    {
        lmdb::txn wtxn = lmdb::txn::begin(env, nullptr);
        lmdb::dbi dbi1 = lmdb::dbi::open(wtxn, "misc", MDB_CREATE);
        lmdb::dbi dbi2 = lmdb::dbi::open(wtxn, "servers", MDB_CREATE);
        wtxn.commit();
    }

    const distplusplus::CompressionType currentCompressionType = config.compressionType();
    std::string_view previousCompressionTypeString;
    {
        lmdb::txn rtxn = lmdb::txn::begin(env, nullptr, O_RDONLY);
        lmdb::dbi dbi = lmdb::dbi::open(rtxn, "misc");
        dbi.get(rtxn, "compressionType", previousCompressionTypeString);
        rtxn.commit();
    }
    const bool compressionChanged = previousCompressionTypeString != std::to_string(currentCompressionType);
    if (compressionChanged) {
        BOOST_LOG_TRIVIAL(debug) << "desired compression type changed to "
                                 << common::compressionType_to_string(currentCompressionType);
    }
    lmdb::txn parent_wtxn = compressionChanged ? lmdb::txn::begin(env, nullptr) : nullptr;

    // if compression changed, we put all servers into existingServers
    // otherwise, expired servers are tracked seperately and added to availableServers if reachable
    std::vector<Server> existingServers;
    std::vector<Server> expiredServers;
    std::vector<Server> availableServers;
    {
        std::string_view key;
        std::string_view value;
        lmdb::txn rtxn = lmdb::txn::begin(env, parent_wtxn, O_RDONLY);
        lmdb::dbi dbi = lmdb::dbi::open(rtxn, "servers");
        lmdb::cursor cursor = lmdb::cursor::open(rtxn, dbi);
        if (cursor.get(key, value, MDB_FIRST)) {
            do {
                if (key.ends_with("_name")) {
                    std::string_view currentLoadView;
                    dbi.get(rtxn, std::string(value) + "_currentLoad", currentLoadView);
                    const auto currentLoad = lmdb::from_sv<uint32_t>(currentLoadView);
                    Server server = {currentLoad, std::string(value)};

                    BOOST_LOG_TRIVIAL(debug) << "registering server from db: " << server.server;
                    // if compression changed, we have to renew all servers anyways
                    if (compressionChanged) {
                        existingServers.emplace_back(server);
                    } else {
                        std::string_view age_view;
                        dbi.get(rtxn, server.server + "_age", age_view);
                        const std::chrono::system_clock::duration age{
                            std::chrono::seconds{lmdb::from_sv<uint64_t>(age_view)}};
                        const auto now = std::chrono::system_clock::now().time_since_epoch();
                        if (now - age > std::chrono::seconds{10}) {
                            expiredServers.emplace_back(server);
                        } else {
                            availableServers.emplace_back(server);
                        }
                    }
                }
            } while (cursor.get(key, value, MDB_NEXT));
        }
        cursor.close();
        rtxn.commit();
    }

    if (compressionChanged) {
        lmdb::dbi dbi = lmdb::dbi::open(parent_wtxn, "misc");
        dbi.put(parent_wtxn, "compressionType", std::to_string(currentCompressionType));
    }

    std::vector<Server> &serverVector = compressionChanged ? existingServers : expiredServers;
    // TODO: not happy with how I handle config.servers()
    serverVector.reserve(serverVector.size() + config.servers().size());
    for (const std::string &server : config.servers()) {
        // TODO: I really don't like this duplicate detection either, perhaps just use std::set everywhere?
        bool exists = false;
        for (const Server &server2 : serverVector) {
            if (server == server2.server) {
                exists = true;
                break;
            }
        }
        for (const Server &server2 : availableServers) {
            if (server == server2.server) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            BOOST_LOG_TRIVIAL(debug) << "registering server from config: " << server;
            serverVector.push_back({0, server});
        } else {
            BOOST_LOG_TRIVIAL(debug) << "server found in config, but already registered from db: " << server;
        }
    }
    if (!serverVector.empty()) {
        lmdb::txn wtxn = lmdb::txn::begin(env, parent_wtxn);
        lmdb::dbi dbi = lmdb::dbi::open(wtxn, "servers");
        availableServers.reserve(availableServers.size() + serverVector.size());
        for (Server &server : serverVector) {
            BOOST_LOG_TRIVIAL(debug) << "updating server " << server.server;
            grpc::ClientContext context;
            grpc::ChannelArguments channelArgs;
            channelArgs.SetMaxReceiveMessageSize(INT_MAX);
            // TODO: grpc docs say "CreateCustomChannel: for testing only" ?
            std::unique_ptr<CompilationServer::Stub> stub = CompilationServer::NewStub(
                grpc::CreateCustomChannel(server.server, grpc::InsecureChannelCredentials(), channelArgs));
            distplusplus::ServerQuery query;
            query.set_compiler(client->compilerName);
            query.set_desiredcompressiontype(config.compressionType());
            distplusplus::QueryAnswer queryAnswer;
            const grpc::Status status = stub->Query(&context, query, &queryAnswer);
            if (status.ok()) {
                server.currentLoad = queryAnswer.currentload();
                updateServer(wtxn, dbi, server.server, queryAnswer);
                availableServers.push_back(server);
                continue;
            }
            if (status.error_code() == grpc::StatusCode::UNAVAILABLE) {
                BOOST_LOG_TRIVIAL(info)
                    << "server " << server.server << " is no longer reachable, purging from db";
            } else {
                BOOST_LOG_TRIVIAL(warning)
                    << "server " << server.server << " replied with grpc error "
                    << common::mapGRPCStatus(status.error_code()) << " to query, purging from db";
            }
            removeServer(wtxn, dbi, server.server);
        }
        wtxn.commit();
    }

    for (const Server &server : availableServers) {
        client->servers.insert(server);
    }

#ifndef NDEBUG
    std::unordered_set<std::string> set;
    set.reserve(availableServers.size());
    for (const Server &server : availableServers) {
        set.insert(server.server);
    }
    if (set.size() < availableServers.size()) {
        std::string err = "availableServers contained non-unique elements: " + [&availableServers]() {
            std::string ret;
            for (const Server &server : availableServers) {
                ret += '\n' + server.server;
            }
            return ret;
        }();
        throw std::runtime_error(err);
    }
#endif

    if (parent_wtxn != nullptr) {
        parent_wtxn.commit();
    }
}

void Client::QueryHelper::updateServer(lmdb::txn &wtxn, lmdb::dbi &dbi, const std::string &server,
                                       const distplusplus::QueryAnswer &queryAnswer) {
    const uint64_t mtime =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    dbi.put(wtxn, server + "_name", server);
    dbi.put(wtxn, server + "_age", lmdb::to_sv<uint64_t>(mtime));
    dbi.put(wtxn, server + "_compressionTypeSupported",
            lmdb::to_sv<bool>(queryAnswer.compressiontypesupported()));
    dbi.put(wtxn, server + "_currentLoad", lmdb::to_sv<uint32_t>(std::min(queryAnswer.currentload(), 100U)));
    dbi.put(wtxn, server + "_maxJobs", lmdb::to_sv<uint64_t>(queryAnswer.maxjobs()));

    std::string_view supportedCompilers;
    std::string newCompilers;
    if (dbi.get(wtxn, server + "_supportedCompilers", supportedCompilers)) {
        std::istringstream istr;
        istr.str(supportedCompilers.data());

        for (std::string compiler; std::getline(istr, compiler, '/');) {
            if (compiler == client->compilerName) {
                if (queryAnswer.compilersupported()) {
                    newCompilers += compiler + '/';
                }
            } else {
                newCompilers += compiler + '/';
            }
        }
        // get will fail if the key does not exist
    } else {
        if (queryAnswer.compilersupported()) {
            newCompilers = client->compilerName + '/';
        }
    }
    dbi.put(wtxn, server + "_supportedCompilers", newCompilers);

    BOOST_LOG_TRIVIAL(trace) << "writing server to db:" << [&]() {
        std::string ret;
        ret += '\n' + server + "_name: " += server;
        ret += '\n' + server + "_age: " += std::to_string(mtime);
        ret += '\n' + server + "_compressionTypeSupported: " +=
            std::to_string(queryAnswer.compressiontypesupported());
        ret += '\n' + server + "_currentLoad: " += std::to_string(std::min(queryAnswer.currentload(), 100U));
        ret += '\n' + server + "_maxJobs: " += std::to_string(queryAnswer.maxjobs());
        return ret;
    }();
}

void Client::QueryHelper::removeServer(lmdb::txn &wtxn, lmdb::dbi &dbi, const std::string &server) {
    dbi.del(wtxn, server + "_name");
    dbi.del(wtxn, server + "_age");
    dbi.del(wtxn, server + "_compressionTypeSupported");
    dbi.del(wtxn, server + "_supportedCompilers");
    dbi.del(wtxn, server + "_currentLoad");
    dbi.del(wtxn, server + "_maxJobs");
}

Client::Client(std::string compilerName) : compilerName(std::move(compilerName)), queryHelper(this) {}

Client::Client(Client &&client) noexcept : queryHelper(std::move(client.queryHelper)) {
    queryHelper.client = this;
}

Client &Client::operator=(Client &&client) noexcept {
    queryHelper = std::move(client.queryHelper);
    queryHelper.client = this;
    return *this;
}

grpc::Status Client::reserve(CompilationServer::Stub &stub) {
    grpc::ClientContext context;
    Reservation reservation;
    ReservationAnswer reservationAnswer;
    grpc::Status ret = stub.Reserve(&context, reservation, &reservationAnswer);
    if (reservationAnswer.success()) {
        if (!reservationAnswer.has_uuid()) {
            BOOST_LOG_TRIVIAL(warning)
                << "host " << context.peer() << " responded to reservation but with no UUID";
            throw FallbackSignal();
        }
        uuid = reservationAnswer.uuid();
    }
    return ret;
}

// NOLINTNEXTLINE
CompileAnswer Client::send(ArgsSpan args, const std::string &fileName, std::string_view fileContent,
                           const std::string &cwd) {

    std::unique_ptr<CompilationServer::Stub> stub;

    // TODO: am I happy with this?
    bool foundServer = false;
    bool allDown = true;
    const std::chrono::system_clock::time_point timeBeforeReservation = std::chrono::system_clock::now();
    while (true) {
        for (const Server &server : servers) {
            grpc::ChannelArguments channelArgs;
            channelArgs.SetMaxReceiveMessageSize(INT_MAX);
            // TODO: grpc docs say "CreateCustomChannel: for testing only" ?
            stub = CompilationServer::NewStub(
                grpc::CreateCustomChannel(server.server, grpc::InsecureChannelCredentials(), channelArgs));
            const grpc::Status status = reserve(*stub);
            const grpc::StatusCode statusCode = status.error_code();
            if (statusCode == grpc::StatusCode::OK) {
                BOOST_LOG_TRIVIAL(debug) << "got reservation uuid " << uuid << " from host " << server.server;
                allDown = false;
                foundServer = true;
                break;
            }
            if (statusCode == grpc::StatusCode::RESOURCE_EXHAUSTED) {
                allDown = false;
                BOOST_LOG_TRIVIAL(debug) << "host " << server.server << " is full, currently processing "
                                         << status.error_details() << " jobs";
                continue;
            }
            if (statusCode == grpc::StatusCode::UNAVAILABLE) {
                // TODO: we should remove it from the db
                BOOST_LOG_TRIVIAL(info)
                    << "host " << server.server << " is unreachable, attempting next host";
                continue;
            }
            // TODO: we should remove it from the db
            BOOST_LOG_TRIVIAL(warning)
                << "host " << server.server << " replied with grpc error "
                << common::mapGRPCStatus(statusCode) << " to reservation, attempting next host";
        }
        // TODO: custom exception types?
        if (allDown) {
            const std::string err_msg("all specified hosts were unreachable");
            BOOST_LOG_TRIVIAL(error) << err_msg;
            throw std::runtime_error(err_msg);
        }
        if (foundServer) {
            break;
        }
        // TODO: perhaps add a delay between retries?
        if (std::chrono::system_clock::now() - timeBeforeReservation >=
            std::chrono::seconds(config.reservationAttemptTimeout())) {
            const std::string err_msg("could not find a vacant host after " +
                                      std::to_string(config.reservationAttemptTimeout()) +
                                      " seconds, aborting");
            BOOST_LOG_TRIVIAL(error) << err_msg;
            throw std::runtime_error(err_msg);
        }
    }

    CompileRequest compileRequest;
    compileRequest.set_uuid(uuid);
    compileRequest.set_compiler(compilerName);
    for (const auto &arg : args) {
        compileRequest.add_argument(arg);
    }

    compileRequest.mutable_inputfile()->set_name(fileName);
    compileRequest.mutable_inputfile()->set_content(fileContent.data(), fileContent.size());
    compileRequest.mutable_inputfile()->set_compressiontype(config.compressionType());
    compileRequest.set_cwd(cwd);
    CompileAnswer compileAnswer;

    grpc::ClientContext context;

    const grpc::Status status = stub->Distribute(&context, compileRequest, &compileAnswer);
    if (status.error_code() == grpc::StatusCode::OK) {
        return compileAnswer;
    }
    // TODO: prolly should have better formatting
    BOOST_LOG_TRIVIAL(error) << "distributing to " << context.peer() << " failed with gRPC error "
                             << common::mapGRPCStatus(status.error_code()) << " " << status.error_message()
                             << " " << status.error_details();
    throw FallbackSignal();
}

Client ClientFactory(const std::string &compilerName) {
    Client client(compilerName);
    client.queryHelper.init();
    return client;
}

} // namespace distplusplus::client
