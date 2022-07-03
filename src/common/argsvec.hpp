#pragma once

#include "bounds_span.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace distplusplus::common {

class Arg final {
private:
    std::variant<char *, std::shared_ptr<const std::string>> data;

public:
    template <class... Args>
    Arg(Args... args) : data(std::make_shared<const std::string>(std::forward<Args>(args)...)) {
        using intuple = std::tuple<Args...>;
        if constexpr (std::tuple_size<intuple>::value > 0) {
            using intype = std::remove_cvref_t<std::tuple_element_t<0, std::tuple<Args...>>>;
            static_assert(!std::is_same_v<intype, char *>,
                          "trying to construct from char* - did you mean to use fromExternal() ?");
        }
    }

    static Arg fromExternal(char *str) {
        Arg ret;
        ret.data = str;
        return ret;
    }

    operator std::string_view() const {
        if (data.index() == 0) {
            return std::get<0>(data);
        }
        return *std::get<1>(data);
    }
    operator std::string() const {
        if (data.index() == 0) {
            return std::get<0>(data);
        }
        return *std::get<1>(data);
    }

    [[nodiscard]] constexpr const char *c_str() const {
        if (data.index() == 0) {
            return std::get<0>(data);
        }
        return std::get<1>(data)->c_str();
    }
};

class ArgsVec final : public std::vector<Arg> {
private:
    using ArgsSpan = BoundsSpan<const ArgsVec::value_type>;
    using Base = std::vector<Arg>;

public:
    // no idea why I have to explicitly declare this
    ArgsVec(std::initializer_list<ArgsVec::value_type> init) {
        reserve(init.size());
        for (const Arg &arg : init) {
            emplace_back(arg);
        }
    }

    ArgsVec() = default;
    ArgsVec(const ArgsVec &) = default;
    ArgsVec(ArgsVec &&) = delete;
    ~ArgsVec() = default;
    ArgsVec &operator=(const ArgsVec &) = default;
    ArgsVec &operator=(ArgsVec &&) = delete;

    template <class InputIt> ArgsVec(InputIt first, InputIt last) : Base(first, last) {}

    operator ArgsSpan() const noexcept { return {cbegin(), cend()}; }
};

using ArgsSpan = BoundsSpan<const ArgsVec::value_type>;

} // namespace distplusplus::common
