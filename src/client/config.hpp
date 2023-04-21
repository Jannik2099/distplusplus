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
    std::filesystem::path _stateDir;
    toml::table configFile;
    std::vector<std::string> _servers;
    std::uint32_t _reservationAttemptTimeout = 10;
    distplusplus::CompressionType _compressionType;
    std::int64_t _compressionLevel;
    bool _fallback = true;

public:
    Config();

    [[nodiscard]] const std::vector<std::string> &servers() const;
    [[nodiscard]] const std::filesystem::path &stateDir() const;
    [[nodiscard]] std::uint32_t reservationAttemptTimeout() const;
    [[nodiscard]] distplusplus::CompressionType compressionType() const;
    [[nodiscard]] std::int64_t compressionLevel() const;
    [[nodiscard]] bool fallback() const;
};

const extern Config config;

}; // namespace distplusplus::client
