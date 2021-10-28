#pragma once

#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace distplusplus::server::parser {

class CannotProcessSignal final {
private:
	const std::string error;

public:
	CannotProcessSignal(std::string error) : error(std::move(error)) {}
	[[nodiscard]] const std::string &what() const { return error; }
};

class Parser final {
private:
	const std::vector<std::string_view> _args;
	static std::vector<std::string_view> parse(const std::vector<std::string_view> &args);

public:
	explicit Parser(const std::vector<std::string_view> &args);
	[[nodiscard]] const std::vector<std::string_view> &args() const;
};

} // namespace distplusplus::server::parser
