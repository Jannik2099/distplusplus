#pragma once

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <filesystem>
#include <functional>
#include <grpcpp/support/status_code_enum.h>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "distplusplus.pb.h"

using std::filesystem::path;

namespace distplusplus::common {

[[maybe_unused]] static void assertAndRaise(bool condition, const std::string &msg) {
#ifdef NDEBUG
	return;
#else
	if (condition) [[likely]] {
		return;
	}
	throw std::runtime_error("assertion failed: " + msg);
#endif
}

void initBoostLogging();

template <class T, std::size_t N = std::dynamic_extent> class BoundsSpan final : private std::span<T, N> {
public:
	using std::span<T, N>::span;
	using std::span<T, N>::data;
	using std::span<T, N>::size;
	using std::span<T, N>::begin;
	using std::span<T, N>::end;

	// otherwise clang never inlines this, leading to a 10x slowdown
	[[gnu::always_inline]] typename std::span<T, N>::value_type operator[](typename std::span<T, N>::size_type idx) const {
		if (idx >= size()) [[unlikely]] {
			throw std::out_of_range("span read out of bounds - wanted index [" + std::to_string(idx) + "] but size is " +
									std::to_string(size()));
		}
		return std::span<T, N>::operator[](idx);
	}
	[[gnu::always_inline]] typename std::span<T, N>::reference operator[](typename std::span<T, N>::size_type idx) {
		if (idx >= size()) [[unlikely]] {
			throw std::out_of_range("span write out of bounds - wanted index [" + std::to_string(idx) + "] but size is " +
									std::to_string(size()));
		}
		return std::span<T, N>::operator[](idx);
	}
	[[nodiscard]] BoundsSpan<T, N> subspan(typename std::span<T, N>::size_type Offset,
										   typename std::span<T, N>::size_type Count = std::dynamic_extent) const {
		if (Offset > N) [[unlikely]] {
			throw std::out_of_range("attempted to create subspan with offset " + std::to_string(Offset) + " greater than length " +
									std::to_string(N));
		}
		if (Count != std::dynamic_extent && Count > N - Offset) [[unlikely]] {
			throw std::out_of_range("attempted to create subspan with length " + std::to_string(Count) +
									" greater than extent minus offset " + std::to_string(N) + " - " + std::to_string(Offset));
		}
		if (Offset > size()) [[unlikely]] {
			throw std::out_of_range("attempted to create subspan with offset " + std::to_string(Offset) + " greater than current length " +
									std::to_string(size()));
		}
		if (Count != std::dynamic_extent && Count > size() - Offset) [[unlikely]] {
			throw std::out_of_range("attempted to create subspan with length " + std::to_string(Count) +
									" greater than current length minus offset " + std::to_string(size()) + " - " + std::to_string(Offset));
		}
		if (Count == std::dynamic_extent) {
			return BoundsSpan(data() + Offset, size() - Offset);
		}
		return BoundsSpan(data() + Offset, Count);
	}
};
template <class It, class EndOrSize> BoundsSpan(It, EndOrSize) -> BoundsSpan<std::remove_reference_t<std::iter_reference_t<It>>>;
template <class T, std::size_t N> BoundsSpan(T (&)[N]) -> BoundsSpan<T, N>; // NOLINT: modernize-avoid-c-arrays
template <class T, std::size_t N> BoundsSpan(std::array<T, N> &) -> BoundsSpan<T, N>;
template <class T, std::size_t N> BoundsSpan(const std::array<T, N> &) -> BoundsSpan<const T, N>;
template <class R> BoundsSpan(R &&) -> BoundsSpan<std::remove_reference_t<std::ranges::range_reference_t<R>>>;

class ScopeGuard {
private:
	bool fuse = true;
	std::function<void()> atexit;

public:
	ScopeGuard() = delete;
	ScopeGuard(const ScopeGuard &) = delete;
	ScopeGuard(ScopeGuard &&) = delete;
	ScopeGuard operator=(const ScopeGuard &) = delete;
	ScopeGuard operator=(ScopeGuard &&) = delete;
	ScopeGuard(std::function<void()> atexit);
	~ScopeGuard();
	void defuse();
};

class ProcessHelper final {
private:
	int _returnCode;
	std::string _stdout;
	std::string _stderr;
	boost::process::ipstream stdoutPipe;
	boost::process::ipstream stderrPipe;
	boost::process::child process;

public:
	ProcessHelper(const boost::filesystem::path &program, const std::vector<std::string> &args);
	ProcessHelper(const boost::filesystem::path &program, const std::vector<std::string> &args, const boost::process::environment &env);
	[[nodiscard]] const int &returnCode() const;
	// stdout / stderr are macros of stdio.h, avoid potential issues
	[[nodiscard]] const std::string &get_stdout() const;
	[[nodiscard]] const std::string &get_stderr() const;
};

class Tempfile final : public path {
private:
	bool cleanup = true;
	char *namePtr;
	void createFileName(const path &path);

public:
	explicit Tempfile(const path &name);
	Tempfile(const path &name, const std::string &content);
	Tempfile(const Tempfile &) = delete;
	Tempfile(Tempfile &&) noexcept = default;
	~Tempfile();

	Tempfile &operator=(const Tempfile &) = delete;
	Tempfile &operator=(Tempfile &&) noexcept = default;

	void disable_cleanup();
};

distplusplus::CompilerType mapCompiler(const std::string &compiler);

const char *mapGRPCStatus(const grpc::StatusCode &status);

} // namespace distplusplus::common
