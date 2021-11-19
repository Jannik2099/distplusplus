#include "common/common.hpp"

using namespace distplusplus::common;

int main() {
	bool control = false;
	{
		ScopeGuard scopeGuard([&control]() { control = true; });
	}
	if (control) {
		return 0;
	}
	return 1;
}
