#pragma once

#include <exception>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/common.hpp"
#include "common/constants.hpp"

using distplusplus::common::BoundsSpan;
using distplusplus::common::Language;
using std::filesystem::path;

namespace distplusplus::client::parser {

class Parser final {
private:
	std::vector<std::string_view> _args;
	path _infile;
	path _outfile;
	std::string _compiler;
	std::optional<std::string> _target;
	std::optional<bool> _canDistribute;
	Language _language = Language::NONE;

	void checkInputFileCandidate(const std::string_view &file);
	void readArgsFile(const path &argsFile);
	void parseArgs(const BoundsSpan<std::string_view> &args);

public:
	explicit Parser(BoundsSpan<std::string_view> &args);
	Parser(const Parser &) = delete;
	Parser(Parser &&) = delete;
	Parser operator=(const Parser &) = delete;
	Parser operator=(Parser &&) = delete;
	~Parser() = default;
	[[nodiscard]] const path &infile() const;
	[[nodiscard]] const path &outfile() const;
	[[nodiscard]] const std::optional<std::string> &target() const;
	// TODO: return BoundsSpan?
	[[nodiscard]] const std::vector<std::string_view> &args() const;
	[[nodiscard]] bool canDistribute() const;
};

class ParserError final : public std::exception {
private:
	std::string error;
	void init(const Parser &parser, const std::string &preamble);

public:
	ParserError(const Parser &parser, const std::string &preamble) noexcept;

	[[nodiscard]] const char *what() const noexcept final;
};

} // namespace distplusplus::client::parser
