#define TESTING_PRIVATE
#include "client/parser.hpp"
#undef TESTING_PRIVATE

#include "common/common.hpp"

#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace distplusplus::client::parser;

int main() {
	bool success = true;

	for (const auto &lang : distplusplus::common::xArgsC) {
		std::list<std::string> argsList;
		distplusplus::common::Tempfile infile("test.c");
		distplusplus::common::Tempfile outfile("test.o");
		argsList.emplace_back("cc");
		argsList.emplace_back("-x");
		argsList.emplace_back(lang);
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
		if (parser._language != distplusplus::common::Language::C) {
			std::cout << "parsed language " << std::to_string(parser._language) << " didn't match "
					  << std::to_string(distplusplus::common::Language::C) << std::endl;
			success = false;
		}
	}

	for (const auto &lang : distplusplus::common::xArgsCXX) {
		std::list<std::string> argsList;
		distplusplus::common::Tempfile infile("test.c");
		distplusplus::common::Tempfile outfile("test.o");
		argsList.emplace_back("cc");
		argsList.emplace_back("-x");
		argsList.emplace_back(lang);
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
			std::cout << "parsed language " << std::to_string(parser._language) << " didn't match "
					  << std::to_string(distplusplus::common::Language::CXX) << std::endl;
			success = false;
		}
	}

	{
		std::list<std::string> argsList;
		distplusplus::common::Tempfile infile("test.c");
		distplusplus::common::Tempfile outfile("test.o");
		argsList.emplace_back("cc");
		argsList.push_back(infile);
		argsList.emplace_back("-c");
		argsList.emplace_back("-o");
		argsList.push_back(outfile);
		argsList.emplace_back("-x");
		argsList.emplace_back("IAMABADLANG");
		std::vector<std::string_view> argsViewVec;
		for (const std::string &arg : argsList) {
			argsViewVec.push_back(arg);
		}
		distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
		Parser parser(argsSpan);
		if (parser.canDistribute()) {
			std::cout << "failed to catch invalid language" << std::endl;
			success = false;
		}
	}

	bool caught = false;
	try {
		std::list<std::string> argsList;
		distplusplus::common::Tempfile infile("test.c");
		distplusplus::common::Tempfile outfile("test.o");
		argsList.emplace_back("cc");
		argsList.push_back(infile);
		argsList.emplace_back("-c");
		argsList.emplace_back("-o");
		argsList.push_back(outfile);
		argsList.emplace_back("-x");
		std::vector<std::string_view> argsViewVec;
		for (const std::string &arg : argsList) {
			argsViewVec.push_back(arg);
		}
		distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
		Parser parser(argsSpan);
	} catch (const ParserError &) {
		caught = true;
	}
	if (!caught) {
		std::cout << "failed to catch -x multiArg as last arg" << std::endl;
		success = false;
	}

	if (!success) {
		return 1;
	}
}
