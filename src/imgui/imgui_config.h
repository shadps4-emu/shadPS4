// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// WARNING: All includes from this file must be relative to allow Dear_ImGui project to compile
//          without having this project include paths.

#include <cstdint>

extern void assert_fail_debug_msg(const char* msg);

#define ImDrawIdx std::uint32_t

#define IM_STRINGIZE(x) IM_STRINGIZE2(x)
#define IM_STRINGIZE2(x) #x
#define IM_ASSERT(_EXPR)                                                                           \
    ([&]() {                                                                                       \
        if (!(_EXPR)) [[unlikely]] {                                                               \
            assert_fail_debug_msg(#_EXPR " at " __FILE__ ":" IM_STRINGIZE(__LINE__));              \
        }                                                                                          \
    }())

#define IMGUI_USE_WCHAR32
#define IMGUI_ENABLE_STB_TRUETYPE
#define IMGUI_DEFINE_MATH_OPERATORS

#define IM_VEC2_CLASS_EXTRA                                                                        \
    constexpr ImVec2(float _v) : x(_v), y(_v) {}