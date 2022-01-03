#include "config.hpp"

#include <boost/log/trivial.hpp>
#include <cstdlib>

namespace distplusplus::client {

static std::filesystem::path getStateDir() {
    std::filesystem::path ret;
    char *var = nullptr;
    if ((var = getenv("DISTPLUSPLUS_STATE_DIR")), var != nullptr) {
        ret = var;
    } else if ((var = getenv("HOME")), var != nullptr) {
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
    char *var = nullptr;
    if ((var = getenv("DISTPLUSPLUS_CONFIG_FILE")), var != nullptr) {
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

    const char *listenAddrEnv = getenv("DISTPLUSPLUS_LISTEN_ADDRESS");
    if (listenAddrEnv != nullptr) {
        ret.emplace_back(listenAddrEnv);
        return ret;
    }

    const toml::array &serverArray = *configFile.get_as<toml::array>("servers");
    for (const auto &server : serverArray) {
        ret.push_back(server.as_string()->get());
    }
    if (ret.empty()) {
        const std::string err_msg("config file " + *configFile.source().path + " provides no hosts");
        BOOST_LOG_TRIVIAL(error) << err_msg;
        throw std::runtime_error(err_msg);
    }
    return ret;
}

Config::Config() : _stateDir(getStateDir()), configFile(getConfigFile()), _servers(getServers(configFile)) {
    if (_servers.empty()) {
        const std::string err_msg("config file " + *configFile.source().path + " provides empty list of hosts");
        BOOST_LOG_TRIVIAL(error) << err_msg;
        throw std::runtime_error(err_msg);
    }
}

const std::vector<std::string> &Config::servers() const { return _servers; }
const std::filesystem::path &Config::stateDir() const { return _stateDir; }
const int &Config::reservationAttemptTimeout() const { return _reservationAttemptTimeout; }

const Config config;
}; // namespace distplusplus::client
