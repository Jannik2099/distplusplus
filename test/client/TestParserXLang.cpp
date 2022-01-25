#define TESTING_PRIVATE
#include "client/parser.hpp"
#undef TESTING_PRIVATE

#include "common/argsvec.hpp"
#include "common/common.hpp"

#include <iostream>

using distplusplus::common::ArgsVec;
using namespace distplusplus::client::parser;

int main() {
    bool success = true;

    for (const char *lang : distplusplus::common::xArgsC) {
        ArgsVec argsVec;
        distplusplus::common::Tempfile infile("test.c");
        distplusplus::common::Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.emplace_back("-x");
        argsVec.push_back(lang);
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        Parser parser(argsVec);
        if (parser._language != distplusplus::common::Language::C) {
            std::cout << "parsed language " << std::to_string(parser._language) << " didn't match "
                      << std::to_string(distplusplus::common::Language::C) << std::endl;
            success = false;
        }
    }

    for (const char *lang : distplusplus::common::xArgsCXX) {
        ArgsVec argsVec;
        distplusplus::common::Tempfile infile("test.c");
        distplusplus::common::Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.emplace_back("-x");
        argsVec.push_back(lang);
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        Parser parser(argsVec);
        if (parser._language != distplusplus::common::Language::CXX) {
            std::cout << "parsed language " << std::to_string(parser._language) << " didn't match "
                      << std::to_string(distplusplus::common::Language::CXX) << std::endl;
            success = false;
        }
    }

    {
        ArgsVec argsVec;
        distplusplus::common::Tempfile infile("test.c");
        distplusplus::common::Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        argsVec.emplace_back("-x");
        argsVec.emplace_back("IAMABADLANG");
        Parser parser(argsVec);
        if (parser.canDistribute()) {
            std::cout << "failed to catch invalid language" << std::endl;
            success = false;
        }
    }

    bool caught = false;
    try {
        ArgsVec argsVec;
        distplusplus::common::Tempfile infile("test.c");
        distplusplus::common::Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        argsVec.emplace_back("-x");
        Parser parser(argsVec);
    } catch (const ParserError &) {
        caught = true;
    }
    if (!caught) {
        std::cout << "failed to catch -x multiArg as last arg" << std::endl;
        success = false;
    }

    if (!success) {
        return 1;
    }
}
