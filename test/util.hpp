#pragma once

#include <random>
#include <string>

std::string randomString(int len = 10) {
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(0, 25);

	constexpr char letters[] = "abcdefghijklmnopqrstuvwxyz";
	char ret[len + 1];
	for (int i = 0; i < len; i++) {
		ret[i] = letters[dist(rng)];
	}
	ret[len] = '\0';
	return ret;
}
