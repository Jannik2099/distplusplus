#pragma once

#include "common/argsvec.hpp"

#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace distplusplus::server::parser {

class CannotProcessSignal final {
private:
    const std::string error;

public:
    CannotProcessSignal(std::string error) : error(std::move(error)) {}
    [[nodiscard]] const std::string &what() const { return error; }
};

class Parser final {
private:
    using ArgsSpan = distplusplus::common::ArgsSpan;
    using ArgsVec = distplusplus::common::ArgsVec;
    using Arg = distplusplus::common::Arg;

    ArgsVec _args;
    void parse(ArgsSpan args);

public:
    explicit Parser(ArgsSpan args);
    [[nodiscard]] const ArgsVec &args() const;
};

} // namespace distplusplus::server::parser
