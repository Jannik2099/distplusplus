#pragma once

#include <filesystem>
#include <string>
#include <toml++/toml.h>
#include <vector>

namespace distplusplus::client {

class Config final {
private:
	const std::filesystem::path _stateDir;
	const toml::table configFile;
	std::vector<std::string> _servers;
	// TODO: implement
	const int _reservationAttemptTimeout = 10;

public:
	Config();

	const std::vector<std::string> &servers() const;
	const std::filesystem::path &stateDir() const;
	const int &reservationAttemptTimeout() const;
};

const extern Config config;

}; // namespace distplusplus::client
