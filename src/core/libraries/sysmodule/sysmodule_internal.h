// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/process.h"

namespace Libraries::SysModule {

s32 isModuleLoaded(s32 id, s32* handle);
bool shouldHideName(const char* module_name);
bool isDebugModule(s32 id);
bool validateModuleId(s32 id);
s32 loadModuleInternal(s32 index, s32 argc, void** argv, s32* res_out);
s32 loadModule(s32 id, s32 argc, void** argv, s32* res_out);

} // namespace Libraries::SysModule