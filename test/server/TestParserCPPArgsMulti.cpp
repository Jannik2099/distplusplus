#include "common/constants.hpp"
#include "server/parser.hpp"

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <string_view>
#include <vector>

using namespace distplusplus::server::parser;
using distplusplus::common::multiArgsCPP;

int main() {
	std::list<std::string> argsList;
	argsList.emplace_back("-c");
	for (const auto &arg : multiArgsCPP) {
		argsList.emplace_back(arg);
		argsList.emplace_back("testarg");
	}
	std::vector<std::string_view> argsVec;
	for (const auto &arg : argsList) {
		argsVec.push_back(arg);
	}
	Parser parser(argsVec);
	for (const auto &argParsed : parser.args()) {
		if (std::any_of(multiArgsCPP.begin(), multiArgsCPP.end(), [&](const auto &arg) { return argParsed == arg; })) {
			std::cout << "preprocessor argument " << argParsed << " was not filtered" << std::endl;
			return 1;
		}
	}
}
