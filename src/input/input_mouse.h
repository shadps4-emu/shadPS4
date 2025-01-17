// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SDL3/SDL.h"
#include "common/types.h"

namespace Input {

void ToggleMouseEnabled();
void SetMouseToJoystick(const int joystick);
void SetMouseParams(const float mouse_deadzone_offset, const float mouse_speed, const float mouse_speed_offset);

// Polls the mouse for changes, and simulates joystick movement from it.
Uint32 MousePolling(void* param, Uint32 id, Uint32 interval);

} // namespace Input
