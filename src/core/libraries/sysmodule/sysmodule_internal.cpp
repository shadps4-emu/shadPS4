// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/file_sys/fs.h"
#include "core/libraries/disc_map/disc_map.h"
#include "core/libraries/font/font.h"
#include "core/libraries/font/fontft.h"
#include "core/libraries/jpeg/jpegenc.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libc_internal/libc_internal.h"
#include "core/libraries/libpng/pngenc.h"
#include "core/libraries/libs.h"
#include "core/libraries/ngs2/ngs2.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/sysmodule/sysmodule_error.h"
#include "core/libraries/sysmodule/sysmodule_internal.h"
#include "core/libraries/sysmodule/sysmodule_table.h"
#include "core/linker.h"
#include "emulator.h"

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

    // libSceGameCustomDialog isn't loadable on SDK >= 7.50
    if (id == 0xb8 && sdk_ver >= Common::ElfInfo::FW_75) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    // libSceNpSnsFacebookDialog isn't loadable on SDK >= 7.00
    if (id == 0xb0 && sdk_ver >= Common::ElfInfo::FW_70) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    // libSceJson isn't loadable on SDK >= 3.00
    if (id == 0x80 && sdk_ver >= Common::ElfInfo::FW_30) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    // Cannot load debug modules on retail hardware.
    if (isDebugModule(id) && !Config::isDevKitConsole()) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    return ORBIS_OK;
}

s32 loadModuleInternal(s32 index, s32 argc, void** argv, s32* res_out) {
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    auto* game_info = Common::Singleton<Common::ElfInfo>::Instance();

    // If the module is already loaded, increment is_loaded and return ORBIS_OK.
    OrbisSysmoduleModuleInternal& mod = g_modules_array[index];
    if (mod.is_loaded > 0) {
        mod.is_loaded++;
        return ORBIS_OK;
    }

    s32 start_result = 0;
    // Most of the logic the actual module has here is to get the correct location of this module.
    // Since we only care about a small subset of LLEs, we can simplify this logic.
    if ((mod.flags & OrbisSysmoduleModuleInternalFlags::IsGame) != 0) {
        std::string guest_path = std::string("/app0/sce_module/").append(mod.name);
        // libSceJobManager and libSceFios2 have debug variants, which are loaded in some cases.
        if ((index == 0xe9 || index == 3) && Config::isDevKitConsole()) {
            guest_path.append("_debug");
        }
        guest_path.append(".prx");
        const auto& host_path = mnt->GetHostPath(guest_path);

        // For convenience, load through linker directly instead of loading through libkernel calls.
        // Library ignores arguments for game modules
        s32 result = linker->LoadAndStartModule(host_path, 0, nullptr, &start_result);
        // If the module is missing, the library prints a very helpful message for developers.
        // We'll just log an error.
        if (result < 0) {
            LOG_ERROR(Lib_SysModule, "Failed to load game library {}", guest_path);
            return result;
        } else {
            // On success, the library validates module params and the module SDK version.
            // We don't store the information this uses, so skip the proper checks.
            mod.handle = result;
            mod.is_loaded++;
        }
    } else {
        // This is not a game library. We'll need to perform some checks,
        // but we don't need to perform the path resolution logic the actual library has.
        std::string mod_name = std::string(mod.name);

        // libSceGnmDriver case
        if (index == 0xd && Config::isDevKitConsole()) {
            // There are some other checks involved here that I am not familiar with.
            // Since we're not exactly running libSceGnmDriver LLE, this shouldn't matter too much.
            mod_name.append("_padebug");
        }

        // libSceSsl2 case
        if (index == 0x27 && false /*needs legacy ssl*/) {
            // Replaces module name with libSceSsl (index 0x15)
            mod_name.clear();
            mod_name.append(g_modules_array[0x15].name);
        }

        // libSceVrTracker case
        if (index == 0xb3 && Config::isDevKitConsole()) {
            mod_name.append("_debug");
        }

        if ((mod.flags & OrbisSysmoduleModuleInternalFlags::IsNeo) == 0 &&
            (mod.flags & OrbisSysmoduleModuleInternalFlags::IsNeoMode) != 0 &&
            Kernel::sceKernelIsNeoMode() == 1) {
            // PS4 Pro running in enhanced mode
            mod_name.append("ForNeoMode");
        } else if ((mod.flags & OrbisSysmoduleModuleInternalFlags::IsNeo) != 0 &&
                   Config::isNeoModeConsole()) {
            // PS4 Pro running in base mode
            mod_name.append("ForNeo");
        }

        // Append .sprx extension.
        mod_name.append(".sprx");

        // Now we need to check if the requested library is allowed to LLE.
        // First, we allow all modules from game-specific sys_modules
        const auto& sys_module_path = Config::getSysModulesPath();
        const auto& game_specific_module_path =
            sys_module_path / game_info->GameSerial() / mod_name;
        if (std::filesystem::exists(game_specific_module_path)) {
            // The requested module is present in the game-specific sys_modules, load it.
            LOG_INFO(Loader, "Loading {} from game serial file {}", mod_name,
                     game_info->GameSerial());
            s32 handle =
                linker->LoadAndStartModule(game_specific_module_path, 0, nullptr, &start_result);
            ASSERT_MSG(handle >= 0, "Failed to load module {}", mod_name);
            mod.handle = handle;
            mod.is_loaded++;
            if (res_out != nullptr) {
                *res_out = start_result;
            }
            return ORBIS_OK;
        }

        // We need to check a few things here.
        // First, check if this is a module we allow LLE for.
        static s32 stub_handle = 100;
        constexpr auto ModulesToLoad = std::to_array<Core::SysModules>(
            {{"libSceNgs2.sprx", &Libraries::Ngs2::RegisterLib},
             {"libSceUlt.sprx", nullptr},
             {"libSceRtc.sprx", &Libraries::Rtc::RegisterLib},
             {"libSceJpegDec.sprx", nullptr},
             {"libSceJpegEnc.sprx", &Libraries::JpegEnc::RegisterLib},
             {"libScePngEnc.sprx", &Libraries::PngEnc::RegisterLib},
             {"libSceJson.sprx", nullptr},
             {"libSceJson2.sprx", nullptr},
             {"libSceLibcInternal.sprx", &Libraries::LibcInternal::RegisterLib},
             {"libSceCesCs.sprx", nullptr},
             {"libSceAudiodec.sprx", nullptr},
             {"libSceFont.sprx", &Libraries::Font::RegisterlibSceFont},
             {"libSceFontFt.sprx", &Libraries::FontFt::RegisterlibSceFontFt},
             {"libSceFreeTypeOt.sprx", nullptr}});

        // Iterate through the allowed array
        const auto it = std::ranges::find_if(
            ModulesToLoad, [&](Core::SysModules module) { return mod_name == module.module_name; });
        if (it == ModulesToLoad.end()) {
            // Not an allowed LLE, stub success without warning.
            mod.is_loaded++;
            // Some internal checks rely on a handle, stub a valid one.
            mod.handle = stub_handle++;
            if (res_out != nullptr) {
                *res_out = ORBIS_OK;
            }
            return ORBIS_OK;
        }

        // Allowed module, check if it exists
        const auto& module_path = sys_module_path / mod_name;
        if (std::filesystem::exists(module_path)) {
            LOG_INFO(Loader, "Loading {}", mod_name);
            s32 handle = linker->LoadAndStartModule(module_path, 0, nullptr, &start_result);
            ASSERT_MSG(handle >= 0, "Failed to load module {}", mod_name);
            mod.handle = handle;
        } else {
            // Allowed LLE that isn't present, log message
            auto& [name, init_func] = *it;
            if (init_func) {
                LOG_INFO(Loader, "Can't Load {} switching to HLE", mod_name);
                init_func(&linker->GetHLESymbols());
            } else {
                LOG_INFO(Loader, "No HLE available for {} module", mod_name);
            }
            mod.handle = stub_handle++;
        }

        // Mark module as loaded.
        mod.is_loaded++;
    }

    // Only successful loads will reach here
    if (res_out != nullptr) {
        *res_out = start_result;
    }

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

    LOG_INFO(Lib_SysModule, "Loading {}", requested_module.name);

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