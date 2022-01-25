#include "client/parser.hpp"
#include "common/argsvec.hpp"
#include "common/common.hpp"

using distplusplus::common::ArgsVec;
using namespace distplusplus::client::parser;

int main() {
    ArgsVec argsVec;
    distplusplus::common::Tempfile infile("test.c");
    distplusplus::common::Tempfile outfile("test.o");
    std::string args = std::string(infile) + " -c -o " + std::string(outfile);
    distplusplus::common::Tempfile argsfile("argsfile", args);
    argsVec.emplace_back("cc");
    argsVec.push_back("@" + std::string(argsfile));
    Parser parser(argsVec);
    if (parser.infile() != infile) {
        return 1;
    }
}
