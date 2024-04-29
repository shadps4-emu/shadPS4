// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace VideoCore {

/// Converts tiled texture data to linear format.
void ConvertTileToLinear(u8* dst, const u8* src, u32 width, u32 height, bool neo);

} // namespace VideoCore
