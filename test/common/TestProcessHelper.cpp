#include "common/argsvec.hpp"
#include "common/process_helper.hpp"

#include <boost/process.hpp>
#include <string>
#include <vector>

using namespace distplusplus::common;

int main() {
    ArgsVec args;
    ProcessHelper processHelper(boost::process::search_path("true"), args);
    return processHelper.returnCode();
}
