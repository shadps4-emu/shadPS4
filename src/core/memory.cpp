// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/debug.h"
#include "common/scope_exit.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/memory_management.h"
#include "core/memory.h"
#include "video_core/renderer_vulkan/vk_instance.h"

namespace Core {

MemoryManager::MemoryManager() {
    // Insert an area that covers direct memory physical block.
    dmem_map.emplace(0, DirectMemoryArea{0, SCE_KERNEL_MAIN_DMEM_SIZE});

    // Insert a virtual memory area that covers the entire area we manage.
    const VAddr system_managed_base = impl.SystemManagedVirtualBase();
    const size_t system_managed_size = impl.SystemManagedVirtualSize();
    const VAddr system_reserved_base = impl.SystemReservedVirtualBase();
    const size_t system_reserved_size = impl.SystemReservedVirtualSize();
    const VAddr user_base = impl.UserVirtualBase();
    const size_t user_size = impl.UserVirtualSize();
    vma_map.emplace(system_managed_base,
                    VirtualMemoryArea{system_managed_base, system_managed_size});
    vma_map.emplace(system_reserved_base,
                    VirtualMemoryArea{system_reserved_base, system_reserved_size});
    vma_map.emplace(user_base, VirtualMemoryArea{user_base, user_size});

    // Log initialization.
    LOG_INFO(Kernel_Vmm, "Usable memory address space: {}_GB",
             (system_managed_size + system_reserved_size + user_size) >> 30);
}

MemoryManager::~MemoryManager() = default;

PAddr MemoryManager::Allocate(PAddr search_start, PAddr search_end, size_t size, u64 alignment,
                              int memory_type) {
    std::scoped_lock lk{mutex};

    auto dmem_area = FindDmemArea(search_start);

    const auto is_suitable = [&] {
        return dmem_area->second.is_free && dmem_area->second.size >= size;
    };
    while (!is_suitable() && dmem_area->second.GetEnd() <= search_end) {
        dmem_area++;
    }
    ASSERT_MSG(is_suitable(), "Unable to find free direct memory area");

    // Align free position
    PAddr free_addr = dmem_area->second.base;
    free_addr = alignment > 0 ? Common::AlignUp(free_addr, alignment) : free_addr;

    // Add the allocated region to the list and commit its pages.
    auto& area = AddDmemAllocation(free_addr, size);
    area.memory_type = memory_type;
    area.is_free = false;
    return free_addr;
}

void MemoryManager::Free(PAddr phys_addr, size_t size) {
    std::scoped_lock lk{mutex};

    const auto dmem_area = FindDmemArea(phys_addr);
    ASSERT(dmem_area != dmem_map.end() && dmem_area->second.base == phys_addr &&
           dmem_area->second.size == size);

    // Release any dmem mappings that reference this physical block.
    std::vector<std::pair<VAddr, u64>> remove_list;
    for (const auto& [addr, mapping] : vma_map) {
        if (mapping.type != VMAType::Direct) {
            continue;
        }
        if (mapping.phys_base <= phys_addr && phys_addr < mapping.phys_base + mapping.size) {
            LOG_INFO(Kernel_Vmm, "Unmaping direct mapping {:#x} with size {:#x}", addr,
                     mapping.size);
            // Unmaping might erase from vma_map. We can't do it here.
            remove_list.emplace_back(addr, mapping.size);
        }
    }
    for (const auto& [addr, size] : remove_list) {
        UnmapMemory(addr, size);
    }

    // Mark region as free and attempt to coalesce it with neighbours.
    auto& area = dmem_area->second;
    area.is_free = true;
    area.memory_type = 0;
    MergeAdjacent(dmem_map, dmem_area);
}

int MemoryManager::Reserve(void** out_addr, VAddr virtual_addr, size_t size, MemoryMapFlags flags,
                           u64 alignment) {
    std::scoped_lock lk{mutex};

    ASSERT_MSG(virtual_addr != 0, "TODO: Reserve address is zero - search for free space");

    VAddr mapped_addr = alignment > 0 ? Common::AlignUp(virtual_addr, alignment) : virtual_addr;

    // Add virtual memory area
    auto& new_vma = AddMapping(mapped_addr, size);
    new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
    new_vma.prot = MemoryProt::NoAccess;
    new_vma.name = "";
    new_vma.type = VMAType::Reserved;

    *out_addr = std::bit_cast<void*>(mapped_addr);
    return ORBIS_OK;
}

int MemoryManager::MapMemory(void** out_addr, VAddr virtual_addr, size_t size, MemoryProt prot,
                             MemoryMapFlags flags, VMAType type, std::string_view name,
                             bool is_exec, PAddr phys_addr, u64 alignment) {
    std::scoped_lock lk{mutex};
    if (type == VMAType::Flexible && flexible_usage + size > total_flexible_size) {
        return SCE_KERNEL_ERROR_ENOMEM;
    }

    // When virtual addr is zero, force it to virtual_base. The guest cannot pass Fixed
    // flag so we will take the branch that searches for free (or reserved) mappings.
    virtual_addr = (virtual_addr == 0) ? impl.SystemManagedVirtualBase() : virtual_addr;
    alignment = alignment > 0 ? alignment : 16_KB;

    VAddr mapped_addr = alignment > 0 ? Common::AlignUp(virtual_addr, alignment) : virtual_addr;
    SCOPE_EXIT {
        auto& new_vma = AddMapping(mapped_addr, size);
        new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
        new_vma.prot = prot;
        new_vma.name = name;
        new_vma.type = type;

        if (type == VMAType::Direct) {
            new_vma.phys_base = phys_addr;
            MapVulkanMemory(mapped_addr, size);
        }
        if (type == VMAType::Flexible) {
            flexible_usage += size;
        }
    };

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
        // If the VMA is free and contains the requested mapping we are done.
        if (it->second.type == VMAType::Free && it->second.Contains(virtual_addr, size)) {
            mapped_addr = virtual_addr;
        } else {
            // Search for the first free VMA that fits our mapping.
            while (it->second.type != VMAType::Free || it->second.size < size) {
                it++;
            }
            ASSERT(it != vma_map.end());
            const auto& vma = it->second;
            mapped_addr = alignment > 0 ? Common::AlignUp(vma.base, alignment) : vma.base;
        }
    }

    // Perform the mapping.
    *out_addr = impl.Map(mapped_addr, size, alignment, phys_addr, is_exec);
    TRACK_ALLOC(*out_addr, size, "VMEM");
    return ORBIS_OK;
}

int MemoryManager::MapFile(void** out_addr, VAddr virtual_addr, size_t size, MemoryProt prot,
                           MemoryMapFlags flags, uintptr_t fd, size_t offset) {
    if (virtual_addr == 0) {
        virtual_addr = impl.SystemManagedVirtualBase();
    } else {
        LOG_INFO(Kernel_Vmm, "Virtual addr {:#x} with size {:#x}", virtual_addr, size);
    }

    VAddr mapped_addr = 0;
    const size_t size_aligned = Common::AlignUp(size, 16_KB);

    // Find first free area to map the file.
    if (False(flags & MemoryMapFlags::Fixed)) {
        auto it = FindVMA(virtual_addr);
        while (it->second.type != VMAType::Free || it->second.size < size_aligned) {
            it++;
        }
        ASSERT(it != vma_map.end());

        mapped_addr = it->second.base;
    }

    if (True(flags & MemoryMapFlags::Fixed)) {
        const auto& vma = FindVMA(virtual_addr)->second;
        const size_t remaining_size = vma.base + vma.size - virtual_addr;
        ASSERT_MSG((vma.type == VMAType::Free || vma.type == VMAType::Reserved) &&
                   remaining_size >= size);

        mapped_addr = virtual_addr;
    }

    // Map the file.
    impl.MapFile(mapped_addr, size, offset, std::bit_cast<u32>(prot), fd);

    // Add virtual memory area
    auto& new_vma = AddMapping(mapped_addr, size_aligned);
    new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
    new_vma.prot = prot;
    new_vma.name = "File";
    new_vma.fd = fd;
    new_vma.type = VMAType::File;

    *out_addr = std::bit_cast<void*>(mapped_addr);
    return ORBIS_OK;
}

void MemoryManager::UnmapMemory(VAddr virtual_addr, size_t size) {
    std::scoped_lock lk{mutex};

    // TODO: Partial unmaps are technically supported by the guest.
    const auto it = vma_map.find(virtual_addr);
    ASSERT_MSG(it != vma_map.end() && it->first == virtual_addr,
               "Attempting to unmap partially mapped range");

    const auto type = it->second.type;
    const bool has_backing = type == VMAType::Direct || type == VMAType::File;
    if (type == VMAType::Direct) {
        UnmapVulkanMemory(virtual_addr, size);
    }
    if (type == VMAType::Flexible) {
        flexible_usage -= size;
    }

    // Mark region as free and attempt to coalesce it with neighbours.
    auto& vma = it->second;
    vma.type = VMAType::Free;
    vma.prot = MemoryProt::NoAccess;
    vma.phys_base = 0;
    MergeAdjacent(vma_map, it);

    // Unmap the memory region.
    impl.Unmap(virtual_addr, size, has_backing);
    TRACK_FREE(virtual_addr, "VMEM");
}

int MemoryManager::QueryProtection(VAddr addr, void** start, void** end, u32* prot) {
    std::scoped_lock lk{mutex};

    const auto it = FindVMA(addr);
    const auto& vma = it->second;
    ASSERT_MSG(vma.type != VMAType::Free, "Provided address is not mapped");

    if (start != nullptr) {
        *start = reinterpret_cast<void*>(vma.base);
    }
    if (end != nullptr) {
        *end = reinterpret_cast<void*>(vma.base + vma.size);
    }
    if (prot != nullptr) {
        *prot = static_cast<u32>(vma.prot);
    }
    return ORBIS_OK;
}

int MemoryManager::VirtualQuery(VAddr addr, int flags,
                                Libraries::Kernel::OrbisVirtualQueryInfo* info) {
    std::scoped_lock lk{mutex};

    auto it = FindVMA(addr);
    if (it->second.type == VMAType::Free && flags == 1) {
        it++;
    }
    if (it->second.type == VMAType::Free) {
        LOG_WARNING(Kernel_Vmm, "VirtualQuery on free memory region");
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    const auto& vma = it->second;
    info->start = vma.base;
    info->end = vma.base + vma.size;
    info->is_flexible.Assign(vma.type == VMAType::Flexible);
    info->is_direct.Assign(vma.type == VMAType::Direct);
    info->is_commited.Assign(vma.type != VMAType::Free);
    if (vma.type == VMAType::Direct) {
        const auto dmem_it = FindDmemArea(vma.phys_base);
        ASSERT(dmem_it != dmem_map.end());
        info->offset = vma.phys_base;
        info->memory_type = dmem_it->second.memory_type;
    }

    return ORBIS_OK;
}

int MemoryManager::DirectMemoryQuery(PAddr addr, bool find_next,
                                     Libraries::Kernel::OrbisQueryInfo* out_info) {
    std::scoped_lock lk{mutex};

    auto dmem_area = FindDmemArea(addr);
    while (dmem_area != dmem_map.end() && dmem_area->second.is_free && find_next) {
        dmem_area++;
    }

    if (dmem_area == dmem_map.end() || dmem_area->second.is_free) {
        LOG_ERROR(Core, "Unable to find allocated direct memory region to query!");
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    const auto& area = dmem_area->second;
    out_info->start = area.base;
    out_info->end = area.GetEnd();
    out_info->memoryType = area.memory_type;
    return ORBIS_OK;
}

int MemoryManager::DirectQueryAvailable(PAddr search_start, PAddr search_end, size_t alignment,
                                        PAddr* phys_addr_out, size_t* size_out) {
    std::scoped_lock lk{mutex};

    auto dmem_area = FindDmemArea(search_start);
    PAddr paddr{};
    size_t max_size{};
    while (dmem_area != dmem_map.end() && dmem_area->second.GetEnd() <= search_end) {
        if (dmem_area->second.size > max_size) {
            paddr = dmem_area->second.base;
            max_size = dmem_area->second.size;
        }
        dmem_area++;
    }

    *phys_addr_out = alignment > 0 ? Common::AlignUp(paddr, alignment) : paddr;
    *size_out = max_size;
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
    ASSERT_MSG((vma.type == VMAType::Free || vma.type == VMAType::Reserved) &&
                   vma.base <= virtual_addr,
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

DirectMemoryArea& MemoryManager::AddDmemAllocation(PAddr addr, size_t size) {
    auto dmem_handle = FindDmemArea(addr);
    ASSERT_MSG(dmem_handle != dmem_map.end(), "Physical address not in dmem_map");

    const DirectMemoryArea& area = dmem_handle->second;
    ASSERT_MSG(area.is_free && area.base <= addr,
               "Adding an allocation to already allocated region");

    const PAddr start_in_area = addr - area.base;
    const PAddr end_in_vma = start_in_area + size;
    ASSERT_MSG(end_in_vma <= area.size, "Mapping cannot fit inside free region");

    if (end_in_vma != area.size) {
        // Split VMA at the end of the allocated region
        Split(dmem_handle, end_in_vma);
    }
    if (start_in_area != 0) {
        // Split VMA at the start of the allocated region
        dmem_handle = Split(dmem_handle, start_in_area);
    }

    return dmem_handle->second;
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

MemoryManager::DMemHandle MemoryManager::Split(DMemHandle dmem_handle, size_t offset_in_area) {
    auto& old_area = dmem_handle->second;
    ASSERT(offset_in_area < old_area.size && offset_in_area > 0);

    auto new_area = old_area;
    old_area.size = offset_in_area;
    new_area.base += offset_in_area;
    new_area.size -= offset_in_area;

    return dmem_map.emplace_hint(std::next(dmem_handle), new_area.base, new_area);
};

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
