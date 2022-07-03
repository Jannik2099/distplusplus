#include "common/scopeguard.hpp"

using namespace distplusplus::common;

int main() {
    bool control = false;
    {
        ScopeGuard scopeGuard([&control]() { control = true; });
        scopeGuard.defuse();
    }
    if (control) {
        return 1;
    }
    return 0;
}
