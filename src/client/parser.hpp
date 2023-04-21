#pragma once

#ifdef TESTING_PRIVATE
#define TESTING_PRIVATE_DISTPLUSPLUS_CLIENT_PARSER
#undef TESTING_PRIVATE
#endif

#include "common/argsvec.hpp"
#include "common/constants.hpp"

#include <exception>
#include <filesystem>
#include <optional>
#include <string>

#ifdef TESTING_PRIVATE_DISTPLUSPLUS_CLIENT_PARSER
#define private public
#endif

namespace distplusplus::client::parser {

class Parser final {
private:
    using ArgsSpan = distplusplus::common::ArgsSpan;
    using ArgsVec = distplusplus::common::ArgsVec;
    using Arg = distplusplus::common::Arg;

    ArgsVec _args;
    Arg _modeArg;
    std::filesystem::path _infile;
    std::filesystem::path _outfile;
    std::string _compiler;
    std::optional<Arg> _target;
    std::optional<bool> _canDistribute;
    distplusplus::common::Language _language = distplusplus::common::Language::NONE;

    void checkInputFileCandidate(const Arg &file);
    void readArgsFile(const std::filesystem::path &argsFile);
    void parseArgs(ArgsSpan args);

public:
    explicit Parser(ArgsSpan args);
    Parser(const Parser &) = delete;
    Parser(Parser &&) = delete;
    Parser operator=(const Parser &) = delete;
    Parser operator=(Parser &&) = delete;
    ~Parser() = default;
    [[nodiscard]] const std::filesystem::path &infile() const;
    [[nodiscard]] const std::filesystem::path &outfile() const;
    [[nodiscard]] std::optional<Arg> target() const;
    [[nodiscard]] const ArgsVec &args() const;
    [[nodiscard]] const Arg &modeArg() const;
    [[nodiscard]] bool canDistribute() const;
};

class ParserError final : public std::exception {
private:
    std::string message;

public:
    explicit ParserError(std::string message) noexcept;
    [[nodiscard]] const char *what() const noexcept override;
};

} // namespace distplusplus::client::parser
