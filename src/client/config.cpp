#include "config.hpp"

#include <algorithm>
#include <array>
#include <boost/log/trivial.hpp>
#include <cstdlib>

namespace distplusplus::client {

static constexpr std::array soundsLikeNo = {"0", "NO", "no", "OFF", "off"};
static constexpr std::array soundsLikeYes = {"1", "YES", "yes", "ON", "on"};

static std::filesystem::path getStateDir() {
    std::filesystem::path ret;
    if (char *var = getenv("DISTPLUSPLUS_STATE_DIR"); var != nullptr) {
        ret = var;
    } else if (var = getenv("HOME"); var != nullptr) {
        ret = std::string(var) + std::string("/.local/state/distplusplus");
    } else {
        const std::string err_msg("could not figure out state directory");
        BOOST_LOG_TRIVIAL(error) << err_msg;
        throw std::runtime_error(err_msg);
    }
    std::filesystem::create_directories(ret);
    return ret;
}

static toml::table getConfigFile() {
    std::filesystem::path ret;
    if (char *var = getenv("DISTPLUSPLUS_CONFIG_FILE"); var != nullptr) {
        ret = var;
    } else {
        ret = "/etc/distplusplus/distplusplus.toml";
    }
    if (!std::filesystem::is_regular_file(ret)) {
        const std::string err_msg("config file " + ret.string() + " does not exist or is not a regular file");
        BOOST_LOG_TRIVIAL(error) << err_msg;
        throw std::runtime_error(err_msg);
    }
    return toml::parse_file(std::string(ret));
}

static std::vector<std::string> getServers(const toml::table &configFile) {
    std::vector<std::string> ret;

    if (const char *listenAddrEnv = getenv("DISTPLUSPLUS_LISTEN_ADDRESS"); listenAddrEnv != nullptr) {
        ret.emplace_back(listenAddrEnv);
        return ret;
    }

    for (const toml::array &serverArray = *configFile.get_as<toml::array>("servers");
         const auto &server : serverArray) {
        ret.push_back(server.as_string()->get());
    }
    if (ret.empty()) {
        const std::string err_msg("config file " + *configFile.source().path + " provides no hosts");
        BOOST_LOG_TRIVIAL(error) << err_msg;
        throw std::runtime_error(err_msg);
    }
    return ret;
}

static distplusplus::CompressionType getCompressionType(const toml::table &configFile) {
    const char *compressionEnv = getenv("DISTPLUSPLUS_COMPRESS");
    std::string compressionString;
    if (compressionEnv != nullptr) {
        compressionString = compressionEnv;
    } else {
        compressionString = configFile["compress"].value_or(DISTPLUSPLUS_DEFAULT_COMPRESSION_STR);
    }
    if (compressionString == "NONE") {
        return NONE;
    }
    if (compressionString == "zstd") {
        return zstd;
    }
    const std::string err_msg("unrecognized compression type " + compressionString);
    BOOST_LOG_TRIVIAL(error) << err_msg;
    throw std::runtime_error(err_msg);
}

static std::int64_t getCompressionLevel(const toml::table &configFile) {
    const char *compressionLevelEnv = getenv("DISTPLUSPLUS_COMPRESSION_LEVEL");
    std::int64_t compressionLevel = 0;
    if (compressionLevelEnv != nullptr) {
        compressionLevel = std::atoll(compressionLevelEnv);
    } else {
        compressionLevel = configFile["compression-level"].value_or(1);
    }
    return compressionLevel;
}

static bool getFallback(const toml::table &configFile) {
    if (const char *fallbackEnv = getenv("DISTPLUSPLUS_FALLBACK"); fallbackEnv != nullptr) {
        const std::string fallbackEnvString = fallbackEnv;
        if (std::ranges::find(soundsLikeNo, fallbackEnvString) != soundsLikeNo.end()) {
            return false;
        }
        if (std::ranges::find(soundsLikeYes, fallbackEnvString) != soundsLikeYes.end()) {
            return true;
        }
    }
    return configFile["fallback"].value_or(true);
}

// really should rework the initialization
Config::Config()
    : _stateDir(getStateDir()), configFile(getConfigFile()), _servers(getServers(configFile)),
      _compressionType(getCompressionType(configFile)), _compressionLevel(getCompressionLevel(configFile)),
      _fallback(getFallback(configFile)) {
    if (_servers.empty()) {
        const std::string err_msg("config file " + *configFile.source().path +
                                  " provides empty list of hosts");
        BOOST_LOG_TRIVIAL(error) << err_msg;
        throw std::runtime_error(err_msg);
    }
}

const std::vector<std::string> &Config::servers() const { return _servers; }
const std::filesystem::path &Config::stateDir() const { return _stateDir; }
const int &Config::reservationAttemptTimeout() const { return _reservationAttemptTimeout; }
distplusplus::CompressionType Config::compressionType() const { return _compressionType; }
std::int64_t Config::compressionLevel() const { return _compressionLevel; }
bool Config::fallback() const { return _fallback; }

const Config config;
}; // namespace distplusplus::client
