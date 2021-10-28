#include "server.hpp"
#include "parser.hpp"

#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <execution>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string_view>

using std::execution::unseq;

namespace distplusplus::server {

namespace _internal {

bool ReservationCompare::operator()(const reservationType &a, const reservationType &b) const { return std::get<1>(a) < std::get<1>(b); }

} // namespace _internal

// TODO: finish

static bool sanitizeFile(const distplusplus::File &file) {
	const std::string filename = file.name();
	if (!(std::filesystem::path(filename).filename() == filename)) {
		return false;
	}
	return true;
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
	for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator("/usr/lib/distcc")) {
		if (file.is_symlink()) {
			if (compiler == file.path().filename()) {
				return true;
			}
		}
	}
	// lazy copy
	for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator("/usr/libexec/distplusplus")) {
		if (file.is_symlink()) {
			if (compiler == file.path().filename()) {
				return true;
			}
		}
	}
	return false;
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
	const std::string clientIP = context->peer();
	const std::string &clientUUID = request->uuid();
	reservationLock.lock();
	if (std::erase_if(reservations, [&clientUUID](const _internal::reservationType &a) { return std::get<0>(a) == clientUUID; }) == 0) {
		reservationLock.unlock();
		const std::string errorMessage = "error: uuid " + clientUUID + " not in reservation list.";
		BOOST_LOG_TRIVIAL(warning) << "client " << clientIP << " sent job but uuid " << clientUUID << " was not in reservation list";
		return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, errorMessage);
	}
	reservationLock.unlock();
	common::ScopeGuard jobCountGuard([&]() { jobsRunning--; });

	// TODO: do this proper
	if (!sanitizeRequest(*request)) {
		BOOST_LOG_TRIVIAL(warning) << "client " << clientIP << " with job " << clientUUID << " protocol violation!";
		return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "protocol violation");
	}

	if (!checkCompilerAllowed(request->compiler())) {
		const std::string errorMessage = "error: compiler " + request->compiler() + " not in allow list";
		BOOST_LOG_TRIVIAL(warning) << "client " << clientIP << " sent job but compiler " << request->compiler() << " was not in allow list";
		return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, errorMessage);
	}

	const distplusplus::common::Tempfile inputFile(clientIP + "." + request->inputfile().name(), request->inputfile().content());

	const distplusplus::common::Tempfile outputFile(clientIP + "." + request->inputfile().name() + ".o");

	std::vector<std::string_view> preArgs(request->argument().begin(), request->argument().end());
	std::vector<std::string_view> args;
	try {
		parser::Parser parser(preArgs);
		args = parser.args();
	} catch (const parser::CannotProcessSignal &signal) {
		BOOST_LOG_TRIVIAL(warning) << "job " << clientUUID << " from host " << clientIP << " aborted due to invalid arguments:\n"
								   << signal.what();
		return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "job aborted due to invalid arguments:\n" + signal.what());
	}

	args.emplace_back("-o");
	args.emplace_back(outputFile.c_str());
	args.emplace_back(inputFile.c_str());

	const boost::filesystem::path compilerPath = boost::process::search_path(request->compiler());
	std::string compilerStdout;
	std::string compilerStderr;
	int returnCode = 0;
	// scope the ipstreams since accessing them after child.wait() is illegal
	{
		boost::process::ipstream stdoutPipe;
		boost::process::ipstream stderrPipe;
		// this is really messy but boost::process::args wouldn't work, need to investigate
		std::vector<std::string> processArgs;
		processArgs.push_back(compilerPath.c_str());
		for (const auto &arg : args) {
			processArgs.push_back(std::string(arg));
		}
		boost::process::child process(processArgs, boost::process::std_out > stdoutPipe, boost::process::std_err > stderrPipe);
		compilerStdout = std::string(std::istreambuf_iterator(stdoutPipe), {});
		compilerStderr = std::string(std::istreambuf_iterator(stderrPipe), {});
		process.wait();
		returnCode = process.exit_code();
	}
	// TODO: proper logging

	answer->set_returncode(returnCode);
	answer->set_stdout(compilerStdout);
	answer->set_stderr(compilerStderr);
	answer->mutable_outputfile()->set_name(outputFile);
	std::ifstream fileStream(outputFile);
	std::stringstream fileContent;
	fileContent << fileStream.rdbuf();
	answer->mutable_outputfile()->set_content(fileContent.str());
	return grpc::Status::OK;
}

Server::Server() {
	reservationReaperThread = std::thread([this] { reservationReaper(); });
}

Server::~Server() {
	reservationReaperKillswitch = true;
	reservationReaperThread.join();
}

} // namespace distplusplus::server
