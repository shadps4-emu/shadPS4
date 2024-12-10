// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
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

int PS4_SYSV_ABI sceKernelIsNeoMode() {
    LOG_DEBUG(Kernel_Sce, "called");
    return Config::isNeoMode();
}

int PS4_SYSV_ABI sceKernelGetCompiledSdkVersion(int* ver) {
    int version = Common::ElfInfo::Instance().RawFirmwareVer();
    *ver = version;
    return (version >= 0) ? ORBIS_OK : ORBIS_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI sceKernelGetCpumode() {
    return 0;
}

void* PS4_SYSV_ABI sceKernelGetProcParam() {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    return linker->GetProcParam();
}

s32 PS4_SYSV_ABI sceKernelLoadStartModule(const char* moduleFileName, size_t args, const void* argp,
                                          u32 flags, const void* pOpt, int* pRes) {
    LOG_INFO(Lib_Kernel, "called filename = {}, args = {}", moduleFileName, args);

    if (flags != 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    const auto path = mnt->GetHostPath(moduleFileName);

    // Load PRX module and relocate any modules that import it.
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    u32 handle = linker->FindByName(path);
    if (handle != -1) {
        return handle;
    }
    handle = linker->LoadModule(path, true);
    auto* module = linker->GetModule(handle);
    linker->RelocateAnyImports(module);

    // If the new module has a TLS image, trigger its load when TlsGetAddr is called.
    if (module->tls.image_size != 0) {
        linker->AdvanceGenerationCounter();
    }

    // Retrieve and verify proc param according to libkernel.
    u64* param = module->GetProcParam<u64*>();
    ASSERT_MSG(!param || param[0] >= 0x18, "Invalid module param size: {}", param[0]);
    s32 ret = module->Start(args, argp, param);
    if (pRes) {
        *pRes = ret;
    }

    return handle;
}

s32 PS4_SYSV_ABI sceKernelDlsym(s32 handle, const char* symbol, void** addrp) {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    auto* module = linker->GetModule(handle);
    *addrp = module->FindByName(symbol);
    if (*addrp == nullptr) {
        return ORBIS_KERNEL_ERROR_ESRCH;
    }
    return ORBIS_OK;
}

static constexpr size_t ORBIS_DBG_MAX_NAME_LENGTH = 256;

struct OrbisModuleInfoForUnwind {
    u64 st_size;
    std::array<char, ORBIS_DBG_MAX_NAME_LENGTH> name;
    VAddr eh_frame_hdr_addr;
    VAddr eh_frame_addr;
    u64 eh_frame_size;
    VAddr seg0_addr;
    u64 seg0_size;
};

s32 PS4_SYSV_ABI sceKernelGetModuleInfoForUnwind(VAddr addr, int flags,
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
        return ORBIS_KERNEL_ERROR_EFAULT;
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

int PS4_SYSV_ABI sceKernelGetModuleInfoFromAddr(VAddr addr, int flags,
                                                Core::OrbisKernelModuleInfoEx* info) {
    LOG_INFO(Lib_Kernel, "called addr = {:#x}, flags = {:#x}", addr, flags);
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    auto* module = linker->FindByAddress(addr);
    *info = module->GetModuleInfoEx();
    return ORBIS_OK;
}

void RegisterProcess(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("WB66evu8bsU", "libkernel", 1, "libkernel", 1, 1, sceKernelGetCompiledSdkVersion);
    LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, sceKernelIsNeoMode);
    LIB_FUNCTION("VOx8NGmHXTs", "libkernel", 1, "libkernel", 1, 1, sceKernelGetCpumode);
    LIB_FUNCTION("959qrazPIrg", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcParam);
    LIB_FUNCTION("wzvqT4UqKX8", "libkernel", 1, "libkernel", 1, 1, sceKernelLoadStartModule);
    LIB_FUNCTION("LwG8g3niqwA", "libkernel", 1, "libkernel", 1, 1, sceKernelDlsym);
    LIB_FUNCTION("RpQJJVKTiFM", "libkernel", 1, "libkernel", 1, 1, sceKernelGetModuleInfoForUnwind);
    LIB_FUNCTION("f7KBOafysXo", "libkernel", 1, "libkernel", 1, 1, sceKernelGetModuleInfoFromAddr);
}

} // namespace Libraries::Kernel
