#pragma once

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

#include "common/common.hpp"
#include "distplusplus.grpc.pb.h"
#include "distplusplus.pb.h"

namespace distplusplus::server {

namespace _internal {

using reservationTypeA = std::string;
using reservationTypeB = std::chrono::time_point<std::chrono::system_clock>;
using reservationType = std::pair<reservationTypeA, reservationTypeB>;

class ReservationCompare final {
public:
	bool operator()(const reservationType &a, const reservationType &b) const;
	bool operator()(const reservationTypeB &a, const reservationType &b) const;
};

} // namespace _internal

class Server final : public distplusplus::CompilationServer::Service {
private:
	void reservationReaper();

	const std::uint64_t jobsMax = 100;
	std::atomic<std::uint64_t> jobsRunning = 0;
	std::atomic<bool> reservationReaperKillswitch = false;
	std::multiset<_internal::reservationType, _internal::ReservationCompare> reservations;
	std::mutex reservationLock;
	std::thread reservationReaperThread;

public:
	grpc::Status Reserve(grpc::ServerContext *context, const distplusplus::Reservation *reservation,
						 distplusplus::ReservationAnswer *answer) final;
	grpc::Status Distribute(grpc::ServerContext *context, const distplusplus::CompileRequest *request,
							distplusplus::CompileAnswer *answer) final;
	Server();
	~Server() final;
	Server(const Server &) = delete;
	Server(Server &&) = delete;
	Server operator=(const Server &) = delete;
	Server operator=(Server &&) = delete;
};

} // namespace distplusplus::server
