#include "common/tempfile.hpp"
#include "util.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace distplusplus::common;

int main() {
    const std::string control = randomString();
    const std::string content = randomString();
    Tempfile tempfile(control, "", content);

    std::ifstream fileStream(tempfile);
    std::stringstream stringStream;
    stringStream << fileStream.rdbuf();
    fileStream.close();

    if (stringStream.str() != content) {
        std::cout << "file content " << stringStream.str() << " did not match " << content << std::endl;
        return 1;
    }
    return 0;
}
