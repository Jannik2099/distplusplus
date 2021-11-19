#include "common/constants.hpp"
#include "server/parser.hpp"

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <string_view>
#include <vector>

using namespace distplusplus::server::parser;
using distplusplus::common::singleArgsCPPStartsWith;

int main() {
	std::list<std::string> argsList;
	argsList.emplace_back("-c");
	for (const auto &arg : singleArgsCPPStartsWith) {
		argsList.emplace_back(std::string(arg) + "test");
	}
	std::vector<std::string_view> argsVec;
	for (const auto &arg : argsList) {
		argsVec.push_back(arg);
	}
	Parser parser(argsVec);
	for (const auto &argParsed : parser.args()) {
		if (std::any_of(singleArgsCPPStartsWith.begin(), singleArgsCPPStartsWith.end(),
						[&](const auto &arg) { return argParsed.starts_with(arg); })) {
			std::cout << "preprocessor argument " << argParsed << " was not filtered" << std::endl;
			return 1;
		}
	}
}
