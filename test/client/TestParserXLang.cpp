#define TESTING_PRIVATE
#include "client/parser.hpp"
#undef TESTING_PRIVATE

#include "common/argsvec.hpp"
#include "common/constants.hpp"
#include "common/tempfile.hpp"

#include <iostream>

using distplusplus::common::ArgsVec;
using distplusplus::common::Tempfile;
using namespace distplusplus::client::parser;

int main() {
    bool success = true;

    for (const char *lang : distplusplus::common::xArgsC) {
        ArgsVec argsVec;
        Tempfile infile("test.c");
        Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.emplace_back("-x");
        argsVec.push_back(lang);
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        Parser parser(argsVec);
        if (parser._language != distplusplus::common::Language::C) {
            const std::string lang_str = distplusplus::common::language_to_string(parser._language);
            std::cout << "parsed language " << lang_str << " didn't match " << lang_str << std::endl;
            success = false;
        }
    }

    for (const char *lang : distplusplus::common::xArgsCXX) {
        ArgsVec argsVec;
        Tempfile infile("test.c");
        Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.emplace_back("-x");
        argsVec.push_back(lang);
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        Parser parser(argsVec);
        if (parser._language != distplusplus::common::Language::CXX) {
            const std::string lang_str = distplusplus::common::language_to_string(parser._language);
            std::cout << "parsed language " << lang_str << " didn't match " << lang_str << std::endl;
            success = false;
        }
    }

    {
        ArgsVec argsVec;
        Tempfile infile("test.c");
        Tempfile outfile("test.o");
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
        Tempfile infile("test.c");
        Tempfile outfile("test.o");
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
