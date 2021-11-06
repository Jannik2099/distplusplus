#include "common/common.hpp"
#include "util.hpp"
#include <string>
#include <iostream>
#include <filesystem>

using namespace distplusplus::common;

int main() {
	const std::string control = randomString();
	std::string filename;
	{
		Tempfile tempfile(control);
		filename = tempfile;
	}
	if(std::filesystem::exists(filename)) {
		std::cout << "file " << filename << " still exists" << std::endl;
		return 1;
	}
	return 0;
}
