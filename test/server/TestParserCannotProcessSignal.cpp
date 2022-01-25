#include "common/argsvec.hpp"
#include "server/parser.hpp"

using distplusplus::common::ArgsVec;
using namespace distplusplus::server::parser;

int main() {
    ArgsVec argsVec = {"test"};
    try {
        Parser parser(argsVec);
    } catch (const CannotProcessSignal &sig) {
        // this is really just to get full coverage
        if (sig.what().empty()) {
            return 1;
        }
        return 0;
    }
    return 1;
}
