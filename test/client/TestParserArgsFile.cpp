#include "client/parser.hpp"
#include "common/common.hpp"
#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace distplusplus::client::parser;

int main() {
	std::list<std::string> argsList;
	distplusplus::common::Tempfile infile("test.c");
	distplusplus::common::Tempfile outfile("test.o");
	std::string args = std::string(infile) + " -c -o " + std::string(outfile);
	distplusplus::common::Tempfile argsfile("argsfile", args);
	argsList.emplace_back("cc");
	argsList.push_back("@" + std::string(argsfile));
	std::vector<std::string_view> argsViewVec;
	for (const std::string &arg : argsList) {
		argsViewVec.push_back(arg);
	}
	distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
	Parser parser(argsSpan);
	if (parser.infile() != infile) {
		return 1;
	}
}
