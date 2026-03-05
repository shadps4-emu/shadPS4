// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/libraries/sysmodule/sysmodule.h"
#include "core/libraries/sysmodule/sysmodule_error.h"
#include "core/libraries/sysmodule/sysmodule_internal.h"
#include "core/linker.h"

namespace Libraries::SysModule {

static std::mutex g_mutex{};

s32 PS4_SYSV_ABI sceSysmoduleGetModuleHandleInternal(OrbisSysModuleInternal id, s32* handle) {
    LOG_INFO(Lib_SysModule, "called");
    if ((id & 0x7fffffff) == 0) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    std::scoped_lock lk{g_mutex};
    return isModuleLoaded(id, handle);
}

s32 PS4_SYSV_ABI sceSysmoduleGetModuleInfoForUnwind(VAddr addr, s32 flags,
                                                    Kernel::OrbisModuleInfoForUnwind* info) {
    LOG_TRACE(Lib_SysModule, "sceSysmoduleGetModuleInfoForUnwind called");
    s32 res = Kernel::sceKernelGetModuleInfoForUnwind(addr, flags, info);
    if (res != ORBIS_OK) {
        return res;
    }

    if (shouldHideName(info->name.data())) {
        std::ranges::fill(info->name, '\0');
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmoduleIsCalledFromSysModule() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmoduleIsCameraPreloaded() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmoduleIsLoaded(OrbisSysModule id) {
    if (id == 0) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    std::scoped_lock lk{g_mutex};
    return isModuleLoaded(id, nullptr);
}

s32 PS4_SYSV_ABI sceSysmoduleIsLoadedInternal(OrbisSysModuleInternal id) {
    if ((id & 0x7fffffff) == 0) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    std::scoped_lock lk{g_mutex};
    return isModuleLoaded(id, nullptr);
}

s32 PS4_SYSV_ABI sceSysmoduleLoadModule(OrbisSysModule id) {
    LOG_INFO(Lib_SysModule, "called, id = {:#x}", id);
    s32 result = validateModuleId(id);
    if (result < ORBIS_OK) {
        return result;
    }

    // Only locks for internal loadModule call.
    {
        std::scoped_lock lk{g_mutex};
        result = loadModule(id, 0, nullptr, nullptr);
    }

    if (result == ORBIS_KERNEL_ERROR_ESTART) {
        s32 sdk_ver = 0;
        result = Kernel::sceKernelGetCompiledSdkVersion(&sdk_ver);
        if (sdk_ver < Common::ElfInfo::FW_115 || result != ORBIS_OK) {
            return ORBIS_KERNEL_ERROR_EINVAL;
        } else {
            return ORBIS_KERNEL_ERROR_ESTART;
        }
    }

    // The real library has some weird workaround for CUSA01478 and CUSA01495 here.
    // Unless this is proven necessary, I don't plan to handle this.
    return result;
}

s32 PS4_SYSV_ABI sceSysmoduleLoadModuleByNameInternal() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmoduleLoadModuleInternal(OrbisSysModuleInternal id) {
    LOG_INFO(Lib_SysModule, "called, id = {:#x}", id);
    s32 result = validateModuleId(id);
    if (result < ORBIS_OK) {
        return result;
    }

    // This specific module ID is loaded unlocked.
    if (id == 0x80000039) {
        return loadModule(id, 0, nullptr, nullptr);
    }
    std::scoped_lock lk{g_mutex};
    return loadModule(id, 0, nullptr, nullptr);
}

s32 PS4_SYSV_ABI sceSysmoduleLoadModuleInternalWithArg() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmoduleMapLibcForLibkernel() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmodulePreloadModuleForLibkernel() {
    LOG_DEBUG(Lib_SysModule, "called");
    return preloadModulesForLibkernel();
}

s32 PS4_SYSV_ABI sceSysmoduleUnloadModule() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmoduleUnloadModuleByNameInternal() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmoduleUnloadModuleInternal() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmoduleUnloadModuleInternalWithArg() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("D8cuU4d72xM", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleGetModuleHandleInternal);
    LIB_FUNCTION("4fU5yvOkVG4", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleGetModuleInfoForUnwind);
    LIB_FUNCTION("ctfO7dQ7geg", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleIsCalledFromSysModule);
    LIB_FUNCTION("no6T3EfiS3E", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleIsCameraPreloaded);
    LIB_FUNCTION("fMP5NHUOaMk", "libSceSysmodule", 1, "libSceSysmodule", sceSysmoduleIsLoaded);
    LIB_FUNCTION("ynFKQ5bfGks", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleIsLoadedInternal);
    LIB_FUNCTION("g8cM39EUZ6o", "libSceSysmodule", 1, "libSceSysmodule", sceSysmoduleLoadModule);
    LIB_FUNCTION("CU8m+Qs+HN4", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleLoadModuleByNameInternal);
    LIB_FUNCTION("39iV5E1HoCk", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleLoadModuleInternal);
    LIB_FUNCTION("hHrGoGoNf+s", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleLoadModuleInternalWithArg);
    LIB_FUNCTION("lZ6RvVl0vo0", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleMapLibcForLibkernel);
    LIB_FUNCTION("DOO+zuW1lrE", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmodulePreloadModuleForLibkernel);
    LIB_FUNCTION("eR2bZFAAU0Q", "libSceSysmodule", 1, "libSceSysmodule", sceSysmoduleUnloadModule);
    LIB_FUNCTION("vpTHmA6Knvg", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleUnloadModuleByNameInternal);
    LIB_FUNCTION("vXZhrtJxkGc", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleUnloadModuleInternal);
    LIB_FUNCTION("aKa6YfBKZs4", "libSceSysmodule", 1, "libSceSysmodule",
                 sceSysmoduleUnloadModuleInternalWithArg);
};

} // namespace Libraries::SysModule
