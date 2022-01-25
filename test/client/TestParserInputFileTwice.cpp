#include "client/parser.hpp"
#include "common/argsvec.hpp"
#include "common/common.hpp"

using distplusplus::common::ArgsVec;
using namespace distplusplus::client::parser;

int main() {
    ArgsVec argsVec;
    distplusplus::common::Tempfile infile("test.c");
    distplusplus::common::Tempfile infile2("test.c");
    distplusplus::common::Tempfile outfile("test.o");
    argsVec.emplace_back("cc");
    argsVec.push_back(infile.c_str());
    argsVec.push_back(infile2.c_str());
    argsVec.emplace_back("-c");
    argsVec.emplace_back("-o");
    argsVec.push_back(outfile.c_str());
    try {
        Parser parser(argsVec);
    } catch (ParserError &) {
        return 0;
    }
    return 1;
}
