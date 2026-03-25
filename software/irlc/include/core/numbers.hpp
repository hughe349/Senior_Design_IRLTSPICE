#pragma once

#include <cstdint>
#include <string_view>

// The value of a component in pico-somethings
typedef struct val_pico {
    uint64_t v;

    // Throws if unknown suffix
    val_pico() = default;
    val_pico(uint64_t v) : v(v) {};
    //                                                        10^12 = 123456789ABC
    static inline val_pico no_scaling(uint64_t v) { return val_pico(v * 1000000000000); };
    static val_pico from_str(std::string_view str);

    bool operator==(val_pico const &other) const;
    bool operator!=(val_pico const &other) const;
    bool operator<(val_pico const &other) const;
    bool operator>(val_pico const &other) const;
    bool operator<=(val_pico const &other) const;
    bool operator>=(val_pico const &other) const;
} val_pico_t;

static inline val_pico_t operator""_p(uint64_t n) { return val_pico_t(n); }

static inline val_pico_t operator""_n(uint64_t n) { return operator""_p(n * 1000); }

static inline val_pico_t operator""_u(uint64_t n) { return operator""_n(n * 1000); }

static inline val_pico_t operator""_m(uint64_t n) { return operator""_u(n * 1000); }

static inline val_pico_t operator""_k(uint64_t n) { return val_pico_t::no_scaling(n * 1000); }

static inline val_pico_t operator""_M(uint64_t n) { return operator""_k(n * 1000); }
