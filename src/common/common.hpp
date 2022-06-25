#pragma once

#include "argsvec.hpp"
#include "distplusplus.pb.h"

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <filesystem>
#include <functional>
#include <grpcpp/support/status_code_enum.h>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using std::filesystem::path;

namespace distplusplus::common {

[[maybe_unused]] static constexpr const char *compressionType_to_string(distplusplus::CompressionType compressionType) {
    switch(compressionType) {
        case NONE:
            return "NONE";
        case zstd:
            return "zstd";
            // NOLINTNEXTLINE(bugprone-branch-clone)
        case CompressionType_INT_MAX_SENTINEL_DO_NOT_USE_:;
        case CompressionType_INT_MIN_SENTINEL_DO_NOT_USE_:;
    }
    throw std::runtime_error("encountered unexpected compression type value " + std::to_string(compressionType));
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

class ScopeGuard {
private:
    bool fuse = true;
    const std::function<void()> atexit;

public:
    ScopeGuard() = delete;
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard(ScopeGuard &&) = delete;
    ScopeGuard operator=(const ScopeGuard &) = delete;
    ScopeGuard operator=(ScopeGuard &&) = delete;
    ScopeGuard(std::function<void()> atexit);
    ~ScopeGuard();
    void defuse();
};

class ProcessHelper final {
private:
    int _returnCode;
    std::string _stdout;
    std::string _stderr;
    boost::process::ipstream stdoutPipe;
    boost::process::ipstream stderrPipe;
    boost::process::child process;

public:
    ProcessHelper(const boost::filesystem::path &program, ArgsVecSpan args);
    ProcessHelper(const boost::filesystem::path &program, ArgsVecSpan args,
                  const boost::process::environment &env);
    [[nodiscard]] const int &returnCode() const;
    // stdout / stderr are macros of stdio.h, avoid potential issues
    [[nodiscard]] const std::string &get_stdout() const;
    [[nodiscard]] const std::string &get_stderr() const;
};

class Tempfile final : public path {
private:
    bool cleanup = true;
    void createFileName(const path &path);

public:
    explicit Tempfile(const path &name);
    Tempfile(const path &name, std::string_view content);
    Tempfile(const Tempfile &) = delete;
    Tempfile(Tempfile &&) noexcept = default;
    ~Tempfile();

    Tempfile &operator=(const Tempfile &) = delete;
    Tempfile &operator=(Tempfile &&) noexcept = default;

    void disable_cleanup();
};

distplusplus::CompilerType mapCompiler(const std::string &compiler);

std::string mapGRPCStatus(const grpc::StatusCode &status);

} // namespace distplusplus::common
