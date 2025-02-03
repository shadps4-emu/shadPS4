// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _MSC_VER
#define BREAKPOINT __debugbreak
#elif defined(__GNUC__)
#define BREAKPOINT __builtin_trap
#else
#error What the fuck is this compiler
#endif

#include <tracy/Tracy.hpp>

static inline bool IsProfilerConnected() {
#if TRACY_ENABLE
    return tracy::GetProfiler().IsConnected();
#else
    return false;
#endif
}

#define TRACY_GPU_ENABLED 0

#define CUSTOM_LOCK(type, varname)                                                                 \
    tracy::LockableCtx varname {                                                                   \
        []() -> const tracy::SourceLocationData* {                                                 \
            static constexpr tracy::SourceLocationData srcloc{nullptr, #type " " #varname,         \
                                                              TracyFile, TracyLine, 0};            \
            return &srcloc;                                                                        \
        }()                                                                                        \
    }

#define TRACK_ALLOC(ptr, size, pool) TracyAllocN(std::bit_cast<void*>(ptr), (size), (pool))
#define TRACK_FREE(ptr, pool) TracyFreeN(std::bit_cast<void*>(ptr), (pool))

enum MarkersPalette : int {
    EmulatorMarkerColor = 0x264653,
    RendererMarkerColor = 0x2a9d8f,
    HleMarkerColor = 0xe9c46a,
    GpuMarkerColor = 0xf4a261,
    Reserved1 = 0xe76f51,
};

#define EMULATOR_TRACE ZoneScopedC(EmulatorMarkerColor)
#define RENDERER_TRACE ZoneScopedC(RendererMarkerColor)
#define HLE_TRACE ZoneScopedC(HleMarkerColor)

#define TRACE_HINT(str) ZoneText(str.data(), str.size())

#define TRACE_WARN(msg)                                                                            \
    [](const auto& msg) { TracyMessageC(msg.c_str(), msg.size(), tracy::Color::DarkOrange); }(msg);
#define TRACE_ERROR(msg)                                                                           \
    [](const auto& msg) { TracyMessageC(msg.c_str(), msg.size(), tracy::Color::Red); }(msg)
#define TRACE_CRIT(msg)                                                                            \
    [](const auto& msg) { TracyMessageC(msg.c_str(), msg.size(), tracy::Color::HotPink); }(msg)

#define GPU_SCOPE_LOCATION(name, color)                                                            \
    tracy::SourceLocationData{name, TracyFunction, TracyFile, (uint32_t)TracyLine, color};

#define MUTEX_LOCATION(name)                                                                       \
    tracy::SourceLocationData{nullptr, name, TracyFile, (uint32_t)TracyLine, 0};

#define FRAME_END FrameMark

#ifdef TRACY_FIBERS
#define FIBER_ENTER(name) TracyFiberEnter(name)
#define FIBER_EXIT TracyFiberLeave
#else
#define FIBER_ENTER(name)
#define FIBER_EXIT
#endif
