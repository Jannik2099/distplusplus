#include "tempfile.hpp"

#include <boost/log/trivial.hpp>
#include <cerrno>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace distplusplus::common {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::filesystem::path tempDir = "/tmp";

} // namespace

void setTempDir(const std::filesystem::path &path) {
    if (!path.is_absolute()) [[unlikely]] {
        const std::string errorMessage = "tried to set temporary dir to non-absolute path " + path.string();
        BOOST_LOG_TRIVIAL(fatal) << errorMessage;
        throw std::invalid_argument(errorMessage);
    }
    if (std::filesystem::exists(path) && !std::filesystem::is_directory(path)) [[unlikely]] {
        const std::string errorMessage = "tried to set temporary dir to existing file " + path.string();
        BOOST_LOG_TRIVIAL(fatal) << errorMessage;
        throw std::invalid_argument(errorMessage);
    }
    tempDir = path;
}

namespace {

// something tells me this is a really bad approach
// we do this because uncaught exceptions do not invoke destructors

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::vector<std::filesystem::path> tempfileVec;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::terminate_handler prev;
void tempfileCleaner() noexcept {
    for (const std::filesystem::path &file : tempfileVec) {
        BOOST_LOG_TRIVIAL(debug) << "deleting temporary file " << file;
        std::error_code err;
        const bool success = std::filesystem::remove(file, err);
        if (!success) {
            if (err.value() != ENOENT) {
                BOOST_LOG_TRIVIAL(fatal) << "failed to delete temporary file " << file << " with error "
                                         << std::to_string(err.value()) << " " << err.message();
            }
        }
    }
    prev();
}

class TempfileCleanerSetter {
public:
    TempfileCleanerSetter() {
        prev = std::get_terminate();
        std::set_terminate(tempfileCleaner);
    }
};

const TempfileCleanerSetter tempfileCleanerSetter;

} // namespace

void Tempfile::createFileName(const path &name, const std::filesystem::path &dirname) {
    const std::string suffix = name.filename().string();
    if (suffix.empty()) [[unlikely]] {
        throw std::invalid_argument("path " + name.string() + " has no filename");
    }
    if (name.filename() != name) [[unlikely]] {
        throw std::invalid_argument("path " + name.string() + " is not a basename");
    }
    if (dirname.filename() != dirname) [[unlikely]] {
        throw std::invalid_argument("dirname " + dirname.string() + " is not a directory name");
    }

    const std::string templateString = tempDir.string() + '/' + dirname.string() + "XXXXXX";
    // char arrays are a special kind of hellspawn
    char *templateCString = strdup(templateString.c_str());
    if (mkdtemp(templateCString) == nullptr) {
        const int err = errno;
        const std::string errorMessage =
            "error creating temporary directory - errno is " + std::to_string(err);
        BOOST_LOG_TRIVIAL(fatal) << errorMessage;
        throw std::runtime_error(errorMessage);
    }
    BOOST_LOG_TRIVIAL(debug) << "created temporary directory " << templateCString;
    this->assign(std::string(templateCString) + '/' + name.string());
    std::ofstream stream(this->string());
    stream.close();
    BOOST_LOG_TRIVIAL(debug) << "created temporary file " << *this;
    tempfileVec.emplace_back(*this);
    free(templateCString); // NOLINT(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
}

Tempfile::Tempfile(const std::filesystem::path &name, const std::filesystem::path &dirname,
                   std::string_view content) {
    createFileName(name, dirname);
    if (!content.empty()) {
        std::ofstream stream(this->string());
        stream << content;
        stream.close();
    }
}

Tempfile::~Tempfile() {
    if (!cleanup) {
        return;
    }
    try {
        BOOST_LOG_TRIVIAL(debug) << "deleting temporary file " << *this;
        std::filesystem::remove(*this);
    } catch (const std::filesystem::filesystem_error &err) {
        if (err.code() != std::errc::no_such_file_or_directory) {
            BOOST_LOG_TRIVIAL(fatal) << "failed to delete temporary file " << *this << " with error "
                                     << err.what();
            throw;
        }
    }
    try {
        BOOST_LOG_TRIVIAL(debug) << "deleting temporary directory " << this->parent_path();
        std::filesystem::remove(this->parent_path());
    } catch (const std::filesystem::filesystem_error &err) {
        if (err.code() != std::errc::no_such_file_or_directory) {
            BOOST_LOG_TRIVIAL(fatal) << "failed to delete temporary directory " << this->parent_path()
                                     << " with error " << err.what();
            throw;
        }
    }
}

void Tempfile::disable_cleanup() { cleanup = false; }

} // namespace distplusplus::common
