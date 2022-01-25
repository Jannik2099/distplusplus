#include "common/argsvec.hpp"
#include "common/common.hpp"

#include <boost/process.hpp>
#include <string>
#include <vector>

using namespace distplusplus::common;

int main() {
    ArgsVec args;
    ProcessHelper processHelper(boost::process::search_path("false"), args);
    if (processHelper.returnCode() != 0) {
        return 0;
    }
    return 1;
}
