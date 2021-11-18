#include "client.hpp"
#include "common/common.hpp"
#include "fallback.hpp"
#include "parser.hpp"
#include "distplusplus.grpc.pb.h"
#include "distplusplus.pb.h"
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

using namespace distplusplus::client;
using distplusplus::common::ProcessHelper;
using distplusplus::common::Tempfile;

static int func(common::BoundsSpan<std::string_view> &argv) {
	if (argv.size() == 1 && std::filesystem::path(argv[0]).stem() == "distplusplus") {
		BOOST_LOG_TRIVIAL(error) << "distplusplus invoked without any arguments";
		return 1;
	}
	const int compilerPos = (std::filesystem::path(argv[0]).stem() == "distplusplus") ? 1 : 0;
	std::string compilerName = std::filesystem::path(argv[compilerPos]).stem();
	const std::string cwd = std::filesystem::current_path();

	const distplusplus::CompilerType compilerType = distplusplus::common::mapCompiler(compilerName);
	if (compilerType == distplusplus::CompilerType::UNKNOWN) {
		// TODO: see how far this gets
		BOOST_LOG_TRIVIAL(warning) << "failed to recognize compiler " << compilerName << " , attempting to proceed";
	}

	auto myspan = argv.subspan(compilerPos + 1);
	auto &spanref = myspan;
	const parser::Parser parser(spanref);
	std::vector<std::string_view> args = parser.args();
	std::list<std::string> argsStore;

	if (!parser.canDistribute()) {
		throw FallbackSignal();
	}

#define XSTRINGIFY(s) STRINGIFY(s)
#define STRINGIFY(s) #s
	if (compilerType == distplusplus::CompilerType::clang) {
		if (!parser.target().has_value()) {
			argsStore.emplace_back(std::string("-target"));
			args.emplace_back(std::string_view(argsStore.back()));
			argsStore.emplace_back(std::string(XSTRINGIFY(DISTPLUSPLUS_BUILD_TARGET)));
			args.emplace_back(std::string_view(argsStore.back()));
			BOOST_LOG_TRIVIAL(debug) << "set clang target to " << XSTRINGIFY(DISTPLUSPLUS_BUILD_TARGET);
		}
	} else if (compilerType == distplusplus::CompilerType::gcc) {
		const std::string gccPrefix(XSTRINGIFY(DISTPLUSPLUS_BUILD_TARGET));
		// only override unspecific gcc ( + version), i.e. not if user set target
		if (compilerName.starts_with("g++")) {
			compilerName = gccPrefix + "-" + compilerName;
			BOOST_LOG_TRIVIAL(debug) << "set unspecified g++ target to " << compilerName;
		} else if (compilerName.starts_with("gcc")) {
			compilerName = gccPrefix + "-" + compilerName;
			BOOST_LOG_TRIVIAL(debug) << "set unspecified gcc target to " << compilerName;
		}
	}
	BOOST_LOG_TRIVIAL(trace) << "args:" << [&args]() {
		std::string ret;
		for (const auto &arg : args) {
			ret.append(" ");
			ret.append(std::string(arg));
		}
		return ret;
	}();

	const std::filesystem::path &cppInfile = parser.infile();
	const std::string fileName = cppInfile.filename();
	const Tempfile cppOutfile(std::string(parser.infile().stem()) + ".i");
	argsStore.emplace_back(std::string(cppInfile));
	args.emplace_back(argsStore.back());
	argsStore.emplace_back("-o");
	args.emplace_back(argsStore.back());
	argsStore.emplace_back(std::string(cppOutfile));
	args.emplace_back(argsStore.back());
	const std::string cpp = (compilerType == distplusplus::CompilerType::clang) ? "clang-cpp" : "cpp";
	std::vector<std::string> argsVec;
	argsVec.reserve(args.size());
	for (const auto &arg : args) {
		argsVec.emplace_back(std::string(arg));
	}
	ProcessHelper Preprocessor(boost::process::search_path(cpp), argsVec);
	args.pop_back();
	args.pop_back();
	args.pop_back();
	argsVec.pop_back();
	argsVec.pop_back();
	argsVec.pop_back();

	std::ifstream cppOutfileStream(cppOutfile);
	std::string cppOutfileContent((std::istreambuf_iterator<char>(cppOutfileStream)), std::istreambuf_iterator<char>());

	const std::string &preprocessorStderr = Preprocessor.get_stderr();
	if (!preprocessorStderr.empty()) {
		std::cerr << preprocessorStderr << std::endl;
	}

	Client client;
	const distplusplus::CompileAnswer answer = client.send(compilerName, argsVec, fileName, cppOutfileContent, cwd);
	if (!answer.outputfile().content().empty()) {
		std::ofstream outStream(parser.outfile());
		outStream << answer.outputfile().content();
		outStream.close();
	}
	if (!answer.stderr().empty()) {
		std::cout << answer.stderr() << std::endl;
	}
	if (!answer.stdout().empty()) {
		std::cout << answer.stdout() << std::endl;
	}
	return answer.returncode();
}

int main(int argc, char *argv[]) {
	distplusplus::common::initBoostLogging();
	std::vector<std::string_view> viewVec;
	for (const auto &arg : BoundsSpan(argv, argc)) {
		viewVec.emplace_back(std::string_view(arg));
	}
	common::BoundsSpan argsSpan(viewVec);
	int ret = 0;
	// uncaught exceptions are not guaranteed to invoke destructors
	// hence catch and rethrow
	try {
		ret = func(argsSpan);
	} catch (FallbackSignal) {
		const int compilerPos = (std::filesystem::path(argsSpan[0]).stem() == "distplusplus") ? 1 : 0;
		std::string compilerPath(argsSpan[compilerPos]);
		std::vector<std::string> args;
		for (const auto &arg : argsSpan.subspan(compilerPos + 1)) {
			args.emplace_back(arg);
		}

		if(!std::filesystem::exists(compilerPath)) {
			return -1;
		}
		ProcessHelper localInvocation(compilerPath, args);
		std::cerr << localInvocation.get_stderr() << std::endl;
		std::cout << localInvocation.get_stdout() << std::endl;
		return localInvocation.returnCode();
	} catch (...) {
		throw;
	}
	return ret;
}
