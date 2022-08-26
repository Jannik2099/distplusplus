#include "common/tempfile.hpp"
#include "util.hpp"

#include <filesystem>
#include <iostream>
#include <string>

using namespace distplusplus::common;

int main() {
    const std::string control = randomString();
    Tempfile tempfile(control);
    const std::string filename = tempfile;
    if (!std::filesystem::exists(filename)) {
        std::cout << "file " << filename << " was not created" << std::endl;
        return 1;
    }
    return 0;
}
