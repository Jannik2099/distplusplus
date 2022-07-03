#include "tempfile.hpp"

#include <boost/log/trivial.hpp>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <gsl/narrow>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace distplusplus::common {

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

void Tempfile::createFileName(const path &path) {
    const std::string suffix = path.filename().string();
    if (suffix.empty()) [[unlikely]] {
        throw std::invalid_argument("path " + path.string() + " has no filename");
    }
    if (path.filename() != path) [[unlikely]] {
        throw std::invalid_argument("path " + path.string() + " is not a basename");
    }
    const std::string templateString = "XXXXXX" + suffix;
    // char arrays are a special kind of hellspawn
    char *templateCString = strdup(templateString.c_str()); // NOLINT(cppcoreguidelines-pro-type-vararg)
    // and so is this function
    const int fileDescriptor = mkstemps(templateCString, gsl::narrow_cast<int>(suffix.length()));
    if (fileDescriptor == -1) {
        const int err = errno;
        const std::string errorMessage = "error creating file descriptor - errno is " + std::to_string(err);
        BOOST_LOG_TRIVIAL(fatal) << errorMessage;
        throw std::runtime_error(errorMessage);
    }
    if (close(fileDescriptor) == -1) {
        const int err = errno;
        const std::string errorMessage = "error closing file descriptor - errno is " + std::to_string(err);
        BOOST_LOG_TRIVIAL(fatal) << errorMessage;
        throw std::runtime_error(errorMessage);
    }
    this->assign(templateCString);
    BOOST_LOG_TRIVIAL(debug) << "created temporary file " << *this;
    tempfileVec.emplace_back(*this);
    free(templateCString); // NOLINT(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
}

Tempfile::Tempfile(const std::filesystem::path &name) { createFileName(name); }

Tempfile::Tempfile(const std::filesystem::path &name, std::string_view content) {
    createFileName(name);
    std::ofstream stream(this->string());
    stream << content;
    stream.close();
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
}

void Tempfile::disable_cleanup() { cleanup = false; }

} // namespace distplusplus::common