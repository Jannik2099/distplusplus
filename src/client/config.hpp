#pragma once

#include "distplusplus.pb.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <toml++/toml.h>
#include <vector>

namespace distplusplus::client {

class Config final {
private:
    const std::filesystem::path _stateDir;
    const toml::table configFile;
    std::vector<std::string> _servers;
    // TODO: implement
    const int _reservationAttemptTimeout = 10;
    const distplusplus::CompressionType _compressionType;
    const std::int64_t _compressionLevel;
    const bool _fallback = true;

public:
    Config();

    [[nodiscard]] const std::vector<std::string> &servers() const;
    [[nodiscard]] const std::filesystem::path &stateDir() const;
    [[nodiscard]] const int &reservationAttemptTimeout() const;
    [[nodiscard]] distplusplus::CompressionType compressionType() const;
    [[nodiscard]] std::int64_t compressionLevel() const;
    [[nodiscard]] bool fallback() const;
};

const extern Config config;

}; // namespace distplusplus::client
