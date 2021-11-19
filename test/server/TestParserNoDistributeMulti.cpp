#include "common/constants.hpp"
#include "server/parser.hpp"

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <string_view>
#include <vector>

using namespace distplusplus::server::parser;
using distplusplus::common::multiArgsNoDistribute;

int main() {
	std::list<std::string> argsList;
	argsList.emplace_back("-c");
	for (const auto &arg : multiArgsNoDistribute) {
		std::vector<std::string_view> argsVec;
		argsList.emplace_back(arg);
		argsList.emplace_back("test");
		for (const auto &arg2 : argsList) {
			argsVec.push_back(arg2);
		}
		try {
			Parser parser(argsVec);
		} catch (const CannotProcessSignal &sig) {
			argsList.pop_back();
			argsList.pop_back();
			continue;
		}
		std::cout << "parser didn't throw on no distrubute arg " << arg << std::endl;
		return 1;
	}
}
