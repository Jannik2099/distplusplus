#undef NDEBUG

#include "common/common.hpp"

using namespace distplusplus::common;

int main() {
    assertAndRaise(true, "this is true");
    return 0;
}
