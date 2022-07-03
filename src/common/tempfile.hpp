#pragma once

#include <filesystem>
#include <string_view>

namespace distplusplus::common {

class Tempfile final : public std::filesystem::path {
private:
    bool cleanup = true;
    void createFileName(const std::filesystem::path &path);

public:
    explicit Tempfile(const std::filesystem::path &name);
    Tempfile(const std::filesystem::path &name, std::string_view content);
    Tempfile(const Tempfile &) = delete;
    Tempfile(Tempfile &&) noexcept = default;
    ~Tempfile();

    Tempfile &operator=(const Tempfile &) = delete;
    Tempfile &operator=(Tempfile &&) noexcept = default;

    void disable_cleanup();
};

} // namespace distplusplus::common