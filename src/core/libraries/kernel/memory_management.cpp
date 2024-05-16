// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#include "common/alignment.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/memory_management.h"
#include "core/memory.h"

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

    auto* memory = Core::Memory::Instance();
    PAddr phys_addr = memory->Allocate(searchStart, searchEnd, len, alignment, memoryType);
    *physAddrOut = static_cast<s64>(phys_addr);
    LOG_INFO(Kernel_Vmm, "physAddrOut = {:#x}", phys_addr);
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

    const VAddr in_addr = reinterpret_cast<VAddr>(*addr);
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);
    auto* memory = Core::Memory::Instance();
    return memory->MapMemory(addr, in_addr, len, mem_prot, map_flags, Core::VMAType::Direct, "",
                             directMemoryStart, alignment);
}

s32 PS4_SYSV_ABI sceKernelMapNamedFlexibleMemory(void** addr_in_out, std::size_t len, int prot,
                                                 int flags, const char* name) {

    if (len == 0 || !Common::is16KBAligned(len)) {
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

    const VAddr in_addr = reinterpret_cast<VAddr>(*addr_in_out);
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);
    auto* memory = Core::Memory::Instance();
    const int ret = memory->MapMemory(addr_in_out, in_addr, len, mem_prot, map_flags,
                                      Core::VMAType::Flexible, name);

    LOG_INFO(Kernel_Vmm, "addr = {}, len = {:#x}, prot = {:#x}, flags = {:#x}",
             fmt::ptr(*addr_in_out), len, prot, flags);
    return ret;
}

s32 PS4_SYSV_ABI sceKernelMapFlexibleMemory(void** addr_in_out, std::size_t len, int prot,
                                            int flags) {
    return sceKernelMapNamedFlexibleMemory(addr_in_out, len, prot, flags, "");
}

} // namespace Libraries::Kernel
