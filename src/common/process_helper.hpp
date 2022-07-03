#pragma once

#include "argsvec.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/process/child.hpp>
#include <boost/process/environment.hpp>
#include <string>

namespace distplusplus::common {

class ProcessHelper final {
private:
    int _returnCode;
    std::string _stdout;
    std::string _stderr;
    boost::process::child process;

public:
    ProcessHelper(const boost::filesystem::path &program, ArgsSpan args, const std::string &cin = "",
                  const boost::process::environment &env = boost::this_process::environment());
    [[nodiscard]] int returnCode() const;
    // stdout / stderr are macros of stdio.h, avoid potential issues
    [[nodiscard]] const std::string &get_stdout() const;
    [[nodiscard]] const std::string &get_stderr() const;
};

} // namespace distplusplus::common