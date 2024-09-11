// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/**
 * @brief A template class that encapsulates a fixed, compile-time constant value.
 *
 * @tparam T The type of the value.
 * @tparam Value The fixed value of type T.
 *
 * This class provides a way to encapsulate a value that is constant and known at compile-time.
 * The value is stored as a private member and cannot be changed. Any attempt to assign a new
 * value to an object of this class will reset it to the fixed value.
 */
template <typename T, T Value>
class FixedValue {
    T m_value{Value};

public:
    constexpr FixedValue() = default;

    constexpr explicit(false) operator T() const {
        return m_value;
    }

    FixedValue& operator=(const T&) {
        m_value = Value;
        return *this;
    }
    FixedValue& operator=(T&&) noexcept {
        m_value = {Value};
        return *this;
    }
};
