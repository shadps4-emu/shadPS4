// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include <fmt/format.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/elf_info.h"
#include "common/error.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/devices/blockpool_device.h"
#include "core/file_sys/devices/dmem_device.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "core/memory/kernel.h"

namespace Libraries::Kernel {

static s32 g_sdk_version = -1;
static s32 g_dmem_fd = -1;
static u32 g_blockpool_fd = -1;
static std::shared_ptr<Core::Devices::BaseDevice> g_dmem_device;
static std::shared_ptr<Core::Devices::BaseDevice> g_blockpool_device;
static Core::MemoryManager* g_memory;

using Core::MemoryMapFlags;
using Core::MemoryProt;

constexpr auto VIRTUAL_MAP_FLAGS =
    MemoryMapFlags::Fixed | MemoryMapFlags::NoOverwrite | MemoryMapFlags::NoCoalesce;
constexpr auto DIRECT_MAP_FLAGS =
    VIRTUAL_MAP_FLAGS | MemoryMapFlags::Stack | MemoryMapFlags::Sanitizer;
constexpr auto DIRECT_MAP2_FLAGS =
    VIRTUAL_MAP_FLAGS | MemoryMapFlags::Sanitizer | MemoryMapFlags::AlignmentMask;
constexpr auto POOL_RESERVE_FLAGS = MemoryMapFlags::Fixed | MemoryMapFlags::NoOverwrite;
constexpr auto DIRECT_MAP2_PROT = MemoryProt::CpuReadWrite | MemoryProt::GpuReadWrite;

constexpr s32 GAME_DMEM_POOL = 1;
constexpr s32 DMEM_POOL_COUNT = 3;
constexpr PAddr TRACE_DMEM_START = 0x3000000000;
constexpr PAddr TRACE_DMEM_END = 0x3020000000;

static bool HasOnlyFlags(s32 flags, MemoryMapFlags allowed) {
    return (static_cast<u32>(flags) & ~static_cast<u32>(allowed)) == 0;
}

static bool HasOnlyProt(s32 prot, MemoryProt allowed) {
    return (static_cast<u32>(prot) & ~static_cast<u32>(allowed)) == 0;
}

u64 PS4_SYSV_ABI sceKernelGetDirectMemorySize() {
    u64 memory_size;
    const s32 ret = g_dmem_device->ioctl(Core::Devices::DmemDevice::GetMemorySize, &memory_size);
    return ret == ORBIS_OK ? memory_size : 0;
}

s32 PS4_SYSV_ABI sceKernelEnableDmemAliasing() {
    LOG_DEBUG(Kernel_Vmm, "called");
    auto* memory = Core::Memory::Instance();
    memory->GetDmemManager().SetAliasing(Core::DmemAliasingMode::AllowGpuReadAliases);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAllocateDirectMemory(PAddr search_start, PAddr search_end, u64 size,
                                               u64 alignment, s32 memory_type, PAddr* out_addr) {
    auto allocate_args = Core::Devices::DmemDevice::AllocateMemoryArgs{
        .search_start = search_start,
        .search_end = search_end,
        .size = size,
        .alignment = alignment,
        .memory_type = memory_type,
    };
    const s32 ret = g_dmem_device->ioctl(Core::Devices::DmemDevice::AllocateMemory, &allocate_args);
    if (ret == ORBIS_OK) {
        *out_addr = allocate_args.search_start;
        LOG_INFO(Kernel_Vmm,
                 "search_start = {:#x}, search_end = {:#x}, size = {:#x}, "
                 "alignment = {:#x}, memory_type = {:#x}, out_addr = {:#x}",
                 search_start, search_end, size, alignment, memory_type,
                 allocate_args.search_start);
    }
    return ret;
}

s32 PS4_SYSV_ABI sceKernelAllocateMainDirectMemory(u64 size, u64 alignment, s32 memory_type,
                                                   PAddr* out_addr) {
    return sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), size, alignment,
                                         memory_type, out_addr);
}

s32 PS4_SYSV_ABI sceKernelCheckedReleaseDirectMemory(u64 start, u64 len) {
    LOG_INFO(Kernel_Vmm, "called start = {:#x}, len = {:#x}", start, len);
    auto release_args = Core::Devices::DmemDevice::ReleaseMemoryArgs{
        .addr = start,
        .size = len,
    };
    return g_dmem_device->ioctl(Core::Devices::DmemDevice::CheckedReleaseMemory, &release_args);
}

s32 PS4_SYSV_ABI sceKernelReleaseDirectMemory(u64 start, u64 len) {
    LOG_INFO(Kernel_Vmm, "called start = {:#x}, len = {:#x}", start, len);
    auto release_args = Core::Devices::DmemDevice::ReleaseMemoryArgs{
        .addr = start,
        .size = len,
    };
    return g_dmem_device->ioctl(Core::Devices::DmemDevice::ReleaseMemory, &release_args);
}

s32 PS4_SYSV_ABI sceKernelAvailableDirectMemorySize(u64 search_start, u64 search_end, u64 alignment,
                                                    PAddr* out_addr, u64* out_size) {
    LOG_INFO(Kernel_Vmm, "called search_start = {:#x}, search_end = {:#x}, alignment = {:#x}",
             search_start, search_end, alignment);

    auto available_args = Core::Devices::DmemDevice::AvailableMemoryArgs{
        .search_start = search_start,
        .search_end = search_end,
        .alignment = alignment,
    };
    const s32 ret =
        g_dmem_device->ioctl(Core::Devices::DmemDevice::AvailableMemorySize, &available_args);
    if (ret == ORBIS_OK) {
        if (out_addr) {
            *out_addr = available_args.search_start;
        }
        if (out_size) {
            *out_size = available_args.out_size;
        }
    }
    return ret;
}

s32 PS4_SYSV_ABI sceKernelVirtualQuery(const void* addr, s32 flags, OrbisVirtualQueryInfo* info,
                                       u64 infoSize) {
    auto* memory = Core::Memory::Instance();
    const s32 ret = memory->VirtualQuery(std::bit_cast<VAddr>(addr), flags, info);
    LOG_INFO(Kernel_Vmm, "called addr = {}, flags = {:#x}, ret = {:#x}", fmt::ptr(addr), flags,
             ret);
    return ret;
}

s32 PS4_SYSV_ABI sceKernelReserveVirtualRange(VAddr* addr, u64 len, s32 flags, u64 alignment) {
    LOG_INFO(Kernel_Vmm, "addr = {:#x}, len = {:#x}, flags = {:#x}, alignment = {:#x}", *addr, len,
             flags, alignment);
    if (!Common::Is16KBAligned(*addr)) {
        LOG_ERROR(Kernel_Vmm, "Address is invalid!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (len == 0 || !Common::Is16KBAligned(len) || !HasOnlyFlags(flags, VIRTUAL_MAP_FLAGS)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 16KB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (alignment != 0) {
        if (!std::has_single_bit(alignment) || !Common::Is16KBAligned(alignment) ||
            std::countr_zero(alignment) >= 31) {
            LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
    } else {
        alignment = 16_KB;
    }

    const auto map_flags = static_cast<MemoryMapFlags>(flags) |
                           Core::MapAlignment(std::countr_zero(alignment)) | MemoryMapFlags::Void |
                           MemoryMapFlags::Shared;
    const s32 result =
        g_memory->MapMemory(addr, len, MemoryProt::NoAccess, map_flags, -1, 0, "anon");
    LOG_INFO(Kernel_Vmm, "out_addr = {:#x}, ret = {:#x}", *addr, result);
    return result;
}

s32 PS4_SYSV_ABI sceKernelMapDirectMemory2(VAddr* addr, u64 len, s32 type, s32 prot, s32 flags,
                                           s64 phys_addr, u64 alignment);

static s32 MapDirectMemoryImpl(s32 pool_id, VAddr* addr, u64 len, s32 prot, s32 flags,
                               s64 phys_addr, u64 alignment) {
    LOG_INFO(Kernel_Vmm,
             "pool_id = {}, in_addr = {:#x}, len = {:#x}, prot = {:#x}, flags = {:#x}, "
             "phys_addr = {:#x}, alignment = {:#x}",
             pool_id, *addr, len, prot, flags, phys_addr, alignment);

    if (static_cast<u32>(pool_id) >= DMEM_POOL_COUNT) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto map_flags = static_cast<MemoryMapFlags>(flags);
    const bool is_trace_memory = phys_addr >= TRACE_DMEM_START && phys_addr < TRACE_DMEM_END;
    if (pool_id == GAME_DMEM_POOL && !is_trace_memory && False(map_flags & MemoryMapFlags::Stack) &&
        Common::ElfInfo::FW_250 <= g_sdk_version) {
        return sceKernelMapDirectMemory2(addr, len, -1, prot, flags, phys_addr, alignment);
    }

    if (!HasOnlyFlags(flags, DIRECT_MAP_FLAGS) || (alignment & (alignment - 1)) != 0 ||
        !HasOnlyProt(prot, MemoryProt::All) ||
        !Common::Is16KBAligned(*addr | alignment | phys_addr | len)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    map_flags &= ~MemoryMapFlags::Stack;
    if (True(map_flags & MemoryMapFlags::Fixed) && *addr == 0) {
        if (Common::ElfInfo::FW_170 <= g_sdk_version) {
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
        map_flags &= ~MemoryMapFlags::Fixed;
        LOG_WARNING(Kernel_Vmm, "Map fixed with addr = 0");
    }

    const s32 alignment_bits = std::bit_width(std::max<u64>(alignment, 16_KB));
    if (alignment_bits >= 32) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    map_flags |= Core::MapAlignment(alignment_bits - 1) | MemoryMapFlags::Shared;

    const auto mem_prot = static_cast<MemoryProt>(prot);
    const s32 ret =
        g_memory->MapMemory(addr, len, mem_prot, map_flags, g_dmem_fd, phys_addr, "anon");
    LOG_INFO(Kernel_Vmm, "out_addr = {:#x}, ret = {:#x}", *addr, ret);
    return ret;
}

static s32 NameDirectMapping(s32 result, VAddr addr, u64 len, const char* name) {
    if (result != ORBIS_OK) {
        return result;
    }
    return sceKernelSetVirtualRangeName(reinterpret_cast<void*>(addr), len, name);
}

s32 PS4_SYSV_ABI sceKernelMapNamedDirectMemory(VAddr* addr, u64 len, s32 prot, s32 flags,
                                               s64 phys_addr, u64 alignment, const char* name) {
    const s32 ret =
        MapDirectMemoryImpl(GAME_DMEM_POOL, addr, len, prot, flags, phys_addr, alignment);
    return NameDirectMapping(ret, *addr, len, name);
}

s32 PS4_SYSV_ABI sceKernelMapDirectMemory(VAddr* addr, u64 len, s32 prot, s32 flags, s64 phys_addr,
                                          u64 alignment) {
    return MapDirectMemoryImpl(GAME_DMEM_POOL, addr, len, prot, flags, phys_addr, alignment);
}

s32 PS4_SYSV_ABI sceKernelInternalMapDirectMemory(s32 pool_id, VAddr* addr, u64 len, s32 prot,
                                                  s32 flags, s64 phys_addr, u64 alignment) {
    return MapDirectMemoryImpl(pool_id, addr, len, prot, flags, phys_addr, alignment);
}

s32 PS4_SYSV_ABI sceKernelInternalMapNamedDirectMemory(s32 pool_id, VAddr* addr, u64 len, s32 prot,
                                                       s32 flags, s64 phys_addr, u64 alignment,
                                                       const char* name) {
    const s32 ret = MapDirectMemoryImpl(pool_id, addr, len, prot, flags, phys_addr, alignment);
    return NameDirectMapping(ret, *addr, len, name);
}

s32 PS4_SYSV_ABI sceKernelMapDirectMemory2(VAddr* addr, u64 len, s32 type, s32 prot, s32 flags,
                                           s64 phys_addr, u64 alignment) {
    LOG_INFO(Kernel_Vmm,
             "in_addr = {:#x}, len = {:#x}, prot = {:#x}, flags = {:#x}, "
             "phys_addr = {:#x}, alignment = {:#x}",
             *addr, len, prot, flags, phys_addr, alignment);

    if (len == 0 || !Common::Is16KBAligned(len)) {
        LOG_ERROR(Kernel_Vmm, "Map size is either zero or not 16KB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (!Common::Is16KBAligned(phys_addr)) {
        LOG_ERROR(Kernel_Vmm, "Start address is not 16KB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (!Common::Is16KBAligned(*addr)) {
        LOG_ERROR(Kernel_Vmm, "Virtual address is not 16KB aligned!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (alignment != 0) {
        if (!std::has_single_bit(alignment) || !Common::Is16KBAligned(alignment) ||
            std::countr_zero(alignment) >= 31) {
            LOG_ERROR(Kernel_Vmm, "Alignment value is invalid!");
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
    }

    if (!HasOnlyFlags(flags, DIRECT_MAP2_FLAGS)) {
        LOG_ERROR(Kernel_Vmm, "Invalid flags {:#x}", flags);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    const s32 checked_prot =
        Common::ElfInfo::FW_300 <= g_sdk_version ? prot : static_cast<u8>(prot);
    if (!HasOnlyProt(checked_prot, DIRECT_MAP2_PROT)) {
        LOG_ERROR(Kernel_Vmm, "Invalid protection {:#x}", prot);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (type < -1 || type > 10) {
        LOG_ERROR(Kernel_Vmm, "Invalid memory type {}", type);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    const auto mem_prot = static_cast<MemoryProt>(checked_prot);
    const auto map_flags = static_cast<MemoryMapFlags>(flags);
    if (True(map_flags & MemoryMapFlags::Sanitizer)) {
        LOG_ERROR(Kernel_Vmm, "Sanitizer mappings require a sanitizer-enabled process");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (True(map_flags & MemoryMapFlags::Fixed) && *addr == 0) {
        LOG_ERROR(Kernel_Vmm, "Fixed mapping requires a non-null address");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    const auto mem_type = static_cast<Core::DmemMemoryType>(type);
    const u32 alignment_shift = alignment != 0 ? std::countr_zero(alignment) : 0;
    const s32 ret = g_memory->MapDirectMemory(addr, len, mem_type, mem_prot, map_flags, phys_addr,
                                              alignment_shift);
    if (ret == ORBIS_OK) {
        LOG_INFO(Kernel_Vmm, "out_addr = {:#x}", *addr);
    }
    return ret;
}

s32 PS4_SYSV_ABI sceKernelMapNamedSystemFlexibleMemory(VAddr* addr, u64 len, s32 prot, s32 flags,
                                                       const char* name) {
    LOG_INFO(Kernel_Vmm, "in_addr = {:#x}, len = {:#x}, prot = {:#x}, flags = {:#x}, name ='{}'",
             *addr, len, prot, flags, name);

    if (len == 0 || !Common::Is16KBAligned(len) || !HasOnlyFlags(flags, VIRTUAL_MAP_FLAGS) ||
        !HasOnlyProt(prot, MemoryProt::All)) {
        LOG_ERROR(Kernel_Vmm, "len is 0 or not 16kb multiple");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto map_flags = static_cast<MemoryMapFlags>(flags);
    if (True(map_flags & MemoryMapFlags::Fixed) && *addr == 0) {
        if (Common::ElfInfo::FW_170 <= g_sdk_version) {
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
        map_flags &= ~MemoryMapFlags::Fixed;
        LOG_WARNING(Kernel_Vmm, "Map fixed with addr = 0");
    }

    if (name == nullptr) {
        LOG_ERROR(Kernel_Vmm, "name is invalid!");
        return ORBIS_KERNEL_ERROR_EFAULT;
    }

    if (std::strlen(name) >= ORBIS_KERNEL_MAXIMUM_NAME_LENGTH) {
        LOG_ERROR(Kernel_Vmm, "name exceeds 32 bytes!");
        return ORBIS_KERNEL_ERROR_ENAMETOOLONG;
    }

    if (*addr == 0) {
        *addr = 0x880000000ULL;
    }

    const auto mem_prot = static_cast<MemoryProt>(prot);
    map_flags |= MemoryMapFlags::Anon | MemoryMapFlags::System;
    const u32 ret = g_memory->MapMemory(addr, len, mem_prot, map_flags, -1, 0, name);
    LOG_INFO(Kernel_Vmm, "out_addr = {:#x}, ret = {:#x}", *addr, ret);
    return ret;
}

s32 PS4_SYSV_ABI sceKernelMapNamedFlexibleMemory(VAddr* addr, u64 len, s32 prot, s32 flags,
                                                 const char* name) {
    LOG_INFO(Kernel_Vmm, "in_addr = {:#x}, len = {:#x}, prot = {:#x}, flags = {:#x}, name ='{}'",
             *addr, len, prot, flags, name);

    if (len == 0 || !Common::Is16KBAligned(len) || !HasOnlyFlags(flags, VIRTUAL_MAP_FLAGS) ||
        !HasOnlyProt(prot, MemoryProt::All)) {
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

    const auto mem_prot = static_cast<MemoryProt>(prot);
    const auto map_flags = static_cast<MemoryMapFlags>(flags) | MemoryMapFlags::Anon;
    const u32 ret = g_memory->MapMemory(addr, len, mem_prot, map_flags, -1, 0, name);
    LOG_INFO(Kernel_Vmm, "out_addr = {:#x}, ret = {:#x}", *addr, ret);
    return ret;
}

s32 PS4_SYSV_ABI sceKernelMapFlexibleMemory(VAddr* addr, u64 len, s32 prot, s32 flags) {
    return sceKernelMapNamedFlexibleMemory(addr, len, prot, flags, "anon");
}

s32 PS4_SYSV_ABI sceKernelQueryMemoryProtection(void* addr, VAddr* start, VAddr* end, u32* prot) {
    LOG_INFO(Kernel_Vmm, "called, addr = {:#x}", VAddr(addr));
    auto* memory = Core::Memory::Instance();
    return memory->QueryProtection(std::bit_cast<VAddr>(addr), start, end, prot);
}

s32 PS4_SYSV_ABI sceKernelMprotect(VAddr addr, u64 size, s32 prot) {
    LOG_INFO(Kernel_Vmm, "called addr = {:#x}, size = {:#x}, prot = {:#x}", addr, size, prot);
    const auto mem_prot = static_cast<MemoryProt>(prot);
    return g_memory->Protect(addr, size, mem_prot);
}

s32 PS4_SYSV_ABI posix_mprotect(VAddr addr, u64 size, s32 prot) {
    s32 result = sceKernelMprotect(addr, size, prot);
    if (result < 0) {
        ErrSceToPosix(result);
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI sceKernelMtypeprotect(VAddr addr, u64 size, s32 mtype, s32 prot) {
    LOG_INFO(Kernel_Vmm, "called addr = {:#x}, size = {:#x}, prot = {:#x}", addr, size, prot);
    const auto mem_prot = static_cast<MemoryProt>(prot);
    const auto mem_type = static_cast<Core::DmemMemoryType>(mtype);
    return g_memory->ProtectType(addr, size, mem_type, mem_prot);
}

s32 PS4_SYSV_ABI sceKernelDirectMemoryQuery(PAddr addr, s32 flags, OrbisQueryInfo* query_info,
                                            u64 info_size) {
    LOG_INFO(Kernel_Vmm, "called addr = {:#x}, flags = {:#x}", addr, flags);
    auto query_args = Core::Devices::DmemDevice::MemoryQueryArgs{
        .pool_id = 0,
        .flags = flags,
        .addr = addr,
        .query_info = query_info,
        .info_size = info_size,
    };
    return g_dmem_device->ioctl(Core::Devices::DmemDevice::MemoryQuery, &query_args);
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

s32 PS4_SYSV_ABI sceKernelGetDirectMemoryType(u64 addr, s32* out_memory_type, VAddr* out_start,
                                              VAddr* out_end) {
    LOG_WARNING(Kernel_Vmm, "called, direct memory addr = {:#x}", addr);
    auto type_args = Core::Devices::DmemDevice::GetMemoryTypeArgs{
        .addr = addr,
    };
    const s32 ret = g_dmem_device->ioctl(Core::Devices::DmemDevice::GetMemoryType, &type_args);
    if (ret == ORBIS_OK) {
        *out_memory_type = type_args.mtype;
        *out_start = type_args.out_start;
        *out_end = type_args.out_end;
    }
    return ret;
}

s32 PS4_SYSV_ABI sceKernelIsStack(VAddr addr, VAddr* out_start, VAddr* out_end) {
    LOG_DEBUG(Kernel_Vmm, "called, addr = {:#x}", addr);

    VAddr start, end;
    u32 prot_raw;
    if (s32 ret = g_memory->QueryProtection(addr, &start, &end, &prot_raw); ret != ORBIS_OK) {
        return ret;
    }

    auto prot = static_cast<MemoryProt>(prot_raw);
    if (False(prot & MemoryProt::GpuReadWrite)) {
        if (out_start) {
            *out_start = 0;
        }
        if (out_end) {
            *out_end = 0;
        }
    } else {
        if (out_start) {
            *out_start = start;
        }
        if (out_end) {
            *out_end = end;
        }
    }

    return ORBIS_OK;
}

u32 PS4_SYSV_ABI sceKernelIsAddressSanitizerEnabled() {
    LOG_DEBUG(Kernel, "called");
    return false;
}

s32 PS4_SYSV_ABI sceKernelBatchMap(OrbisKernelBatchMapEntry* entries, s32 numEntries,
                                   s32* numEntriesOut) {
    return sceKernelBatchMap2(entries, numEntries, numEntriesOut,
                              static_cast<s32>(MemoryMapFlags::Fixed));
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
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_UNMAP: {
            result = sceKernelMunmap(entries[i].start, entries[i].length);
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_PROTECT: {
            result = sceKernelMprotect(entries[i].start, entries[i].length, entries[i].protection);
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_MAP_FLEXIBLE: {
            result = sceKernelMapNamedFlexibleMemory(&entries[i].start, entries[i].length,
                                                     entries[i].protection, flags, "anon");
            break;
        }
        case MemoryOpTypes::ORBIS_KERNEL_MAP_OP_TYPE_PROTECT: {
            result = sceKernelMtypeprotect(entries[i].start, entries[i].length, entries[i].type,
                                           entries[i].protection);
            break;
        }
        default: {
            UNREACHABLE();
        }
        }

        if (result != ORBIS_OK) {
            LOG_ERROR(Kernel_Vmm, "failed with error code {:#x}", result);
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

    g_memory->NameVirtualRange(std::bit_cast<VAddr>(addr), len, name);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelClearVirtualRangeName(const void* addr, u64 len) {
    g_memory->NameVirtualRange(std::bit_cast<VAddr>(addr), len, "");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolExpand(u64 search_start, u64 search_end, u64 size,
                                           u64 alignment, u64* phys_addr_out) {
    if (alignment - 1 & alignment) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto expand_args = Core::Devices::BlockpoolDevice::PoolExpandArgs{
        .size = size,
        .start = search_start,
        .end = search_end,
        .flags =
            alignment ? static_cast<u32>(Core::MapAlignment(std::bit_width(alignment) - 1)) : 0,
    };
    const s32 ret =
        g_blockpool_device->ioctl(Core::Devices::BlockpoolDevice::PoolExpand, &expand_args);
    if (ret == ORBIS_OK) {
        *phys_addr_out = expand_args.start;
        LOG_INFO(Kernel_Vmm,
                 "search_start = {:#x}, search_end = {:#x}, len = {:#x}, alignment = {:#x}, "
                 "out_addr = {:#x}",
                 search_start, search_end, size, alignment, expand_args.start);
    }
    return ret;
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolReserve(VAddr addr_in, u64 size, u64 alignment, s32 flags,
                                            VAddr* addr_out) {
    LOG_INFO(Kernel_Vmm, "addr_in = {:#x}, size = {:#x}, alignment = {:#x}, flags = {:#x}", addr_in,
             size, alignment, flags);

    if (!HasOnlyFlags(flags, POOL_RESERVE_FLAGS)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    auto map_flags = static_cast<MemoryMapFlags>(flags) | MemoryMapFlags::Shared;
    if (alignment != 0) {
        if (!std::has_single_bit(alignment)) {
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
        map_flags |= Core::MapAlignment(std::bit_width(alignment) - 1);
    }

    auto* memory = Core::Memory::Instance();
    const auto ret =
        memory->MapMemory(&addr_in, size, MemoryProt::CpuReadWrite | MemoryProt::GpuReadWrite,
                          map_flags, g_blockpool_fd, 0, "anon");
    if (ret == ORBIS_OK) {
        *addr_out = addr_in;
    }
    LOG_INFO(Kernel_Vmm, "out_addr = {:#x}, ret = {:#x}", *addr_out, ret);
    return ret;
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolCommit(VAddr addr, u64 len, s32 type, s32 prot, s32 flags) {
    LOG_INFO(Kernel_Vmm, "addr = {:#x}, len = {:#x}, type = {:#x}, prot = {:#x}, flags = {:#x}",
             addr, len, type, prot, flags);
    const auto mem_prot = static_cast<MemoryProt>(prot);
    return g_memory->PoolMap(addr, len, mem_prot, type);
}

s32 PS4_SYSV_ABI sceKernelMemoryPoolDecommit(VAddr addr, u64 len, s32 flags) {
    LOG_INFO(Kernel_Vmm, "addr = {:#x}, len = {:#x}, flags = {:#x}", addr, len, flags);
    return g_memory->PoolUnmap(addr, len);
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

s32 PS4_SYSV_ABI sceKernelMemoryPoolGetBlockStats(OrbisKernelMemoryPoolBlockStats* out_stats,
                                                  u64 size) {
    OrbisKernelMemoryPoolBlockStats stats;
    const s32 ret =
        g_blockpool_device->ioctl(Core::Devices::BlockpoolDevice::GetBlockStats, &stats);
    if (ret == ORBIS_OK) {
        std::memcpy(out_stats, &stats,
                    std::min<u64>(size, sizeof(OrbisKernelMemoryPoolBlockStats)));
    }
    return ret;
}

VAddr PS4_SYSV_ABI posix_mmap(VAddr addr, u64 len, s32 prot, s32 flags, s32 fd, s64 phys_addr) {
    LOG_INFO(
        Kernel_Vmm,
        "called addr = {:#x}, len = {:#x}, prot = {:#x}, flags = {:#x}, fd = {}, phys_addr = {:#x}",
        addr, len, prot, flags, fd, phys_addr);

    if (len == 0) {
        // If length is 0, mmap returns EINVAL.
        ErrSceToPosix(ORBIS_KERNEL_ERROR_EINVAL);
        return -1;
    }

    const auto mem_prot = static_cast<MemoryProt>(prot);
    const auto mem_flags = static_cast<MemoryMapFlags>(flags);
    const s32 result = g_memory->MapMemory(&addr, len, mem_prot, mem_flags, fd, phys_addr, "anon");
    LOG_INFO(Kernel_Vmm, "out_addr = {:#x}, ret = {:#x}", addr, result);

    if (result != ORBIS_OK) {
        // If the memory mappings fail, mmap sets errno to the appropriate error code,
        // then returns (void*)-1;
        ErrSceToPosix(result);
        return -1;
    }

    return addr;
}

s32 PS4_SYSV_ABI sceKernelMmap(VAddr addr, u64 len, s32 prot, s32 flags, s32 fd, s64 phys_addr,
                               VAddr* res) {
    VAddr addr_out = posix_mmap(addr, len, prot, flags, fd, phys_addr);

    if (addr_out == -1) {
        // posix_mmap failed, calculate and return the appropriate kernel error code using errno.
        LOG_ERROR(Kernel_Fs, "error = {}", *__Error());
        return ErrnoToSceKernelError(*__Error());
    }

    // Set the outputted address
    *res = addr_out;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelConfiguredFlexibleMemorySize(u64* out_size) {
    if (out_size == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    *out_size = g_memory->GetTotalFlexibleSize();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelMunmap(VAddr addr, u64 len) {
    LOG_INFO(Kernel_Vmm, "addr = {:#x}, len = {:#x}", addr, len);
    if (len == 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    return g_memory->UnmapMemory(addr, len);
}

s32 PS4_SYSV_ABI posix_munmap(VAddr addr, u64 len) {
    s32 result = sceKernelMunmap(addr, len);
    if (result < 0) {
        LOG_ERROR(Kernel_Pthread, "error = {}", result);
        ErrSceToPosix(result);
        return -1;
    }
    return result;
}

s32 PS4_SYSV_ABI posix_mlock(const void* addr, u64 len) {
    if (!addr || len == 0) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    const VAddr raw_addr = reinterpret_cast<VAddr>(addr);
    if (raw_addr + len < raw_addr) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    LOG_DEBUG(Kernel_Vmm, "(STUBBED) addr = {}, len = {:#x}", fmt::ptr(addr), len);
    return 0;
}

s32 PS4_SYSV_ABI sceKernelMlock(void* addr, u64 len) {
    s32 result = posix_mlock(addr, len);
    if (result < 0) {
        LOG_ERROR(Kernel_Vmm, "addr = {}, len = {:#x}, error = {:#x}", fmt::ptr(addr), len,
                  *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_munlock(const void* addr, u64 len) {
    if (!addr || len == 0) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    const VAddr raw_addr = reinterpret_cast<VAddr>(addr);
    if (raw_addr + len < raw_addr) {
        *__Error() = POSIX_EINVAL;
        return -1;
    }

    LOG_DEBUG(Kernel_Vmm, "(STUBBED) addr = {}, len = {:#x}", fmt::ptr(addr), len);
    return 0;
}

s32 PS4_SYSV_ABI sceKernelMunlock(void* addr, u64 len) {
    s32 result = posix_munlock(addr, len);
    if (result < 0) {
        LOG_ERROR(Kernel_Vmm, "addr = {}, len = {:#x}, error = {:#x}", fmt::ptr(addr), len,
                  *__Error());
        return ErrnoToSceKernelError(*__Error());
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelReleaseFlexibleMemory(void* addr, u64 len) {
    const VAddr raw_addr = reinterpret_cast<VAddr>(addr);

    if (!addr || len == 0 || !Common::Is16KBAligned(raw_addr | len)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    LOG_DEBUG(Kernel_Vmm, "addr = {}, len = {:#x}", fmt::ptr(addr), len);

#if defined(_WIN32)
    if (!VirtualAlloc(addr, len, MEM_RESET, PAGE_READWRITE)) {
        LOG_ERROR(Kernel_Vmm, "VirtualAlloc failed: {}", Common::GetLastErrorMsg());
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
#elif defined(__APPLE__)
    if (madvise(addr, len, MADV_FREE) != 0) {
        LOG_ERROR(Kernel_Vmm, "madvise failed: {}", Common::GetLastErrorMsg());
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
#else
    if (madvise(addr, len, MADV_DONTNEED) != 0) {
        LOG_ERROR(Kernel_Vmm, "madvise failed: {}", Common::GetLastErrorMsg());
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
#endif

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_msync(void* addr, u64 len, s32 flags) {
    LOG_ERROR(Kernel_Vmm, "(STUBBED) called, addr = {}, len = {:#x}, flags = {}", fmt::ptr(addr),
              len, flags);
    return ORBIS_OK;
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
    ASSERT_MSG(sceKernelGetCompiledSdkVersion(&g_sdk_version) == ORBIS_OK,
               "Failed to get compiled SDK verision.");

    g_memory = Core::Memory::Instance();

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();

    g_dmem_fd = h->CreateHandle();
    g_dmem_device = Core::Devices::DmemDevice::Create(g_dmem_fd, nullptr, 0, 0);
    auto* file = h->GetFile(g_dmem_fd);
    file->is_opened = true;
    file->type = Core::FileSys::FileType::Device;
    file->device = g_dmem_device;

    g_blockpool_fd = h->CreateHandle();
    g_blockpool_device = Core::Devices::BlockpoolDevice::Create(g_blockpool_fd, nullptr, 0, 0);
    file = h->GetFile(g_blockpool_fd);
    file->is_opened = true;
    file->type = Core::FileSys::FileType::Blockpool;
    file->device = g_blockpool_device;

    LIB_FUNCTION("usHTMoFoBTM", "libkernel_dmem_aliasing2", 1, "libkernel",
                 sceKernelEnableDmemAliasing);
    LIB_FUNCTION("usHTMoFoBTM", "libkernel", 1, "libkernel", sceKernelEnableDmemAliasing);
    LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", sceKernelAllocateDirectMemory);
    LIB_FUNCTION("B+vc2AO2Zrc", "libkernel", 1, "libkernel", sceKernelAllocateMainDirectMemory);
    LIB_FUNCTION("C0f7TJcbfac", "libkernel", 1, "libkernel", sceKernelAvailableDirectMemorySize);
    LIB_FUNCTION("hwVSPCmp5tM", "libkernel", 1, "libkernel", sceKernelCheckedReleaseDirectMemory);
    LIB_FUNCTION("rVjRvHJ0X6c", "libkernel", 1, "libkernel", sceKernelVirtualQuery);
    LIB_FUNCTION("7oxv3PPCumo", "libkernel", 1, "libkernel", sceKernelReserveVirtualRange);
    LIB_FUNCTION("BC+OG5m9+bw", "libkernel", 1, "libkernel", sceKernelGetDirectMemoryType);
    LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", sceKernelGetDirectMemorySize);
    LIB_FUNCTION("yDBwVAolDgg", "libkernel", 1, "libkernel", sceKernelIsStack);
    LIB_FUNCTION("jh+8XiK4LeE", "libkernel", 1, "libkernel", sceKernelIsAddressSanitizerEnabled);
    LIB_FUNCTION("NcaWUxfMNIQ", "libkernel", 1, "libkernel", sceKernelMapNamedDirectMemory);
    LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", sceKernelMapDirectMemory);
    LIB_FUNCTION("BQQniolj9tQ", "libkernel", 1, "libkernel", sceKernelMapDirectMemory2);
    LIB_FUNCTION("xAKAe6kFk90", "libkernel", 1, "libkernel", sceKernelInternalMapDirectMemory);
    LIB_FUNCTION("0ttVHjPWLsE", "libkernel", 1, "libkernel", sceKernelInternalMapNamedDirectMemory);
    LIB_FUNCTION("WFcfL2lzido", "libkernel", 1, "libkernel", sceKernelQueryMemoryProtection);
    LIB_FUNCTION("BHouLQzh0X0", "libkernel", 1, "libkernel", sceKernelDirectMemoryQuery);
    LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", sceKernelReleaseDirectMemory);
    LIB_FUNCTION("PGhQHd-dzv8", "libkernel", 1, "libkernel", sceKernelMmap);
    LIB_FUNCTION("cQke9UuBQOk", "libkernel", 1, "libkernel", sceKernelMunmap);
    LIB_FUNCTION("mL8NDH86iQI", "libkernel", 1, "libkernel", sceKernelMapNamedFlexibleMemory);
    LIB_FUNCTION("kc+LEEIYakc", "libkernel", 1, "libkernel", sceKernelMapNamedSystemFlexibleMemory);
    LIB_FUNCTION("aNz11fnnzi4", "libkernel", 1, "libkernel", sceKernelAvailableFlexibleMemorySize);
    LIB_FUNCTION("aNz11fnnzi4", "libkernel_avlfmem", 1, "libkernel",
                 sceKernelAvailableFlexibleMemorySize);
    LIB_FUNCTION("IWIBBdTHit4", "libkernel", 1, "libkernel", sceKernelMapFlexibleMemory);
    LIB_FUNCTION("p5EcQeEeJAE", "libkernel", 1, "libkernel", _sceKernelRtldSetApplicationHeapAPI);
    LIB_FUNCTION("2SKEx6bSq-4", "libkernel", 1, "libkernel", sceKernelBatchMap);
    LIB_FUNCTION("kBJzF8x4SyE", "libkernel", 1, "libkernel", sceKernelBatchMap2);
    LIB_FUNCTION("DGMG3JshrZU", "libkernel", 1, "libkernel", sceKernelSetVirtualRangeName);
    LIB_FUNCTION("mkgXxsoxWHg", "libkernel", 1, "libkernel", sceKernelClearVirtualRangeName);
    LIB_FUNCTION("n1-v6FgU7MQ", "libkernel", 1, "libkernel", sceKernelConfiguredFlexibleMemorySize);

    LIB_FUNCTION("vSMAm3cxYTY", "libkernel", 1, "libkernel", sceKernelMprotect);
    LIB_FUNCTION("YQOfxL4QfeU", "libkernel", 1, "libkernel", posix_mprotect);
    LIB_FUNCTION("YQOfxL4QfeU", "libScePosix", 1, "libkernel", posix_mprotect);
    LIB_FUNCTION("9bfdLIyuwCY", "libkernel", 1, "libkernel", sceKernelMtypeprotect);

    // Memory pool
    LIB_FUNCTION("qCSfqDILlns", "libkernel", 1, "libkernel", sceKernelMemoryPoolExpand);
    LIB_FUNCTION("pU-QydtGcGY", "libkernel", 1, "libkernel", sceKernelMemoryPoolReserve);
    LIB_FUNCTION("Vzl66WmfLvk", "libkernel", 1, "libkernel", sceKernelMemoryPoolCommit);
    LIB_FUNCTION("LXo1tpFqJGs", "libkernel", 1, "libkernel", sceKernelMemoryPoolDecommit);
    LIB_FUNCTION("YN878uKRBbE", "libkernel", 1, "libkernel", sceKernelMemoryPoolBatch);
    LIB_FUNCTION("bvD+95Q6asU", "libkernel", 1, "libkernel", sceKernelMemoryPoolGetBlockStats);

    LIB_FUNCTION("BPE9s9vQQXo", "libkernel", 1, "libkernel", posix_mmap);
    LIB_FUNCTION("BPE9s9vQQXo", "libScePosix", 1, "libkernel", posix_mmap);
    LIB_FUNCTION("UqDGjXA5yUM", "libkernel", 1, "libkernel", posix_munmap);
    LIB_FUNCTION("UqDGjXA5yUM", "libScePosix", 1, "libkernel", posix_munmap);
    LIB_FUNCTION("3k6kx-zOOSQ", "libkernel", 1, "libkernel", sceKernelMlock);
    LIB_FUNCTION("mTBZfEal2Bw", "libkernel", 1, "libkernel", posix_mlock);
    LIB_FUNCTION("mTBZfEal2Bw", "libScePosix", 1, "libkernel", posix_mlock);
    LIB_FUNCTION("xQIIfJ860sk", "libkernel", 1, "libkernel", sceKernelMunlock);
    LIB_FUNCTION("OG4RsDwLguo", "libkernel", 1, "libkernel", posix_munlock);
    LIB_FUNCTION("OG4RsDwLguo", "libScePosix", 1, "libkernel", posix_munlock);
    LIB_FUNCTION("teiItL2boFw", "libkernel", 1, "libkernel", sceKernelReleaseFlexibleMemory);
    LIB_FUNCTION("tZY4+SZNFhA", "libkernel", 1, "libkernel", posix_msync);
    LIB_FUNCTION("tZY4+SZNFhA", "libScePosix", 1, "libkernel", posix_msync);

    // PRT memory management
    LIB_FUNCTION("BohYr-F7-is", "libkernel", 1, "libkernel", sceKernelSetPrtAperture);
    LIB_FUNCTION("L0v2Go5jOuM", "libkernel", 1, "libkernel", sceKernelGetPrtAperture);
}

} // namespace Libraries::Kernel
