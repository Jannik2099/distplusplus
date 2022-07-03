#pragma once

#include "functional"

namespace distplusplus::common {

// normally I'd use a seperate TU for the function implementations, but it's really not worth it here

class ScopeGuard {
private:
    bool fuse = true;
    const std::function<void()> atexit;

public:
    ScopeGuard() = delete;
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard(ScopeGuard &&) = delete;
    ScopeGuard operator=(const ScopeGuard &) = delete;
    ScopeGuard operator=(ScopeGuard &&) = delete;
    ScopeGuard(std::function<void()> atexit) : atexit(std::move(atexit)) {}
    ~ScopeGuard() {
        if (fuse) {
            atexit();
        }
    }
    void defuse() { fuse = false; }
};

} // namespace distplusplus::common