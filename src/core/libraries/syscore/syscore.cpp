// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/syscore/syscore.h"
#include "core/libraries/syscore/syscore_error.h"

namespace Libraries::SysCore {

static std::atomic_bool g_is_initialized = false;

s32 PS4_SYSV_ABI sceApplicationInitialize() {
    LOG_INFO(Lib_SysCore, "called");
    if (g_is_initialized.exchange(true)) {
        return ORBIS_SYSCORE_ERROR_ALREADY_INITIALIZED;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceApplicationFinalize() {
    LOG_INFO(Lib_SysCore, "called");
    if (!g_is_initialized.exchange(false)) {
        return ORBIS_SYSCORE_ERROR_NOT_INITIALIZED;
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("XFYItOxS6r0", "libSceSysCore", 1, "libSceSysCore", sceApplicationInitialize);
    LIB_FUNCTION("SfPuQO4vW9M", "libSceSysCore", 1, "libSceSysCore", sceApplicationFinalize);
}

} // namespace Libraries::SysCore
