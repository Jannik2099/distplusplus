#include "common/common.hpp"

#include <stdexcept>

using namespace distplusplus::common;

int main() {
    try {
        Tempfile tempfile("/bad");
    } catch (std::invalid_argument) {
        return 0;
    }
    return 1;
}
