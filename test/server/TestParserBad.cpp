#include "common/argsvec.hpp"
#include "server/parser.hpp"

using distplusplus::common::ArgsVec;
using namespace distplusplus::server::parser;

int main() {
    ArgsVec argsVec = {"test"};
    try {
        Parser parser(argsVec);
    } catch (const CannotProcessSignal &sig) {
        return 0;
    }
    return 1;
}
