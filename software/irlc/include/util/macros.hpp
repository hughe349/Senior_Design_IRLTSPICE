#pragma once

#include <array>

template <class... Ts> struct overloads : Ts... {
    using Ts::operator()...;
};

template <typename T> consteval std::array<T, 0> arrify() { return std::array<T, 0>(); }
template <typename... Ts> consteval auto arrify(Ts... elems) { return std::to_array({elems...}); }

template <size_t N> struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }

    char value[N];
};
