#include "client/parser.hpp"
#include "common/argsvec.hpp"
#include "common/tempfile.hpp"

#include <iostream>
#include <string_view>

using distplusplus::common::ArgsVec;
using distplusplus::common::Tempfile;
using namespace distplusplus::client::parser;

int main() {
    bool success = true;
    for (const char *arg : distplusplus::common::multiArgsCPP) {
        {
            ArgsVec argsVec;
            Tempfile infile("test.c");
            Tempfile outfile("test.o");
            argsVec.emplace_back("cc");
            argsVec.push_back(infile.c_str());
            argsVec.emplace_back("-c");
            argsVec.emplace_back("-o");
            argsVec.push_back(outfile.c_str());
            argsVec.emplace_back(arg);
            argsVec.emplace_back("IAMANARG");
            Parser parser(argsVec);
            const auto &parsedArgs = parser.args();
            for (int i = 0; i < parsedArgs.size(); i++) {
                if (std::string_view(parsedArgs.at(i)) == arg) {
                    if (std::string_view(parsedArgs.at(i + 1)) != "IAMANARG") {
                        std::cout << "failed to parse multiArg " << arg << std::endl;
                        success = false;
                    }
                }
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
            argsVec.emplace_back(arg);
            bool caught = false;
            try {
                Parser parser(argsVec);
            } catch (const ParserError &) {
                caught = true;
            }
            if (!caught) {
                std::cout << "failed to catch " << arg << " multiArg as last arg" << std::endl;
                success = false;
            }
        }
    }

    if (!success) {
        return 1;
    }
}
