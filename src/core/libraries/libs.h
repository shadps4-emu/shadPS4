// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include "common/logging/log.h"
#include "core/loader/elf.h"
#include "core/loader/symbols_resolver.h"

template <size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }

    char value[N];
};

template <StringLiteral name, class F, F f>
struct wrapper_impl;

template <StringLiteral name, class R, class... Args, PS4_SYSV_ABI R (*f)(Args...)>
struct wrapper_impl<name, PS4_SYSV_ABI R (*)(Args...), f> {
    static R PS4_SYSV_ABI wrap(Args... args) {
        if (std::string_view(name.value) != "scePthreadEqual" &&
            std::string_view(name.value) != "sceUserServiceGetEvent" &&
            !std::string_view(name.value).contains("scePthreadMutex") &&
            !std::string_view(name.value).contains("pthread_mutex")) {
            // LOG_WARNING(Core_Linker, "Function {} called", name.value);
        }
        if constexpr (std::is_same_v<R, s32> || std::is_same_v<R, u32>) {
            const u32 ret = f(args...);
            if (ret != 0 && !std::string_view(name.value).contains("pthread_equal")) {
                LOG_WARNING(Core_Linker, "Function {} returned {:#x}", name.value, ret);
            }
            return ret;
        }
        // stuff
        return f(args...);
    }
};

template <StringLiteral name, class F, F f>
constexpr auto wrapper = wrapper_impl<name, F, f>::wrap;

// #define W(foo) wrapper<#foo, decltype(&foo), foo>
#define W(foo) foo

#define LIB_FUNCTION(nid, lib, libversion, mod, moduleVersionMajor, moduleVersionMinor, f)         \
    {                                                                                              \
        Core::Loader::SymbolResolver sr{};                                                         \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.module_version_major = moduleVersionMajor;                                              \
        sr.module_version_minor = moduleVersionMinor;                                              \
        sr.type = Core::Loader::SymbolType::Function;                                              \
        {                                                                                          \
            auto func = reinterpret_cast<u64>(wrapper<#f, decltype(&f), f>);                       \
            sym->AddSymbol(sr, func);                                                              \
        }                                                                                          \
    }

#define LIB_OBJ(nid, lib, libversion, mod, moduleVersionMajor, moduleVersionMinor, function)       \
    {                                                                                              \
        Core::Loader::SymbolResolver sr{};                                                         \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.module_version_major = moduleVersionMajor;                                              \
        sr.module_version_minor = moduleVersionMinor;                                              \
        sr.type = Core::Loader::SymbolType::Object;                                                \
        auto func = reinterpret_cast<u64>(function);                                               \
        sym->AddSymbol(sr, func);                                                                  \
    }

namespace Libraries {

void InitHLELibs(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries
