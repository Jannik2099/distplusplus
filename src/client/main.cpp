#include "client.hpp"
#include "common/argsvec.hpp"
#include "common/common.hpp"
#include "common/compression_helper.hpp"
#include "config.hpp"
#include "distplusplus.grpc.pb.h"
#include "distplusplus.pb.h"
#include "fallback.hpp"
#include "parser.hpp"

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
#include <utility>
#include <vector>

using namespace distplusplus::client;
using distplusplus::common::Arg;
using distplusplus::common::ArgsVec;
using distplusplus::common::ArgsVecSpan;
using distplusplus::common::BoundsSpan;
using distplusplus::common::ProcessHelper;
using distplusplus::common::Tempfile;

static int func(ArgsVecSpan argv) { // NOLINT(readability-function-cognitive-complexity)
    if (argv.size() == 1 && std::filesystem::path(argv[0]).stem() == "distplusplus") {
        BOOST_LOG_TRIVIAL(error) << "distplusplus invoked without any arguments";
        return -1;
    }
    const int compilerPos = (std::filesystem::path(argv[0]).stem() == "distplusplus") ? 1 : 0;
    std::string compilerName = std::filesystem::path(argv[compilerPos]).stem();
    const std::string cwd = std::filesystem::current_path();

    const distplusplus::CompilerType compilerType = distplusplus::common::mapCompiler(compilerName);
    if (compilerType == distplusplus::CompilerType::UNKNOWN) {
        // TODO: see how far this gets
        BOOST_LOG_TRIVIAL(warning) << "failed to recognize compiler " << compilerName
                                   << " , attempting to proceed";
    }

    ArgsVecSpan compilerArgs = argv.subspan(compilerPos + 1);
    const parser::Parser &parser = [compilerArgs]() {
        try {
            return parser::Parser(compilerArgs);
        } catch (const parser::ParserError &e) {
            BOOST_LOG_TRIVIAL(warning)
                << "caught error in parser: \"" << e.what() << "\" - trying local fallback";
            throw FallbackSignal();
        }
    }();
    ArgsVec args = parser.args();

    if (!parser.canDistribute()) {
        throw FallbackSignal();
    }

#define XSTRINGIFY(s) STRINGIFY(s) // NOLINT
#define STRINGIFY(s) #s            // NOLINT

    if (compilerType == distplusplus::CompilerType::clang) {
        if (!parser.target().has_value()) {
            args.emplace_back("-target");
            args.emplace_back(XSTRINGIFY(DISTPLUSPLUS_BUILD_TARGET));
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
        for (const Arg &arg : args) {
            ret.append(" ");
            ret.append(arg);
        }
        return ret;
    }();

    const std::filesystem::path &cppInfile = parser.infile();
    const std::string fileName = cppInfile.filename();
    const Tempfile cppOutfile(std::string(parser.infile().stem()) + ".i");

    args.emplace_back(cppInfile.c_str());
    args.emplace_back("-o");
    args.emplace_back(cppOutfile.c_str());
    args.emplace_back("-E");
    ProcessHelper Preprocessor(boost::process::search_path(compilerName), args);
    args.pop_back();
    args.pop_back();
    args.pop_back();
    args.pop_back();
    args.emplace_back(parser.modeArg());

    std::ifstream cppOutfileStream(cppOutfile);
    std::string cppOutfileContent((std::istreambuf_iterator<char>(cppOutfileStream)),
                                  std::istreambuf_iterator<char>());
    // TODO: could stream directly into compressor
    const distplusplus::common::Compressor compressor(config.compressionType(), config.compressionLevel(),
                                                      cppOutfileContent);

    const std::string &preprocessorStderr = Preprocessor.get_stderr();
    if (!preprocessorStderr.empty()) {
        std::cerr << preprocessorStderr << std::endl;
    }

    Client client = ClientFactory(compilerName);
    const distplusplus::CompileAnswer answer = client.send(args, fileName, compressor.data(), cwd);
    if (!answer.outputfile().content().empty()) {
        const distplusplus::common::Decompressor decompressor(answer.outputfile().compressiontype(),
                                                              answer.outputfile().content());
        std::ofstream outStream(parser.outfile());
        outStream << decompressor.data();
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

int main(int argc, char *argv[]) { // NOLINT(bugprone-exception-escape)
    distplusplus::common::initBoostLogging();
    ArgsVec argsVec;
    for (char *arg : BoundsSpan(argv, argc)) {
        argsVec.emplace_back(Arg::fromExternal(arg));
    }
    ArgsVecSpan argsSpan(argsVec);
    int ret = 0;
    // uncaught exceptions are not guaranteed to invoke destructors
    // hence catch and rethrow
    try {
        ret = func(argsSpan);
    } catch (FallbackSignal) {
        if (!config.fallback()) {
            BOOST_LOG_TRIVIAL(error) << "failed to distribute and local fallback is disabled, exiting";
            return -1;
        }
        const int compilerPos = (std::filesystem::path(argsSpan[0]).stem() == "distplusplus") ? 1 : 0;
        std::string compilerPath(argsSpan[compilerPos]);
        // otherwise we would recurse on ourself
        // TODO: the conversion to path & c_str is kinda hacky, should probably fix it at some point
        if (compilerPos == 0) {
            if (compilerPath.starts_with("/usr/libexec/distplusplus") ||
                compilerPath.starts_with("/usr/lib/distcc")) {
                compilerPath =
                    boost::process::search_path(std::filesystem::path(compilerPath).stem().c_str()).c_str();
            }
        }
        if (std::filesystem::path(compilerPath).stem() == compilerPath) {
            compilerPath =
                boost::process::search_path(std::filesystem::path(compilerPath).stem().c_str()).c_str();
        }

        if (!std::filesystem::exists(compilerPath)) {
            BOOST_LOG_TRIVIAL(error) << "specified compiler " << argsSpan[compilerPos].c_str()
                                     << " not found";
            return -1;
        }

        ArgsVecSpan compilerArgs(argsSpan.begin() + compilerPos + 1, argsSpan.end());
        ProcessHelper localInvocation(compilerPath, compilerArgs);
        std::cerr << localInvocation.get_stderr() << std::endl;
        std::cout << localInvocation.get_stdout() << std::endl;
        return localInvocation.returnCode();
    }
    // TODO: this destroys the call stack. Perhaps find another solution to calling Tempfile destructors?
    catch (...) {
        throw;
    }
    return ret;
}
