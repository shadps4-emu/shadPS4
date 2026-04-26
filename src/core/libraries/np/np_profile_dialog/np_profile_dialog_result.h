// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/system/commondialog.h"

struct OrbisNpProfileDialogResult {
    s32 result;
    Libraries::CommonDialog::Result userAction;
    void* userData;
    u8 reserved[32];
};
