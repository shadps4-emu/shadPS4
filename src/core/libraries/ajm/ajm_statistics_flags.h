// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

enum class AjmStatisticsFlags : u64 {
    Memory = 1 << 0,
    EnginePerCodec = 1 << 15,
    Engine = 1 << 16,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmStatisticsFlags)
