// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bit>
#include <cstring>
#include <string_view>

#include "common/singleton.h"
#include "common/types.h"
#include "core/memory/address_space.h"
#include "core/memory/blockpool.h"
#include "core/memory/budget.h"
#include "core/memory/dmem_map.h"
#include "core/memory/flexible_pool.h"
#include "core/memory/vm_map.h"

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
constexpr u64 GUEST_PAGE_BITS = 14;
constexpr u64 GUEST_PAGE_SIZE = 1ULL << GUEST_PAGE_BITS;

class MemoryManager {
    static constexpr u64 PAGE_SIZE = 0x4000;
    static constexpr u64 PAGE_MASK = PAGE_SIZE - 1;

public:
    explicit MemoryManager();
    ~MemoryManager();

    void SetRasterizer(Vulkan::Rasterizer* rasterizer_) {
        blockpool.SetRasterizer(rasterizer_);
        vm_map.SetRasterizer(rasterizer_);
        dmem.SetRasterizer(rasterizer_);
        rasterizer = rasterizer_;
    }

    AddressSpace& GetAddressSpace() {
        return impl;
    }

    Blockpool& GetBlockpool() {
        return blockpool;
    }

    DmemManager& GetDmemManager() {
        return dmem;
    }

    FlexibleMemoryPool& GetFlexiblePool() {
        return flex_pool;
    }

    VmMap& GetVmMap() {
        return vm_map;
    }

    BudgetState& GetBudget() {
        return budget;
    }

    u64 GetTotalDirectSize() const {
        return total_direct_size;
    }

    u64 GetTotalFlexibleSize() const {
        return total_flexible_size;
    }

    u64 GetAvailableFlexibleSize() const {
        return budget.mlock_limit[u32(BudgetPtype::BigApp)] -
               budget.mlock_used[u32(BudgetPtype::BigApp)];
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
        if (virtual_addr < vm_map.MinOffset() || virtual_addr >= vm_map.MaxOffset()) {
            return false;
        }
        return size <= vm_map.MaxOffset() - virtual_addr;
    }

    bool TryWriteBacking(void* address, const void* data, u64 size) {
        const auto* src = static_cast<const u8*>(data);
        return ForEachBackingInRange(std::bit_cast<VAddr>(address), size,
                                     [&](u64 offset, u64 backing_size, u8* backing) {
                                         std::memcpy(backing, src + offset, backing_size);
                                     });
    }

    u64 ClampRangeSize(VAddr virtual_addr, u64 size);

    void SetPrtArea(u32 id, VAddr address, u64 size);

    void CopySparseMemory(VAddr source, u8* dest, u64 size);

    void SetupMemoryRegions(u64 flexible_size, bool use_extended_mem1, bool use_extended_mem2);

    s32 MapMemory(VAddr* out_addr, u64 size, MemoryProt prot, MemoryMapFlags flags, s32 fd,
                  u64 offset, std::string_view name);

    s32 UnmapMemory(VAddr virtual_addr, u64 size);

    s32 MapDirectMemory(VAddr* out_addr, u64 size, DmemMemoryType mtype, MemoryProt prot,
                        MemoryMapFlags flags, PAddr phys_addr, u32 alignment_shift);

    s32 PoolMap(VAddr virtual_addr, u64 size, MemoryProt prot, s32 mtype);

    s32 PoolUnmap(VAddr virtual_addr, u64 size);

    s32 QueryProtection(VAddr addr, VAddr* start, VAddr* end, u32* prot);

    s32 Protect(VAddr start, u64 size, MemoryProt new_prot);

    s32 ProtectType(VAddr start, u64 size, DmemMemoryType new_mtype, MemoryProt new_prot);

    s32 VirtualQuery(VAddr addr, s32 flags, ::Libraries::Kernel::OrbisVirtualQueryInfo* info);

    void NameVirtualRange(VAddr virtual_addr, u64 size, std::string_view name);

    void InvalidateMemory(VAddr addr, u64 size) const;

    template <typename Func>
    bool ForEachBackingInRange(VAddr addr, u64 size, Func&& func) {
        std::shared_lock lk{vm_map.lock};
        auto [entry, found] = vm_map.LookupEntryReadOnly(addr);
        if (!found) {
            return false;
        }

        u64 cursor = 0;
        VAddr current = addr;
        while (entry != vm_map.GetTree().end() && size) {
            if (entry->start > current) {
                // Skip unmapped space between entries.
                const u64 gap = std::min<u64>(entry->start - current, size);
                cursor += gap;
                current += gap;
                size -= gap;
                if (size == 0) {
                    break;
                }
            }
            const u64 clip_start = current - entry->start;
            const u64 clip_offset = clip_start + (entry->IsBlockpool() ? 0 : entry->offset);
            const u64 clip_size = std::min<u64>(entry->end - current, size);
            if (entry->object && entry->object->HasPhysicalBacking()) {
                u8* backing_base = impl.BackingBase();
                entry->object->ForEachBacking(
                    clip_offset, clip_size, [&](u64 phys_addr, u64 backing_size, u64 range_offset) {
                        func(cursor + range_offset, backing_size, backing_base + phys_addr);
                    });
            }
            cursor += clip_size;
            current += clip_size;
            size -= clip_size;
            entry++;
        }
        return true;
    }

private:
    AddressSpace impl;
    BudgetState budget;
    Blockpool blockpool;
    DmemManager dmem;
    VmMap vm_map;
    FlexibleMemoryPool flex_pool;
    Vulkan::Rasterizer* rasterizer;
    u64 total_direct_size{};
    u64 total_flexible_size{};
    s32 sdk_version{};

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
