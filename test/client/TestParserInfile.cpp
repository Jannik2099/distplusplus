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
	argsList.push_back("cc");
	argsList.push_back(infile);
	argsList.push_back("-c");
	argsList.push_back("-o");
	argsList.push_back(outfile);
	std::vector<std::string_view> argsViewVec;
	for (const std::string &arg : argsList) {
		argsViewVec.push_back(arg);
	}
	distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
	Parser parser(argsSpan);
	if (parser.infile() != infile) {
		std::cout << "parsed infile " << parser.infile() << " didn't match specified " << infile;
		return 1;
	}
}
