// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <exception>
#include <string>
#include <utility>

#include <fmt/format.h>

namespace Shader {

class Exception : public std::exception {
public:
    explicit Exception(std::string message) noexcept : err_message{std::move(message)} {}

    [[nodiscard]] const char* what() const noexcept override {
        return err_message.c_str();
    }

    void Prepend(std::string_view prepend) {
        err_message.insert(0, prepend);
    }

    void Append(std::string_view append) {
        err_message += append;
    }

private:
    std::string err_message;
};

class LogicError : public Exception {
public:
    template <typename... Args>
    explicit LogicError(const char* message, Args&&... args)
        : Exception{fmt::format(fmt::runtime(message), std::forward<Args>(args)...)} {}
};

class RuntimeError : public Exception {
public:
    template <typename... Args>
    explicit RuntimeError(const char* message, Args&&... args)
        : Exception{fmt::format(fmt::runtime(message), std::forward<Args>(args)...)} {}
};

class NotImplementedException : public Exception {
public:
    template <typename... Args>
    explicit NotImplementedException(const char* message, Args&&... args)
        : Exception{fmt::format(fmt::runtime(message), std::forward<Args>(args)...)} {
        Append(" is not implemented");
    }
};

class InvalidArgument : public Exception {
public:
    template <typename... Args>
    explicit InvalidArgument(const char* message, Args&&... args)
        : Exception{fmt::format(fmt::runtime(message), std::forward<Args>(args)...)} {}
};

} // namespace Shader
