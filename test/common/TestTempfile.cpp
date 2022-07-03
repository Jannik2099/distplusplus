#include "common/tempfile.hpp"
#include "util.hpp"

#include <filesystem>
#include <iostream>
#include <string>

using namespace distplusplus::common;

int main() {
    const std::string control = randomString();
    std::string filename;
    {
        Tempfile tempfile(control);
        filename = tempfile;
    }
    if (std::filesystem::exists(filename)) {
        std::cout << "file " << filename << " still exists" << std::endl;
        return 1;
    }
    return 0;
}
