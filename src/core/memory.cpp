// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/scope_exit.h"
#include "core/libraries/error_codes.h"
#include "core/memory.h"

namespace Core {

MemoryManager::MemoryManager() {
    // Insert a virtual memory area that covers the user area.
    const size_t user_size = USER_MAX - USER_MIN - 1;
    vma_map.emplace(USER_MIN, VirtualMemoryArea{USER_MIN, user_size});

    // Insert a virtual memory area that covers the system managed area.
    const size_t sys_size = SYSTEM_MANAGED_MAX - SYSTEM_MANAGED_MIN - 1;
    vma_map.emplace(SYSTEM_MANAGED_MIN, VirtualMemoryArea{SYSTEM_MANAGED_MIN, sys_size});
}

MemoryManager::~MemoryManager() = default;

PAddr MemoryManager::Allocate(PAddr search_start, PAddr search_end, size_t size, u64 alignment,
                              int memory_type) {
    PAddr free_addr = 0;

    // Iterate through allocated blocked and find the next free position
    for (const auto& block : allocations) {
        const PAddr end = block.base + block.size;
        free_addr = std::max(end, free_addr);
    }

    // Align free position
    free_addr = Common::alignUp(free_addr, alignment);
    ASSERT(free_addr >= search_start && free_addr + size <= search_end);

    // Add the allocated region to the list and commit its pages.
    allocations.emplace_back(free_addr, size, memory_type);
    return free_addr;
}

void MemoryManager::Free(PAddr phys_addr, size_t size) {
    const auto it = std::ranges::find_if(allocations, [&](const auto& alloc) {
        return alloc.base == phys_addr && alloc.size == size;
    });
    ASSERT(it != allocations.end());

    // Free the ranges.
    allocations.erase(it);
}

int MemoryManager::MapMemory(void** out_addr, VAddr virtual_addr, size_t size, MemoryProt prot,
                             MemoryMapFlags flags, VMAType type, std::string_view name,
                             PAddr phys_addr, u64 alignment) {
    VAddr mapped_addr = alignment > 0 ? Common::alignUp(virtual_addr, alignment) : virtual_addr;
    SCOPE_EXIT {
        auto& new_vma = AddMapping(mapped_addr, size);
        new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
        new_vma.prot = prot;
        new_vma.name = name;
        new_vma.type = type;
    };

    // When virtual addr is zero let the address space manager pick the address.
    // Alignment matters here as we let the OS pick the address.
    if (virtual_addr == 0) {
        *out_addr = impl.Map(virtual_addr, size, alignment);
        mapped_addr = std::bit_cast<VAddr>(*out_addr);
        return ORBIS_OK;
    }

    // Fixed mapping means the virtual address must exactly match the provided one.
    if (True(flags & MemoryMapFlags::Fixed) && True(flags & MemoryMapFlags::NoOverwrite)) {
        // This should return SCE_KERNEL_ERROR_ENOMEM but shouldn't normally happen.
        const auto& vma = FindVMA(mapped_addr)->second;
        const u32 remaining_size = vma.base + vma.size - mapped_addr;
        ASSERT_MSG(vma.type == VMAType::Free && remaining_size >= size);
    }

    // Find the first free area starting with provided virtual address.
    if (False(flags & MemoryMapFlags::Fixed)) {
        auto it = FindVMA(mapped_addr);
        while (it->second.type != VMAType::Free || it->second.size < size) {
            it++;
        }
        ASSERT(it != vma_map.end());
        if (alignment > 0) {
            ASSERT_MSG(it->second.base % alignment == 0, "Free region base is not aligned");
        }
        mapped_addr = it->second.base;
    }

    // Perform the mapping.
    *out_addr = impl.Map(mapped_addr, size);
    return ORBIS_OK;
}

void MemoryManager::UnmapMemory(VAddr virtual_addr, size_t size) {
    // TODO: Partial unmaps are technically supported by the guest.
    const auto it = vma_map.find(virtual_addr);
    ASSERT_MSG(it != vma_map.end() && it->first == virtual_addr,
               "Attempting to unmap partially mapped range");

    // Mark region as free and attempt to coalesce it with neighbours.
    auto& vma = it->second;
    vma.type = VMAType::Free;
    vma.prot = MemoryProt::NoAccess;
    vma.phys_base = 0;
    MergeAdjacent(it);

    // Unmap the memory region.
    impl.Unmap(virtual_addr, size);
}

VirtualMemoryArea& MemoryManager::AddMapping(VAddr virtual_addr, size_t size) {
    auto vma_handle = FindVMA(virtual_addr);
    ASSERT_MSG(vma_handle != vma_map.end(), "Virtual address not in vm_map");

    const VirtualMemoryArea& vma = vma_handle->second;
    ASSERT_MSG(vma.type == VMAType::Free, "Adding a mapping to already mapped region");

    const VAddr start_in_vma = virtual_addr - vma.base;
    const VAddr end_in_vma = start_in_vma + size;
    ASSERT_MSG(end_in_vma <= vma.size, "Mapping cannot fit inside free region");

    if (end_in_vma != vma.size) {
        // Split VMA at the end of the allocated region
        Split(vma_handle, end_in_vma);
    }
    if (start_in_vma != 0) {
        // Split VMA at the start of the allocated region
        vma_handle = Split(vma_handle, start_in_vma);
    }

    return vma_handle->second;
}

MemoryManager::VMAHandle MemoryManager::Split(VMAHandle vma_handle, u32 offset_in_vma) {
    auto& old_vma = vma_handle->second;
    ASSERT(offset_in_vma < old_vma.size && offset_in_vma > 0);

    auto new_vma = old_vma;
    old_vma.size = offset_in_vma;
    new_vma.base += offset_in_vma;
    new_vma.size -= offset_in_vma;

    if (new_vma.type == VMAType::Direct) {
        new_vma.phys_base += offset_in_vma;
    }
    return vma_map.emplace_hint(std::next(vma_handle), new_vma.base, new_vma);
}

MemoryManager::VMAHandle MemoryManager::MergeAdjacent(VMAHandle iter) {
    const auto next_vma = std::next(iter);
    if (next_vma != vma_map.end() && iter->second.CanMergeWith(next_vma->second)) {
        iter->second.size += next_vma->second.size;
        vma_map.erase(next_vma);
    }

    if (iter != vma_map.begin()) {
        auto prev_vma = std::prev(iter);
        if (prev_vma->second.CanMergeWith(iter->second)) {
            prev_vma->second.size += iter->second.size;
            vma_map.erase(iter);
            iter = prev_vma;
        }
    }

    return iter;
}

} // namespace Core
