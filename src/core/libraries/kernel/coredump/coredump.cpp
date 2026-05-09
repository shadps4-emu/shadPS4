// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/kernel/coredump/coredump.h"
#include "core/libraries/kernel/coredump/coredump_error.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

s32 PS4_SYSV_ABI sceCoredumpRegisterCoredumpHandler(OrbisCoredumpHandler handler, u64 stack_size,
                                                    void* common) {
    LOG_ERROR(Lib_Kernel, "(STUBBED) called, handler = {}, stack_size = {:#x}, common = {}",
              fmt::ptr(handler), stack_size, fmt::ptr(common));
    return ORBIS_OK;
}

void RegisterCoredump(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("8zLSfEfW5AU", "libSceCoredump", 1, "libkernel",
                 sceCoredumpRegisterCoredumpHandler);
}

} // namespace Libraries::Kernel