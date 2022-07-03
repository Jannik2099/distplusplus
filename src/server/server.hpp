#pragma once

#include "common/compression_helper.hpp"
#include "distplusplus.grpc.pb.h"
#include "distplusplus.pb.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <grpcpp/channel.h>
#include <grpcpp/server.h>
#include <grpcpp/server_context.h>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace distplusplus::server {

namespace {

using reservationTypeA = std::string;
using reservationTypeB = std::chrono::time_point<std::chrono::system_clock>;
using reservationType = std::pair<reservationTypeA, reservationTypeB>;

class ReservationCompare final {
public:
    // NOLINTNEXTLINE(readability-identifier-length)
    bool operator()(const reservationType &a, const reservationType &b) const;
    // NOLINTNEXTLINE(readability-identifier-length)
    bool operator()(const reservationTypeB &a, const reservationType &b) const;
};

} // namespace

class Server final : public distplusplus::CompilationServer::Service {
private:
    void reservationReaper();
    const distplusplus::common::CompressorFactory compressorFactory;
    const std::uint64_t jobsMax;
    const std::uint64_t reservationTimeout;
    std::atomic<std::uint64_t> jobsRunning = 0;
    std::multiset<reservationType, ReservationCompare> reservations;
    std::mutex reservationLock;
    std::jthread reservationReaperThread = std::jthread(&Server::reservationReaper, this);

public:
    grpc::Status Query(grpc::ServerContext *context, const distplusplus::ServerQuery *query,
                       distplusplus::QueryAnswer *answer) final;
    grpc::Status Reserve(grpc::ServerContext *context, const distplusplus::Reservation *reservation,
                         distplusplus::ReservationAnswer *answer) final;
    grpc::Status Distribute(grpc::ServerContext *context, const distplusplus::CompileRequest *request,
                            distplusplus::CompileAnswer *answer) final;
    Server() = delete;
    Server(std::uint64_t maxJobs, std::uint64_t reservationTimeout,
           distplusplus::common::CompressorFactory compressorFactory);
    ~Server() final = default;
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server operator=(const Server &) = delete;
    Server operator=(Server &&) = delete;
};

} // namespace distplusplus::server
