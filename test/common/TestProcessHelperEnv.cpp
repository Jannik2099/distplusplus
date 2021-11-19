#include "common/common.hpp"
#include "util.hpp"
#include <boost/process.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace distplusplus::common;

int main() {
	std::string control = randomString();
	boost::process::environment env = boost::this_process::environment();
	env["MY_CONTROL_VAR"] = control;
	std::vector<std::string> args = {"-c", "echo -n ${MY_CONTROL_VAR}"};
	ProcessHelper processHelper(boost::process::search_path("bash"), args, env);
	if (processHelper.returnCode() != 0) {
		std::cout << "process returned " << std::to_string(processHelper.returnCode()) << std::endl;
		return 1;
	}
	if (processHelper.get_stdout() == control) {
		return 0;
	}
	std::cout << "expected stdout " << control << " but got " << processHelper.get_stdout() << std::endl;
	return 1;
}
