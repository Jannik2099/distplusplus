#include "client/parser.hpp"
#include "common/common.hpp"

#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace distplusplus::client::parser;

int main() {
	bool success = true;

	{
		std::list<std::string> argsList;
		distplusplus::common::Tempfile infile("test.c");
		distplusplus::common::Tempfile outfile("test.o");
		argsList.emplace_back("cc");
		argsList.push_back(infile);
		argsList.emplace_back("-c");
		argsList.emplace_back("-o");
		argsList.push_back(outfile);
		argsList.emplace_back("--target=IAMATARGET");
		std::vector<std::string_view> argsViewVec;
		for (const std::string &arg : argsList) {
			argsViewVec.push_back(arg);
		}
		distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
		Parser parser(argsSpan);
		if (parser.target().value_or("IAMNOTTHETARGET") != "IAMATARGET") {
			std::cout << "failed to detect target from single arg" << std::endl;
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
		argsList.emplace_back("-target");
		argsList.emplace_back("IAMATARGET");
		std::vector<std::string_view> argsViewVec;
		for (const std::string &arg : argsList) {
			argsViewVec.push_back(arg);
		}
		distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
		Parser parser(argsSpan);
		if (parser.target().value_or("IAMNOTTHETARGET") != "IAMATARGET") {
			std::cout << "failed to detect target from single arg" << std::endl;
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
		argsList.emplace_back("-target");
		std::vector<std::string_view> argsViewVec;
		for (const std::string &arg : argsList) {
			argsViewVec.push_back(arg);
		}
		distplusplus::common::BoundsSpan argsSpan(argsViewVec.begin(), argsViewVec.end());
		bool caught = false;
		try {
			Parser parser(argsSpan);
		} catch (const ParserError &) {
			caught = true;
		}
		if (!caught) {
			std::cout << "failed to catch -target multiArg as last arg" << std::endl;
			success = false;
		}
	}

	if (!success) {
		return 1;
	}
}
