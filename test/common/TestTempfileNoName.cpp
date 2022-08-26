#include "common/tempfile.hpp"

#include <stdexcept>

using namespace distplusplus::common;

int main() {
    try {
        Tempfile tempfile("");
    } catch (const std::invalid_argument &) {
        return 0;
    }
    return 1;
}
