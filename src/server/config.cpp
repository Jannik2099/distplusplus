#include "config.hpp"

#include "common/common.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <gsl/narrow>
#include <gsl/util>
#include <string>
#include <thread>

namespace distplusplus::server {

template <typename Type>
static Type getFromEnv(const boost::program_options::variables_map &varMap, const char *confName,
                       const char *envName);

template <>
std::string getFromEnv(const boost::program_options::variables_map &varMap, const char *confName,
                       const char *envName) {
    auto ret = varMap[confName].as<std::string>();
    if (const char *env = getenv(envName); env != nullptr) {
        BOOST_LOG_TRIVIAL(debug) << "config option " << confName << " overridden via env " << envName
                                 << "\nprevious value was: " << ret << "\nnew value is: " << ret;
        ret = env;
    }
    return ret;
}

template <>
std::uint64_t getFromEnv(const boost::program_options::variables_map &varMap, const char *confName,
                         const char *envName) {
    auto ret = varMap[confName].as<std::uint64_t>();
    if (const char *env = getenv(envName); env != nullptr) {
        errno = 0;
        const auto ret2 = strtoull(env, nullptr, 10);
        if (const int err = errno; err != 0) {
            const std::string errorMessage = "error converting " + std::string(envName) + " env " +
                                             std::string(env) + " to unsigned long long - errno is " +
                                             std::to_string(err);
            BOOST_LOG_TRIVIAL(fatal) << errorMessage;
            throw std::runtime_error(errorMessage);
        }
        BOOST_LOG_TRIVIAL(debug) << "config option " << confName << " overridden via env " << envName
                                 << "\nprevious value was: " << ret << "\nnew value is: " << ret2;
        ret = ret2;
    }
    return ret;
}

template <>
std::int64_t getFromEnv(const boost::program_options::variables_map &varMap, const char *confName,
                        const char *envName) {
    auto ret = varMap[confName].as<std::int64_t>();
    if (const char *env = getenv(envName); env != nullptr) {
        errno = 0;
        const auto ret2 = strtoll(env, nullptr, 10);
        if (const int err = errno; err != 0) {
            const std::string errorMessage = "error converting " + std::string(envName) + " env " +
                                             std::string(env) + " to unsigned long long - errno is " +
                                             std::to_string(err);
            BOOST_LOG_TRIVIAL(fatal) << errorMessage;
            throw std::runtime_error(errorMessage);
        }
        BOOST_LOG_TRIVIAL(debug) << "config option " << confName << " overridden via env " << envName
                                 << "\nprevious value was: " << ret << "\nnew value is: " << ret2;
        ret = ret2;
    }
    return ret;
}

Config getConfig(std::span<char *> argv) {
    const std::uint64_t coreCount = std::thread::hardware_concurrency();
    if (coreCount == 0) {
        BOOST_LOG_TRIVIAL(fatal) << "failed to detect number of cores";
        exit(1);
    }

    std::string enabledCompressionAlgs = "[NONE";
#ifdef DISTPLUSPLUS_WITH_ZSTD
    enabledCompressionAlgs += ", zstd";
#endif
    enabledCompressionAlgs += "]";

    boost::program_options::options_description desc;
    // clang-format off
    desc.add_options()
        ("compress",
         boost::program_options::value<std::string>()->default_value(DISTPLUSPLUS_DEFAULT_COMPRESSION_STR),
         ("compression algorithm to use, must be one of " + enabledCompressionAlgs).c_str())
        ("compression-level",
         boost::program_options::value<std::int64_t>()->default_value(1),
         "compression level, specific values depend on used algorithm. Accepts positive and negative values")
        ("help",
         "show this help")
        ("jobs",
         boost::program_options::value<std::uint64_t>()->default_value(coreCount),
         "number of maximum jobs being processed")
        ("listen-address",
         boost::program_options::value<std::string>()->default_value("127.0.0.1:3633"),
         "listen address, can be an IP or unix socket")
        ("log-level",
         boost::program_options::value<std::string>(),
         "log level, must be one of [fatal, error, warning, info, debug, trace]")
        ("reservation-timeout",
         boost::program_options::value<std::uint64_t>()->default_value(1),
         "reservation timeout in seconds")
    ;
    // clang-format on
    boost::program_options::variables_map varMap;
    boost::program_options::store(
        boost::program_options::parse_command_line(gsl::narrow<int>(argv.size()), argv.data(), desc), varMap);
    boost::program_options::notify(varMap);

    if (varMap.count("help") != 0) {
        std::cout << desc << std::endl;
        exit(0);
    }

    // set up log level before processing other options
    if (varMap.count("log-level") != 0) {
        const std::string logLevel = varMap["log-level"].as<std::string>();
        distplusplus::common::initBoostLogging(logLevel);
    } else {
        distplusplus::common::initBoostLogging();
    }

    const auto compressionType = [&varMap, &enabledCompressionAlgs] {
        const auto userCompressionType = getFromEnv<std::string>(varMap, "compress", "DISTPLUSPLUS_COMPRESS");
        if (userCompressionType == "NONE") {
            return distplusplus::CompressionType::NONE;
        }
        if (userCompressionType == "zstd") {
            return distplusplus::CompressionType::zstd;
        }
        BOOST_LOG_TRIVIAL(error) << "provided compression algorithm " << userCompressionType << " not in "
                                 << enabledCompressionAlgs;
        exit(1);
    }();
    BOOST_LOG_TRIVIAL(info) << "compression set to "
                            << distplusplus::common::compressionType_to_string(compressionType);

    const auto compressionLevel =
        getFromEnv<std::int64_t>(varMap, "compression-level", "DISTPLUSPLUS_COMPRESSION_LEVEL");
    BOOST_LOG_TRIVIAL(info) << "compression level set to " << std::to_string(compressionLevel);

    const auto maxJobs = getFromEnv<std::uint64_t>(varMap, "jobs", "DISTPLUSPLUS_JOBS");
    if (maxJobs == 0) {
        BOOST_LOG_TRIVIAL(error) << "max jobs must be greater than 0";
        exit(1);
    }
    BOOST_LOG_TRIVIAL(info) << "maximum job count set to " << std::to_string(maxJobs);

    const auto listenAddress =
        getFromEnv<std::string>(varMap, "listen-address", "DISTPLUSPLUS_LISTEN_ADDRESS");
    BOOST_LOG_TRIVIAL(info) << "listening on " << listenAddress;

    const auto reservationTimeout =
        getFromEnv<std::uint64_t>(varMap, "reservation-timeout", "DISTPLUSPLUS_RESERVATION_TIMEOUT");
    if (reservationTimeout == 0) {
        BOOST_LOG_TRIVIAL(warning) << "reservation timeout set to 0 - was this intentional?";
    }
    BOOST_LOG_TRIVIAL(info) << "reservation timeout set to " << std::to_string(reservationTimeout);

    Config ret;
    ret.compressionType = compressionType;
    ret.compressionLevel = compressionLevel;
    ret.maxJobs = maxJobs;
    ret.listenAddress = listenAddress;
    ret.reservationTimeout = reservationTimeout;

    return ret;
}

} // namespace distplusplus::server
