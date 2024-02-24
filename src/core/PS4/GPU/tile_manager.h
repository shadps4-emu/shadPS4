// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace GPU {

void convertTileToLinear(void* dst, const void* src, u32 width, u32 height, bool neo);

} // namespace GPU
