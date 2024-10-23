// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
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
    return (version > 0) ? ORBIS_OK : ORBIS_KERNEL_ERROR_EINVAL;
}

int PS4_SYSV_ABI sceKernelGetCpumode() {
    return 0;
}

void* PS4_SYSV_ABI sceKernelGetProcParam() {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    return linker->GetProcParam();
}

void RegisterProcess(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("WB66evu8bsU", "libkernel", 1, "libkernel", 1, 1, sceKernelGetCompiledSdkVersion);
    LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, sceKernelIsNeoMode);
    LIB_FUNCTION("VOx8NGmHXTs", "libkernel", 1, "libkernel", 1, 1, sceKernelGetCpumode);
    LIB_FUNCTION("959qrazPIrg", "libkernel", 1, "libkernel", 1, 1, sceKernelGetProcParam);
}

} // namespace Libraries::Kernel
