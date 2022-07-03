#include "client/fallback.hpp"
#include "client/parser.hpp"
#include "common/argsvec.hpp"
#include "common/constants.hpp"
#include "common/tempfile.hpp"

#include <iostream>

using distplusplus::client::FallbackSignal;
using namespace distplusplus::common;
using namespace distplusplus::client::parser;

int main() {
    bool success = true;
    for (const char *arg : singleArgsNoDistribute) {
        bool caught = false;
        ArgsVec argsVec;
        Tempfile infile("test.c");
        Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        argsVec.push_back(arg);
        try {
            Parser parser(argsVec);
        } catch (FallbackSignal &) {
            caught = true;
        }
        if (!caught) {
            success = false;
            std::cout << "bad argument " << arg << " didn't throw FallbackSignal" << std::endl;
        }
    }
    for (const char *arg : singleArgsNoDistributeStartsWith) {
        bool caught = false;
        ArgsVec argsVec;
        Tempfile infile("test.c");
        Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        argsVec.push_back(arg);
        try {
            Parser parser(argsVec);
        } catch (FallbackSignal &) {
            caught = true;
        }
        if (!caught) {
            success = false;
            std::cout << "bad argument " << arg << " didn't throw FallbackSignal" << std::endl;
        }
    }
    for (const auto &arg : multiArgsNoDistribute) {
        bool caught = false;
        ArgsVec argsVec;
        Tempfile infile("test.c");
        Tempfile outfile("test.o");
        argsVec.emplace_back("cc");
        argsVec.push_back(infile.c_str());
        argsVec.emplace_back("-c");
        argsVec.emplace_back("-o");
        argsVec.push_back(outfile.c_str());
        argsVec.push_back(arg);
        argsVec.emplace_back("test");
        try {
            Parser parser(argsVec);
        } catch (FallbackSignal &) {
            caught = true;
        }
        if (!caught) {
            success = false;
            std::cout << "bad argument " << arg << " didn't throw FallbackSignal" << std::endl;
        }
    }
    if (!success) {
        return 1;
    }
}
