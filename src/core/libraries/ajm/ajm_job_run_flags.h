// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

enum class AjmJobRunFlags : u64 {
    GetCodecInfo = 1 << 0,
    MultipleFrames = 1 << 1,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmJobRunFlags)
