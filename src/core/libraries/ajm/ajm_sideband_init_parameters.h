// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/ajm/ajm_dec_at9_initialize_parameters.h"

union AjmSidebandInitParameters {
    AjmDecAt9InitializeParameters at9;
    u8 reserved[8];
};
