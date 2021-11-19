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
	distplusplus::common::Tempfile infile2("test.c");
	distplusplus::common::Tempfile outfile("test.o");
	argsList.emplace_back("cc");
	argsList.push_back(infile);
	argsList.push_back(infile2);
	argsList.emplace_back("-c");
	argsList.emplace_back("-o");
	argsList.push_back(outfile);
	std::vector<std::string_view> argsViewVec;
	for (const std::string &arg : argsList) {
		argsViewVec.push_back(arg);
	}
	distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
	try {
		Parser parser(argsSpan);
	} catch (ParserError &) {
		return 0;
	}
	return 1;
}
