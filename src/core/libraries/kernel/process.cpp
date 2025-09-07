// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/linker.h"

namespace Libraries::Kernel {

s32 PS4_SYSV_ABI sceKernelIsInSandbox() {
    return 1;
}

s32 PS4_SYSV_ABI sceKernelIsNeoMode() {
    return Config::isNeoModeConsole() &&
           Common::ElfInfo::Instance().GetPSFAttributes().support_neo_mode;
}

s32 PS4_SYSV_ABI sceKernelGetCompiledSdkVersion(s32* ver) {
    s32 version = Common::ElfInfo::Instance().RawFirmwareVer();
    *ver = version;
    return (version >= 0) ? ORBIS_OK : ORBIS_KERNEL_ERROR_EINVAL;
}

s32 PS4_SYSV_ABI sceKernelGetCpumode() {
    return 0;
}

void* PS4_SYSV_ABI sceKernelGetProcParam() {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    return linker->GetProcParam();
}

s32 PS4_SYSV_ABI sceKernelLoadStartModule(const char* moduleFileName, u64 args, const void* argp,
                                          u32 flags, const void* pOpt, s32* pRes) {
    LOG_INFO(Lib_Kernel, "called filename = {}, args = {}", moduleFileName, args);
    ASSERT(flags == 0);

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    auto* linker = Common::Singleton<Core::Linker>::Instance();

    std::filesystem::path path;
    std::string guest_path(moduleFileName);

    s32 handle = -1;

    if (guest_path[0] == '/') {
        // try load /system/common/lib/ +path
        // try load /system/priv/lib/   +path
        path = mnt->GetHostPath(guest_path);
        handle = linker->LoadAndStartModule(path, args, argp, pRes);
        if (handle != -1)
            return handle;
    } else {
        if (!guest_path.contains('/')) {
            path = mnt->GetHostPath("/app0/" + guest_path);
            handle = linker->LoadAndStartModule(path, args, argp, pRes);
            if (handle != -1)
                return handle;
            // if ((flags & 0x10000) != 0)
            //  try load /system/priv/lib/   +basename
            //  try load /system/common/lib/ +basename
        } else {
            path = mnt->GetHostPath(guest_path);
            handle = linker->LoadAndStartModule(path, args, argp, pRes);
            if (handle != -1)
                return handle;
        }
    }

    return ORBIS_KERNEL_ERROR_ENOENT;
}

s32 PS4_SYSV_ABI sceKernelDlsym(s32 handle, const char* symbol, void** addrp) {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    auto* module = linker->GetModule(handle);
    if (module == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    *addrp = module->FindByName(symbol);
    if (*addrp == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetModuleInfoForUnwind(VAddr addr, s32 flags,
                                                 OrbisModuleInfoForUnwind* info) {
    if (flags >= 3) {
        std::memset(info, 0, sizeof(OrbisModuleInfoForUnwind));
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (!info) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    if (info->st_size < sizeof(OrbisModuleInfoForUnwind)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    // Find module that contains specified address.
    LOG_INFO(Lib_Kernel, "called addr = {:#x}, flags = {:#x}", addr, flags);
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    auto* module = linker->FindByAddress(addr);
    if (!module) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    const auto mod_info = module->GetModuleInfoEx();

    // Fill in module info.
    std::memset(info, 0, sizeof(OrbisModuleInfoForUnwind));
    info->name = mod_info.name;
    info->eh_frame_hdr_addr = mod_info.eh_frame_hdr_addr;
    info->eh_frame_addr = mod_info.eh_frame_addr;
    info->eh_frame_size = mod_info.eh_frame_size;
    info->seg0_addr = mod_info.segments[0].address;
    info->seg0_size = mod_info.segments[0].size;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetModuleInfoFromAddr(VAddr addr, s32 flags,
                                                Core::OrbisKernelModuleInfoEx* info) {
    if (flags >= 3) {
        std::memset(info, 0, sizeof(Core::OrbisKernelModuleInfoEx));
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (info == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    LOG_INFO(Lib_Kernel, "called addr = {:#x}, flags = {:#x}", addr, flags);
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    auto* module = linker->FindByAddress(addr);
    if (!module) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }

    *info = module->GetModuleInfoEx();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetModuleInfo(s32 handle, Core::OrbisKernelModuleInfo* info) {
    if (info == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    if (info->st_size != sizeof(Core::OrbisKernelModuleInfo)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* linker = Common::Singleton<Core::Linker>::Instance();
    auto* module = linker->GetModule(handle);
    if (module == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    *info = module->GetModuleInfo();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetModuleInfoInternal(s32 handle, Core::OrbisKernelModuleInfoEx* info) {
    if (info == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    if (info->st_size != sizeof(Core::OrbisKernelModuleInfoEx)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* linker = Common::Singleton<Core::Linker>::Instance();
    auto* module = linker->GetModule(handle);
    if (module == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    *info = module->GetModuleInfoEx();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetModuleList(s32* handles, u64 num_array, u64* out_count) {
    if (handles == nullptr || out_count == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    auto* linker = Common::Singleton<Core::Linker>::Instance();
    u64 count = 0;
    auto* module = linker->GetModule(count);
    while (module != nullptr && count < num_array) {
        handles[count] = count;
        count++;
        module = linker->GetModule(count);
    }

    if (count == num_array && module != nullptr) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }

    *out_count = count;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI exit(s32 status) {
    UNREACHABLE_MSG("Exiting with status code {}", status);
    return 0;
}

void RegisterProcess(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("xeu-pV8wkKs", "libkernel", 1, "libkernel", 1, 1, sceKernelIsInSandbox);
    LIB_FUNCTION("WB66evu8bsU", "libkernel", 1, "libkernel", 1, 1, sceKernelGetCompiledSdkVersion);
    LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, sceKernelIsNeoMode);
    LIB_FUNCTION("VOx8NGmHXTs", "libkernel", 1, "libkernel", 1, 1, sceKernelGetCpumode);
    LIB_FUNCTION("959qrazPIrg", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcParam);
    LIB_FUNCTION("wzvqT4UqKX8", "libkernel", 1, "libkernel", 1, 1, sceKernelLoadStartModule);
    LIB_FUNCTION("LwG8g3niqwA", "libkernel", 1, "libkernel", 1, 1, sceKernelDlsym);
    LIB_FUNCTION("RpQJJVKTiFM", "libkernel", 1, "libkernel", 1, 1, sceKernelGetModuleInfoForUnwind);
    LIB_FUNCTION("f7KBOafysXo", "libkernel", 1, "libkernel", 1, 1, sceKernelGetModuleInfoFromAddr);
    LIB_FUNCTION("kUpgrXIrz7Q", "libkernel", 1, "libkernel", 1, 1, sceKernelGetModuleInfo);
    LIB_FUNCTION("HZO7xOos4xc", "libkernel", 1, "libkernel", 1, 1, sceKernelGetModuleInfoInternal);
    LIB_FUNCTION("IuxnUuXk6Bg", "libkernel", 1, "libkernel", 1, 1, sceKernelGetModuleList);
    LIB_FUNCTION("6Z83sYWFlA8", "libkernel", 1, "libkernel", 1, 1, exit);
}

} // namespace Libraries::Kernel
