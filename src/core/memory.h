// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include "common/enum.h"
#include "common/shared_first_mutex.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/address_space.h"
#include "core/libraries/kernel/memory.h"

namespace Vulkan {
class Rasterizer;
}

namespace Libraries::Kernel {
struct OrbisQueryInfo;
}

namespace Core::Devtools::Widget {
class MemoryMapViewer;
}

namespace Core {

constexpr u64 DEFAULT_MAPPING_BASE = 0x200000000;

enum class MemoryProt : u32 {
    NoAccess = 0,
    CpuRead = 1,
    CpuWrite = 2,
    CpuReadWrite = 3,
    CpuExec = 4,
    GpuRead = 16,
    GpuWrite = 32,
    GpuReadWrite = 48,
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryProt)

enum class MemoryMapFlags : u32 {
    NoFlags = 0,
    Shared = 1,
    Private = 2,
    Fixed = 0x10,
    NoOverwrite = 0x80,
    Void = 0x100,
    Stack = 0x400,
    NoSync = 0x800,
    Anon = 0x1000,
    NoCore = 0x20000,
    NoCoalesce = 0x400000,
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryMapFlags)

enum class PhysicalMemoryType : u32 {
    Free = 0,
    Allocated = 1,
    Mapped = 2,
    Pooled = 3,
    Committed = 4,
    Flexible = 5,
};

struct PhysicalMemoryArea {
    PAddr base = 0;
    u64 size = 0;
    s32 memory_type = 0;
    PhysicalMemoryType dma_type = PhysicalMemoryType::Free;

    PAddr GetEnd() const {
        return base + size;
    }

    bool CanMergeWith(const PhysicalMemoryArea& next) const {
        if (base + size != next.base) {
            return false;
        }
        if (memory_type != next.memory_type) {
            return false;
        }
        if (dma_type != next.dma_type) {
            return false;
        }
        return true;
    }
};

enum class VMAType : u32 {
    Free = 0,
    Reserved = 1,
    Direct = 2,
    Flexible = 3,
    Pooled = 4,
    PoolReserved = 5,
    Stack = 6,
    Code = 7,
    File = 8,
};

struct VirtualMemoryArea {
    VAddr base = 0;
    u64 size = 0;
    std::map<uintptr_t, PhysicalMemoryArea> phys_areas;
    VMAType type = VMAType::Free;
    MemoryProt prot = MemoryProt::NoAccess;
    std::string name = "";
    s32 fd = 0;
    bool disallow_merge = false;

    bool Contains(VAddr addr, u64 size) const {
        return addr >= base && (addr + size) <= (base + this->size);
    }

    bool Overlaps(VAddr addr, u64 size) const {
        return addr <= (base + this->size) && (addr + size) >= base;
    }

    bool IsFree() const noexcept {
        return type == VMAType::Free;
    }

    bool IsMapped() const noexcept {
        return type != VMAType::Free && type != VMAType::Reserved && type != VMAType::PoolReserved;
    }

    bool CanMergeWith(VirtualMemoryArea& next) {
        if (disallow_merge || next.disallow_merge) {
            return false;
        }
        if (base + size != next.base) {
            return false;
        }
        if (type == VMAType::Direct && next.type == VMAType::Direct) {
            auto& last_phys = std::prev(phys_areas.end())->second;
            auto& first_next_phys = next.phys_areas.begin()->second;
            if (last_phys.base + last_phys.size != first_next_phys.base ||
                last_phys.memory_type != first_next_phys.memory_type) {
                return false;
            }
        }
        if (prot != next.prot || type != next.type) {
            return false;
        }
        if (name.compare(next.name) != 0) {
            return false;
        }

        return true;
    }
};

class MemoryManager {
    using PhysMap = std::map<PAddr, PhysicalMemoryArea>;
    using PhysHandle = PhysMap::iterator;

    using VMAMap = std::map<VAddr, VirtualMemoryArea>;
    using VMAHandle = VMAMap::iterator;

public:
    explicit MemoryManager();
    ~MemoryManager();

    void SetRasterizer(Vulkan::Rasterizer* rasterizer_) {
        rasterizer = rasterizer_;
    }

    AddressSpace& GetAddressSpace() {
        return impl;
    }

    u64 GetTotalDirectSize() const {
        return total_direct_size;
    }

    u64 GetTotalFlexibleSize() const {
        return total_flexible_size;
    }

    u64 GetAvailableFlexibleSize() const {
        return total_flexible_size - flexible_usage;
    }

    VAddr SystemReservedVirtualBase() noexcept {
        return impl.SystemReservedVirtualBase();
    }

    bool IsValidGpuMapping(VAddr virtual_addr, u64 size) {
        // The PS4's GPU can only handle 40 bit addresses.
        const VAddr max_gpu_address{0x10000000000};
        return virtual_addr + size < max_gpu_address;
    }

    bool IsValidMapping(const VAddr virtual_addr, const u64 size = 0) {
        const auto end_it = std::prev(vma_map.end());
        const VAddr end_addr = end_it->first + end_it->second.size;

        // If the address fails boundary checks, return early.
        if (virtual_addr < vma_map.begin()->first || virtual_addr >= end_addr) {
            return false;
        }

        // If size is zero and boundary checks succeed, then skip more robust checking
        if (size == 0) {
            return true;
        }

        // Now make sure the full address range is contained in vma_map.
        auto vma_handle = FindVMA(virtual_addr);
        auto addr_to_check = virtual_addr;
        u64 size_to_validate = size;
        while (vma_handle != vma_map.end() && size_to_validate > 0) {
            const auto offset_in_vma = addr_to_check - vma_handle->second.base;
            const auto size_in_vma =
                std::min<u64>(vma_handle->second.size - offset_in_vma, size_to_validate);
            size_to_validate -= size_in_vma;
            addr_to_check += size_in_vma;
            vma_handle++;

            // Make sure there isn't any gap here
            if (size_to_validate > 0 && vma_handle != vma_map.end() &&
                addr_to_check != vma_handle->second.base) {
                return false;
            }
        }

        // If we reach this point and size to validate is not positive, then this mapping is valid.
        return size_to_validate <= 0;
    }

    u64 ClampRangeSize(VAddr virtual_addr, u64 size);

    void SetPrtArea(u32 id, VAddr address, u64 size);

    void CopySparseMemory(VAddr source, u8* dest, u64 size);

    bool TryWriteBacking(void* address, const void* data, u64 size);

    void SetupMemoryRegions(u64 flexible_size, bool use_extended_mem1, bool use_extended_mem2);

    PAddr PoolExpand(PAddr search_start, PAddr search_end, u64 size, u64 alignment);

    PAddr Allocate(PAddr search_start, PAddr search_end, u64 size, u64 alignment, s32 memory_type);

    s32 Free(PAddr phys_addr, u64 size, bool is_checked);

    s32 PoolCommit(VAddr virtual_addr, u64 size, MemoryProt prot, s32 mtype);

    s32 MapMemory(void** out_addr, VAddr virtual_addr, u64 size, MemoryProt prot,
                  MemoryMapFlags flags, VMAType type, std::string_view name = "anon",
                  bool validate_dmem = false, PAddr phys_addr = -1, u64 alignment = 0);

    s32 MapFile(void** out_addr, VAddr virtual_addr, u64 size, MemoryProt prot,
                MemoryMapFlags flags, s32 fd, s64 phys_addr);

    s32 PoolDecommit(VAddr virtual_addr, u64 size);

    s32 UnmapMemory(VAddr virtual_addr, u64 size);

    s32 QueryProtection(VAddr addr, void** start, void** end, u32* prot);

    s32 Protect(VAddr addr, u64 size, MemoryProt prot);

    s64 ProtectBytes(VAddr addr, VirtualMemoryArea& vma_base, u64 size, MemoryProt prot);

    s32 VirtualQuery(VAddr addr, s32 flags, ::Libraries::Kernel::OrbisVirtualQueryInfo* info);

    s32 DirectMemoryQuery(PAddr addr, bool find_next,
                          ::Libraries::Kernel::OrbisQueryInfo* out_info);

    s32 DirectQueryAvailable(PAddr search_start, PAddr search_end, u64 alignment,
                             PAddr* phys_addr_out, u64* size_out);

    s32 GetDirectMemoryType(PAddr addr, s32* directMemoryTypeOut, void** directMemoryStartOut,
                            void** directMemoryEndOut);

    s32 IsStack(VAddr addr, void** start, void** end);

    s32 SetDirectMemoryType(VAddr addr, u64 size, s32 memory_type);

    void NameVirtualRange(VAddr virtual_addr, u64 size, std::string_view name);

    s32 GetMemoryPoolStats(::Libraries::Kernel::OrbisKernelMemoryPoolBlockStats* stats);

    void InvalidateMemory(VAddr addr, u64 size) const;

private:
    VMAHandle FindVMA(VAddr target) {
        return std::prev(vma_map.upper_bound(target));
    }

    PhysHandle FindDmemArea(PAddr target) {
        return std::prev(dmem_map.upper_bound(target));
    }

    PhysHandle FindFmemArea(PAddr target) {
        return std::prev(fmem_map.upper_bound(target));
    }

    bool HasPhysicalBacking(VirtualMemoryArea vma) {
        return vma.type == VMAType::Direct || vma.type == VMAType::Flexible ||
               vma.type == VMAType::Pooled;
    }

    VMAHandle CreateArea(VAddr virtual_addr, u64 size, MemoryProt prot, MemoryMapFlags flags,
                         VMAType type, std::string_view name, u64 alignment);

    VAddr SearchFree(VAddr virtual_addr, u64 size, u32 alignment);

    VMAHandle MergeAdjacent(VMAMap& map, VMAHandle iter);

    PhysHandle MergeAdjacent(PhysMap& map, PhysHandle iter);

    VMAHandle CarveVMA(VAddr virtual_addr, u64 size);

    PhysHandle CarvePhysArea(PhysMap& map, PAddr addr, u64 size);

    VMAHandle Split(VMAHandle vma_handle, u64 offset_in_vma);

    PhysHandle Split(PhysMap& map, PhysHandle dmem_handle, u64 offset_in_area);

    u64 UnmapBytesFromEntry(VAddr virtual_addr, VirtualMemoryArea vma_base, u64 size);

    s32 UnmapMemoryImpl(VAddr virtual_addr, u64 size);

private:
    AddressSpace impl;
    PhysMap dmem_map;
    PhysMap fmem_map;
    VMAMap vma_map;
    Common::SharedFirstMutex mutex{};
    std::mutex unmap_mutex{};
    u64 total_direct_size{};
    u64 total_flexible_size{};
    u64 flexible_usage{};
    u64 pool_budget{};
    s32 sdk_version{};
    Vulkan::Rasterizer* rasterizer{};

    struct PrtArea {
        VAddr start;
        VAddr end;
        bool mapped;

        bool Overlaps(VAddr test_address, u64 test_size) const {
            const VAddr overlap_end = test_address + test_size;
            return start < overlap_end && test_address < end;
        }
    };
    std::array<PrtArea, 3> prt_areas{};

    friend class ::Core::Devtools::Widget::MemoryMapViewer;
};

using Memory = Common::Singleton<MemoryManager>;

} // namespace Core
