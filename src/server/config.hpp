#pragma once

#include "distplusplus.pb.h"

#include <cstdint>
#include <span>
#include <string>

namespace distplusplus::server {

struct Config {
    distplusplus::CompressionType compressionType;
    std::int64_t compressionLevel;
    std::uint64_t maxJobs;
    std::string listenAddress;
    std::uint64_t reservationTimeout;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
[[nodiscard]] Config getConfig(std::span<char *> argv);

} // namespace distplusplus::server
