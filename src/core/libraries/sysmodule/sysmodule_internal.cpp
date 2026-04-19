// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/emulator_settings.h"
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
#include "core/libraries/system_gesture/system_gesture.h"
#include "core/linker.h"
#include "emulator.h"

namespace Libraries::SysModule {

s32 getModuleHandle(s32 id, s32* handle) {
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
    if (isDebugModule(id) && !EmulatorSettings.IsDevKit()) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    return ORBIS_OK;
}

s32 loadModuleInternal(s32 index, s32 argc, const void* argv, s32* res_out) {
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
        guest_path.append(".prx");
        const auto& host_path = mnt->GetHostPath(guest_path);

        // For convenience, load through linker directly instead of loading through libkernel calls.
        s32 result = linker->LoadAndStartModule(host_path, argc, argv, &start_result);
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
        if (index == 0xd && EmulatorSettings.IsDevKit()) {
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
        if (index == 0xb3 && EmulatorSettings.IsDevKit()) {
            mod_name.append("_debug");
        }

        if ((mod.flags & OrbisSysmoduleModuleInternalFlags::IsNeo) == 0 &&
            (mod.flags & OrbisSysmoduleModuleInternalFlags::IsNeoMode) != 0 &&
            Kernel::sceKernelIsNeoMode() == 1) {
            // PS4 Pro running in enhanced mode
            mod_name.append("ForNeoMode");
        } else if ((mod.flags & OrbisSysmoduleModuleInternalFlags::IsNeo) != 0 &&
                   EmulatorSettings.IsNeo()) {
            // PS4 Pro running in base mode
            mod_name.append("ForNeo");
        }

        // Append .sprx extension.
        mod_name.append(".sprx");

        // Now we need to check if the requested library is allowed to LLE.
        // First, we allow all modules from game-specific sys_modules
        const auto& sys_module_path = EmulatorSettings.GetSysModulesDir();
        const auto& game_specific_module_path =
            sys_module_path / game_info->GameSerial() / mod_name;
        if (std::filesystem::exists(game_specific_module_path)) {
            // The requested module is present in the game-specific sys_modules, load it.
            LOG_INFO(Loader, "Loading {} from game serial file {}", mod_name,
                     game_info->GameSerial());
            s32 handle =
                linker->LoadAndStartModule(game_specific_module_path, argc, argv, &start_result);
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
             {"libSceFreeTypeOt.sprx", nullptr},
             {"libSceSystemGesture.sprx", &Libraries::SystemGesture::RegisterLib}});

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
            s32 handle = linker->LoadAndStartModule(module_path, argc, argv, &start_result);
            ASSERT_MSG(handle >= 0, "Failed to load module {}", mod_name);
            mod.handle = handle;
        } else {
            // Allowed LLE that isn't present, log message
            auto& [name, init_func] = *it;
            if (init_func) {
                LOG_INFO(Loader, "Can't Load {} switching to HLE", mod_name);
                init_func(&linker->GetHLESymbols());

                // When loading HLEs, we need to relocate imports
                // This ensures later module loads can see our HLE functions.
                linker->RelocateAllImports();
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

s32 loadModule(s32 id, s32 argc, const void* argv, s32* res_out) {
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
        if ((!EmulatorSettings.IsDevKit() &&
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

s32 unloadModule(s32 id, s32 argc, const void* argv, s32* res_out, bool is_internal) {
    OrbisSysmoduleModuleInternal mod{};
    for (s32 i = 0; i < g_modules_array.size(); i++) {
        mod = g_modules_array[i];
        if (mod.id != id) {
            continue;
        }

        // Skips checking libSceDiscMap
        if (i == 0x22) {
            continue;
        }

        // If the module is loaded once, and is part of the second preload list,
        // then return OK and do nothing.
        for (s32 index : g_preload_list_2) {
            if (index == i && mod.is_loaded == 1) {
                return ORBIS_OK;
            }
        }

        // Found the correct module.
        break;
    }

    // If we failed to locate the module, return invalid id.
    if (mod.id != id || mod.id == 0) {
        return ORBIS_SYSMODULE_INVALID_ID;
    }

    // If the module has no dependencies, then return an internal error.
    if (mod.num_to_load == 0 || mod.to_load == nullptr) {
        return ORBIS_SYSMODULE_LOCK_FAILED;
    }

    // Unload the module and it's dependencies
    for (s64 i = 0; i < mod.num_to_load; i++) {
        OrbisSysmoduleModuleInternal dep_mod = g_modules_array[mod.to_load[i]];
        // If this is a debug module and we're not emulating a devkit, skip it.
        if ((dep_mod.flags & OrbisSysmoduleModuleInternalFlags::IsDebug) != 0 &&
            !EmulatorSettings.IsDevKit()) {
            continue;
        }

        // If the module to unload is marked as unloaded, then return not loaded
        if (dep_mod.is_loaded == 0) {
            return ORBIS_SYSMODULE_NOT_LOADED;
        }

        // By this point, all necessary checks are performed, decrement the load count.
        dep_mod.is_loaded--;

        // Normally, this is where the real library would actually unload the module,
        // through a call to sceKernelStopUnloadModule.
        // As we don't implement module unloading, this behavior is skipped.

        // Stub success during requested module unload.
        if (i == 0 && res_out != nullptr) {
            *res_out = ORBIS_OK;
        }
    }
    return ORBIS_OK;
}

s32 preloadModulesForLibkernel() {
    // For now, default to loading g_preload_list_3.
    // As far as I can tell, g_preload_list_1 seems to be some sort of list with libs
    // that games don't typically use, and g_preload_list_2 is just a reorganized version of 3.
    s32 sdk_ver = 0;
    ASSERT_MSG(Kernel::sceKernelGetCompiledSdkVersion(&sdk_ver) == 0,
               "Failed to get compiled SDK version");
    for (u32 module_index : g_preload_list_3) {
        // As per usual, these are arrays of indexes for g_modules_array
        // libSceDbg, libScePerf, libSceMat, and libSceRazorCpu_debug.
        // These are skipped unless this console is a devkit.
        if ((module_index == 0x12 || module_index == 0x1e || module_index == 0x24 ||
             module_index == 0x26) &&
            !EmulatorSettings.IsDevKit()) {
            continue;
        }

        // libSceDiscMap case, skipped on newer SDK versions.
        if (module_index == 0x22 && sdk_ver >= Common::ElfInfo::FW_20) {
            continue;
        }

        // libSceDbgAssist is skipped on non-testkit consoles.
        // For now, stub check to non-devkit.
        if (module_index == 0x23 && !EmulatorSettings.IsDevKit()) {
            continue;
        }

        // libSceRazorCpu, skipped for old non-devkit consoles.
        if (module_index == 0x25 && sdk_ver < Common::ElfInfo::FW_45 &&
            !EmulatorSettings.IsDevKit()) {
            continue;
        }

        // libSceHttp2, skipped for SDK versions below 7.00.
        if (module_index == 0x28 && sdk_ver < Common::ElfInfo::FW_70) {
            continue;
        }

        // libSceNpWebApi2 and libSceNpGameIntent, skipped for SDK versions below 7.50
        if ((module_index == 0x29 || module_index == 0x2a) && sdk_ver < Common::ElfInfo::FW_75) {
            continue;
        }

        // Load the actual module
        s32 result = loadModuleInternal(module_index, 0, nullptr, nullptr);
        if (result != ORBIS_OK) {
            // On real hardware, module preloading must succeed or the game will abort.
            // To enable users to test homebrew easier, we'll log a critical error instead.
            LOG_CRITICAL(Lib_SysModule, "Failed to preload {}, expect crashes",
                         g_modules_array[module_index].name);
        }
    }
    return ORBIS_OK;
}

} // namespace Libraries::SysModule
