// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "common/singleton.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "core/memory.h"

namespace Libraries::Kernel {

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize() {
    LOG_TRACE(Kernel_Vmm, "called");
    const auto* memory = Core::Memory::Instance();
    return memory->GetTotalDirectSize();
}

s32 PS4_SYSV_ABI sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len,
                                               u64 alignment, s32 memoryType, s64* physAddrOut) {
    if (searchStart < 0 || searchEnd < 0) {
        LOG_ERROR(Kernel_Vmm, "Invalid parameters!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (len <= 0 || !Common::Is16KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Length {:#x} is invalid!", len);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0 && !Common::Is16KBAligned(alignment)) {
        LOG_ERROR(Kernel_Vmm, "Alignment {:#x} is invalid!", alignment);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (memoryType > 10) {
        LOG_ERROR(Kernel_Vmm, "Memory type {:#x} is invalid!", memoryType);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (physAddrOut == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Result physical address pointer is null!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    const bool is_in_range = searchEnd - searchStart >= len;
    if (searchEnd <= searchStart || searchEnd < len || !is_in_range) {
        LOG_ERROR(Kernel_Vmm,
                  "Provided address range is too small!"
                  " searchStart = {:#x}, searchEnd = {:#x}, length = {:#x}",
                  searchStart, searchEnd, len);
        return ORBIS_KERNEL_ERROR_EAGAIN;
    }

    auto* memory = Core::Memory::Instance();
    PAddr phys_addr = memory->Allocate(searchStart, searchEnd, len, alignment, memoryType);
    if (phys_addr == -1) {
        return ORBIS_KERNEL_ERROR_EAGAIN;
    }

    *physAddrOut = static_cast<s64>(phys_addr);

    LOG_INFO(Kernel_Vmm,
             "searchStart = {:#x}, searchEnd = {:#x}, len = {:#x}, "
             "alignment = {:#x}, memoryType = {:#x}, physAddrOut = {:#x}",
             searchStart, searchEnd, len, alignment, memoryType, phys_addr);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAllocateMainDirectMemory(u64 len, u64 alignment, s32 memoryType,
                                                   s64* physAddrOut) {
    const auto searchEnd = static_cast<s64>(sceKernelGetDirectMemorySize());
    return sceKernelAllocateDirectMemory(0, searchEnd, len, alignment, memoryType, physAddrOut);
}

s32 PS4_SYSV_ABI sceKernelCheckedReleaseDirectMemory(u64 start, u64 len) {
    if (len == 0) {
        return ORBIS_OK;
    }
    LOG_INFO(Kernel_Vmm, "called start = {:#x}, len = {:#x}", start, len);
    auto* memory = Core::Memory::Instance();
    memory->Free(start, len);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelReleaseDirectMemory(u64 start, u64 len) {
    if (len == 0) {
        return ORBIS_OK;
    }
    auto* memory = Core::Memory::Instance();
    memory->Free(start, len);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAvailableDirectMemorySize(u64 searchStart, u64 searchEnd, u64 alignment,
                                                    u64* physAddrOut, u64* sizeOut) {
    LOG_INFO(Kernel_Vmm, "called searchStart = {:#x}, searchEnd = {:#x}, alignment = {:#x}",
             searchStart, searchEnd, alignment);

    if (physAddrOut == nullptr || sizeOut == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* memory = Core::Memory::Instance();

    PAddr physAddr{};
    u64 size{};
    s32 result = memory->DirectQueryAvailable(searchStart, searchEnd, alignment, &physAddr, &size);

    if (size == 0) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }

    *physAddrOut = static_cast<u64>(physAddr);
    *sizeOut = size;

    return result;
}

s32 PS4_SYSV_ABI sceKernelVirtualQuery(const void* addr, s32 flags, OrbisVirtualQueryInfo* info,
                                       u64 infoSize) {
    LOG_INFO(Kernel_Vmm, "called addr = {}, flags = {:#x}", fmt::ptr(addr), flags);
    auto* memory = Core::Memory::Instance();
    return memory->VirtualQuery(std::bit_cast<VAddr>(addr), flags, info);
}

s32 PS4_SYSV_ABI sceKernelReserveVirtualRange(void** addr, u64 len, s32 flags, u64 alignment) {
    LOG_INFO(Kernel_Vmm, "addr = {}, len = {:#x}, flags = {:#x}, alignment = {:#x}",
             fmt::ptr(*addr), len, flags, alignment);
    if (addr == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Address is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (len == 0 || !Common::Is16KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 16KB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0) {
        if ((!std::has_single_bit(alignment) && !Common::Is16KBAligned(alignment))) {
            LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
    }

    auto* memory = Core::Memory::Instance();
    const VAddr in_addr = reinterpret_cast<VAddr>(*addr);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);

    s32 result = memory->MapMemory(addr, in_addr, len, Core::MemoryProt::NoAccess, map_flags,
                                   Core::VMAType::Reserved, "anon", false, -1, alignment);
    if (result == 0) {
        LOG_INFO(Kernel_Vmm, "out_addr = {}", fmt::ptr(*addr));
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelMapNamedDirectMemory(void** addr, u64 len, s32 prot, s32 flags,
                                               s64 directMemoryStart, u64 alignment,
                                               const char* name) {
    LOG_INFO(Kernel_Vmm,
             "in_addr = {}, len = {:#x}, prot = {:#x}, flags = {:#x}, "
             "directMemoryStart = {:#x}, alignment = {:#x}, name = '{}'",
             fmt::ptr(*addr), len, prot, flags, directMemoryStart, alignment, name);

    if (len == 0 || !Common::Is16KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 16KB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (!Common::Is16KBAligned(directMemoryStart)) {
        LOG_ERROR(Kernel_Vmm, "Start address is not 16KB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (alignment != 0) {
        if ((!std::has_single_bit(alignment) && !Common::Is16KBAligned(alignment))) {
            LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
    }

    if (std::strlen(name) >= ORBIS_KERNEL_MAXIMUM_NAME_LENGTH) {
        LOG_ERROR(Kernel_Vmm, "name exceeds 32 bytes!");
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    const VAddr in_addr = reinterpret_cast<VAddr>(*addr);
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);

    auto* memory = Core::Memory::Instance();
    const auto ret =
        memory->MapMemory(addr, in_addr, len, mem_prot, map_flags, Core::VMAType::Direct, name,
                          false, directMemoryStart, alignment);

    LOG_INFO(Kernel_Vmm, "out_addr = {}", fmt::ptr(*addr));
    return ret;
}

s32 PS4_SYSV_ABI sceKernelMapDirectMemory(void** addr, u64 len, s32 prot, s32 flags,
                                          s64 directMemoryStart, u64 alignment) {
    LOG_INFO(Kernel_Vmm, "called, redirected to sceKernelMapNamedDirectMemory");
    return sceKernelMapNamedDirectMemory(addr, len, prot, flags, directMemoryStart, alignment,
                                         "anon");
}

s32 PS4_SYSV_ABI sceKernelMapDirectMemory2(void** addr, u64 len, s32 type, s32 prot, s32 flags,
                                           s64 phys_addr, u64 alignment) {
    LOG_INFO(Kernel_Vmm, "called, redirected to sceKernelMapNamedDirectMemory");
    const s32 ret =
        sceKernelMapNamedDirectMemory(addr, len, prot, flags, phys_addr, alignment, "anon");

    if (ret == 0) {
        auto* memory = Core::Memory::Instance();
        memory->SetDirectMemoryType(phys_addr, type);
    }
    return ret;
}

s32 PS4_SYSV_ABI sceKernelMapNamedFlexibleMemory(void** addr_in_out, u64 len, s32 prot, s32 flags,
                                                 const char* name) {
    LOG_INFO(Kernel_Vmm, "in_addr = {}, len = {:#x}, prot = {:#x}, flags = {:#x}, name = '{}'",
             fmt::ptr(*addr_in_out), len, prot, flags, name);
    if (len == 0 || !Common::Is16KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "len is 0 or not 16kb multiple");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (name == nullptr) {
        LOG_ERROR(Kernel_Vmm, "name is invalid!");
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    if (std::strlen(name) >= ORBIS_KERNEL_MAXIMUM_NAME_LENGTH) {
        LOG_ERROR(Kernel_Vmm, "name exceeds 32 bytes!");
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    const VAddr in_addr = reinterpret_cast<VAddr>(*addr_in_out);
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);
    auto* memory = Core::Memory::Instance();
    const auto ret = memory->MapMemory(addr_in_out, in_addr, len, mem_prot, map_flags,
                                       Core::VMAType::Flexible, name);
    LOG_INFO(Kernel_Vmm, "out_addr = {}", fmt::ptr(*addr_in_out));
    return ret;
}

s32 PS4_SYSV_ABI sceKernelMapFlexibleMemory(void** addr_in_out, u64 len, s32 prot, s32 flags) {
    return sceKernelMapNamedFlexibleMemory(addr_in_out, len, prot, flags, "anon");
}

s32 PS4_SYSV_ABI sceKernelQueryMemoryProtection(void* addr, void** start, void** end, u32* prot) {
    auto* memory = Core::Memory::Instance();
    return memory->QueryProtection(std::bit_cast<VAddr>(addr), start, end, prot);
}

s32 PS4_SYSV_ABI sceKernelMprotect(const void* addr, u64 size, s32 prot) {
    LOG_INFO(Kernel_Vmm, "called addr = {}, size = {:#x}, prot = {:#x}", fmt::ptr(addr), size,
             prot);
    Core::MemoryManager* memory_manager = Core::Memory::Instance();
    Core::MemoryProt protection_flags = static_cast<Core::MemoryProt>(prot);
    return memory_manager->Protect(std::bit_cast<VAddr>(addr), size, protection_flags);
}

s32 PS4_SYSV_ABI posix_mprotect(const void* addr, u64 size, s32 prot) {
    s32 result = sceKernelMprotect(addr, size, prot);
    if (result < 0) {
        ErrSceToPosix(result);
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelMtypeprotect(const void* addr, u64 size, s32 mtype, s32 prot) {
    LOG_INFO(Kernel_Vmm, "called addr = {}, size = {:#x}, prot = {:#x}", fmt::ptr(addr), size,
             prot);
    Core::MemoryManager* memory_manager = Core::Memory::Instance();
    Core::MemoryProt protection_flags = static_cast<Core::MemoryProt>(prot);
    return memory_manager->Protect(std::bit_cast<VAddr>(addr), size, protection_flags);
}

s32 PS4_SYSV_ABI sceKernelDirectMemoryQuery(u64 offset, s32 flags, OrbisQueryInfo* query_info,
                                            u64 infoSize) {
    LOG_INFO(Kernel_Vmm, "called offset = {:#x}, flags = {:#x}", offset, flags);
    auto* memory = Core::Memory::Instance();
    return memory->DirectMemoryQuery(offset, flags == 1, query_info);
}

s32 PS4_SYSV_ABI sceKernelAvailableFlexibleMemorySize(u64* out_size) {
    auto* memory = Core::Memory::Instance();
    *out_size = memory->GetAvailableFlexibleSize();
    LOG_INFO(Kernel_Vmm, "called size = {:#x}", *out_size);
    return ORBIS_OK;
}

void PS4_SYSV_ABI _sceKernelRtldSetApplicationHeapAPI(void* func[]) {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    linker->SetHeapAPI(func);
}

s32 PS4_SYSV_ABI sceKernelGetDirectMemoryType(u64 addr, s32* directMemoryTypeOut,
                                              void** directMemoryStartOut,
                                              void** directMemoryEndOut) {
    LOG_WARNING(Kernel_Vmm, "called, direct memory addr = {:#x}", addr);
    auto* memory = Core::Memory::Instance();
    return memory->GetDirectMemoryType(addr, directMemoryTypeOut, directMemoryStartOut,
                                       directMemoryEndOut);
}

s32 PS4_SYSV_ABI sceKernelIsStack(void* addr, void** start, void** end) {
    LOG_DEBUG(Kernel_Vmm, "called, addr = {}", fmt::ptr(addr));
    auto* memory = Core::Memory::Instance();
    return memory->IsStack(std::bit_cast<VAddr>(addr), start, end);
}

u32 PS4_SYSV_ABI sceKernelIsAddressSanitizerEnabled() {
    LOG_DEBUG(Kernel, "called");
    return false;
}

s32 PS4_SYSV_ABI sceKernelBatchMap(OrbisKernelBatchMapEntry* entries, s32 numEntries,
                                   s32* numEntriesOut) {
    return sceKernelBatchMap2(entries, numEntries, numEntriesOut,
                              MemoryFlags::SCE_KERNEL_MAP_FIXED); // 0x10, 0x410?
}

s32 PS4_SYSV_ABI sceKernelBatchMap2(OrbisKernelBatchMapEntry* entries, s32 numEntries,
                                    s32* numEntriesOut, s32 flags) {
    s32 result = ORBIS_OK;
    s32 processed = 0;
    for (s32 i = 0; i < numEntries; i++, processed++) {
        if (entries == nullptr || entries[i].length == 0 || entries[i].operation > 4) {
            result = ORBIS_KERNEL_ERROR_EINVAL;
            break; // break and assign a value to numEntriesOut.
        }

        switch (entries[i].operation) {
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_MAP_DIRECT: {
            result = sceKernelMapNamedDirectMemory(&entries[i].start, entries[i].length,
                                                   entries[i].protection, flags,
                                                   static_cast<s64>(entries[i].offset), 0, "anon");
            LOG_INFO(Kernel_Vmm,
                     "entry = {}, operation = {}, len = {:#x}, offset = {:#x}, type = {}, "
                     "result = {}",
                     i, entries[i].operation, entries[i].length, entries[i].offset,
                     (u8)entries[i].type, result);
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_UNMAP: {
            result = sceKernelMunmap(entries[i].start, entries[i].length);
            LOG_INFO(Kernel_Vmm, "entry = {}, operation = {}, len = {:#x}, result = {}", i,
                     entries[i].operation, entries[i].length, result);
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_PROTECT: {
            result = sceKernelMprotect(entries[i].start, entries[i].length, entries[i].protection);
            LOG_INFO(Kernel_Vmm, "entry = {}, operation = {}, len = {:#x}, result = {}", i,
                     entries[i].operation, entries[i].length, result);
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_MAP_FLEXIBLE: {
            result = sceKernelMapNamedFlexibleMemory(&entries[i].start, entries[i].length,
                                                     entries[i].protection, flags, "anon");
            LOG_INFO(Kernel_Vmm,
                     "entry = {}, operation = {}, len = {:#x}, type = {}, "
                     "result = {}",
                     i, entries[i].operation, entries[i].length, (u8)entries[i].type, result);
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_TYPE_PROTECT: {
            result = sceKernelMtypeprotect(entries[i].start, entries[i].length, entries[i].type,
                                           entries[i].protection);
            LOG_INFO(Kernel_Vmm, "entry = {}, operation = {}, len = {:#x}, result = {}", i,
                     entries[i].operation, entries[i].length, result);
            break;
        }
        default: {
            UNREACHABLE();
        }
        }

        if (result != ORBIS_OK) {
            break;
        }
    }
    if (numEntriesOut != NULL) { // can be zero. do not return an error code.
        *numEntriesOut = processed;
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelSetVirtualRangeName(const void* addr, u64 len, const char* name) {
    if (name == nullptr) {
        LOG_ERROR(Kernel_Vmm, "name is invalid!");
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    if (std::strlen(name) >= ORBIS_KERNEL_MAXIMUM_NAME_LENGTH) {
        LOG_ERROR(Kernel_Vmm, "name exceeds 32 bytes!");
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    auto* memory = Core::Memory::Instance();
    memory->NameVirtualRange(std::bit_cast<VAddr>(addr), len, name);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolExpand(u64 searchStart, u64 searchEnd, u64 len, u64 alignment,
                                           u64* physAddrOut) {
    if (searchStart < 0 || searchEnd <= searchStart) {
        LOG_ERROR(Kernel_Vmm, "Provided address range is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (len <= 0 || !Common::Is64KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Provided length {:#x} is invalid!", len);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0 && !Common::Is64KBAligned(alignment)) {
        LOG_ERROR(Kernel_Vmm, "Alignment {:#x} is invalid!", alignment);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (physAddrOut == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Result physical address pointer is null!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    const bool is_in_range = searchEnd - searchStart >= len;
    if (searchEnd <= searchStart || searchEnd < len || !is_in_range) {
        LOG_ERROR(Kernel_Vmm,
                  "Provided address range is too small!"
                  " searchStart = {:#x}, searchEnd = {:#x}, length = {:#x}",
                  searchStart, searchEnd, len);
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }

    auto* memory = Core::Memory::Instance();
    PAddr phys_addr = memory->PoolExpand(searchStart, searchEnd, len, alignment);
    if (phys_addr == -1) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }

    *physAddrOut = static_cast<s64>(phys_addr);

    LOG_INFO(Kernel_Vmm,
             "searchStart = {:#x}, searchEnd = {:#x}, len = {:#x}, alignment = {:#x}, physAddrOut "
             "= {:#x}",
             searchStart, searchEnd, len, alignment, phys_addr);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolReserve(void* addr_in, u64 len, u64 alignment, s32 flags,
                                            void** addr_out) {
    LOG_INFO(Kernel_Vmm, "addr_in = {}, len = {:#x}, alignment = {:#x}, flags = {:#x}",
             fmt::ptr(addr_in), len, alignment, flags);

    if (len == 0 || !Common::Is2MBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 2MB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0) {
        if ((!std::has_single_bit(alignment) && !Common::Is2MBAligned(alignment))) {
            LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
    }

    auto* memory = Core::Memory::Instance();
    const VAddr in_addr = reinterpret_cast<VAddr>(addr_in);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);
    u64 map_alignment = alignment == 0 ? 2_MB : alignment;

    return memory->MapMemory(addr_out, std::bit_cast<VAddr>(addr_in), len,
                             Core::MemoryProt::NoAccess, map_flags, Core::VMAType::PoolReserved,
                             "anon", false, -1, map_alignment);
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolCommit(void* addr, u64 len, s32 type, s32 prot, s32 flags) {
    if (addr == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Address is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (len == 0 || !Common::Is64KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 64KB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    LOG_INFO(Kernel_Vmm, "addr = {}, len = {:#x}, type = {:#x}, prot = {:#x}, flags = {:#x}",
             fmt::ptr(addr), len, type, prot, flags);

    const VAddr in_addr = reinterpret_cast<VAddr>(addr);
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    auto* memory = Core::Memory::Instance();
    return memory->PoolCommit(in_addr, len, mem_prot);
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolDecommit(void* addr, u64 len, s32 flags) {
    if (addr == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Address is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (len == 0 || !Common::Is64KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 64KB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    LOG_INFO(Kernel_Vmm, "addr = {}, len = {:#x}, flags = {:#x}", fmt::ptr(addr), len, flags);

    const VAddr pool_addr = reinterpret_cast<VAddr>(addr);
    auto* memory = Core::Memory::Instance();

    return memory->PoolDecommit(pool_addr, len);
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolBatch(const OrbisKernelMemoryPoolBatchEntry* entries, s32 count,
                                          s32* num_processed, s32 flags) {
    if (entries == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    s32 result = ORBIS_OK;
    s32 processed = 0;

    for (s32 i = 0; i < count; i++, processed++) {
        OrbisKernelMemoryPoolBatchEntry entry = entries[i];
        switch (entry.opcode) {
        case OrbisKernelMemoryPoolOpcode::Commit: {
            result = sceKernelMemoryPoolCommit(entry.commit_params.addr, entry.commit_params.len,
                                               entry.commit_params.type, entry.commit_params.prot,
                                               entry.flags);
            break;
        }
        case OrbisKernelMemoryPoolOpcode::Decommit: {
            result = sceKernelMemoryPoolDecommit(entry.decommit_params.addr,
                                                 entry.decommit_params.len, entry.flags);
            break;
        }
        case OrbisKernelMemoryPoolOpcode::Protect: {
            result = sceKernelMprotect(entry.protect_params.addr, entry.protect_params.len,
                                       entry.protect_params.prot);
            break;
        }
        case OrbisKernelMemoryPoolOpcode::TypeProtect: {
            result = sceKernelMtypeprotect(
                entry.type_protect_params.addr, entry.type_protect_params.len,
                entry.type_protect_params.type, entry.type_protect_params.prot);
            break;
        }
        case OrbisKernelMemoryPoolOpcode::Move: {
            UNREACHABLE_MSG("Unimplemented sceKernelMemoryPoolBatch opcode Move");
        }
        default: {
            result = ORBIS_KERNEL_ERROR_EINVAL;
            break;
        }
        }

        if (result != ORBIS_OK) {
            break;
        }
    }

    if (num_processed != nullptr) {
        *num_processed = processed;
    }
    return result;
}

void* PS4_SYSV_ABI posix_mmap(void* addr, u64 len, s32 prot, s32 flags, s32 fd, s64 phys_addr) {
    LOG_INFO(
        Kernel_Vmm,
        "called addr = {}, len = {:#x}, prot = {:#x}, flags = {:#x}, fd = {}, phys_addr = {:#x}",
        fmt::ptr(addr), len, prot, flags, fd, phys_addr);

    if (len == 0) {
        // If length is 0, mmap returns EINVAL.
        ErrSceToPosix(ORBIS_KERNEL_ERROR_EINVAL);
        return reinterpret_cast<void*>(-1);
    }

    void* addr_out;
    auto* memory = Core::Memory::Instance();
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    const auto mem_flags = static_cast<Core::MemoryMapFlags>(flags);
    const auto is_exec = True(mem_prot & Core::MemoryProt::CpuExec);

    s32 result = ORBIS_OK;
    if (fd == -1) {
        result = memory->MapMemory(&addr_out, std::bit_cast<VAddr>(addr), len, mem_prot, mem_flags,
                                   Core::VMAType::Flexible, "anon", is_exec);
    } else {
        result = memory->MapFile(&addr_out, std::bit_cast<VAddr>(addr), len, mem_prot, mem_flags,
                                 fd, phys_addr);
    }

    if (result != ORBIS_OK) {
        // If the memory mappings fail, mmap sets errno to the appropriate error code,
        // then returns (void*)-1;
        ErrSceToPosix(result);
        return reinterpret_cast<void*>(-1);
    }

    return addr_out;
}

s32 PS4_SYSV_ABI sceKernelMmap(void* addr, u64 len, s32 prot, s32 flags, s32 fd, s64 phys_addr,
                               void** res) {
    void* addr_out = posix_mmap(addr, len, prot, flags, fd, phys_addr);

    if (addr_out == reinterpret_cast<void*>(-1)) {
        // posix_mmap failed, calculate and return the appropriate kernel error code using errno.
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }

    // Set the outputted address
    *res = addr_out;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelConfiguredFlexibleMemorySize(u64* sizeOut) {
    if (sizeOut == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* memory = Core::Memory::Instance();
    *sizeOut = memory->GetTotalFlexibleSize();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMunmap(void* addr, u64 len) {
    LOG_INFO(Kernel_Vmm, "addr = {}, len = {:#x}", fmt::ptr(addr), len);
    if (len == 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    auto* memory = Core::Memory::Instance();
    return memory->UnmapMemory(std::bit_cast<VAddr>(addr), len);
}

s32 PS4_SYSV_ABI posix_munmap(void* addr, u64 len) {
    s32 result = sceKernelMunmap(addr, len);
    if (result < 0) {
        LOG_ERROR(Kernel_Pthread, "posix_munmap: error = {}", result);
        ErrSceToPosix(result);
        return -1;
    }
    return result;
}

static constexpr s32 MAX_PRT_APERTURES = 3;
static constexpr VAddr PRT_AREA_START_ADDR = 0x1000000000;
static constexpr u64 PRT_AREA_SIZE = 0xec00000000;
static std::array<std::pair<VAddr, u64>, MAX_PRT_APERTURES> PrtApertures{};

s32 PS4_SYSV_ABI sceKernelSetPrtAperture(s32 id, VAddr address, u64 size) {
    if (id < 0 || id >= MAX_PRT_APERTURES) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (address < PRT_AREA_START_ADDR || address + size > PRT_AREA_START_ADDR + PRT_AREA_SIZE) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (address % 4096 != 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    LOG_WARNING(Kernel_Vmm,
                "PRT aperture id = {}, address = {:#x}, size = {:#x} is set but not used", id,
                address, size);

    auto* memory = Core::Memory::Instance();
    memory->SetPrtArea(id, address, size);

    PrtApertures[id] = {address, size};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelGetPrtAperture(s32 id, VAddr* address, u64* size) {
    if (id < 0 || id >= MAX_PRT_APERTURES) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::tie(*address, *size) = PrtApertures[id];
    return ORBIS_OK;
}

void RegisterMemory(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, sceKernelAllocateDirectMemory);
    LIB_FUNCTION("B+vc2AO2Zrc", "libkernel", 1, "libkernel", 1, 1,
                 sceKernelAllocateMainDirectMemory);
    LIB_FUNCTION("C0f7TJcbfac", "libkernel", 1, "libkernel", 1, 1,
                 sceKernelAvailableDirectMemorySize);
    LIB_FUNCTION("hwVSPCmp5tM", "libkernel", 1, "libkernel", 1, 1,
                 sceKernelCheckedReleaseDirectMemory);
    LIB_FUNCTION("rVjRvHJ0X6c", "libkernel", 1, "libkernel", 1, 1, sceKernelVirtualQuery);
    LIB_FUNCTION("7oxv3PPCumo", "libkernel", 1, "libkernel", 1, 1, sceKernelReserveVirtualRange);
    LIB_FUNCTION("BC+OG5m9+bw", "libkernel", 1, "libkernel", 1, 1, sceKernelGetDirectMemoryType);
    LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, sceKernelGetDirectMemorySize);
    LIB_FUNCTION("yDBwVAolDgg", "libkernel", 1, "libkernel", 1, 1, sceKernelIsStack);
    LIB_FUNCTION("jh+8XiK4LeE", "libkernel", 1, "libkernel", 1, 1,
                 sceKernelIsAddressSanitizerEnabled);
    LIB_FUNCTION("NcaWUxfMNIQ", "libkernel", 1, "libkernel", 1, 1, sceKernelMapNamedDirectMemory);
    LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, sceKernelMapDirectMemory);
    LIB_FUNCTION("BQQniolj9tQ", "libkernel", 1, "libkernel", 1, 1, sceKernelMapDirectMemory2);
    LIB_FUNCTION("WFcfL2lzido", "libkernel", 1, "libkernel", 1, 1, sceKernelQueryMemoryProtection);
    LIB_FUNCTION("BHouLQzh0X0", "libkernel", 1, "libkernel", 1, 1, sceKernelDirectMemoryQuery);
    LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
    LIB_FUNCTION("PGhQHd-dzv8", "libkernel", 1, "libkernel", 1, 1, sceKernelMmap);
    LIB_FUNCTION("cQke9UuBQOk", "libkernel", 1, "libkernel", 1, 1, sceKernelMunmap);
    LIB_FUNCTION("mL8NDH86iQI", "libkernel", 1, "libkernel", 1, 1, sceKernelMapNamedFlexibleMemory);
    LIB_FUNCTION("aNz11fnnzi4", "libkernel", 1, "libkernel", 1, 1,
                 sceKernelAvailableFlexibleMemorySize);
    LIB_FUNCTION("aNz11fnnzi4", "libkernel_avlfmem", 1, "libkernel", 1, 1,
                 sceKernelAvailableFlexibleMemorySize);
    LIB_FUNCTION("IWIBBdTHit4", "libkernel", 1, "libkernel", 1, 1, sceKernelMapFlexibleMemory);
    LIB_FUNCTION("p5EcQeEeJAE", "libkernel", 1, "libkernel", 1, 1,
                 _sceKernelRtldSetApplicationHeapAPI);
    LIB_FUNCTION("2SKEx6bSq-4", "libkernel", 1, "libkernel", 1, 1, sceKernelBatchMap);
    LIB_FUNCTION("kBJzF8x4SyE", "libkernel", 1, "libkernel", 1, 1, sceKernelBatchMap2);
    LIB_FUNCTION("DGMG3JshrZU", "libkernel", 1, "libkernel", 1, 1, sceKernelSetVirtualRangeName);
    LIB_FUNCTION("n1-v6FgU7MQ", "libkernel", 1, "libkernel", 1, 1,
                 sceKernelConfiguredFlexibleMemorySize);

    LIB_FUNCTION("vSMAm3cxYTY", "libkernel", 1, "libkernel", 1, 1, sceKernelMprotect);
    LIB_FUNCTION("YQOfxL4QfeU", "libkernel", 1, "libkernel", 1, 1, posix_mprotect);
    LIB_FUNCTION("YQOfxL4QfeU", "libScePosix", 1, "libkernel", 1, 1, posix_mprotect);
    LIB_FUNCTION("9bfdLIyuwCY", "libkernel", 1, "libkernel", 1, 1, sceKernelMtypeprotect);

    // Memory pool
    LIB_FUNCTION("qCSfqDILlns", "libkernel", 1, "libkernel", 1, 1, sceKernelMemoryPoolExpand);
    LIB_FUNCTION("pU-QydtGcGY", "libkernel", 1, "libkernel", 1, 1, sceKernelMemoryPoolReserve);
    LIB_FUNCTION("Vzl66WmfLvk", "libkernel", 1, "libkernel", 1, 1, sceKernelMemoryPoolCommit);
    LIB_FUNCTION("LXo1tpFqJGs", "libkernel", 1, "libkernel", 1, 1, sceKernelMemoryPoolDecommit);
    LIB_FUNCTION("YN878uKRBbE", "libkernel", 1, "libkernel", 1, 1, sceKernelMemoryPoolBatch);

    LIB_FUNCTION("BPE9s9vQQXo", "libkernel", 1, "libkernel", 1, 1, posix_mmap);
    LIB_FUNCTION("BPE9s9vQQXo", "libScePosix", 1, "libkernel", 1, 1, posix_mmap);
    LIB_FUNCTION("UqDGjXA5yUM", "libkernel", 1, "libkernel", 1, 1, posix_munmap);
    LIB_FUNCTION("UqDGjXA5yUM", "libScePosix", 1, "libkernel", 1, 1, posix_munmap);

    // PRT memory management
    LIB_FUNCTION("BohYr-F7-is", "libkernel", 1, "libkernel", 1, 1, sceKernelSetPrtAperture);
    LIB_FUNCTION("L0v2Go5jOuM", "libkernel", 1, "libkernel", 1, 1, sceKernelGetPrtAperture);
}

} // namespace Libraries::Kernel
