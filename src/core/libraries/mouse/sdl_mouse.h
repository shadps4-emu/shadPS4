// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "SDL3/SDL_events.h"
#include "common/types.h"
#include "mouse.h"

namespace Libraries::Mouse {
bool PushSDLEvent(SDL_Event const& e);
}