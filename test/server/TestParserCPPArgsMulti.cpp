#include "common/argsvec.hpp"
#include "common/constants.hpp"
#include "server/parser.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>

using distplusplus::common::Arg;
using distplusplus::common::ArgsVec;
using distplusplus::common::multiArgsCPP;
using namespace distplusplus::server::parser;

int main() {
    ArgsVec argsVec;
    argsVec.emplace_back("-c");
    for (const char *arg : multiArgsCPP) {
        argsVec.push_back(arg);
        argsVec.emplace_back("testarg");
    }
    Parser parser(argsVec);
    for (const Arg &argParsed : parser.args()) {
        if (std::any_of(multiArgsCPP.begin(), multiArgsCPP.end(),
                        [&](const char *argComp) { return std::string_view(argParsed) == argComp; })) {
            std::cout << "preprocessor argument " << std::string_view(argParsed) << " was not filtered"
                      << std::endl;
            return 1;
        }
    }
}
