#include "common/common.hpp"
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
		tempfile.disable_cleanup();
		filename = tempfile;
	}
	if (!std::filesystem::exists(filename)) {
		std::cout << "file " << filename << " does not exist" << std::endl;
		return 1;
	}
	std::filesystem::remove(filename);
	return 0;
}
