#include "common/common.hpp"

#include <boost/process.hpp>
#include <string>
#include <vector>

using namespace distplusplus::common;

int main() {
	std::vector<std::string> args;
	ProcessHelper processHelper(boost::process::search_path("true"), args);
	return processHelper.returnCode();
}
