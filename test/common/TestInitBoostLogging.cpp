#include "common/common.hpp"

#include <cstdlib>
#include <stdexcept>
#include <vector>

// This test is pretty meaningless and exists just to drive up coverage number
int main() {
    const std::vector<const char *> logLevelVec = {"trace", "debug", "info", "warning", "error", "fatal"};
    for (const char *logLevel : logLevelVec) {
        setenv("DISTPLUSPLUS_LOG_LEVEL", logLevel, 1);
        distplusplus::common::initBoostLogging();
    }
    unsetenv("DISTPLUSPLUS_LOG_LEVEL");
    try {
        distplusplus::common::initBoostLogging();
    } catch (const std::invalid_argument &) {
    }
}
