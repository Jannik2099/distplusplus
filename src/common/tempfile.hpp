#pragma once

#include <filesystem>
#include <string_view>

namespace distplusplus::common {

void setTempDir(const std::filesystem::path &path);

class Tempfile final : public std::filesystem::path {
private:
    bool cleanup = true;
    void createFileName(const std::filesystem::path &name, const std::filesystem::path &dirname);

public:
    Tempfile(const std::filesystem::path &name, const std::filesystem::path &dirname = "",
             std::string_view content = "");
    Tempfile(const Tempfile &) = delete;
    Tempfile(Tempfile &&) noexcept = default;
    ~Tempfile();

    Tempfile &operator=(const Tempfile &) = delete;
    Tempfile &operator=(Tempfile &&) noexcept = default;

    void disable_cleanup();
};

} // namespace distplusplus::common
