#pragma once

#include "argsvec.hpp"
#include "distplusplus.pb.h"

#include <filesystem>
#include <grpcpp/support/status_code_enum.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace distplusplus::common {

[[maybe_unused]] static constexpr const char *
compressionType_to_string(distplusplus::CompressionType compressionType) {
    switch (compressionType) {
    case NONE:
        return "NONE";
    case zstd:
        return "zstd";
        // NOLINTNEXTLINE(bugprone-branch-clone)
    case CompressionType_INT_MAX_SENTINEL_DO_NOT_USE_:;
    case CompressionType_INT_MIN_SENTINEL_DO_NOT_USE_:;
    }
    throw std::runtime_error("encountered unexpected compression type value " +
                             std::to_string(compressionType));
}

[[maybe_unused]] static void assertAndRaise(bool condition, const std::string &msg) {
#ifdef NDEBUG
    return;
#else
    if (condition) [[likely]] {
        return;
    }
    throw std::runtime_error("assertion failed: " + msg);
#endif
}

void initBoostLogging();
void initBoostLogging(const std::string &level);

distplusplus::CompilerType mapCompiler(const std::string &compiler);

std::string mapGRPCStatus(const grpc::StatusCode &status);

} // namespace distplusplus::common
