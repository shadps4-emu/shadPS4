// SPDX-FileCopyrightText: Copyright 2026 shadBloodborne Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/process.h"

namespace Libraries::SysModule {

s32 getModuleHandle(s32 id, s32* handle);
bool shouldHideName(const char* module_name);
bool isDebugModule(s32 id);
bool validateModuleId(s32 id);
s32 loadModuleInternal(s32 index, s32 argc, const void* argv, s32* res_out);
s32 loadModule(s32 id, s32 argc, const void* argv, s32* res_out);
s32 unloadModule(s32 id, s32 argc, const void* argv, s32* res_out, bool is_internal);
s32 preloadModulesForLibkernel();

} // namespace Libraries::SysModule