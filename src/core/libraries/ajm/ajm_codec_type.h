// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

enum class AjmCodecType : u32 {
    Mp3Dec = 0,
    At9Dec = 1,
    M4aacDec = 2,
    Max = 23,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmCodecType);
