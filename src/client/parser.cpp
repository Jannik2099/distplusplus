#include "parser.hpp"

#include "common/constants.hpp"
#include "fallback.hpp"

#include <algorithm>
#include <array>
#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <type_traits>
#include <utility>

using std::execution::unseq;
using namespace distplusplus::common;

namespace distplusplus::client::parser {

static std::list<std::vector<std::string>>
    fileArgsVec; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static std::vector<std::vector<std::string_view>>
    fileArgsViewVec; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void Parser::checkInputFileCandidate(const std::string_view &file) {
    const path filePath(file);
    const path extension = filePath.extension();
    if (std::find(inputFileExtension.begin(), inputFileExtension.end(), extension) !=
        inputFileExtension.end()) {
        if (!_infile.empty()) {
            throw ParserError("multiple input files parsed: " + _infile.string() + " & " + std::string(file));
        }
        // language can be overridden with -x
        if (_language == Language::NONE) {
            if (std::find(inputFileExtensionC.begin(), inputFileExtensionC.end(), extension) !=
                inputFileExtensionC.end()) {
                BOOST_LOG_TRIVIAL(trace) << "detected language as C from file extension ." << extension;
                _language = Language::C;
            } else if (std::find(inputFileExtensionCXX.begin(), inputFileExtensionCXX.end(), extension) !=
                       inputFileExtensionCXX.end()) {
                BOOST_LOG_TRIVIAL(trace) << "detected language as C++ from file extension ." << extension;
                _language = Language::CXX;
            }
        }
        _infile = file;
    } else {
        _args.push_back(file);
    }
}

void Parser::readArgsFile(const path &argsFile) { // NOLINT(misc-no-recursion)
    std::ifstream fileStream(argsFile);
    std::vector<std::string> myArgs;
    while (!fileStream.eof()) {
        std::string arg;
        std::getline(fileStream, arg, ' ');
        myArgs.push_back(arg);
    }
    // this ain't pretty
    fileArgsVec.push_back(myArgs);
    std::vector<std::string_view> fileArgsView;
    for (const auto &arg : fileArgsVec.back()) {
        fileArgsView.emplace_back(std::string_view(arg));
    }
    fileArgsViewVec.push_back(fileArgsView);
    parseArgs(BoundsSpan(fileArgsViewVec.back()));
}

// This function is genuine hell and I do not know how to make it better
// every branch in the main loop MUST be elseif.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Parser::parseArgs(const BoundsSpan<std::string_view> &args) { // NOLINT(misc-no-recursion)
    _args.reserve(args.size() + _args.size());
    for (std::size_t i = 0; i < args.size(); i++) {
        const std::string_view arg = args[i];
        if (std::any_of(unseq, singleArgsNoDistribute.begin(), singleArgsNoDistribute.end(),
                        [&](const auto &arg) { return args[i] == arg; })) {
            BOOST_LOG_TRIVIAL(info) << "cannot distribute because of arg " << arg;
            throw FallbackSignal();
        }
        if (std::any_of(unseq, singleArgsNoDistributeStartsWith.begin(),
                        singleArgsNoDistributeStartsWith.end(),
                        [&](const auto &arg) { return args[i].starts_with(arg); })) {
            BOOST_LOG_TRIVIAL(info) << "cannot distribute because of arg " << arg;
            throw FallbackSignal();
        }
        if (std::any_of(unseq, multiArgsNoDistribute.begin(), multiArgsNoDistribute.end(),
                        [&](const auto &arg) { return args[i] == arg; })) {
            BOOST_LOG_TRIVIAL(info) << "cannot distribute because of arg " << arg;
            throw FallbackSignal();
        }
        // we assume files do not start with -
        if (arg.starts_with("@")) {
            const path argsFile = arg.substr(1);
            if (!std::filesystem::is_regular_file(argsFile)) {
                throw ParserError("argument file: " + std::string(arg) + " doesn't seem to exist");
            }
            readArgsFile(argsFile);
        } else if (arg == "-o") {
            if (i + 1 == args.size()) {
                throw ParserError("output file specifier is the last argument");
            }
            if (!_outfile.empty()) {
                throw ParserError("multiple output files parsed: " + _outfile.string() + " & " +
                                  std::string(args[i]));
            }
            i++;
            _outfile = args[i];
        } else if (arg == "-c" || arg == "-S") {
            _modeArg = arg;
            if (!_canDistribute.has_value()) {
                BOOST_LOG_TRIVIAL(trace) << "can distribute because of arg " << arg;
                _canDistribute = true;
            }
        } else if (arg == "-x") {
            if (i + 1 == args.size()) {
                throw ParserError("multiArg -x is the last argument");
            }
            _args.push_back(args[i]);
            i++;
            _args.push_back(args[i]);
            if (std::find(xArgsC.begin(), xArgsC.end(), args[i]) != xArgsC.end()) {
                _language = Language::C;
            } else if (std::find(xArgsCXX.begin(), xArgsCXX.end(), args[i]) != xArgsCXX.end()) {
                _language = Language::CXX;
            } else {
                BOOST_LOG_TRIVIAL(warning)
                    << "uncategorized -x option: \"-x " << args[i] << "\" - cannot distribute";
                _canDistribute = false;
            }
        } else if (arg == "-target") {
            if (i + 1 == args.size()) {
                throw ParserError("clang -target is the last argument");
            }
            _args.push_back(arg);
            i++;
            const std::string target(args[i]);
            _target = target;
            _args.push_back(target);
        } else if (arg.starts_with("--target=")) {
            const std::string::size_type pos = arg.find('=');
            const std::string_view target = arg.substr(pos + 1);
            _target = target;
            _args.push_back(arg);
        } else if (std::find(multiArgsCPP.begin(), multiArgsCPP.end(), arg) != multiArgsCPP.end()) {
            if (i + 1 == args.size()) {
                throw ParserError("multi argument " + std::string(arg) + " is the last argument");
            }
            _args.push_back(args[i]);
            i++;
            _args.push_back(args[i]);
        } else if (std::filesystem::is_regular_file(arg)) {
            checkInputFileCandidate(arg);
        } else {
            _args.push_back(arg);
        }
    }
}

Parser::Parser(BoundsSpan<std::string_view> &args) {
    parseArgs(args);
    if (!_canDistribute.has_value()) {
        _canDistribute = false;
    }
    if (!_infile.empty() && _infile.filename() == "conftest.c") {
        _canDistribute = false;
    }
    if (!_canDistribute.value()) {
        return;
    }
    if (_infile.empty()) {
        throw ParserError("input file not specified or does not exist");
    }
    if (_outfile.empty()) {
        _outfile = std::string(_infile.stem()) + ".o";
    }
    if (_language == Language::NONE) {
        throw ParserError("failed to determine language");
    }
}

const std::filesystem::path &Parser::infile() const { return _infile; }

const std::filesystem::path &Parser::outfile() const { return _outfile; }

const std::optional<std::string> &Parser::target() const { return _target; }

const std::vector<std::string_view> &Parser::args() const { return _args; }

const std::string &Parser::modeArg() const { return _modeArg; }

bool Parser::canDistribute() const { return _canDistribute.value(); }

ParserError::ParserError(std::string message) noexcept : message(std::move(message)) {}

const char *ParserError::what() const noexcept { return message.c_str(); }

} // namespace distplusplus::client::parser
