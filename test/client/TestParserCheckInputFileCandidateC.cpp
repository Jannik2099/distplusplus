#define TESTING_PRIVATE
#include "client/parser.hpp"
#undef TESTING_PRIVATE
#include "common/argsvec.hpp"
#include "common/common.hpp"
#include "common/constants.hpp"

#include <iostream>

using distplusplus::common::ArgsVec;
using namespace distplusplus::client::parser;

int main() {
    bool success = true;
    for (const char *extension : distplusplus::common::inputFileExtensionC) {
        ArgsVec argsVec;
        distplusplus::common::Tempfile infile("test" + std::string(extension));
        distplusplus::common::Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        Parser parser(argsVec);
        if (parser._language != distplusplus::common::Language::C) {
            success = false;
            std::cout << "input file extension " << extension << " was recognized as " << parser._language
                      << std::endl;
        }
    }
    if (!success) {
        return 1;
    }
}
