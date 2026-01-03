// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

struct OrbisSystemGestureTouchRecognizer;
struct OrbisSystemGestureRectangle;
struct OrbisSystemGestureTouchRecognizerInformation;

namespace Libraries::SystemGesture {
s32 sceSystemGestureUpdateTouchRecognizerRectangleInternal(
    s32 handle_index, OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureRectangle* rectangle); // FUN_010054c0
s32 PS4_SYSV_ABI sceSystemGestureGetTouchRecognizerInformationInternal(
    s32 handle_index, OrbisSystemGestureTouchRecognizer* touchRecognizer,
    OrbisSystemGestureTouchRecognizerInformation* information); // FUN_01005500
} // namespace Libraries::SystemGesture