#include "common/common.hpp"

#include <grpc/status.h>
#include <iostream>

using grpc::StatusCode;
using namespace distplusplus::common;

#define XSTRINGIFY(s) STRINGIFY(s) // NOLINT
#define STRINGIFY(s) #s            // NOLINT
// NOLINTNEXTLINE
#define checkStatusMap(statusCode)                                                                           \
    if (mapGRPCStatus(StatusCode::statusCode) != XSTRINGIFY(statusCode)) {                                   \
        ret = 1;                                                                                             \
        std::cerr << "returned status code " << mapGRPCStatus(StatusCode::statusCode) << " didn't match "    \
                  << XSTRINGIFY(statusCode) << std::endl;                                                    \
    }

// This test is pretty meaningless and exists just to drive up coverage numbers
int main() {
    int ret = 0;
    checkStatusMap(DO_NOT_USE);
    checkStatusMap(OK);
    checkStatusMap(CANCELLED);
    checkStatusMap(UNKNOWN);
    checkStatusMap(INVALID_ARGUMENT);
    checkStatusMap(DEADLINE_EXCEEDED);
    checkStatusMap(NOT_FOUND);
    checkStatusMap(ALREADY_EXISTS);
    checkStatusMap(PERMISSION_DENIED);
    checkStatusMap(RESOURCE_EXHAUSTED);
    checkStatusMap(FAILED_PRECONDITION);
    checkStatusMap(ABORTED);
    checkStatusMap(OUT_OF_RANGE);
    checkStatusMap(UNIMPLEMENTED);
    checkStatusMap(INTERNAL);
    checkStatusMap(UNAVAILABLE);
    checkStatusMap(DATA_LOSS);
    checkStatusMap(UNAUTHENTICATED);
    bool caught = false;
    try {
        mapGRPCStatus(static_cast<StatusCode>(
            99999)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    } catch (const std::runtime_error &) {
        caught = true;
    }
    if (!caught) {
        ret = 1;
    }
    return ret;
}
