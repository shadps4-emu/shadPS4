// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

enum class AjmJobSidebandFlags : u64 {
    GaplessDecode = 1 << 0,
    Format = 1 << 1,
    Stream = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmJobSidebandFlags)
