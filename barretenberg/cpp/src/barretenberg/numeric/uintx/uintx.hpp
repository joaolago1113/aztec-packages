// === AUDIT STATUS ===
// internal:    { status: not started, auditors: [], date: YYYY-MM-DD }
// external_1:  { status: not started, auditors: [], date: YYYY-MM-DD }
// external_2:  { status: not started, auditors: [], date: YYYY-MM-DD }
// =====================

/**
 * uintx
 * Copyright Aztec 2020
 *
 * An unsigned 512 bit integer type.
 *
 * Constructor and all methods are constexpr. Ideally, uintx should be able to be treated like any other literal
 *type.
 *
 * Not optimized for performance, this code doesn"t touch any of our hot paths when constructing PLONK proofs
 **/
#pragma once

#include "../uint256/uint256.hpp"
#include "barretenberg/common/assert.hpp"
#include "barretenberg/common/throw_or_abort.hpp"
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace bb::numeric {

template <class base_uint> class uintx {
  public:
    constexpr uintx(const uint64_t data = 0)
        : lo(data)
        , hi(base_uint(0))
    {}

    constexpr uintx(const base_uint input_lo)
        : lo(input_lo)
        , hi(base_uint(0))
    {}

    constexpr uintx(const base_uint input_lo, const base_uint input_hi)
        : lo(input_lo)
        , hi(input_hi)
    {}

    constexpr uintx(const uintx& other)
        : lo(other.lo)
        , hi(other.hi)
    {}

    constexpr uintx(uintx&& other) noexcept = default;

    static constexpr size_t length() { return 2 * base_uint::length(); }
    constexpr uintx& operator=(const uintx& other) = default;
    constexpr uintx& operator=(uintx&& other) noexcept = default;

    constexpr ~uintx() = default;
    explicit constexpr operator bool() const { return static_cast<bool>(lo.data[0]); };
    explicit constexpr operator uint8_t() const { return static_cast<uint8_t>(lo.data[0]); };
    explicit constexpr operator uint16_t() const { return static_cast<uint16_t>(lo.data[0]); };
    explicit constexpr operator uint32_t() const { return static_cast<uint32_t>(lo.data[0]); };
    explicit constexpr operator uint64_t() const { return static_cast<uint64_t>(lo.data[0]); };

    explicit constexpr operator base_uint() const { return lo; }

    [[nodiscard]] constexpr bool get_bit(uint64_t bit_index) const;
    [[nodiscard]] constexpr uint64_t get_msb() const;
    constexpr uintx slice(uint64_t start, uint64_t end) const;

    constexpr uintx operator+(const uintx& other) const;
    constexpr uintx operator-(const uintx& other) const;
    constexpr uintx operator-() const;

    constexpr uintx operator*(const uintx& other) const;
    constexpr uintx operator/(const uintx& other) const;
    constexpr uintx operator%(const uintx& other) const;

    constexpr std::pair<uintx, uintx> mul_extended(const uintx& other) const;

    constexpr uintx operator>>(uint64_t other) const;
    constexpr uintx operator<<(uint64_t other) const;

    constexpr uintx operator&(const uintx& other) const;
    constexpr uintx operator^(const uintx& other) const;
    constexpr uintx operator|(const uintx& other) const;
    constexpr uintx operator~() const;

    constexpr bool operator==(const uintx& other) const;
    constexpr bool operator!=(const uintx& other) const;
    constexpr bool operator!() const;

    constexpr bool operator>(const uintx& other) const;
    constexpr bool operator<(const uintx& other) const;
    constexpr bool operator>=(const uintx& other) const;
    constexpr bool operator<=(const uintx& other) const;

    constexpr uintx& operator+=(const uintx& other)
    {
        *this = *this + other;
        return *this;
    };
    constexpr uintx& operator-=(const uintx& other)
    {
        *this = *this - other;
        return *this;
    };
    constexpr uintx& operator*=(const uintx& other)
    {
        *this = *this * other;
        return *this;
    };
    constexpr uintx& operator/=(const uintx& other)
    {
        *this = *this / other;
        return *this;
    };
    constexpr uintx& operator%=(const uintx& other)
    {
        *this = *this % other;
        return *this;
    };

    constexpr uintx& operator++()
    {
        *this += uintx(1);
        return *this;
    };
    constexpr uintx& operator--()
    {
        *this -= uintx(1);
        return *this;
    };

    constexpr uintx& operator&=(const uintx& other)
    {
        *this = *this & other;
        return *this;
    };
    constexpr uintx& operator^=(const uintx& other)
    {
        *this = *this ^ other;
        return *this;
    };
    constexpr uintx& operator|=(const uintx& other)
    {
        *this = *this | other;
        return *this;
    };

    constexpr uintx& operator>>=(const uint64_t other)
    {
        *this = *this >> other;
        return *this;
    };
    constexpr uintx& operator<<=(const uint64_t other)
    {
        *this = *this << other;
        return *this;
    };

    constexpr uintx invmod(const uintx& modulus) const;
    constexpr uintx unsafe_invmod(const uintx& modulus) const;

    base_uint lo;
    base_uint hi;

    template <base_uint modulus> constexpr std::pair<uintx, uintx> barrett_reduction() const;
    constexpr std::pair<uintx, uintx> divmod(const uintx& b) const;
    constexpr std::pair<uintx, uintx> divmod_base(const uintx& b) const;
};

template <typename B, typename Params> inline void read(B& it, uintx<Params>& value)
{
    using serialize::read;
    Params a;
    Params b;
    read(it, b);
    read(it, a);
    value = uintx<Params>(a, b);
}

template <typename B, typename Params> inline void write(B& it, uintx<Params> const& value)
{
    using serialize::write;
    write(it, value.hi);
    write(it, value.lo);
}

template <class base_uint> inline std::ostream& operator<<(std::ostream& os, uintx<base_uint> const& a)
{
    os << a.lo << ", " << a.hi << std::endl;
    return os;
}

using uint512_t = uintx<numeric::uint256_t>;
using uint1024_t = uintx<uint512_t>;

} // namespace bb::numeric

#include "./uintx_impl.hpp"

using bb::numeric::uint1024_t; // NOLINT
using bb::numeric::uint512_t;  // NOLINT
