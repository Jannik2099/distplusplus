#include "client/parser.hpp"
#include "common/argsvec.hpp"
#include "common/common.hpp"

#include <iostream>
#include <string_view>

using distplusplus::common::ArgsVec;
using namespace distplusplus::client::parser;

int main() {
    bool success = true;

    {
        ArgsVec argsVec;
        distplusplus::common::Tempfile infile("test.c");
        distplusplus::common::Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        argsVec.emplace_back("--target=IAMATARGET");
        Parser parser(argsVec);
        if (std::string_view(parser.target().value_or("IAMNOTTHETARGET")) != "IAMATARGET") {
            std::cout << "failed to detect target from single arg" << std::endl;
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
        argsVec.emplace_back("-target");
        argsVec.emplace_back("IAMATARGET");
        Parser parser(argsVec);
        if (std::string_view(parser.target().value_or("IAMNOTTHETARGET")) != "IAMATARGET") {
            std::cout << "failed to detect target from single arg" << std::endl;
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
        argsVec.emplace_back("-target");
        bool caught = false;
        try {
            Parser parser(argsVec);
        } catch (const ParserError &) {
            caught = true;
        }
        if (!caught) {
            std::cout << "failed to catch -target multiArg as last arg" << std::endl;
            success = false;
        }
    }

    if (!success) {
        return 1;
    }
}
