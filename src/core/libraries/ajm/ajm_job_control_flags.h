// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

enum class AjmJobControlFlags : u64 {
    Reset = 1 << 0,
    Initialize = 1 << 1,
    Resample = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmJobControlFlags)
