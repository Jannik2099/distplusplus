#pragma once

#include <span>
#include <stdexcept>
#include <string>

namespace distplusplus::common {

template <class T, std::size_t N = std::dynamic_extent> class BoundsSpan final : private std::span<T, N> {
public:
    using std::span<T, N>::span;
    using std::span<T, N>::data;
    using std::span<T, N>::size;
    using std::span<T, N>::begin;
    using std::span<T, N>::end;

    // otherwise clang never inlines this, leading to a 10x slowdown
    [[gnu::always_inline]] typename std::span<T, N>::value_type
    operator[](typename std::span<T, N>::size_type idx) const {
        if (idx >= size()) [[unlikely]] {
            throw std::out_of_range("span read out of bounds - wanted index [" + std::to_string(idx) +
                                    "] but size is " + std::to_string(size()));
        }
        return std::span<T, N>::operator[](idx);
    }
    [[gnu::always_inline]] typename std::span<T, N>::reference
    operator[](typename std::span<T, N>::size_type idx) {
        if (idx >= size()) [[unlikely]] {
            throw std::out_of_range("span write out of bounds - wanted index [" + std::to_string(idx) +
                                    "] but size is " + std::to_string(size()));
        }
        return std::span<T, N>::operator[](idx);
    }
    [[nodiscard]] BoundsSpan<T, N>
    subspan(typename std::span<T, N>::size_type Offset,
            typename std::span<T, N>::size_type Count = std::dynamic_extent) const {
        if (Offset > N) [[unlikely]] {
            throw std::out_of_range("attempted to create subspan with offset " + std::to_string(Offset) +
                                    " greater than length " + std::to_string(N));
        }
        if (Count != std::dynamic_extent && Count > N - Offset) [[unlikely]] {
            throw std::out_of_range("attempted to create subspan with length " + std::to_string(Count) +
                                    " greater than extent minus offset " + std::to_string(N) + " - " +
                                    std::to_string(Offset));
        }
        if (Offset > size()) [[unlikely]] {
            throw std::out_of_range("attempted to create subspan with offset " + std::to_string(Offset) +
                                    " greater than current length " + std::to_string(size()));
        }
        if (Count != std::dynamic_extent && Count > size() - Offset) [[unlikely]] {
            throw std::out_of_range("attempted to create subspan with length " + std::to_string(Count) +
                                    " greater than current length minus offset " + std::to_string(size()) +
                                    " - " + std::to_string(Offset));
        }
        if (Count == std::dynamic_extent) {
            return BoundsSpan(data() + Offset, size() - Offset);
        }
        return BoundsSpan(data() + Offset, Count);
    }
};
template <class It, class EndOrSize>
BoundsSpan(It, EndOrSize) -> BoundsSpan<std::remove_reference_t<std::iter_reference_t<It>>>;
template <class T, std::size_t N>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
BoundsSpan(T (&)[N])->BoundsSpan<T, N>;
template <class T, std::size_t N> BoundsSpan(std::array<T, N> &) -> BoundsSpan<T, N>;
template <class T, std::size_t N> BoundsSpan(const std::array<T, N> &) -> BoundsSpan<const T, N>;
template <class R> BoundsSpan(R &&) -> BoundsSpan<std::remove_reference_t<std::ranges::range_reference_t<R>>>;

} // namespace distplusplus::common
