#undef NDEBUG

#include "common/common.hpp"

using namespace distplusplus::common;

int main() {
	try {
		assertAndRaise(false, "false is false");
	} catch (std::runtime_error) {
		return 0;
	}
	return 1;
}
