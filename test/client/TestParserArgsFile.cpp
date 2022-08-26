#include "client/parser.hpp"
#include "common/argsvec.hpp"
#include "common/tempfile.hpp"

#include <string>

using distplusplus::common::ArgsVec;
using distplusplus::common::Tempfile;
using namespace distplusplus::client::parser;

int main() {
    ArgsVec argsVec;
    Tempfile infile("test.c");
    Tempfile outfile("test.o");
    std::string args = std::string(infile) + " -c -o " + std::string(outfile);
    Tempfile argsfile("argsfile", "", args);
    argsVec.emplace_back("cc");
    argsVec.push_back("@" + std::string(argsfile));
    Parser parser(argsVec);
    if (parser.infile() != infile) {
        return 1;
    }
}
