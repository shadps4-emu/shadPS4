// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <mutex>
#include <string_view>
#include "common/enum.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/address_space.h"
#include "core/libraries/kernel/memory_management.h"

namespace Vulkan {
class Rasterizer;
}

namespace Libraries::Kernel {
struct OrbisQueryInfo;
}

namespace Core {

enum class MemoryProt : u32 {
    NoAccess = 0,
    CpuRead = 1,
    CpuReadWrite = 2,
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
    NoOverwrite = 0x0080,
    NoSync = 0x800,
    NoCore = 0x20000,
    NoCoalesce = 0x400000,
};
DECLARE_ENUM_FLAG_OPERATORS(MemoryMapFlags)

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

struct DirectMemoryArea {
    PAddr base = 0;
    size_t size = 0;
    int memory_type = 0;
    bool is_pooled = false;
    bool is_free = true;

    PAddr GetEnd() const {
        return base + size;
    }

    bool CanMergeWith(const DirectMemoryArea& next) const {
        if (base + size != next.base) {
            return false;
        }
        if (is_free != next.is_free) {
            return false;
        }
        return true;
    }
};

struct VirtualMemoryArea {
    VAddr base = 0;
    size_t size = 0;
    PAddr phys_base = 0;
    VMAType type = VMAType::Free;
    MemoryProt prot = MemoryProt::NoAccess;
    bool disallow_merge = false;
    std::string name = "";
    uintptr_t fd = 0;
    bool is_exec = false;

    bool Contains(VAddr addr, size_t size) const {
        return addr >= base && (addr + size) <= (base + this->size);
    }

    bool IsFree() const noexcept {
        return type == VMAType::Free;
    }

    bool IsMapped() const noexcept {
        return type != VMAType::Free && type != VMAType::Reserved && type != VMAType::PoolReserved;
    }

    bool CanMergeWith(const VirtualMemoryArea& next) const {
        if (disallow_merge || next.disallow_merge) {
            return false;
        }
        if (base + size != next.base) {
            return false;
        }
        if (type == VMAType::Direct && phys_base + size != next.phys_base) {
            return false;
        }
        if (prot != next.prot || type != next.type) {
            return false;
        }
        return true;
    }
};

class MemoryManager {
    using DMemMap = std::map<PAddr, DirectMemoryArea>;
    using DMemHandle = DMemMap::iterator;

    using VMAMap = std::map<VAddr, VirtualMemoryArea>;
    using VMAHandle = VMAMap::iterator;

public:
    explicit MemoryManager();
    ~MemoryManager();

    void SetRasterizer(Vulkan::Rasterizer* rasterizer_) {
        rasterizer = rasterizer_;
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

    bool TryWriteBacking(void* address, const void* data, u32 num_bytes);

    void SetupMemoryRegions(u64 flexible_size);

    PAddr PoolExpand(PAddr search_start, PAddr search_end, size_t size, u64 alignment);

    PAddr Allocate(PAddr search_start, PAddr search_end, size_t size, u64 alignment,
                   int memory_type);

    void Free(PAddr phys_addr, size_t size);

    int PoolReserve(void** out_addr, VAddr virtual_addr, size_t size, MemoryMapFlags flags,
                    u64 alignment = 0);

    int Reserve(void** out_addr, VAddr virtual_addr, size_t size, MemoryMapFlags flags,
                u64 alignment = 0);

    int PoolCommit(VAddr virtual_addr, size_t size, MemoryProt prot);

    int MapMemory(void** out_addr, VAddr virtual_addr, size_t size, MemoryProt prot,
                  MemoryMapFlags flags, VMAType type, std::string_view name = "",
                  bool is_exec = false, PAddr phys_addr = -1, u64 alignment = 0);

    int MapFile(void** out_addr, VAddr virtual_addr, size_t size, MemoryProt prot,
                MemoryMapFlags flags, uintptr_t fd, size_t offset);

    void PoolDecommit(VAddr virtual_addr, size_t size);

    void UnmapMemory(VAddr virtual_addr, size_t size);

    int QueryProtection(VAddr addr, void** start, void** end, u32* prot);

    int Protect(VAddr addr, size_t size, MemoryProt prot);

    int VirtualQuery(VAddr addr, int flags, ::Libraries::Kernel::OrbisVirtualQueryInfo* info);

    int DirectMemoryQuery(PAddr addr, bool find_next,
                          ::Libraries::Kernel::OrbisQueryInfo* out_info);

    int DirectQueryAvailable(PAddr search_start, PAddr search_end, size_t alignment,
                             PAddr* phys_addr_out, size_t* size_out);

    int GetDirectMemoryType(PAddr addr, int* directMemoryTypeOut, void** directMemoryStartOut,
                            void** directMemoryEndOut);

    void NameVirtualRange(VAddr virtual_addr, size_t size, std::string_view name);

private:
    VMAHandle FindVMA(VAddr target) {
        return std::prev(vma_map.upper_bound(target));
    }

    DMemHandle FindDmemArea(PAddr target) {
        return std::prev(dmem_map.upper_bound(target));
    }

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

    VAddr SearchFree(VAddr virtual_addr, size_t size, u32 alignment = 0);

    VMAHandle CarveVMA(VAddr virtual_addr, size_t size);

    DMemHandle CarveDmemArea(PAddr addr, size_t size);

    VMAHandle Split(VMAHandle vma_handle, size_t offset_in_vma);

    DMemHandle Split(DMemHandle dmem_handle, size_t offset_in_area);

    void UnmapMemoryImpl(VAddr virtual_addr, size_t size);

private:
    AddressSpace impl;
    DMemMap dmem_map;
    VMAMap vma_map;
    std::mutex mutex;
    size_t total_direct_size{};
    size_t total_flexible_size{};
    size_t flexible_usage{};
    Vulkan::Rasterizer* rasterizer{};
};

using Memory = Common::Singleton<MemoryManager>;

} // namespace Core
