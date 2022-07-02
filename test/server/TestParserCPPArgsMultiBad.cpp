#include "common/argsvec.hpp"
#include "common/constants.hpp"
#include "server/parser.hpp"

#include <iostream>
#include <ranges>
#include <string>
#include <string_view>

using distplusplus::common::ArgsVec;
using distplusplus::common::multiArgsCPP;
using distplusplus::common::singleArgsCPPStartsWith;
using namespace distplusplus::server::parser;

int main() {
    ArgsVec argsVec;
    argsVec.emplace_back("-c");
    for (const auto &arg : multiArgsCPP) {
        auto starts_with = [&arg](decltype(arg) rhs) { return std::string(arg).starts_with(rhs); };
        if (std::ranges::find_if(singleArgsCPPStartsWith, starts_with) != singleArgsCPPStartsWith.end()) {
            // If the arg is also in singleArgsCPPStartsWith, the parser will not throw a CannotProcessSignal
            continue;
        }
        argsVec.emplace_back(arg);
        try {
            Parser parser(argsVec);
        } catch (const CannotProcessSignal &) {
            argsVec.pop_back();
            continue;
        }
        std::cout << "parser didn't throw on " << arg << " as last argument" << std::endl;
        return 1;
    }
}
