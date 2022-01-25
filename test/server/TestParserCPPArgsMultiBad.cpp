#include "common/argsvec.hpp"
#include "common/constants.hpp"
#include "server/parser.hpp"

#include <iostream>
#include <string_view>

using distplusplus::common::ArgsVec;
using distplusplus::common::multiArgsCPP;
using namespace distplusplus::server::parser;

int main() {
    ArgsVec argsVec;
    argsVec.emplace_back("-c");
    for (const auto &arg : multiArgsCPP) {
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
