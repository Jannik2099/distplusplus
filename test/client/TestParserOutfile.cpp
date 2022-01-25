#include "client/parser.hpp"
#include "common/argsvec.hpp"
#include "common/common.hpp"

#include <iostream>

using distplusplus::common::ArgsVec;
using namespace distplusplus::client::parser;

int main() {
    ArgsVec argsVec;
    distplusplus::common::Tempfile infile("test.c");
    distplusplus::common::Tempfile outfile("test.o");
    argsVec.emplace_back("cc");
    argsVec.push_back(infile.c_str());
    argsVec.emplace_back("-c");
    argsVec.emplace_back("-o");
    argsVec.push_back(outfile.c_str());
    Parser parser(argsVec);
    if (parser.outfile() != outfile) {
        std::cout << "parsed outfile " << parser.outfile() << " didn't match specified " << outfile;
        return 1;
    }
}
