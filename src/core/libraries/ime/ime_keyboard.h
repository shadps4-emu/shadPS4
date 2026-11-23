// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/ime/ime_common.h"

union SDL_Event;

namespace Libraries::Ime {

void InitializeImeKeyboard();
bool ProcessImeKeyboardEvent(SDL_Event& event);
bool IsImeKeyboardActive();
void ConfigureImeKeyboard(bool active, s32 user_id = -1,
                          const OrbisImeParamExtended* extended = nullptr);
bool HasImeKeyboardHandler(OrbisImeEventHandler handler);
Error DispatchImeKeyboard(OrbisImeEventHandler handler);

} // namespace Libraries::Ime
