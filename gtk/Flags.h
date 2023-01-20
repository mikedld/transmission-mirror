// This file Copyright © 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <initializer_list>
#include <type_traits>

// NOLINTBEGIN(bugprone-macro-parentheses)

#define DEFINE_FLAGS_OPERATORS(FlagType) \
    constexpr inline Flags<FlagType> operator|(FlagType lhs, FlagType rhs) noexcept \
    { \
        return { lhs, rhs }; \
    }

// NOLINTEND(bugprone-macro-parentheses)

template<typename T>
class Flags
{
public:
    using FlagType = T;
    using ValueType = std::underlying_type_t<FlagType>;

    static_assert(std::is_enum_v<FlagType> && !std::is_convertible_v<FlagType, ValueType>);

public:
    constexpr Flags() noexcept = default;

    constexpr Flags(FlagType flag) noexcept
    {
        set(flag);
    }

    constexpr Flags(std::initializer_list<FlagType> flags) noexcept
    {
        for (auto const flag : flags)
        {
            set(flag);
        }
    }

    constexpr bool none() const noexcept
    {
        return value_ == 0;
    }

    constexpr bool any() const noexcept
    {
        return !none();
    }

    constexpr bool test(FlagType flag) const noexcept
    {
        return (value_ & get_mask(flag)) != 0;
    }

    constexpr bool test(Flags rhs) const noexcept
    {
        return (value_ & rhs.value_) != 0;
    }

    constexpr void set(FlagType flag) noexcept
    {
        value_ |= get_mask(flag);
    }

    constexpr Flags operator|(Flags rhs) noexcept
    {
        return Flags(value_ | rhs.value_);
    }

    constexpr Flags& operator|=(Flags rhs) noexcept
    {
        value_ |= rhs.value_;
        return *this;
    }

    constexpr Flags operator~() const noexcept
    {
        return Flags(~value_);
    }

private:
    constexpr explicit Flags(ValueType value) noexcept
        : value_(value)
    {
    }

    static constexpr ValueType get_mask(FlagType flag) noexcept
    {
        return ValueType{ 1 } << static_cast<ValueType>(flag);
    }

private:
    ValueType value_ = {};
};
