#include "common/common.hpp"

#include <iostream>
#include <tuple>
#include <utility>
#include <vector>

using distplusplus::CompilerType;

int main() {
    // took a dump of my /usr/lib/distcc
    const std::vector<std::pair<std::string, CompilerType>> compilers = {
        {"c++", CompilerType::UNKNOWN},
        {"c99", CompilerType::UNKNOWN},
        {"cc", CompilerType::UNKNOWN},
        {"clang", CompilerType::clang},
        {"clang++", CompilerType::clang},
        {"clang++-12", CompilerType::clang},
        {"clang-12", CompilerType::clang},
        {"g++", CompilerType::gcc},
        {"g++-11.2.0", CompilerType::gcc},
        {"gcc", CompilerType::gcc},
        {"gcc-11.2.0", CompilerType::gcc},
        {"x86_64-pc-linux-gnu-c++", CompilerType::UNKNOWN},
        {"x86_64-pc-linux-gnu-cc", CompilerType::UNKNOWN},
        {"x86_64-pc-linux-gnu-clang", CompilerType::clang},
        {"x86_64-pc-linux-gnu-clang++", CompilerType::clang},
        {"x86_64-pc-linux-gnu-clang++-12", CompilerType::clang},
        {"x86_64-pc-linux-gnu-clang-12", CompilerType::clang},
        {"x86_64-pc-linux-gnu-g++", CompilerType::gcc},
        {"x86_64-pc-linux-gnu-g++-11.2.0", CompilerType::gcc},
        {"x86_64-pc-linux-gnu-gcc", CompilerType::gcc},
        {"x86_64-pc-linux-gnu-gcc-11.2.0", CompilerType::gcc}};

    for (const auto &pair : compilers) {
        const auto compiler = std::get<0>(pair);
        const auto control = std::get<1>(pair);
        const auto res = distplusplus::common::mapCompiler(compiler);
        if (res != control) {
            std::cout << "failed to detect compiler " << compiler << std::endl;
            std::cout << "expected " << control << std::endl;
            std::cout << "got " << res << std::endl;
            return 1;
        }
    }
    return 0;
}
