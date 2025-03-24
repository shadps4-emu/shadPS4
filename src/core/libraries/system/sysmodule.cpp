// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 300
#include <magic_enum/magic_enum.hpp>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/sysmodule.h"
#include "core/libraries/system/system_error.h"

namespace Libraries::SysModule {

int PS4_SYSV_ABI sceSysmoduleGetModuleHandleInternal() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSysmoduleGetModuleInfoForUnwind(VAddr addr, s32 flags, void* info) {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    Kernel::OrbisModuleInfoForUnwind module_info;
    module_info.st_size = 0x130;
    s32 res = Kernel::sceKernelGetModuleInfoForUnwind(addr, flags, &module_info);
    return res;
}

int PS4_SYSV_ABI sceSysmoduleIsCalledFromSysModule() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleIsCameraPreloaded() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleIsLoaded(OrbisSysModule id) {
    LOG_ERROR(Lib_SysModule, "(DUMMY) called module = {}", magic_enum::enum_name(id));
    if (static_cast<u16>(id) == 0) {
        LOG_ERROR(Lib_SysModule, "Invalid sysmodule ID: {:#x}", static_cast<u16>(id));
        return ORBIS_SYSMODULE_INVALID_ID;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleIsLoadedInternal(OrbisSysModuleInternal id) {
    LOG_ERROR(Lib_SysModule, "(DUMMY) called module = {:#x}", static_cast<u32>(id));
    if ((static_cast<u32>(id) & 0x7FFFFFFF) == 0) {
        LOG_ERROR(Lib_SysModule, "Invalid internal sysmodule ID: {:#x}", static_cast<u32>(id));
        return ORBIS_SYSMODULE_INVALID_ID;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleLoadModule(OrbisSysModule id) {
    auto color_name = magic_enum::enum_name(id);
    LOG_ERROR(Lib_SysModule, "(DUMMY) called module = {}", magic_enum::enum_name(id));
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleLoadModuleByNameInternal() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleLoadModuleInternal() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleLoadModuleInternalWithArg() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleMapLibcForLibkernel() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmodulePreloadModuleForLibkernel() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleUnloadModule() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleUnloadModuleByNameInternal() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleUnloadModuleInternal() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSysmoduleUnloadModuleInternalWithArg() {
    LOG_ERROR(Lib_SysModule, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceSysmodule(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("D8cuU4d72xM", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleGetModuleHandleInternal);
    LIB_FUNCTION("4fU5yvOkVG4", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleGetModuleInfoForUnwind);
    LIB_FUNCTION("ctfO7dQ7geg", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleIsCalledFromSysModule);
    LIB_FUNCTION("no6T3EfiS3E", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleIsCameraPreloaded);
    LIB_FUNCTION("fMP5NHUOaMk", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleIsLoaded);
    LIB_FUNCTION("ynFKQ5bfGks", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleIsLoadedInternal);
    LIB_FUNCTION("g8cM39EUZ6o", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleLoadModule);
    LIB_FUNCTION("CU8m+Qs+HN4", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleLoadModuleByNameInternal);
    LIB_FUNCTION("39iV5E1HoCk", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleLoadModuleInternal);
    LIB_FUNCTION("hHrGoGoNf+s", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleLoadModuleInternalWithArg);
    LIB_FUNCTION("lZ6RvVl0vo0", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleMapLibcForLibkernel);
    LIB_FUNCTION("DOO+zuW1lrE", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmodulePreloadModuleForLibkernel);
    LIB_FUNCTION("eR2bZFAAU0Q", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleUnloadModule);
    LIB_FUNCTION("vpTHmA6Knvg", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleUnloadModuleByNameInternal);
    LIB_FUNCTION("vXZhrtJxkGc", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleUnloadModuleInternal);
    LIB_FUNCTION("aKa6YfBKZs4", "libSceSysmodule", 1, "libSceSysmodule", 1, 1,
                 sceSysmoduleUnloadModuleInternalWithArg);
};

} // namespace Libraries::SysModule
