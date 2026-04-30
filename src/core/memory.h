// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <shared_mutex>
#include <string>
#include <string_view>

#include "common/assert.h"
#include "common/enum.h"
#include "common/shared_first_mutex.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/address_space.h"
#include "core/blockpool.h"
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
    Committed = 3,
    Flexible = 4,
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
    Stack = 5,
    Code = 6,
    File = 7,
};

struct VirtualMemoryArea {
    VAddr base = 0;
    u64 size = 0;
    PAddr phys_base = 0;
    VMAType type = VMAType::Free;
    MemoryProt prot = MemoryProt::NoAccess;
    std::string name = "";
    s32 fd = 0;
    bool disallow_merge = false;
    std::vector<DmemBlock> blocks;

    bool Contains(VAddr addr, u64 size) const {
        return addr >= base && (addr + size) <= (base + this->size);
    }

    bool Overlaps(VAddr addr, u64 size) const {
        return addr < (base + this->size) && (addr + size) > base;
    }

    bool IsFree() const noexcept {
        return type == VMAType::Free;
    }

    bool IsMapped() const noexcept {
        return type != VMAType::Free && type != VMAType::Reserved;
    }

    bool HasPhysicalBacking() const noexcept {
        return type == VMAType::Direct || type == VMAType::Flexible || type == VMAType::Pooled;
    }

    bool CanMergeWith(VirtualMemoryArea& next) {
        if (disallow_merge || next.disallow_merge) {
            return false;
        }
        if (base + size != next.base) {
            return false;
        }
        if ((type == VMAType::Direct || type == VMAType::Flexible) &&
            phys_base + size != next.phys_base) {
            return false;
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

    Blockpool& GetBlockpool() {
        return blockpool;
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

    bool ForEachBackingRegion(VAddr virtual_addr, u64 size, auto&& func) {
        std::shared_lock lk{mutex};

        const VAddr base_addr = virtual_addr;
        auto vma = FindVMA(virtual_addr);
        while (vma->second.Overlaps(virtual_addr, size)) {
            if (!vma->second.HasPhysicalBacking()) {
                return false;
            }
            if (vma->second.type == VMAType::Pooled) {
                return false;
            }
            const u64 start_in_vma = virtual_addr - vma->first;
            const u64 size_in_vma = std::min<u64>(vma->second.size - start_in_vma, size);
            u8* backing = impl.BackingBase() + vma->second.phys_base + start_in_vma;
            func(virtual_addr - base_addr, size_in_vma, backing);
            size -= size_in_vma;
            virtual_addr += size_in_vma;
            ++vma;
        }

        return true;
    }

    bool IsValidMapping(const VAddr virtual_addr, const u64 size = 0);

    u64 ClampRangeSize(VAddr virtual_addr, u64 size);

    void SetPrtArea(u32 id, VAddr address, u64 size);

    void CopySparseMemory(VAddr source, u8* dest, u64 size);

    void SetupMemoryRegions(u64 flexible_size, bool use_extended_mem1, bool use_extended_mem2);

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

    VMAHandle CreateArea(VAddr virtual_addr, u64 size, MemoryProt prot, MemoryMapFlags flags,
                         VMAType type, std::string_view name, u64 alignment);

    VAddr SearchFree(VAddr virtual_addr, u64 size, u32 alignment);

    template <typename Handle>
    Handle MergeAdjacent(auto& handle_map, Handle iter) {
        const auto next_vma = std::next(iter);
        if (next_vma != handle_map.end() && iter->second.CanMergeWith(next_vma->second)) {
            iter->second.size += next_vma->second.size;
            handle_map.erase(next_vma);
        }

        if (iter != handle_map.begin()) {
            auto prev_vma = std::prev(iter);
            if (prev_vma->second.CanMergeWith(iter->second)) {
                prev_vma->second.size += iter->second.size;
                handle_map.erase(iter);
                iter = prev_vma;
            }
        }

        return iter;
    }

    VMAHandle CarveVMA(VAddr virtual_addr, u64 size);

    PhysHandle CarvePhysArea(PhysMap& map, PAddr addr, u64 size);

    VMAHandle Split(VMAHandle vma_handle, u64 offset_in_vma);

    PhysHandle Split(PhysMap& map, PhysHandle dmem_handle, u64 offset_in_area);

    u64 UnmapBytesFromEntry(VAddr virtual_addr, const VirtualMemoryArea& vma_base, u64 size);

    s32 UnmapMemoryImpl(VAddr virtual_addr, u64 size);

private:
    AddressSpace impl;
    PhysMap dmem_map;
    PhysMap fmem_map;
    VMAMap vma_map;
    Blockpool blockpool;
    Common::SharedFirstMutex mutex{};
    u64 total_direct_size{};
    u64 total_flexible_size{};
    u64 flexible_usage{};
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
