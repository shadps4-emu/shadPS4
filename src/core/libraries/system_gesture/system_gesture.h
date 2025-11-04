// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SystemGesture {

int PS4_SYSV_ABI sceSystemGestureAppendTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureClose();
int PS4_SYSV_ABI sceSystemGestureCreateTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureFinalizePrimitiveTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByIndex();
int PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID();
int PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEvents();
int PS4_SYSV_ABI sceSystemGestureGetPrimitiveTouchEventsCount();
int PS4_SYSV_ABI sceSystemGestureGetTouchEventByEventID();
int PS4_SYSV_ABI sceSystemGestureGetTouchEventByIndex();
int PS4_SYSV_ABI sceSystemGestureGetTouchEvents();
int PS4_SYSV_ABI sceSystemGestureGetTouchEventsCount();
int PS4_SYSV_ABI sceSystemGestureGetTouchRecognizerInformation();
int PS4_SYSV_ABI sceSystemGestureInitializePrimitiveTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureOpen();
int PS4_SYSV_ABI sceSystemGestureRemoveTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureResetPrimitiveTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureResetTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureUpdateAllTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureUpdatePrimitiveTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizer();
int PS4_SYSV_ABI sceSystemGestureUpdateTouchRecognizerRectangle();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SystemGesture
