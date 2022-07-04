#include "parser.hpp"

#include "common/constants.hpp"

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <cstddef>

using namespace distplusplus::common;

namespace distplusplus::server::parser {

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Parser::parse(ArgsSpan args) {
    _args.reserve(args.size());
    bool canDistribute = false;
    for (std::size_t i = 0; i < args.size(); i++) {
        const Arg &arg = args[i];
        const std::string_view argView(arg);
        if (std::ranges::any_of(singleArgsNoDistribute,
                                [argView](const char *argComp) { return argView == argComp; })) {
            throw CannotProcessSignal("encountered non-distributable argument " + std::string(arg));
        }
        if (std::ranges::any_of(multiArgsNoDistribute,
                                [argView](const char *argComp) { return argView == argComp; })) {
            throw CannotProcessSignal("encountered non-distributable argument " + std::string(arg));
        }
        if (std::ranges::any_of(singleArgsNoDistributeStartsWith,
                                [argView](const char *argComp) { return argView.starts_with(argComp); })) {
            throw CannotProcessSignal("encountered non-distributable argument " + std::string(arg));
        }
        // filter out cpp flags
        if (std::ranges::any_of(singleArgsCPP,
                                [argView](const char *argComp) { return argView == argComp; })) {
            BOOST_LOG_TRIVIAL(trace) << "filtered preprocessor argument " << arg.c_str();
            continue;
        }
        if (std::ranges::any_of(multiArgsCPP,
                                [argView](const char *argComp) { return argView == argComp; })) {
            if (i == args.size() - 1) {
                throw CannotProcessSignal("tried to filter multi arg " + std::string(arg) +
                                          " but it is the last argument");
            }
            BOOST_LOG_TRIVIAL(trace) << "filtered preprocessor argument " << args[i].c_str() << " "
                                     << args[i + 1].c_str();
            i++;
            continue;
        }
        if (std::ranges::any_of(singleArgsCPPStartsWith,
                                [argView](const char *argComp) { return argView.starts_with(argComp); })) {
            BOOST_LOG_TRIVIAL(trace) << "filtered preprocessor argument " << arg.c_str();
            continue;
        }
        if (argView == "-c") {
            canDistribute = true;
        }
        _args.push_back(arg);
    }
    if (!canDistribute) {
        throw CannotProcessSignal("invocation seems non-distributable - called for link?");
    }
}

Parser::Parser(ArgsSpan args) { parse(args); }

const ArgsVec &Parser::args() const { return _args; }

} // namespace distplusplus::server::parser
