#include "common/argsvec.hpp"
#include "common/constants.hpp"
#include "server/parser.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>

using distplusplus::common::ArgsVec;
using distplusplus::common::singleArgsCPP;
using distplusplus::common::singleArgsNoDistribute;
using namespace distplusplus::server::parser;

int main() {
    ArgsVec argsVec;
    argsVec.emplace_back("-c");
    for (const auto &arg : singleArgsCPP) {
        if (std::find(singleArgsNoDistribute.begin(), singleArgsNoDistribute.end(), arg) !=
            singleArgsNoDistribute.end()) {
            continue;
        }
        argsVec.emplace_back(arg);
    }
    Parser parser(argsVec);
    for (const auto &argParsed : parser.args()) {
        if (std::any_of(singleArgsCPP.begin(), singleArgsCPP.end(),
                        [&](const char *argComp) { return std::string_view(argParsed) == argComp; })) {
            std::cout << "preprocessor argument " << std::string_view(argParsed) << " was not filtered"
                      << std::endl;
            return 1;
        }
    }
}
