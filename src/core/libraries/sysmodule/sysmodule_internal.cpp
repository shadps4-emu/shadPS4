// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/sysmodule/sysmodule_error.h"
#include "core/libraries/sysmodule/sysmodule_internal.h"
#include "core/libraries/sysmodule/sysmodule_table.h"

namespace Libraries::SysModule {

s32 isModuleLoaded(s32 id, s32* handle) {
    if (id == 0) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }
    for (OrbisSysmoduleModuleInternal mod : g_modules_array) {
        if (mod.id != id) {
            continue;
        }
        if (mod.is_loaded < 1) {
            return ORBIS_SYSMODULE_NOT_LOADED;
        }
        if (handle != nullptr) {
            *handle = mod.handle;
        }
        return ORBIS_OK;
    }
    return ORBIS_SYSMODULE_INVALID_ID;
}

bool shouldHideName(const char* module_name) {
    for (u64 i = 0; i < g_num_modules; i++) {
        OrbisSysmoduleModuleInternal mod = g_modules_array[i];
        if ((mod.flags & OrbisSysmoduleModuleInternalFlags::IsGame) == 0) {
            continue;
        }
        u64 name_length = std::strlen(mod.name);
        char name_copy[0x100];
        std::strncpy(name_copy, mod.name, sizeof(name_copy));
        // Module table stores names without extensions, so check with .prx appended to the name.
        std::strncpy(&name_copy[name_length], ".prx", 4);
        s32 result = std::strncmp(module_name, name_copy, sizeof(name_copy));
        if (result == 0) {
            return true;
        }

        // libSceFios2 and libc are checked as both sprx or prx modules.
        if (i == 3) {
            result = std::strncmp(module_name, "libSceFios2.sprx", sizeof(name_copy));
        } else if (i == 4) {
            result = std::strncmp(module_name, "libc.sprx", sizeof(name_copy));
        }

        if (result == 0) {
            return true;
        }
    }
    return false;
}

bool isDebugModule(s32 id) {
    for (OrbisSysmoduleModuleInternal mod : g_modules_array) {
        if (mod.id == id && (mod.flags & OrbisSysmoduleModuleInternalFlags::IsDebug) != 0) {
            return true;
        }
    }
    return false;
}

bool validateModuleId(s32 id) {
    if ((id & 0x7fffffff) == 0) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    s32 sdk_ver = 0;
    ASSERT_MSG(!Kernel::sceKernelGetCompiledSdkVersion(&sdk_ver),
               "Failed to retrieve compiled SDK version");
    if (id == 0xb8 && sdk_ver >= Common::ElfInfo::FW_75) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }
    if (id == 0xb0 && sdk_ver >= Common::ElfInfo::FW_70) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }
    if (id == 0x80 && sdk_ver >= Common::ElfInfo::FW_30) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }
    if (isDebugModule(id) && !Config::isDevKitConsole()) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }
    return ORBIS_OK;
}

s32 loadModuleInternal(s32 index, s32 argc, void** argv, s32* res_out) {
    // If the module is already loaded, increment is_loaded and return ORBIS_OK.
    OrbisSysmoduleModuleInternal& mod = g_modules_array[index];
    if (mod.is_loaded > 0) {
        mod.is_loaded++;
        return ORBIS_OK;
    }

    LOG_ERROR(Lib_SysModule, "(STUBBED) Loading {}", mod.name);
    return ORBIS_OK;
}

s32 loadModule(s32 id, s32 argc, void** argv, s32* res_out) {
    // Retrieve the module to load from the table
    OrbisSysmoduleModuleInternal requested_module{};
    for (OrbisSysmoduleModuleInternal mod : g_modules_array) {
        if (mod.id == id) {
            requested_module = mod;
            break;
        }
    }
    if (requested_module.id != id || requested_module.id == 0) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    // Every module has a pointer to an array of indexes to modules that need loading.
    if (requested_module.to_load == nullptr) {
        // Seems like ORBIS_SYSMODULE_LOCK_FAILED is a generic internal error.
        return ORBIS_SYSMODULE_LOCK_FAILED;
    }

    // Loop through every module that requires loading, in reverse order
    for (s64 i = requested_module.num_to_load - 1; i >= 0; i--) {
        // Modules flagged as debug modules only load for devkits
        u32 mod_index = requested_module.to_load[i];
        if ((!Config::isDevKitConsole() &&
             g_modules_array[mod_index].flags & OrbisSysmoduleModuleInternalFlags::IsDebug) != 0) {
            continue;
        }

        // Arguments and result should only be applied to the requested module
        // Dependencies don't receive these values.
        s32 result = 0;
        if (i != 0) {
            result = loadModuleInternal(mod_index, 0, nullptr, nullptr);
        } else {
            result = loadModuleInternal(mod_index, argc, argv, res_out);
        }

        // If loading any module fails, abort there.
        if (result != ORBIS_OK) {
            return result;
        }
    }
    return ORBIS_OK;
}

} // namespace Libraries::SysModule