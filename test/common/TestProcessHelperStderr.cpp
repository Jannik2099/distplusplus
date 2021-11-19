#include "common/common.hpp"
#include "util.hpp"
#include <boost/process.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace distplusplus::common;

int main() {
	std::string control = randomString();
	std::vector<std::string> args = {"-c", "echo -n " + control + " 1>&2"};
	ProcessHelper processHelper(boost::process::search_path("bash"), args);
	if (processHelper.returnCode() != 0) {
		std::cout << "process returned " << std::to_string(processHelper.returnCode()) << std::endl;
		return 1;
	}
	if (processHelper.get_stderr() == control) {
		return 0;
	}
	std::cout << "expected stderr " << control << " but got " << processHelper.get_stderr() << std::endl;
	return 1;
}
