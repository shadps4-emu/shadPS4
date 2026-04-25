// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

enum class OrbisImeDialogEndStatus : u32 {
    Ok = 0,
    UserCanceled = 1,
    Aborted = 2,
};
