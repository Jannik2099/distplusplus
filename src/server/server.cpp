#include "server.hpp"
#include "parser.hpp"

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <functional>
#include <google/protobuf/util/json_util.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string_view>

namespace distplusplus::server {

namespace _internal {

bool ReservationCompare::operator()(const reservationType &a, const reservationType &b) const { return std::get<1>(a) < std::get<1>(b); }

} // namespace _internal

// TODO: finish

static bool sanitizeFile(const distplusplus::File &file) {
	const std::string &filename = file.name();
	return std::filesystem::path(filename).filename() == filename;
}

static bool sanitizeRequest(const distplusplus::CompileRequest &request) {

	const std::string &compiler = request.compiler();
	if (!(std::filesystem::path(compiler).filename() == compiler)) {
		return false;
	}
	return sanitizeFile(request.inputfile());
}

// TODO: this is NOT secure yet and purely for convenience
static bool checkCompilerAllowed(const std::string &compiler) {
	// TODO: check for dir priveleges
	std::function<bool(const std::filesystem::directory_entry &)> checkIfSymlink = [&compiler](const auto &file) {
		return file.is_symlink() && compiler == file.path().filename();
	};

	return std::ranges::any_of(std::filesystem::directory_iterator("/usr/lib/distcc"), checkIfSymlink);
	// lazy copy
	return std::ranges::any_of(std::filesystem::directory_iterator("/usr/libexec/distplusplus"), checkIfSymlink);
}

void Server::reservationReaper() {
	while (!reservationReaperKillswitch) {
		const _internal::reservationTypeB timestamp = std::chrono::system_clock::now();
		// TODO: make configurable
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::lock_guard lockGuard(reservationLock);
		const auto before = reservations.size();
		const auto bound = reservations.lower_bound(std::pair("", timestamp));
		reservations.erase(reservations.begin(), bound);
		const auto after = reservations.size();
		const auto diff = before - after;
		jobsRunning -= diff;
		if (diff > 0) {
			BOOST_LOG_TRIVIAL(info) << "reaped " << diff << " stale reservations - consider increasing reservation timeout";
		}
	}
}

grpc::Status Server::Reserve([[maybe_unused]] grpc::ServerContext *context, [[maybe_unused]] const distplusplus::Reservation *reservation,
							 distplusplus::ReservationAnswer *answer) {
	if (jobsRunning < jobsMax) {
		std::uint64_t jobsCur = jobsRunning.load();
		if (std::atomic_compare_exchange_strong(&jobsRunning, &jobsCur, jobsCur + 1)) {
			common::ScopeGuard jobCountGuard([&]() { jobsRunning--; });
			const std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
			reservations.emplace_hint(reservations.end(), std::pair(uuid, std::chrono::system_clock::now()));
			answer->set_uuid(uuid);
			answer->set_success(true);
			BOOST_LOG_TRIVIAL(debug) << "reserved job " << uuid << " for client " << context->peer();
			jobCountGuard.defuse();
			return grpc::Status::OK;
		}
	}
	answer->set_success(false);
	return grpc::Status::OK;
}

grpc::Status Server::Distribute(grpc::ServerContext *context, const distplusplus::CompileRequest *request,
								distplusplus::CompileAnswer *answer) {
	try {
		const std::string clientIP = context->peer();
		const std::string &clientUUID = request->uuid();
		BOOST_LOG_TRIVIAL(trace) << "client " << clientIP << " sent RPC " << [&request]() {
			std::string ret;
			google::protobuf::util::MessageToJsonString(*request, &ret);
			return ret;
		}();
		reservationLock.lock();
		if (std::erase_if(reservations, [&clientUUID](const _internal::reservationType &a) { return std::get<0>(a) == clientUUID; }) == 0) {
			reservationLock.unlock();
			const std::string errorMessage = "error: uuid " + clientUUID + " not in reservation list.";
			BOOST_LOG_TRIVIAL(warning) << "client " << clientIP << " sent job but uuid " << clientUUID << " was not in reservation list";
			return {grpc::StatusCode::FAILED_PRECONDITION, errorMessage};
		}
		reservationLock.unlock();
		common::ScopeGuard jobCountGuard([&]() { jobsRunning--; });

		// TODO: do this proper
		if (!sanitizeRequest(*request)) {
			BOOST_LOG_TRIVIAL(warning) << "client " << clientIP << " with job " << clientUUID << " protocol violation!";
			return {grpc::StatusCode::INVALID_ARGUMENT, "protocol violation"};
		}

		if (!checkCompilerAllowed(request->compiler())) {
			const std::string errorMessage = "error: compiler " + request->compiler() + " not in allow list";
			BOOST_LOG_TRIVIAL(warning) << "client " << clientIP << " sent job but compiler " << request->compiler()
									   << " was not in allow list";
			return {grpc::StatusCode::INVALID_ARGUMENT, errorMessage};
		}

		// the client could be a path to an unix socket too
		std::string clientIPDelimited = clientIP;
		std::replace(clientIPDelimited.begin(), clientIPDelimited.end(), '/', '-');
		const distplusplus::common::Tempfile inputFile(clientIPDelimited + "." + request->inputfile().name(), request->inputfile().content());

		const distplusplus::common::Tempfile outputFile(clientIPDelimited + "." + request->inputfile().name() + ".o");

		std::vector<std::string_view> preArgs(request->argument().begin(), request->argument().end());
		std::vector<std::string_view> args;
		try {
			parser::Parser parser(preArgs);
			args = parser.args();
		} catch (const parser::CannotProcessSignal &signal) {
			BOOST_LOG_TRIVIAL(warning) << "job " << clientUUID << " from host " << clientIP << " aborted due to invalid arguments:\n"
									   << signal.what();
			return {grpc::StatusCode::INVALID_ARGUMENT, "job aborted due to invalid arguments:\n" + signal.what()};
		}

		args.emplace_back("-o");
		args.emplace_back(outputFile.c_str());
		args.emplace_back(inputFile.c_str());

		const boost::filesystem::path compilerPath = boost::process::search_path(request->compiler());
		std::vector<std::string> processArgs;
		processArgs.reserve(args.size());
		for (const auto &arg : args) {
			processArgs.emplace_back(arg);
		}
		distplusplus::common::ProcessHelper compilerProcess(compilerPath, processArgs);
		// TODO: proper logging

		answer->set_returncode(compilerProcess.returnCode());
		answer->set_stdout(compilerProcess.get_stdout());
		answer->set_stderr(compilerProcess.get_stderr());
		answer->mutable_outputfile()->set_name(outputFile);
		std::ifstream fileStream(outputFile);
		std::stringstream fileContent;
		fileContent << fileStream.rdbuf();
		answer->mutable_outputfile()->set_content(fileContent.str());
		return grpc::Status::OK;
	} catch (const std::exception &e) {
		std::cerr << "exception in gRPC function: " << e.what() << std::endl;
		std::abort();
	}
}

Server::Server() {
	reservationReaperThread = std::thread([this] { reservationReaper(); });
}

Server::~Server() {
	reservationReaperKillswitch = true;
	reservationReaperThread.join();
}

} // namespace distplusplus::server
