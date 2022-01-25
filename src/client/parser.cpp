#include "parser.hpp"

#include "common/constants.hpp"
#include "fallback.hpp"

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <utility>

using std::execution::unseq;
using namespace distplusplus::common;

namespace distplusplus::client::parser {

void Parser::checkInputFileCandidate(const Arg &file) {
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
        _infile = path(file);
    } else {
        _args.push_back(file);
    }
}

void Parser::readArgsFile(const path &argsFile) { // NOLINT(misc-no-recursion)
    std::ifstream fileStream(argsFile);
    ArgsVec fileArgs;
    while (!fileStream.eof()) {
        std::string arg;
        std::getline(fileStream, arg, ' ');
        fileArgs.push_back(arg);
    }
    parseArgs(fileArgs);
}

// This function is genuine hell and I do not know how to make it better
// every branch in the main loop MUST be elseif.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Parser::parseArgs(ArgsVecSpan args) { // NOLINT(misc-no-recursion)
    _args.reserve(args.size() + _args.size());
    for (std::size_t i = 0; i < args.size(); i++) {
        const Arg &arg = args[i];
        const std::string_view argView(arg);
        if (std::any_of(unseq, singleArgsNoDistribute.begin(), singleArgsNoDistribute.end(),
                        [argView](const char *argComp) { return argView == argComp; })) {
            BOOST_LOG_TRIVIAL(info) << "cannot distribute because of arg " << arg.c_str();
            throw FallbackSignal();
        }
        if (std::any_of(unseq, singleArgsNoDistributeStartsWith.begin(),
                        singleArgsNoDistributeStartsWith.end(),
                        [argView](const char *argComp) { return argView.starts_with(argComp); })) {
            BOOST_LOG_TRIVIAL(info) << "cannot distribute because of arg " << arg.c_str();
            throw FallbackSignal();
        }
        if (std::any_of(unseq, multiArgsNoDistribute.begin(), multiArgsNoDistribute.end(),
                        [argView](const char *argComp) { return argView == argComp; })) {
            BOOST_LOG_TRIVIAL(info) << "cannot distribute because of arg " << arg.c_str();
            throw FallbackSignal();
        }
        // we assume files do not start with -
        if (argView.starts_with("@")) {
            const path argsFile = argView.substr(1);
            if (!std::filesystem::is_regular_file(argsFile)) {
                throw ParserError("argument file: " + std::string(arg) + " doesn't seem to exist");
            }
            readArgsFile(argsFile);
        } else if (argView == "-o") {
            if (i + 1 == args.size()) {
                throw ParserError("output file specifier is the last argument");
            }
            if (!_outfile.empty()) {
                throw ParserError("multiple output files parsed: " + _outfile.string() + " & " +
                                  std::string(args[i]));
            }
            i++;
            _outfile = std::filesystem::path(args[i]);
        } else if (argView == "-c" || argView == "-S") {
            _modeArg = arg;
            if (!_canDistribute.has_value()) {
                BOOST_LOG_TRIVIAL(trace) << "can distribute because of arg " << arg.c_str();
                _canDistribute = true;
            }
        } else if (argView == "-x") {
            if (i + 1 == args.size()) {
                throw ParserError("multiArg -x is the last argument");
            }
            _args.push_back(args[i]);
            i++;
            _args.push_back(args[i]);
            if (std::find(xArgsC.begin(), xArgsC.end(), std::string_view(args[i])) != xArgsC.end()) {
                _language = Language::C;
            } else if (std::find(xArgsCXX.begin(), xArgsCXX.end(), std::string_view(args[i])) !=
                       xArgsCXX.end()) {
                _language = Language::CXX;
            } else {
                BOOST_LOG_TRIVIAL(warning) << "uncategorized -x option: \"-x " << std::string_view(args[i])
                                           << "\" - cannot distribute";
                _canDistribute = false;
            }
        } else if (argView == "-target") {
            if (i + 1 == args.size()) {
                throw ParserError("clang -target is the last argument");
            }
            _args.push_back(args[i]);
            i++;
            _target = args[i];
            _args.push_back(args[i]);
        } else if (argView.starts_with("--target=")) {
            const std::string::size_type pos = argView.find('=');
            _target = Arg(argView.substr(pos + 1));
            _args.push_back(arg);
        } else if (std::find(multiArgsCPP.begin(), multiArgsCPP.end(), std::string_view(arg)) !=
                   multiArgsCPP.end()) {
            if (i + 1 == args.size()) {
                throw ParserError("multi argument " + std::string(arg) + " is the last argument");
            }
            _args.push_back(args[i]);
            i++;
            _args.push_back(args[i]);
        } else if (std::filesystem::is_regular_file(std::string_view(arg))) {
            checkInputFileCandidate(args[i]);
        } else {
            _args.push_back(arg);
        }
    }
}

Parser::Parser(ArgsVecSpan args) {
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

std::optional<Arg> Parser::target() const { return _target; }

const ArgsVec &Parser::args() const { return _args; }

const Arg &Parser::modeArg() const { return _modeArg; }

bool Parser::canDistribute() const { return _canDistribute.value(); }

ParserError::ParserError(std::string message) noexcept : message(std::move(message)) {}

const char *ParserError::what() const noexcept { return message.c_str(); }

} // namespace distplusplus::client::parser
