// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/memory_management.h"
#include "core/libraries/kernel/physical_memory.h"
#include "core/virtual_memory.h"

namespace Libraries::Kernel {

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize() {
    LOG_WARNING(Kernel_Vmm, "called");
    return SCE_KERNEL_MAIN_DMEM_SIZE;
}

int PS4_SYSV_ABI sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len,
                                               u64 alignment, int memoryType, s64* physAddrOut) {
    LOG_INFO(Kernel_Vmm,
             "searchStart = {:#x}, searchEnd = {:#x}, len = {:#x}, alignment = {:#x}, memoryType = "
             "{:#x}",
             searchStart, searchEnd, len, alignment, memoryType);

    if (searchStart < 0 || searchEnd <= searchStart) {
        LOG_ERROR(Kernel_Vmm, "Provided address range is invalid!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    const bool is_in_range = (searchStart < len && searchEnd > len);
    if (len <= 0 || !Common::is16KBAligned(len) || !is_in_range) {
        LOG_ERROR(Kernel_Vmm, "Provided address range is invalid!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if ((alignment != 0 || Common::is16KBAligned(alignment)) && !std::has_single_bit(alignment)) {
        LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (physAddrOut == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Result physical address pointer is null!");
        return SCE_KERNEL_ERROR_EINVAL;
    }

    u64 physical_addr = 0;
    auto* physical_memory = Common::Singleton<PhysicalMemory>::Instance();
    if (!physical_memory->Alloc(searchStart, searchEnd, len, alignment, &physical_addr,
                                memoryType)) {
        LOG_CRITICAL(Kernel_Vmm, "Unable to allocate physical memory");
        return SCE_KERNEL_ERROR_EAGAIN;
    }
    *physAddrOut = static_cast<s64>(physical_addr);
    LOG_INFO(Kernel_Vmm, "physAddrOut = {:#x}", physical_addr);
    return SCE_OK;
}

int PS4_SYSV_ABI sceKernelMapDirectMemory(void** addr, u64 len, int prot, int flags,
                                          s64 directMemoryStart, u64 alignment) {
    LOG_INFO(
        Kernel_Vmm,
        "len = {:#x}, prot = {:#x}, flags = {:#x}, directMemoryStart = {:#x}, alignment = {:#x}",
        len, prot, flags, directMemoryStart, alignment);

    if (len == 0 || !Common::is16KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 16KB aligned!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (!Common::is16KBAligned(directMemoryStart)) {
        LOG_ERROR(Kernel_Vmm, "Start address is not 16KB aligned!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0) {
        if ((!std::has_single_bit(alignment) && !Common::is16KBAligned(alignment))) {
            LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
            return SCE_KERNEL_ERROR_EINVAL;
        }
    }

    VirtualMemory::MemoryMode cpu_mode = VirtualMemory::MemoryMode::NoAccess;

    switch (prot) {
    case 0x03:
        cpu_mode = VirtualMemory::MemoryMode::ReadWrite;
        break;
    case 0x32:
    case 0x33: // SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_READ|SCE_KERNEL_PROT_GPU_ALL
        cpu_mode = VirtualMemory::MemoryMode::ReadWrite;
        break;
    default:
        UNREACHABLE();
    }

    auto in_addr = reinterpret_cast<u64>(*addr);
    u64 out_addr = 0;

    if (flags == 0) {
        out_addr = VirtualMemory::memory_alloc_aligned(in_addr, len, cpu_mode, alignment);
    }
    LOG_INFO(Kernel_Vmm, "in_addr = {:#x}, out_addr = {:#x}", in_addr, out_addr);

    *addr = reinterpret_cast<void*>(out_addr); // return out_addr to first functions parameter

    if (out_addr == 0) {
        return SCE_KERNEL_ERROR_ENOMEM;
    }

    auto* physical_memory = Common::Singleton<PhysicalMemory>::Instance();
    if (!physical_memory->Map(out_addr, directMemoryStart, len, prot, cpu_mode)) {
        UNREACHABLE();
    }

    return SCE_OK;
}

} // namespace Libraries::Kernel
