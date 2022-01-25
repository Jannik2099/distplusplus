#pragma once

#ifdef TESTING_PRIVATE
#define TESTING_PRIVATE_DISTPLUSPLUS_CLIENT_PARSER
#undef TESTING_PRIVATE
#endif

#include "common/argsvec.hpp"
#include "common/common.hpp"
#include "common/constants.hpp"

#include <exception>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef TESTING_PRIVATE_DISTPLUSPLUS_CLIENT_PARSER
#define private public
#endif

using distplusplus::common::Language;
using std::filesystem::path;

namespace distplusplus::client::parser {

class Parser final {
private:
    using ArgsVecSpan = distplusplus::common::ArgsVecSpan;
    using ArgsVec = distplusplus::common::ArgsVec;
    using Arg = distplusplus::common::Arg;

    ArgsVec _args;
    Arg _modeArg;
    path _infile;
    path _outfile;
    std::string _compiler;
    std::optional<Arg> _target;
    std::optional<bool> _canDistribute;
    Language _language = Language::NONE;

    void checkInputFileCandidate(const Arg &file);
    void readArgsFile(const path &argsFile);
    void parseArgs(ArgsVecSpan args);

public:
    explicit Parser(ArgsVecSpan args);
    Parser(const Parser &) = delete;
    Parser(Parser &&) = delete;
    Parser operator=(const Parser &) = delete;
    Parser operator=(Parser &&) = delete;
    ~Parser() = default;
    [[nodiscard]] const path &infile() const;
    [[nodiscard]] const path &outfile() const;
    [[nodiscard]] std::optional<Arg> target() const;
    [[nodiscard]] const ArgsVec &args() const;
    [[nodiscard]] const Arg &modeArg() const;
    [[nodiscard]] bool canDistribute() const;
};

class ParserError final : public std::exception {
private:
    const std::string message;

public:
    ParserError(std::string message) noexcept;
    [[nodiscard]] const char *what() const noexcept final;
};

} // namespace distplusplus::client::parser
