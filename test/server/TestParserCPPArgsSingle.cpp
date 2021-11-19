#include "common/constants.hpp"
#include "server/parser.hpp"

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <string_view>
#include <vector>

using namespace distplusplus::server::parser;
using namespace distplusplus::common;

int main() {
	std::list<std::string> argsList;
	argsList.emplace_back("-c");
	for (const auto &arg : singleArgsCPP) {
		if (std::find(singleArgsNoDistribute.begin(), singleArgsNoDistribute.end(), arg) != singleArgsNoDistribute.end()) {
			continue;
		}
		argsList.emplace_back(arg);
	}
	std::vector<std::string_view> argsVec;
	for (const auto &arg : argsList) {
		argsVec.push_back(arg);
	}
	Parser parser(argsVec);
	for (const auto &argParsed : parser.args()) {
		if (std::any_of(singleArgsCPP.begin(), singleArgsCPP.end(), [&](const auto &arg) { return argParsed == arg; })) {
			std::cout << "preprocessor argument " << argParsed << " was not filtered" << std::endl;
			return 1;
		}
	}
}
