// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/ime/ime_dialog_end_status.h"

struct OrbisImeDialogResult {
    OrbisImeDialogEndStatus endstatus;
    s32 reserved[12];
};
