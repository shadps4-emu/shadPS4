// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SystemGesture {

int PS4_SYSV_ABI sceSystemGestureUpdatePrimitiveTouchRecognizer(s32 handle);
int PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizer(s32 handle);
int PS4_SYSV_ABI sceSystemGestureGetTouchEventsCount(s32 handle);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SystemGesture
