#include "common.hpp"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <optional>
#include <string>

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
