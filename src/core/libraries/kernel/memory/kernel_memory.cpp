// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include <common/singleton.h>
#include <core/libraries/error_codes.h>
#include <core/libraries/libs.h>
#include <core/virtual_memory.h>
#include "flexible_memory.h"
#include "kernel_memory.h"

namespace Libraries::Kernel {

bool Is16KBMultiple(u64 n) {
    return ((n % (16ull * 1024) == 0));
}
s32 PS4_SYSV_ABI sceKernelMapNamedFlexibleMemory(void** addr_in_out, std::size_t len, int prot,
                                                 int flags, const char* name) {

    LOG_INFO(Kernel_Vmm, "len = {:#x}, prot = {:#x}, flags = {:#x}, name = {}", len, prot, flags,
             name);

    if (len == 0 || !Is16KBMultiple(len)) {
        LOG_ERROR(Kernel_Vmm, "len is 0 or not 16kb multiple");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    static constexpr size_t MaxNameSize = 32;
    if (std::strlen(name) > MaxNameSize) {
        LOG_ERROR(Kernel_Vmm, "name exceeds 32 bytes!");
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    if (name == nullptr) {
        LOG_ERROR(Kernel_Vmm, "name is invalid!");
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    VirtualMemory::MemoryMode cpu_mode = VirtualMemory::MemoryMode::NoAccess;

    switch (prot) {
    case 0x3:
        cpu_mode = VirtualMemory::MemoryMode::ReadWrite;
        break;
    default:
        UNREACHABLE();
    }

    auto in_addr = reinterpret_cast<u64>(*addr_in_out);
    auto out_addr = VirtualMemory::memory_alloc(in_addr, len, cpu_mode);
    *addr_in_out = reinterpret_cast<void*>(out_addr);

    auto* flexible_memory = Common::Singleton<FlexibleMemory>::Instance();

    if (!flexible_memory->Map(out_addr, len, prot, cpu_mode)) {
        UNREACHABLE();
    }

    if (out_addr == 0) {
        LOG_ERROR(Kernel_Vmm, "Can't allocate address");
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }
    LOG_INFO(Kernel_Vmm, "in_addr = {:#x} out_addr = {:#x}", in_addr, out_addr);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMapFlexibleMemory(void** addr_in_out, std::size_t len, int prot,
                                            int flags) {
    return sceKernelMapNamedFlexibleMemory(addr_in_out, len, prot, flags, "");
}

void RegisterKernelMemory(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("mL8NDH86iQI", "libkernel", 1, "libkernel", 1, 1, sceKernelMapNamedFlexibleMemory);
    LIB_FUNCTION("IWIBBdTHit4", "libkernel", 1, "libkernel", 1, 1, sceKernelMapFlexibleMemory);
}

} // namespace Libraries::Kernel