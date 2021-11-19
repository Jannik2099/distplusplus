#include "server/parser.hpp"

#include <string>
#include <string_view>
#include <vector>

using namespace distplusplus::server::parser;

int main() {
	const std::string arg = "test";
	const std::vector<std::string_view> argsVec = {arg};
	try {
		Parser parser(argsVec);
	} catch (const CannotProcessSignal &sig) {
		return 0;
	}
	return 1;
}
