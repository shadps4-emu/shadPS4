// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "camera.h"

namespace Libraries::Camera {

void ConvertRGBA8888ToRAW16(const u8* src, u16* dst, int width, int height);
void ConvertRGBA8888ToRAW8(const u8* src, u8* dst, int width, int height);
void ConvertRGBA8888ToY8(const u8* src, u8* dst, int width, int height);
void ConvertRGBA8888ToY16(const u8* src, u16* dst, int width, int height);
void ConvertYUY2ToY8(const u8* src, u8* dst, int width, int height);
void ConvertYUY2ToY16(const u8* src, u16* dst, int width, int height);

s32 SizeOfBaseFormat(OrbisCameraBaseFormat f);
s32 SizeOfScaleFormat(OrbisCameraScaleFormat f);

} // namespace Libraries::Camera