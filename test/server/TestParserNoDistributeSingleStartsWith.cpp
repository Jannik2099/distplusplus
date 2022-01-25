#include "common/argsvec.hpp"
#include "common/constants.hpp"
#include "server/parser.hpp"

#include <iostream>

using distplusplus::common::ArgsVec;
using distplusplus::common::singleArgsNoDistributeStartsWith;
using namespace distplusplus::server::parser;

int main() {
    ArgsVec argsVec;
    argsVec.emplace_back("-c");
    for (const char *arg : singleArgsNoDistributeStartsWith) {
        argsVec.emplace_back(std::string(arg) + "test");
        try {
            Parser parser(argsVec);
        } catch (const CannotProcessSignal &) {
            argsVec.pop_back();
            continue;
        }
        std::cout << "parser didn't throw on no distrubute arg " << arg << std::endl;
        return 1;
    }
}
