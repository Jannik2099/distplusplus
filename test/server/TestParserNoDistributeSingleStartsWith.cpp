#include "common/constants.hpp"
#include "server/parser.hpp"

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <string_view>
#include <vector>

using namespace distplusplus::server::parser;
using distplusplus::common::singleArgsNoDistributeStartsWith;

int main() {
	std::list<std::string> argsList;
	argsList.emplace_back("-c");
	for (const auto &arg : singleArgsNoDistributeStartsWith) {
		std::vector<std::string_view> argsVec;
		argsList.emplace_back(std::string(arg) + "test");
		for (const auto &arg2 : argsList) {
			argsVec.push_back(arg2);
		}
		try {
			Parser parser(argsVec);
		} catch (const CannotProcessSignal &sig) {
			argsList.pop_back();
			continue;
		}
		std::cout << "parser didn't throw on no distrubute arg " << arg << std::endl;
		return 1;
	}
}
