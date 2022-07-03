#include "common.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/pipe.hpp>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <gsl/gsl_util>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <unistd.h>
#include <utility>

using std::filesystem::filesystem_error;

namespace distplusplus::common {

static void initBoostLoggingCore(const std::optional<std::string> &logLevelOpt) {
    const char *ret = getenv("DISTPLUSPLUS_LOG_LEVEL");
    std::string logLevel;
    if (ret == nullptr) {
        logLevel = logLevelOpt.value_or("warning");
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
        BOOST_LOG_TRIVIAL(error) << errorMessage;
        throw std::invalid_argument(errorMessage);
    }
}

void initBoostLogging() {
    const std::optional<std::string> logLevelOpt = {};
    initBoostLoggingCore(logLevelOpt);
}

void initBoostLogging(const std::string &level) {
    const std::optional<std::string> logLevelOpt = level;
    initBoostLoggingCore(logLevelOpt);
}

ScopeGuard::ScopeGuard(std::function<void()> atexit) : atexit(std::move(atexit)) {}
ScopeGuard::~ScopeGuard() {
    if (fuse) {
        atexit();
    }
}
void ScopeGuard::defuse() { fuse = false; }

ProcessHelper::ProcessHelper(const boost::filesystem::path &program, ArgsSpan args, const std::string &cin,
                             const boost::process::environment &env) {
    // a segfault in boost is really shitty to debug - assert sanity beforehand
    assertAndRaise(!program.empty(), "passed program is empty");
    assertAndRaise(program.has_filename(), "passed program " + program.string() + " is misshaped");
    assertAndRaise(boost::filesystem::is_regular_file(program),
                   "passed program " + program.string() + " does not exist");
    // TODO: find out if there's a way to construct boost args without copying?
    std::vector<std::string> rawArgs;
    rawArgs.reserve(args.size());
    for (const ArgsVec::value_type &str : args) {
        rawArgs.emplace_back(str);
    }

    boost::asio::io_service ios;
    std::future<std::string> errfut;
    std::future<std::string> outfut;
    process = boost::process::child(
        program, rawArgs, env,
        boost::process::std_in<boost::asio::buffer(cin), boost::process::std_out> outfut,
        boost::process::std_err > errfut, ios);
    ios.run();
    process.wait();
    _returnCode = process.exit_code();
    _stderr = errfut.get();
    _stdout = outfut.get();
    BOOST_LOG_TRIVIAL(trace) << "process invocation:\n"
                             << "command: " << program << '\n'
                             << "args: " <<
        [&rawArgs]() {
            std::string ret;
            for (const auto &arg : rawArgs) {
                ret += arg + ' ';
            }
            return ret;
        }() << '\n'
                             << "stdin: " << cin << '\n'
                             << "stderr: " << _stderr << "stdout: " << _stdout
                             << "exit code: " << std::to_string(_returnCode);
}

const int &ProcessHelper::returnCode() const { return _returnCode; }
const std::string &ProcessHelper::get_stderr() const { return _stderr; }
const std::string &ProcessHelper::get_stdout() const { return _stdout; }

void Tempfile::createFileName(const path &path) {
    const std::string suffix = path.filename().string();
    if (suffix.empty()) [[unlikely]] {
        throw std::invalid_argument("path " + path.string() + " has no filename");
    }
    if (path.filename() != path) [[unlikely]] {
        throw std::invalid_argument("path " + path.string() + " is not a basename");
    }
    const std::string templateString = "XXXXXX" + suffix;
    // char arrays are a special kind of hellspawn
    char *templateCString = strdup(templateString.c_str()); // NOLINT(cppcoreguidelines-pro-type-vararg)
    // and so is this function
    const int fileDescriptor = mkstemps(templateCString, gsl::narrow_cast<int>(suffix.length()));
    if (fileDescriptor == -1) {
        const int err = errno;
        const std::string errorMessage = "error creating file descriptor - errno is " + std::to_string(err);
        BOOST_LOG_TRIVIAL(fatal) << errorMessage;
        throw std::runtime_error(errorMessage);
    }
    if (close(fileDescriptor) == -1) {
        const int err = errno;
        const std::string errorMessage = "error closing file descriptor - errno is " + std::to_string(err);
        BOOST_LOG_TRIVIAL(fatal) << errorMessage;
        throw std::runtime_error(errorMessage);
    }
    this->assign(templateCString);
    free(templateCString); // NOLINT(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
}

Tempfile::Tempfile(const std::filesystem::path &name) { createFileName(name); }

Tempfile::Tempfile(const std::filesystem::path &name, std::string_view content) {
    createFileName(name);
    std::ofstream stream(this->string());
    stream << content;
    stream.close();
}

Tempfile::~Tempfile() {
    if (!cleanup) {
        return;
    }
    try {
        BOOST_LOG_TRIVIAL(debug) << "deleting temporary file " << *this;
        std::filesystem::remove(*this);
    } catch (const filesystem_error &err) {
        if (err.code() != std::errc::no_such_file_or_directory) {
            BOOST_LOG_TRIVIAL(fatal) << "failed to delete temporary file " << *this << " with error "
                                     << err.what();
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
