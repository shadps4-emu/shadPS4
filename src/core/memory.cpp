// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/scope_exit.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/memory_management.h"
#include "core/memory.h"
#include "video_core/renderer_vulkan/vk_instance.h"

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
    PAddr free_addr = search_start;

    // Iterate through allocated blocked and find the next free position
    for (const auto& block : allocations) {
        const PAddr end = block.base + block.size;
        free_addr = std::max(end, free_addr);
    }

    // Align free position
    free_addr = alignment > 0 ? Common::AlignUp(free_addr, alignment) : free_addr;
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
    VAddr mapped_addr = alignment > 0 ? Common::AlignUp(virtual_addr, alignment) : virtual_addr;
    SCOPE_EXIT {
        auto& new_vma = AddMapping(mapped_addr, size);
        new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
        new_vma.prot = prot;
        new_vma.name = name;
        new_vma.type = type;

        if (type == VMAType::Direct) {
            MapVulkanMemory(mapped_addr, size);
        }
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
        const size_t remaining_size = vma.base + vma.size - mapped_addr;
        ASSERT_MSG(vma.type == VMAType::Free && remaining_size >= size);
    }

    // Find the first free area starting with provided virtual address.
    if (False(flags & MemoryMapFlags::Fixed)) {
        auto it = FindVMA(mapped_addr);
        while (it->second.type != VMAType::Free || it->second.size < size) {
            it++;
        }
        ASSERT(it != vma_map.end());
        mapped_addr = alignment > 0 ? Common::AlignUp(it->second.base, alignment) : it->second.base;
    }

    // Perform the mapping.
    *out_addr = impl.Map(mapped_addr, size, alignment, phys_addr);
    return ORBIS_OK;
}

void MemoryManager::UnmapMemory(VAddr virtual_addr, size_t size) {
    // TODO: Partial unmaps are technically supported by the guest.
    const auto it = vma_map.find(virtual_addr);
    ASSERT_MSG(it != vma_map.end() && it->first == virtual_addr,
               "Attempting to unmap partially mapped range");

    if (it->second.type == VMAType::Direct) {
        UnmapVulkanMemory(virtual_addr, size);
    }

    // Mark region as free and attempt to coalesce it with neighbours.
    auto& vma = it->second;
    vma.type = VMAType::Free;
    vma.prot = MemoryProt::NoAccess;
    vma.phys_base = 0;
    MergeAdjacent(it);

    // Unmap the memory region.
    impl.Unmap(virtual_addr, size);
}

int MemoryManager::QueryProtection(VAddr addr, void** start, void** end, u32* prot) {
    const auto it = FindVMA(addr);
    const auto& vma = it->second;
    ASSERT_MSG(vma.type != VMAType::Free, "Provided address is not mapped");

    *start = reinterpret_cast<void*>(vma.base);
    *end = reinterpret_cast<void*>(vma.base + vma.size);
    *prot = static_cast<u32>(vma.prot);
    return ORBIS_OK;
}

int MemoryManager::DirectMemoryQuery(PAddr addr, bool find_next,
                                     Libraries::Kernel::OrbisQueryInfo* out_info) {
    const auto it = std::ranges::find_if(allocations, [&](const DirectMemoryArea& alloc) {
        return alloc.base <= addr && addr < alloc.base + alloc.size;
    });
    if (it == allocations.end()) {
        return SCE_KERNEL_ERROR_EACCES;
    }

    out_info->start = it->base;
    out_info->end = it->base + it->size;
    out_info->memoryType = it->memory_type;
    return ORBIS_OK;
}

std::pair<vk::Buffer, size_t> MemoryManager::GetVulkanBuffer(VAddr addr) {
    auto it = mapped_memories.upper_bound(addr);
    it = std::prev(it);
    ASSERT(it != mapped_memories.end() && it->first <= addr);
    return std::make_pair(*it->second.buffer, addr - it->first);
}

VirtualMemoryArea& MemoryManager::AddMapping(VAddr virtual_addr, size_t size) {
    auto vma_handle = FindVMA(virtual_addr);
    ASSERT_MSG(vma_handle != vma_map.end(), "Virtual address not in vm_map");

    const VirtualMemoryArea& vma = vma_handle->second;
    ASSERT_MSG(vma.type == VMAType::Free && vma.base <= virtual_addr,
               "Adding a mapping to already mapped region");

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

MemoryManager::VMAHandle MemoryManager::Split(VMAHandle vma_handle, size_t offset_in_vma) {
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

void MemoryManager::MapVulkanMemory(VAddr addr, size_t size) {
    return;
    const vk::Device device = instance->GetDevice();
    const auto memory_props = instance->GetPhysicalDevice().getMemoryProperties();
    void* host_pointer = reinterpret_cast<void*>(addr);
    const auto host_mem_props = device.getMemoryHostPointerPropertiesEXT(
        vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT, host_pointer);
    ASSERT(host_mem_props.memoryTypeBits != 0);

    int mapped_memory_type = -1;
    auto find_mem_type_with_flag = [&](const vk::MemoryPropertyFlags flags) {
        u32 host_mem_types = host_mem_props.memoryTypeBits;
        while (host_mem_types != 0) {
            // Try to find a cached memory type
            mapped_memory_type = std::countr_zero(host_mem_types);
            host_mem_types -= (1 << mapped_memory_type);

            if ((memory_props.memoryTypes[mapped_memory_type].propertyFlags & flags) == flags) {
                return;
            }
        }

        mapped_memory_type = -1;
    };

    // First try to find a memory that is both coherent and cached
    find_mem_type_with_flag(vk::MemoryPropertyFlagBits::eHostCoherent |
                            vk::MemoryPropertyFlagBits::eHostCached);
    if (mapped_memory_type == -1)
        // Then only coherent (lower performance)
        find_mem_type_with_flag(vk::MemoryPropertyFlagBits::eHostCoherent);

    if (mapped_memory_type == -1) {
        LOG_CRITICAL(Render_Vulkan, "No coherent memory available for memory mapping");
        mapped_memory_type = std::countr_zero(host_mem_props.memoryTypeBits);
    }

    const vk::StructureChain alloc_info = {
        vk::MemoryAllocateInfo{
            .allocationSize = size,
            .memoryTypeIndex = static_cast<uint32_t>(mapped_memory_type),
        },
        vk::ImportMemoryHostPointerInfoEXT{
            .handleType = vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT,
            .pHostPointer = host_pointer,
        },
    };

    const auto [it, new_memory] = mapped_memories.try_emplace(addr);
    ASSERT_MSG(new_memory, "Attempting to remap already mapped vulkan memory");

    auto& memory = it->second;
    memory.backing = device.allocateMemoryUnique(alloc_info.get());

    constexpr vk::BufferUsageFlags MapFlags =
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer;

    const vk::StructureChain buffer_info = {
        vk::BufferCreateInfo{
            .size = size,
            .usage = MapFlags,
            .sharingMode = vk::SharingMode::eExclusive,
        },
        vk::ExternalMemoryBufferCreateInfoKHR{
            .handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eHostAllocationEXT,
        }};
    memory.buffer = device.createBufferUnique(buffer_info.get());
    device.bindBufferMemory(*memory.buffer, *memory.backing, 0);
}

void MemoryManager::UnmapVulkanMemory(VAddr addr, size_t size) {
    return;
    const auto it = mapped_memories.find(addr);
    ASSERT(it != mapped_memories.end() && it->second.buffer_size == size);
    mapped_memories.erase(it);
}

} // namespace Core
