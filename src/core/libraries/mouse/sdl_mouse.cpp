// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/kernel/time.h"
#include "sdl_mouse.h"

namespace Libraries::Mouse {

static u64 GetIndexFromSdlHandle(SDL_MouseID id) {
    return g_is_merged_mode             ? 0
           : mouse_sdl_handles[0] == id ? 0
           : mouse_sdl_handles[1] == id ? 1
                                        : 2;
}

constexpr u32 OrbisButtonFromSDL(Uint8 button) {
    switch (button) {
    case SDL_BUTTON_LEFT:
        return 1u << 0;
    case SDL_BUTTON_RIGHT:
        return 1u << 1;
    case SDL_BUTTON_MIDDLE:
        return 1u << 2;
    case SDL_BUTTON_X1:
        return 1u << 3;
    case SDL_BUTTON_X2:
        return 1u << 4;
    default:
        return 0;
    }
}

bool PushSDLEvent(SDL_Event const& e) {
    static OrbisMouseData current_state[2]{};
    if (!EmulatorSettings.IsMiceUsedAsMice()) {
        return false;
    }
    if (!g_lib_init) {
        return false;
    }
    switch (e.type) {
    default:
        return false;
    case SDL_EVENT_MOUSE_REMOVED: {
        LOG_INFO(Lib_Mouse, "Mouse removed, id: {}", e.mdevice.which);
        if (g_is_merged_mode) {
            break;
        }
        u64 index = GetIndexFromSdlHandle(e.mdevice.which);
        if (index == 2) {
            return false;
        }
        mouse_sdl_handles[index] = -1;
        mouse_states[index].Push(current_state[index]);
        break;
    }
    case SDL_EVENT_MOUSE_MOTION: {
        u64 index = GetIndexFromSdlHandle(e.motion.which);
        if (index == 2) {
            index = GetIndexFromSdlHandle(-1);
            if (index < 2)
                mouse_sdl_handles[index] = e.motion.which;
            else
                return false;
        }
        auto& s = current_state[index];
        s.x_axis = (s32)e.motion.xrel;
        s.y_axis = (s32)e.motion.yrel;
        s.wheel = 0;
        s.timestamp = Libraries::Kernel::sceKernelGetProcessTime();

        mouse_states[index].Push(s);
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        u64 index = GetIndexFromSdlHandle(e.button.which);
        if (index >= 2)
            return false;

        auto& s = current_state[index];
        s.x_axis = 0;
        s.y_axis = 0;
        s.wheel = 0;
        s.buttons |= OrbisButtonFromSDL(e.button.button);
        s.timestamp = Libraries::Kernel::sceKernelGetProcessTime();

        mouse_states[index].Push(s);
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        u64 index = GetIndexFromSdlHandle(e.button.which);
        if (index >= 2)
            return false;

        auto& s = current_state[index];
        s.x_axis = 0;
        s.y_axis = 0;
        s.wheel = 0;
        s.buttons &= ~OrbisButtonFromSDL(e.button.button);
        s.timestamp = Libraries::Kernel::sceKernelGetProcessTime();

        mouse_states[index].Push(s);
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL: {
        u64 index = GetIndexFromSdlHandle(e.wheel.which);
        if (index >= 2)
            return false;

        auto& s = current_state[index];
        s.x_axis = 0;
        s.y_axis = 0;
        s.wheel = e.wheel.y;
        s.tilt = e.wheel.x;
        s.timestamp = Libraries::Kernel::sceKernelGetProcessTime();

        mouse_states[index].Push(s);
        break;
    }
    }
    return true;
}
} // namespace Libraries::Mouse
