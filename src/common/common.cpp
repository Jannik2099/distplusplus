#include "common.hpp"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <gsl/gsl_util>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <unistd.h>
#include <utility>

using std::filesystem::filesystem_error;

namespace distplusplus::common {

void initBoostLogging() {
	const char *ret = getenv("DISTPLUSPLUS_LOG_LEVEL");
	std::string logLevel;
	if (ret == nullptr) {
		logLevel = "warning";
	} else {
		logLevel = ret;
	}

	if (logLevel == "trace") {
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
	} else if (logLevel == "debug") {
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
	} else if (logLevel == "info") {
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
	} else if (logLevel == "warning") {
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::warning);
	} else if (logLevel == "error") {
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::error);
	} else if (logLevel == "fatal") {
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::fatal);
	} else {
		const std::string errorMessage = "unrecognized log level " + logLevel;
		BOOST_LOG_TRIVIAL(fatal) << errorMessage;
		throw std::invalid_argument(errorMessage);
	}
}

ScopeGuard::ScopeGuard(std::function<void()> atexit) : atexit(std::move(atexit)) {}
ScopeGuard::~ScopeGuard() {
	if (fuse) {
		atexit();
	}
}
void ScopeGuard::defuse() { fuse = false; }

ProcessHelper::ProcessHelper(const boost::filesystem::path &program, const std::vector<std::string> &args,
							 const boost::process::environment &env) {
	// a segfault in boost is really shitty to debug - assert sanity beforehand
	assertAndRaise(!program.empty(), "passed program is empty");
	assertAndRaise(program.has_filename(), "passed program " + program.string() + " is misshaped");
	assertAndRaise(boost::filesystem::is_regular_file(program), "passed program " + program.string() + " does not exist");
	for (const auto &arg : args) {
		assertAndRaise(!arg.empty(), "passed argument is empty");
	}
	process = boost::process::child(program, args, env, boost::process::std_out > stdoutPipe, boost::process::std_err > stderrPipe);
	process.wait();
	_returnCode = process.exit_code();
	_stdout = std::string(std::istreambuf_iterator(stdoutPipe), {});
	_stderr = std::string(std::istreambuf_iterator(stderrPipe), {});
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init) clang-tidy does not properly evaluate into the called constructor
ProcessHelper::ProcessHelper(const boost::filesystem::path &program, const std::vector<std::string> &args) {
	*this = ProcessHelper(program, args, boost::this_process::environment());
}

const int &ProcessHelper::returnCode() const { return _returnCode; }
const std::string &ProcessHelper::get_stderr() const { return _stderr; }
const std::string &ProcessHelper::get_stdout() const { return _stdout; }

void Tempfile::createFileName(const path &path) {
	auto suffix = path.filename().string();
	if (suffix.empty()) [[unlikely]] {
		throw std::invalid_argument("path " + path.string() + " has no filename");
	}
	if (path.filename() != path) [[unlikely]] {
		throw std::invalid_argument("path " + path.string() + " is not a basename");
	}
	std::string templateString = "XXXXXX" + suffix;
	// char arrays are a special kind of hellspawn
	char *templateCString = strdup(templateString.c_str()); // NOLINT(cppcoreguidelines-pro-type-vararg)
	// and so is this function
	const int fileDescriptor = mkstemps(templateCString, gsl::narrow_cast<int>(suffix.size() / sizeof(char)));
	if (fileDescriptor == -1) {
		// TODO: error handling
	}
	if (close(fileDescriptor) == -1) {
		// TODO: error handling
	}
	this->assign(templateCString);
	namePtr = templateCString;
}

Tempfile::Tempfile(const std::filesystem::path &name) { createFileName(name); }

Tempfile::Tempfile(const std::filesystem::path &name, const std::string &content) {
	createFileName(name);
	std::ofstream stream(this->string());
	stream << content;
	stream.close();
}

Tempfile::~Tempfile() {
	free(namePtr); // NOLINT(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
	if (!cleanup) {
		return;
	}
	try {
		BOOST_LOG_TRIVIAL(debug) << "deleting temporary file " << *this;
		std::filesystem::remove(*this);
	} catch (const filesystem_error &err) {
		if (err.code() != std::errc::no_such_file_or_directory) {
			BOOST_LOG_TRIVIAL(fatal) << "failed to delete temporary file " << *this << " with error " << err.what();
			throw;
		}
	}
}

void Tempfile::disable_cleanup() { cleanup = false; }

distplusplus::CompilerType mapCompiler(const std::string &compiler) {
	if (compiler.find("clang") != std::string::npos) {
		BOOST_LOG_TRIVIAL(debug) << "detected compiler as clang";
		return distplusplus::CompilerType::clang;
	}
	if (compiler.find("gcc") != std::string::npos) {
		BOOST_LOG_TRIVIAL(debug) << "detected compiler as gcc";
		return distplusplus::CompilerType::gcc;
	}
	// This must come AFTER clang since clang++ evaluates to g++
	if (compiler.find("g++") != std::string::npos) {
		BOOST_LOG_TRIVIAL(debug) << "detected compiler as gcc";
		return distplusplus::CompilerType::gcc;
	}
	BOOST_LOG_TRIVIAL(debug) << "detected compiler as UNKNOWN";
	return distplusplus::CompilerType::UNKNOWN;
}

std::string mapGRPCStatus(const grpc::StatusCode &status) {
	switch (status) {
	case grpc::StatusCode::DO_NOT_USE:
		return "DO_NOT_USE";
	case grpc::StatusCode::OK:
		return "OK";
	case grpc::StatusCode::CANCELLED:
		return "CANCELLED";
	case grpc::StatusCode::UNKNOWN:
		return "UNKNOWN";
	case grpc::StatusCode::INVALID_ARGUMENT:
		return "INVALID_ARGUMENT";
	case grpc::StatusCode::DEADLINE_EXCEEDED:
		return "DEADLINE_EXCEEDED";
	case grpc::StatusCode::NOT_FOUND:
		return "NOT_FOUND";
	case grpc::StatusCode::ALREADY_EXISTS:
		return "ALREADY_EXISTS";
	case grpc::StatusCode::PERMISSION_DENIED:
		return "PERMISSION_DENIED";
	case grpc::StatusCode::RESOURCE_EXHAUSTED:
		return "RESOURCE_EXHAUSTED";
	case grpc::StatusCode::FAILED_PRECONDITION:
		return "FAILED_PRECONDITION";
	case grpc::StatusCode::ABORTED:
		return "ABORTED";
	case grpc::StatusCode::OUT_OF_RANGE:
		return "OUT_OF_RANGE";
	case grpc::StatusCode::UNIMPLEMENTED:
		return "UNIMPLEMENTED";
	case grpc::StatusCode::INTERNAL:
		return "INTERNAL";
	case grpc::StatusCode::UNAVAILABLE:
		return "UNAVAILABLE";
	case grpc::StatusCode::DATA_LOSS:
		return "DATA_LOSS";
	case grpc::StatusCode::UNAUTHENTICATED:
		return "UNAUTHENTICATED";
	}
	throw std::runtime_error("unhandled status in mapGRPCStatus " + std::to_string(status));
}

} // namespace distplusplus::common
