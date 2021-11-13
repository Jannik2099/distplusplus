#define TESTING_PRIVATE
#include "client/parser.hpp"
#undef TESTING_PRIVATE
#include "common/common.hpp"
#include "common/constants.hpp"
#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace distplusplus::client::parser;

int main() {
	bool success = true;
	for (const auto &extension : distplusplus::common::inputFileExtensionCXX) {
		std::list<std::string> argsList;
		distplusplus::common::Tempfile infile("test" + std::string(extension));
		distplusplus::common::Tempfile outfile("test.o");
		argsList.emplace_back("c++");
		argsList.push_back(infile);
		argsList.emplace_back("-c");
		argsList.emplace_back("-o");
		argsList.push_back(outfile);
		std::vector<std::string_view> argsViewVec;
		for (const std::string &arg : argsList) {
			argsViewVec.push_back(arg);
		}
		distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
		Parser parser(argsSpan);
		if (parser._language != distplusplus::common::Language::CXX) {
			success = false;
			std::cout << "input file extension " << extension << " was recognized as " << parser._language << std::endl;
		}
	}
	if(!success) {
		return 1;
	}
}
