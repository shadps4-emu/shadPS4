// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/kernel/coredump/coredump.h"
#include "core/libraries/kernel/coredump/coredump_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

OrbisCoredumpHandler g_coredump_handler{};
void* g_coredump_common{};
s32 g_sdk_ver{};

s32 PS4_SYSV_ABI sceCoredumpRegisterCoredumpHandler(OrbisCoredumpHandler handler, u64 stack_size,
                                                    void* common) {
    LOG_WARNING(Lib_Kernel, "(STUBBED) called, handler = {}, stack_size = {:#x}, common = {}",
                fmt::ptr(handler), stack_size, fmt::ptr(common));
    if (g_coredump_handler) {
        LOG_ERROR(Lib_Kernel, "Coredump handler is already registered");
        return ORBIS_COREDUMP_ERROR_ALREADY_REGISTERED;
    }
    if (!handler || stack_size >= 0x20000000) {
        LOG_ERROR(Lib_Kernel, "Invalid coredump handler parameters");
        return ORBIS_COREDUMP_ERROR_PARAM;
    }

    // Real library starts a separate thread for coredump handling.
    // For this stub, just store the globals.
    g_coredump_handler = handler;
    g_coredump_common = common;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCoredumpUnregisterCoredumpHandler() {
    LOG_INFO(Lib_Kernel, "called");
    if (!g_coredump_handler) {
        LOG_ERROR(Lib_Kernel, "No coredump handler to unregister");
        return ORBIS_COREDUMP_ERROR_NOT_REGISTERED;
    }

    g_coredump_handler = nullptr;
    return ORBIS_OK;
}

void RegisterCoredump(Core::Loader::SymbolsResolver* sym) {
    ASSERT_MSG(sceKernelGetCompiledSdkVersion(&g_sdk_ver) == ORBIS_OK,
               "Failed to retrieve SDK version");

    LIB_FUNCTION("8zLSfEfW5AU", "libSceCoredump", 1, "libkernel",
                 sceCoredumpRegisterCoredumpHandler);
    LIB_FUNCTION("fFkhOgztiCA", "libSceCoredump", 1, "libkernel",
                 sceCoredumpUnregisterCoredumpHandler);
}

} // namespace Libraries::Kernel