// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <string>

#include "core/libraries/ime/ime_common.h"
#include "core/libraries/ime/ime_kb_language.h"

union SDL_Event;

namespace Libraries::Ime {

struct ImeKeyboardContext {
    ImeKbVariant variant;
    u32 mode;
    u64 generation = 0;
    bool operator==(const ImeKeyboardContext&) const = default;
};

struct ImeKeyboardText {
    std::string text;
    std::optional<ImeKeyboardContext> context;
};

void DiscardImeKeyboardContexts();
void TrackImeKeyboardEvents(const SDL_Event* event, u32 first, u32 last);
std::optional<ImeKeyboardContext> TakeImeKeyboardContext(u32 event_id);

enum class ImeKbKeyAction : u8;
void QueueImeOskKey(ImeKbKeyAction action, const char* text, ImeKbVariant variant, bool uppercase,
                    bool repeat = false, bool shortcut = false);
void InitializeImeKeyboard();
std::optional<ImeKeyboardContext> GetImeExternalKeyboardContext(bool settled_only = false);
void SetImeKeyboardCompositionState(bool preedit, bool selected);
bool ProcessImeKeyboardEvent(SDL_Event& event);
bool IsImeKeyboardActive();
void ConfigureImeKeyboard(bool active, s32 user_id = -1,
                          const OrbisImeParamExtended* extended = nullptr);
bool HasImeKeyboardHandler(OrbisImeEventHandler handler);
Error DispatchImeKeyboard(OrbisImeEventHandler handler);

} // namespace Libraries::Ime
