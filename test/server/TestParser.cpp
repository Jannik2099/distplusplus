#include "common/argsvec.hpp"
#include "server/parser.hpp"

using distplusplus::common::ArgsVec;
using namespace distplusplus::server::parser;

int main() {
    ArgsVec argsVec = {"-c"};
    Parser parser(argsVec);
}
