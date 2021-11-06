#include "common/common.hpp"

#undef NDEBUG
#define NDEBUG 1

using namespace distplusplus::common;

int main() {
	try {
		assertAndRaise(false, "false is false");
	} catch (std::runtime_error) {
		return 0;
	}
	return 1;
}
