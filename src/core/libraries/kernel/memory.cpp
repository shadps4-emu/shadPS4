// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "core/memory.h"

namespace Libraries::Kernel {

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize() {
    LOG_WARNING(Kernel_Vmm, "called");
    const auto* memory = Core::Memory::Instance();
    return memory->GetTotalDirectSize();
}

int PS4_SYSV_ABI sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len,
                                               u64 alignment, int memoryType, s64* physAddrOut) {
    if (searchStart < 0 || searchEnd <= searchStart) {
        LOG_ERROR(Kernel_Vmm, "Provided address range is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const bool is_in_range = searchEnd - searchStart >= len;
    if (len <= 0 || !Common::Is16KBAligned(len) || !is_in_range) {
        LOG_ERROR(Kernel_Vmm, "Provided address range is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0 && !Common::Is16KBAligned(alignment)) {
        LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (physAddrOut == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Result physical address pointer is null!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* memory = Core::Memory::Instance();
    PAddr phys_addr = memory->Allocate(searchStart, searchEnd, len, alignment, memoryType);
    *physAddrOut = static_cast<s64>(phys_addr);

    LOG_INFO(Kernel_Vmm,
             "searchStart = {:#x}, searchEnd = {:#x}, len = {:#x}, "
             "alignment = {:#x}, memoryType = {:#x}, physAddrOut = {:#x}",
             searchStart, searchEnd, len, alignment, memoryType, phys_addr);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAllocateMainDirectMemory(size_t len, size_t alignment, int memoryType,
                                                   s64* physAddrOut) {
    const auto searchEnd = static_cast<s64>(sceKernelGetDirectMemorySize());
    return sceKernelAllocateDirectMemory(0, searchEnd, len, alignment, memoryType, physAddrOut);
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

    if (physAddrOut == nullptr || sizeOut == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (searchEnd > sceKernelGetDirectMemorySize()) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (searchEnd <= searchStart) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }

    auto* memory = Core::Memory::Instance();

    PAddr physAddr{};
    size_t size{};
    s32 result = memory->DirectQueryAvailable(searchStart, searchEnd, alignment, &physAddr, &size);

    if (size == 0) {
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }

    *physAddrOut = static_cast<u64>(physAddr);
    *sizeOut = size;

    return result;
}

s32 PS4_SYSV_ABI sceKernelVirtualQuery(const void* addr, int flags, OrbisVirtualQueryInfo* info,
                                       size_t infoSize) {
    LOG_INFO(Kernel_Vmm, "called addr = {}, flags = {:#x}", fmt::ptr(addr), flags);
    if (!addr) {
        return ORBIS_KERNEL_ERROR_EACCES;
    }
    auto* memory = Core::Memory::Instance();
    return memory->VirtualQuery(std::bit_cast<VAddr>(addr), flags, info);
}

s32 PS4_SYSV_ABI sceKernelReserveVirtualRange(void** addr, u64 len, int flags, u64 alignment) {
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
    memory->Reserve(addr, in_addr, len, map_flags, alignment);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelMapNamedDirectMemory(void** addr, u64 len, int prot, int flags,
                                               s64 directMemoryStart, u64 alignment,
                                               const char* name) {
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

    const VAddr in_addr = reinterpret_cast<VAddr>(*addr);
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);
    SCOPE_EXIT {
        LOG_INFO(Kernel_Vmm,
                 "in_addr = {:#x}, out_addr = {}, len = {:#x}, prot = {:#x}, flags = {:#x}, "
                 "directMemoryStart = {:#x}, "
                 "alignment = {:#x}",
                 in_addr, fmt::ptr(*addr), len, prot, flags, directMemoryStart, alignment);
    };

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
    SCOPE_EXIT {
        LOG_INFO(Kernel_Vmm,
                 "in_addr = {:#x}, out_addr = {}, len = {:#x}, prot = {:#x}, flags = {:#x}",
                 in_addr, fmt::ptr(*addr_in_out), len, prot, flags);
    };
    auto* memory = Core::Memory::Instance();
    return memory->MapMemory(addr_in_out, in_addr, len, mem_prot, map_flags,
                             Core::VMAType::Flexible, name);
}

s32 PS4_SYSV_ABI sceKernelMapFlexibleMemory(void** addr_in_out, std::size_t len, int prot,
                                            int flags) {
    return sceKernelMapNamedFlexibleMemory(addr_in_out, len, prot, flags, "");
}

int PS4_SYSV_ABI sceKernelQueryMemoryProtection(void* addr, void** start, void** end, u32* prot) {
    auto* memory = Core::Memory::Instance();
    return memory->QueryProtection(std::bit_cast<VAddr>(addr), start, end, prot);
}

int PS4_SYSV_ABI sceKernelMProtect(const void* addr, size_t size, int prot) {
    Core::MemoryManager* memory_manager = Core::Memory::Instance();
    Core::MemoryProt protection_flags = static_cast<Core::MemoryProt>(prot);
    return memory_manager->Protect(std::bit_cast<VAddr>(addr), size, protection_flags);
}

int PS4_SYSV_ABI sceKernelMTypeProtect(const void* addr, size_t size, int mtype, int prot) {
    Core::MemoryManager* memory_manager = Core::Memory::Instance();
    Core::MemoryProt protection_flags = static_cast<Core::MemoryProt>(prot);
    return memory_manager->Protect(std::bit_cast<VAddr>(addr), size, protection_flags);
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

void PS4_SYSV_ABI _sceKernelRtldSetApplicationHeapAPI(void* func[]) {
    auto* linker = Common::Singleton<Core::Linker>::Instance();
    linker->SetHeapAPI(func);
}

int PS4_SYSV_ABI sceKernelGetDirectMemoryType(u64 addr, int* directMemoryTypeOut,
                                              void** directMemoryStartOut,
                                              void** directMemoryEndOut) {
    LOG_WARNING(Kernel_Vmm, "called, direct memory addr = {:#x}", addr);
    auto* memory = Core::Memory::Instance();
    return memory->GetDirectMemoryType(addr, directMemoryTypeOut, directMemoryStartOut,
                                       directMemoryEndOut);
}

s32 PS4_SYSV_ABI sceKernelBatchMap(OrbisKernelBatchMapEntry* entries, int numEntries,
                                   int* numEntriesOut) {
    return sceKernelBatchMap2(entries, numEntries, numEntriesOut,
                              MemoryFlags::SCE_KERNEL_MAP_FIXED); // 0x10, 0x410?
}

s32 PS4_SYSV_ABI sceKernelBatchMap2(OrbisKernelBatchMapEntry* entries, int numEntries,
                                    int* numEntriesOut, int flags) {
    int result = ORBIS_OK;
    int processed = 0;
    for (int i = 0; i < numEntries; i++, processed++) {
        if (entries == nullptr || entries[i].length == 0 || entries[i].operation > 4) {
            result = ORBIS_KERNEL_ERROR_EINVAL;
            break; // break and assign a value to numEntriesOut.
        }

        switch (entries[i].operation) {
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_MAP_DIRECT: {
            result = sceKernelMapNamedDirectMemory(&entries[i].start, entries[i].length,
                                                   entries[i].protection, flags,
                                                   static_cast<s64>(entries[i].offset), 0, "");
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
            result = sceKernelMProtect(entries[i].start, entries[i].length, entries[i].protection);
            LOG_INFO(Kernel_Vmm, "entry = {}, operation = {}, len = {:#x}, result = {}", i,
                     entries[i].operation, entries[i].length, result);
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_MAP_FLEXIBLE: {
            result = sceKernelMapNamedFlexibleMemory(&entries[i].start, entries[i].length,
                                                     entries[i].protection, flags, "");
            LOG_INFO(Kernel_Vmm,
                     "entry = {}, operation = {}, len = {:#x}, type = {}, "
                     "result = {}",
                     i, entries[i].operation, entries[i].length, (u8)entries[i].type, result);
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_TYPE_PROTECT: {
            result = sceKernelMTypeProtect(entries[i].start, entries[i].length, entries[i].type,
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

s32 PS4_SYSV_ABI sceKernelSetVirtualRangeName(const void* addr, size_t len, const char* name) {
    static constexpr size_t MaxNameSize = 32;
    if (std::strlen(name) > MaxNameSize) {
        LOG_ERROR(Kernel_Vmm, "name exceeds 32 bytes!");
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    if (name == nullptr) {
        LOG_ERROR(Kernel_Vmm, "name is invalid!");
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    auto* memory = Core::Memory::Instance();
    memory->NameVirtualRange(std::bit_cast<VAddr>(addr), len, name);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolExpand(u64 searchStart, u64 searchEnd, size_t len,
                                           size_t alignment, u64* physAddrOut) {
    if (searchStart < 0 || searchEnd <= searchStart) {
        LOG_ERROR(Kernel_Vmm, "Provided address range is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const bool is_in_range = searchEnd - searchStart >= len;
    if (len <= 0 || !Common::Is64KBAligned(len) || !is_in_range) {
        LOG_ERROR(Kernel_Vmm, "Provided address range is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (alignment != 0 && !Common::Is64KBAligned(alignment)) {
        LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (physAddrOut == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Result physical address pointer is null!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* memory = Core::Memory::Instance();
    PAddr phys_addr = memory->PoolExpand(searchStart, searchEnd, len, alignment);
    *physAddrOut = static_cast<s64>(phys_addr);

    LOG_INFO(Kernel_Vmm,
             "searchStart = {:#x}, searchEnd = {:#x}, len = {:#x}, alignment = {:#x}, physAddrOut "
             "= {:#x}",
             searchStart, searchEnd, len, alignment, phys_addr);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolReserve(void* addrIn, size_t len, size_t alignment, int flags,
                                            void** addrOut) {
    LOG_INFO(Kernel_Vmm, "addrIn = {}, len = {:#x}, alignment = {:#x}, flags = {:#x}",
             fmt::ptr(addrIn), len, alignment, flags);

    if (addrIn == nullptr) {
        LOG_ERROR(Kernel_Vmm, "Address is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
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
    const VAddr in_addr = reinterpret_cast<VAddr>(addrIn);
    const auto map_flags = static_cast<Core::MemoryMapFlags>(flags);
    memory->PoolReserve(addrOut, in_addr, len, map_flags, alignment);

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolCommit(void* addr, size_t len, int type, int prot, int flags) {
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

s32 PS4_SYSV_ABI sceKernelMemoryPoolDecommit(void* addr, size_t len, int flags) {
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
    memory->PoolDecommit(pool_addr, len);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelMmap(void* addr, u64 len, int prot, int flags, int fd, size_t offset,
                               void** res) {
    LOG_INFO(Kernel_Vmm, "called addr = {}, len = {}, prot = {}, flags = {}, fd = {}, offset = {}",
             fmt::ptr(addr), len, prot, flags, fd, offset);
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto* memory = Core::Memory::Instance();
    const auto mem_prot = static_cast<Core::MemoryProt>(prot);
    const auto mem_flags = static_cast<Core::MemoryMapFlags>(flags);
    if (fd == -1) {
        return memory->MapMemory(res, std::bit_cast<VAddr>(addr), len, mem_prot, mem_flags,
                                 Core::VMAType::Flexible);
    } else {
        const uintptr_t handle = h->GetFile(fd)->f.GetFileMapping();
        return memory->MapFile(res, std::bit_cast<VAddr>(addr), len, mem_prot, mem_flags, handle,
                               offset);
    }
}

void* PS4_SYSV_ABI posix_mmap(void* addr, u64 len, int prot, int flags, int fd, u64 offset) {
    void* ptr;
    LOG_INFO(Kernel_Vmm, "posix mmap redirect to sceKernelMmap");
    int result = sceKernelMmap(addr, len, prot, flags, fd, offset, &ptr);
    ASSERT(result == 0);
    return ptr;
}

s32 PS4_SYSV_ABI sceKernelConfiguredFlexibleMemorySize(u64* sizeOut) {
    if (sizeOut == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto* memory = Core::Memory::Instance();
    *sizeOut = memory->GetTotalFlexibleSize();
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceKernelMunmap(void* addr, size_t len) {
    LOG_INFO(Kernel_Vmm, "addr = {}, len = {:#x}", fmt::ptr(addr), len);
    if (len == 0) {
        return ORBIS_OK;
    }
    auto* memory = Core::Memory::Instance();
    return memory->UnmapMemory(std::bit_cast<VAddr>(addr), len);
}

int PS4_SYSV_ABI posix_munmap(void* addr, size_t len) {
    int result = sceKernelMunmap(addr, len);
    if (result < 0) {
        LOG_ERROR(Kernel_Pthread, "posix_munmap: error = {}", result);
        ErrSceToPosix(result);
        return -1;
    }
    return result;
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
    LIB_FUNCTION("NcaWUxfMNIQ", "libkernel", 1, "libkernel", 1, 1, sceKernelMapNamedDirectMemory);
    LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, sceKernelMapDirectMemory);
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

    LIB_FUNCTION("9bfdLIyuwCY", "libkernel", 1, "libkernel", 1, 1, sceKernelMTypeProtect);
    LIB_FUNCTION("vSMAm3cxYTY", "libkernel", 1, "libkernel", 1, 1, sceKernelMProtect);

    // Memory pool
    LIB_FUNCTION("qCSfqDILlns", "libkernel", 1, "libkernel", 1, 1, sceKernelMemoryPoolExpand);
    LIB_FUNCTION("pU-QydtGcGY", "libkernel", 1, "libkernel", 1, 1, sceKernelMemoryPoolReserve);
    LIB_FUNCTION("Vzl66WmfLvk", "libkernel", 1, "libkernel", 1, 1, sceKernelMemoryPoolCommit);
    LIB_FUNCTION("LXo1tpFqJGs", "libkernel", 1, "libkernel", 1, 1, sceKernelMemoryPoolDecommit);

    LIB_FUNCTION("BPE9s9vQQXo", "libkernel", 1, "libkernel", 1, 1, posix_mmap);
    LIB_FUNCTION("BPE9s9vQQXo", "libScePosix", 1, "libkernel", 1, 1, posix_mmap);
    LIB_FUNCTION("UqDGjXA5yUM", "libkernel", 1, "libkernel", 1, 1, posix_munmap);
    LIB_FUNCTION("UqDGjXA5yUM", "libScePosix", 1, "libkernel", 1, 1, posix_munmap);
}

} // namespace Libraries::Kernel
