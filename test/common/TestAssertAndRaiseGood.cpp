#include "common/common.hpp"

#undef NDEBUG
#define NDEBUG 1

using namespace distplusplus::common;

int main() {
	assertAndRaise(true, "this is true");
	return 0;
}
