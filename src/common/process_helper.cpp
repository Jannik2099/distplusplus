#include "process_helper.hpp"

#include "common.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <future>

namespace distplusplus::common {

ProcessHelper::ProcessHelper(const boost::filesystem::path &program, ArgsSpan args, const std::string &cin,
                             const boost::process::environment &env) {
    // a segfault in boost is really shitty to debug - assert sanity beforehand
    assertAndRaise(!program.empty(), "passed program is empty");
    assertAndRaise(program.has_filename(), "passed program " + program.string() + " is misshaped");
    assertAndRaise(boost::filesystem::is_regular_file(program),
                   "passed program " + program.string() + " does not exist");
    // TODO: find out if there's a way to construct boost args without copying?
    std::vector<std::string> rawArgs;
    rawArgs.reserve(args.size());
    for (const ArgsVec::value_type &str : args) {
        rawArgs.emplace_back(str);
    }

    boost::asio::io_service ios;
    std::future<std::string> errfut;
    std::future<std::string> outfut;
    process = boost::process::child(
        program, rawArgs, env,
        boost::process::std_in<boost::asio::buffer(cin), boost::process::std_out> outfut,
        boost::process::std_err > errfut, ios);
    ios.run();
    process.wait();
    _returnCode = process.exit_code();
    _stderr = errfut.get();
    _stdout = outfut.get();
    BOOST_LOG_TRIVIAL(trace) << "process invocation:\n"
                             << "command: " << program << '\n'
                             << "args: " <<
        [&rawArgs]() {
            std::string ret;
            for (const auto &arg : rawArgs) {
                ret += arg + ' ';
            }
            return ret;
        }() << '\n'
                             << "stdin: " << cin << '\n'
                             << "stderr: " << _stderr << "stdout: " << _stdout
                             << "exit code: " << std::to_string(_returnCode);
}

int ProcessHelper::returnCode() const { return _returnCode; }
const std::string &ProcessHelper::get_stderr() const { return _stderr; }
const std::string &ProcessHelper::get_stdout() const { return _stdout; }

} // namespace distplusplus::common