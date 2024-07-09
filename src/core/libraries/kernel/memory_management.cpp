// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#include "common/alignment.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/memory_management.h"
#include "core/linker.h"
#include "core/memory.h"

namespace Libraries::Kernel {

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize() {
    LOG_WARNING(Kernel_Vmm, "called");
    return SCE_KERNEL_MAIN_DMEM_SIZE;
}

int PS4_SYSV_ABI sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len,
                                               u64 alignment, int memoryType, s64* physAddrOut) {
    if (searchStart < 0 || searchEnd <= searchStart) {
        LOG_ERROR(Kernel_Vmm, "Provided address range is invalid!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    const bool is_in_range = searchEnd - searchStart >= len;
    if (len <= 0 || !Common::Is16KBAligned(len) || !is_in_range) {
        LOG_ERROR(Kernel_Vmm, "Provided address range is invalid!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0 && !Common::Is16KBAligned(alignment)) {
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

    LOG_INFO(Kernel_Vmm,
             "searchStart = {:#x}, searchEnd = {:#x}, len = {:#x}, "
             "alignment = {:#x}, memoryType = {:#x}, physAddrOut = {:#x}",
             searchStart, searchEnd, len, alignment, memoryType, phys_addr);

    return SCE_OK;
}

s32 PS4_SYSV_ABI sceKernelAllocateMainDirectMemory(size_t len, size_t alignment, int memoryType,
                                                   s64* physAddrOut) {
    return sceKernelAllocateDirectMemory(0, SCE_KERNEL_MAIN_DMEM_SIZE, len, alignment, memoryType,
                                         physAddrOut);
}

s32 PS4_SYSV_ABI sceKernelCheckedReleaseDirectMemory(u64 start, size_t len) {
    LOG_INFO(Kernel_Vmm, "called start = {:#x}, len = {:#x}", start, len);
    auto* memory = Core::Memory::Instance();
    memory->Free(start, len);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelReleaseDirectMemory(u64 start, size_t len) {
    auto* memory = Core::Memory::Instance();
    memory->Free(start, len);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAvailableDirectMemorySize(u64 searchStart, u64 searchEnd,
                                                    size_t alignment, u64* physAddrOut,
                                                    size_t* sizeOut) {
    LOG_WARNING(Kernel_Vmm, "called searchStart = {:#x}, searchEnd = {:#x}, alignment = {:#x}",
                searchStart, searchEnd, alignment);
    auto* memory = Core::Memory::Instance();

    PAddr physAddr;
    s32 size = memory->DirectQueryAvailable(searchStart, searchEnd, alignment, &physAddr, sizeOut);
    *physAddrOut = static_cast<u64>(physAddr);

    return size;
}

s32 PS4_SYSV_ABI sceKernelVirtualQuery(const void* addr, int flags, OrbisVirtualQueryInfo* info,
                                       size_t infoSize) {
    LOG_INFO(Kernel_Vmm, "called addr = {}, flags = {:#x}", fmt::ptr(addr), flags);
    if (!addr) {
        return SCE_KERNEL_ERROR_EACCES;
    }
    auto* memory = Core::Memory::Instance();
    return memory->VirtualQuery(std::bit_cast<VAddr>(addr), flags, info);
}

s32 PS4_SYSV_ABI sceKernelReserveVirtualRange(void** addr, u64 len, int flags, u64 alignment) {
    LOG_INFO(Kernel_Vmm, "addr = {}, len = {:#x}, flags = {:#x}, alignment = {:#x}",
             fmt::ptr(*addr), len, flags, alignment);

    if (addr == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Address is invalid!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (len == 0 || !Common::Is16KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 16KB aligned!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0) {
        if ((!std::has_single_bit(alignment) && !Common::Is16KBAligned(alignment))) {
            LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
            return SCE_KERNEL_ERROR_EINVAL;
        }
    }

    auto* memory = Core::Memory::Instance();
    const VAddr in_addr = reinterpret_cast<VAddr>(*addr);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);
    memory->Reserve(addr, in_addr, len, map_flags, alignment);

    return SCE_OK;
}

int PS4_SYSV_ABI sceKernelMapNamedDirectMemory(void** addr, u64 len, int prot, int flags,
                                               s64 directMemoryStart, u64 alignment,
                                               const char* name) {
    LOG_INFO(Kernel_Vmm,
             "addr = {}, len = {:#x}, prot = {:#x}, flags = {:#x}, directMemoryStart = {:#x}, "
             "alignment = {:#x}",
             fmt::ptr(*addr), len, prot, flags, directMemoryStart, alignment);

    if (len == 0 || !Common::Is16KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 16KB aligned!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (!Common::Is16KBAligned(directMemoryStart)) {
        LOG_ERROR(Kernel_Vmm, "Start address is not 16KB aligned!");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0) {
        if ((!std::has_single_bit(alignment) && !Common::Is16KBAligned(alignment))) {
            LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
            return SCE_KERNEL_ERROR_EINVAL;
        }
    }

    const VAddr in_addr = reinterpret_cast<VAddr>(*addr);
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);
    auto* memory = Core::Memory::Instance();
    return memory->MapMemory(addr, in_addr, len, mem_prot, map_flags, Core::VMAType::Direct, "",
                             false, directMemoryStart, alignment);
}

int PS4_SYSV_ABI sceKernelMapDirectMemory(void** addr, u64 len, int prot, int flags,
                                          s64 directMemoryStart, u64 alignment) {
    LOG_INFO(Kernel_Vmm, "called, redirected to sceKernelMapNamedDirectMemory");
    return sceKernelMapNamedDirectMemory(addr, len, prot, flags, directMemoryStart, alignment, "");
}

s32 PS4_SYSV_ABI sceKernelMapNamedFlexibleMemory(void** addr_in_out, std::size_t len, int prot,
                                                 int flags, const char* name) {

    if (len == 0 || !Common::Is16KBAligned(len)) {
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

int PS4_SYSV_ABI sceKernelQueryMemoryProtection(void* addr, void** start, void** end, u32* prot) {
    auto* memory = Core::Memory::Instance();
    return memory->QueryProtection(std::bit_cast<VAddr>(addr), start, end, prot);
}

int PS4_SYSV_ABI sceKernelDirectMemoryQuery(u64 offset, int flags, OrbisQueryInfo* query_info,
                                            size_t infoSize) {
    LOG_WARNING(Kernel_Vmm, "called offset = {:#x}, flags = {:#x}", offset, flags);
    auto* memory = Core::Memory::Instance();
    return memory->DirectMemoryQuery(offset, flags == 1, query_info);
}

s32 PS4_SYSV_ABI sceKernelAvailableFlexibleMemorySize(size_t* out_size) {
    auto* memory = Core::Memory::Instance();
    *out_size = memory->GetAvailableFlexibleSize();
    LOG_INFO(Kernel_Vmm, "called size = {:#x}", *out_size);
    return ORBIS_OK;
}

void PS4_SYSV_ABI _sceKernelRtldSetApplicationHeapAPI(void* func) {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    linker->SetHeapApiFunc(func);
}

} // namespace Libraries::Kernel
