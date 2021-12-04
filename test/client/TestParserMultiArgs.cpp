#include "client/parser.hpp"
#include "common/common.hpp"

#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace distplusplus::client::parser;

int main() {
	bool success = true;
	for (const auto &arg : distplusplus::common::multiArgsCPP) {
		{
			std::list<std::string> argsList;
			distplusplus::common::Tempfile infile("test.c");
			distplusplus::common::Tempfile outfile("test.o");
			argsList.emplace_back("cc");
			argsList.push_back(infile);
			argsList.emplace_back("-c");
			argsList.emplace_back("-o");
			argsList.push_back(outfile);
			argsList.emplace_back(arg);
			argsList.emplace_back("IAMANARG");
			std::vector<std::string_view> argsViewVec;
			for (const std::string &arg : argsList) {
				argsViewVec.push_back(arg);
			}
			distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
			Parser parser(argsSpan);
			const auto &parsedArgs = parser.args();
			for (int i = 0; i < parsedArgs.size(); i++) {
				if (parsedArgs.at(i) == arg) {
					if (parsedArgs.at(i + 1) != "IAMANARG") {
						std::cout << "failed to parse multiArg " << arg << std::endl;
						success = false;
					}
				}
			}
		}

		{
			std::list<std::string> argsList;
			distplusplus::common::Tempfile infile("test.c");
			distplusplus::common::Tempfile outfile("test.o");
			argsList.emplace_back("cc");
			argsList.push_back(infile);
			argsList.emplace_back("-c");
			argsList.emplace_back("-o");
			argsList.push_back(outfile);
			argsList.emplace_back(arg);
			std::vector<std::string_view> argsViewVec;
			for (const std::string &arg : argsList) {
				argsViewVec.push_back(arg);
			}
			distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
			bool caught = false;
			try {
				Parser parser(argsSpan);
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
