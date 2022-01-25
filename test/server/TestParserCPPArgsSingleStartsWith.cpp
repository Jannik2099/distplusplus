#include "common/argsvec.hpp"
#include "common/constants.hpp"
#include "server/parser.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>

using distplusplus::common::ArgsVec;
using distplusplus::common::singleArgsCPPStartsWith;
using namespace distplusplus::server::parser;

int main() {
    ArgsVec argsVec;
    argsVec.emplace_back("-c");
    for (const char *arg : singleArgsCPPStartsWith) {
        argsVec.emplace_back(std::string(arg) + "test");
    }
    Parser parser(argsVec);
    for (const auto &argParsed : parser.args()) {
        if (std::any_of(
                singleArgsCPPStartsWith.begin(), singleArgsCPPStartsWith.end(),
                [&](const char *argComp) { return std::string_view(argParsed).starts_with(argComp); })) {
            std::cout << "preprocessor argument " << std::string_view(argParsed) << " was not filtered"
                      << std::endl;
            return 1;
        }
    }
}
