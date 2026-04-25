// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

constexpr u32 ORBIS_AT9_CONFIG_DATA_SIZE = 4;

struct AjmDecAt9InitializeParameters {
    u8 config_data[ORBIS_AT9_CONFIG_DATA_SIZE];
    u32 reserved;
};
