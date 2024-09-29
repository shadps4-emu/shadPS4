// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/cpu_management.h"

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

} // namespace Libraries::Kernel
