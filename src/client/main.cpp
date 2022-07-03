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
#include <cstdio>
#include <fcntl.h>
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
using distplusplus::common::ArgsSpan;
using distplusplus::common::ArgsVec;
using distplusplus::common::BoundsSpan;
using distplusplus::common::ProcessHelper;
using distplusplus::common::Tempfile;

// Whoever the fuck designed C / POSIX IO and thought "yeah this is good" should be tied for digital war
// crimes and thrown in the Sarlacc pit
static bool stdin_empty() {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
    char buf[1];

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    const int oldflags = fcntl(fileno(stdin), F_GETFL);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
    const ssize_t num_read = read(fileno(stdin), &buf, 1);
    const int err = errno;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    fcntl(fileno(stdin), F_SETFL, oldflags);

    if (err == EAGAIN || err == EWOULDBLOCK) {
        return true;
    }

    if (num_read == 0) {
        return true;
    }
    ungetc(buf[0], stdin);
    return false;
}

static int func(ArgsSpan argv) { // NOLINT(readability-function-cognitive-complexity)
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

    ArgsSpan compilerArgs = argv.subspan(compilerPos + 1);
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
        std::cerr << preprocessorStderr;
    }
    if (Preprocessor.returnCode() != 0) {
        return Preprocessor.returnCode();
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
        std::cout << answer.stderr();
    }
    if (!answer.stdout().empty()) {
        std::cout << answer.stdout();
    }
    BOOST_LOG_TRIVIAL(trace) << "remote invocation returned " << std::to_string(answer.returncode());
    return answer.returncode();
}

// NOLINTNEXTLINE(bugprone-exception-escape, readability-function-cognitive-complexity)
int main(int argc, char *argv[]) {
    distplusplus::common::initBoostLogging();
    ArgsVec argsVec;
    for (char *arg : BoundsSpan(argv, argc)) {
        argsVec.emplace_back(Arg::fromExternal(arg));
    }
    ArgsSpan argsSpan(argsVec);
    int ret = 0;
    // uncaught exceptions are not guaranteed to invoke destructors
    // hence catch and rethrow
    try {
        if (!stdin_empty()) {
            BOOST_LOG_TRIVIAL(info) << "cannot distribute because stdin is not empty";
            throw FallbackSignal();
        }
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

        const auto cin = stdin_empty() ? "" : std::string(std::istreambuf_iterator<char>(std::cin), {});
        ArgsSpan compilerArgs(argsSpan.begin() + compilerPos + 1, argsSpan.end());
        ProcessHelper localInvocation(compilerPath, compilerArgs, cin);
        std::cerr << localInvocation.get_stderr();
        std::cout << localInvocation.get_stdout();
        BOOST_LOG_TRIVIAL(trace) << "local invocation returned "
                                 << std::to_string(localInvocation.returnCode());
        return localInvocation.returnCode();
    }
    // TODO: this destroys the call stack. Perhaps find another solution to calling Tempfile destructors?
    catch (...) {
        throw;
    }
    return ret;
}
