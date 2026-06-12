// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <core/libraries/kernel/time.h>
#include "common/logging/log.h"
#include "sdl_mouse.h"

namespace Libraries::Mouse {

static u64 GetIndexFromSdlHandle(SDL_MouseID id) {
    return mouse_sdl_handles[0] == id ? 0 : mouse_sdl_handles[1] == id ? 1 : 2;
}

bool PushSDLEvent(SDL_Event const& e) {
    if (!g_lib_init) {
        return false;
    }
    switch (e.type) {
    case SDL_EVENT_MOUSE_ADDED: {
        LOG_INFO(Lib_Mouse, "Mouse added, id: {}", e.mdevice.which);
        u64 index = GetIndexFromSdlHandle(-1); // first open slot
        if (index == 2) {
            return false;
        }
        mouse_sdl_handles[index] = e.mdevice.which;
        break;
    }
    case SDL_EVENT_MOUSE_REMOVED: {
        LOG_INFO(Lib_Mouse, "Mouse removed, id: {}", e.mdevice.which);
        u64 index = GetIndexFromSdlHandle(e.mdevice.which);
        if (index == 2) {
            return false;
        }
        mouse_sdl_handles[index] = -1;
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
        mouse_states[index].Push({.timestamp = Libraries::Kernel::sceKernelGetProcessTime(),
                                  .x_axis = (s32)e.motion.xrel,
                                  .y_axis = (s32)e.motion.yrel});
        break;
    }
    default:
        return false;
    }
    return true;
}
} // namespace Libraries::Mouse
