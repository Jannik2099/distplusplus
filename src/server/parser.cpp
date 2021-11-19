#include "parser.hpp"
#include "common/common.hpp"
#include "common/constants.hpp"
#include <algorithm>
#include <boost/log/trivial.hpp>
#include <execution>
#include <type_traits>

using std::execution::unseq;

using namespace distplusplus::common;

namespace distplusplus::server::parser {

std::vector<std::string_view> Parser::parse(const std::vector<std::string_view> &args) {
	std::vector<std::string_view> myargs;
	myargs.reserve(args.size());
	bool canDistribute = false;
	for (std::remove_reference<decltype(args)>::type::size_type i = 0; i < args.size(); i++) {
		if (std::any_of(unseq, singleArgsNoDistribute.begin(), singleArgsNoDistribute.end(),
						[&](const auto &arg) { return args[i] == arg; })) {
			throw CannotProcessSignal("encountered non-distributable argument " + std::string(args[i]));
		}
		if (std::any_of(unseq, singleArgsNoDistributeStartsWith.begin(), singleArgsNoDistributeStartsWith.end(),
						[&](const auto &arg) { return args[i].starts_with(arg); })) {
			throw CannotProcessSignal("encountered non-distributable argument " + std::string(args[i]));
		}
		if (std::any_of(unseq, multiArgsNoDistribute.begin(), multiArgsNoDistribute.end(),
						[&](const auto &arg) { return args[i] == arg; })) {
			throw CannotProcessSignal("encountered non-distributable argument " + std::string(args[i]));
		}
		// filter out cpp flags
		if (std::any_of(unseq, singleArgsCPP.begin(), singleArgsCPP.end(), [&](const auto &arg) { return args[i] == arg; })) {
			BOOST_LOG_TRIVIAL(trace) << "filtered preprocessor argument " << args[i];
			continue;
		}
		if (std::any_of(unseq, singleArgsCPPStartsWith.begin(), singleArgsCPPStartsWith.end(),
						[&](const auto &arg) { return args[i].starts_with(arg); })) {
			BOOST_LOG_TRIVIAL(trace) << "filtered preprocessor argument " << args[i];
			continue;
		}
		if (std::any_of(unseq, multiArgsCPP.begin(), multiArgsCPP.end(), [&](const auto &arg) { return args[i] == arg; })) {
			if (i == args.size() - 1) {
				throw CannotProcessSignal("tried to filter multi arg " + std::string(args[i]) + " but it is the last argument");
			}
			BOOST_LOG_TRIVIAL(trace) << "filtered preprocessor argument " << args[i] << " " << args[i + 1];
			i++;
			continue;
		}
		if (args[i] == "-c") {
			canDistribute = true;
		}
		myargs.push_back(args[i]);
	}
	if (!canDistribute) {
		throw CannotProcessSignal("invocation seems non-distributable - called for link?");
	}
	myargs.shrink_to_fit();
	return myargs;
}

Parser::Parser(const std::vector<std::string_view> &args) : _args(parse(args)) {}

const std::vector<std::string_view> &distplusplus::server::parser::Parser::args() const { return _args; }

} // namespace distplusplus::server::parser
