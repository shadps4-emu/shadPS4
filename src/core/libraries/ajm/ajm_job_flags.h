// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/ajm/ajm_job_run_flags.h"
#include "core/libraries/ajm/ajm_job_control_flags.h"
#include "core/libraries/ajm/ajm_job_sideband_flags.h"

union AjmJobFlags {
    u64 raw;
    struct {
        u64 version : 3;
        u64 codec : 8;
        AjmJobRunFlags run_flags : 2;
        AjmJobControlFlags control_flags : 3;
        u64 reserved : 29;
        AjmJobSidebandFlags sideband_flags : 3;
    };
};
