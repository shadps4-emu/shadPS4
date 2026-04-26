// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

enum class OrbisNpProfileDialogMode : u32 {
    ORBIS_NP_PROFILE_DIALOG_MODE_INVALID = 0,
    ORBIS_NP_PROFILE_DIALOG_MODE_NORMAL = 1,
    ORBIS_NP_PROFILE_DIALOG_MODE_FRIEND_REQUEST = 2,
    ORBIS_NP_PROFILE_DIALOG_MODE_ADD_TO_BLOCK_LIST = 3,
    ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT = 4,
};
