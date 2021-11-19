#include "server/parser.hpp"

#include <string>
#include <string_view>
#include <vector>

using namespace distplusplus::server::parser;

int main() {
	const std::string arg = "-c";
	const std::vector<std::string_view> argsVec = {arg};
	Parser parser(argsVec);
}
