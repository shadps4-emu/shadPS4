// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/process.h"

namespace Libraries::SysModule {

s32 isModuleLoaded(s32 id, s32* handle);

} // namespace Libraries::SysModule