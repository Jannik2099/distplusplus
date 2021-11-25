#include "client/fallback.hpp"
#include "client/parser.hpp"
#include "common/common.hpp"
#include "common/constants.hpp"

#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace distplusplus::client::parser;
using namespace distplusplus::common;
using distplusplus::client::FallbackSignal;

int main() {
	bool success = true;
	for (const auto &arg : singleArgsNoDistribute) {
		bool caught = false;
		std::list<std::string> argsList;
		Tempfile infile("test.c");
		Tempfile outfile("test.o");
		argsList.emplace_back("cc");
		argsList.push_back(infile);
		argsList.emplace_back("-c");
		argsList.emplace_back("-o");
		argsList.push_back(outfile);
		argsList.push_back(arg);
		std::vector<std::string_view> argsViewVec;
		for (const std::string &arg : argsList) {
			argsViewVec.push_back(arg);
		}
		BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
		try {
			Parser parser(argsSpan);
		} catch (FallbackSignal &) {
			caught = true;
		}
		if (!caught) {
			success = false;
			std::cout << "bad argument " << arg << " didn't throw FallbackSignal" << std::endl;
		}
	}
	for (const auto &arg : singleArgsNoDistributeStartsWith) {
		bool caught = false;
		std::list<std::string> argsList;
		Tempfile infile("test.c");
		Tempfile outfile("test.o");
		argsList.emplace_back("cc");
		argsList.push_back(infile);
		argsList.emplace_back("-c");
		argsList.emplace_back("-o");
		argsList.push_back(outfile);
		argsList.push_back(arg);
		std::vector<std::string_view> argsViewVec;
		for (const std::string &arg : argsList) {
			argsViewVec.push_back(arg);
		}
		BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
		try {
			Parser parser(argsSpan);
		} catch (FallbackSignal &) {
			caught = true;
		}
		if (!caught) {
			success = false;
			std::cout << "bad argument " << arg << " didn't throw FallbackSignal" << std::endl;
		}
	}
	for (const auto &arg : multiArgsNoDistribute) {
		bool caught = false;
		std::list<std::string> argsList;
		Tempfile infile("test.c");
		Tempfile outfile("test.o");
		argsList.emplace_back("cc");
		argsList.push_back(infile);
		argsList.emplace_back("-c");
		argsList.emplace_back("-o");
		argsList.push_back(outfile);
		argsList.push_back(arg);
		argsList.push_back("test");
		std::vector<std::string_view> argsViewVec;
		for (const std::string &arg : argsList) {
			argsViewVec.push_back(arg);
		}
		BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
		try {
			Parser parser(argsSpan);
		} catch (FallbackSignal &) {
			caught = true;
		}
		if (!caught) {
			success = false;
			std::cout << "bad argument " << arg << " didn't throw FallbackSignal" << std::endl;
		}
	}
	if (!success) {
		return 1;
	}
}
