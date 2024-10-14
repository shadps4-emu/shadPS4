// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/config.h"
#include "common/debug.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/memory_management.h"
#include "core/memory.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

namespace Core {

constexpr u64 SCE_DEFAULT_FLEXIBLE_MEMORY_SIZE = 448_MB;

MemoryManager::MemoryManager() {
    // Set up the direct and flexible memory regions.
    SetupMemoryRegions(SCE_DEFAULT_FLEXIBLE_MEMORY_SIZE);

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

void MemoryManager::SetupMemoryRegions(u64 flexible_size) {
    const auto total_size =
        Config::isNeoMode() ? SCE_KERNEL_MAIN_DMEM_SIZE_PRO : SCE_KERNEL_MAIN_DMEM_SIZE;
    total_flexible_size = flexible_size;
    total_direct_size = total_size - flexible_size;

    // Insert an area that covers direct memory physical block.
    // Note that this should never be called after direct memory allocations have been made.
    dmem_map.clear();
    dmem_map.emplace(0, DirectMemoryArea{0, total_direct_size});

    LOG_INFO(Kernel_Vmm, "Configured memory regions: flexible size = {:#x}, direct size = {:#x}",
             total_flexible_size, total_direct_size);
}

PAddr MemoryManager::PoolExpand(PAddr search_start, PAddr search_end, size_t size, u64 alignment) {
    std::scoped_lock lk{mutex};

    auto dmem_area = FindDmemArea(search_start);

    const auto is_suitable = [&] {
        const auto aligned_base = alignment > 0 ? Common::AlignUp(dmem_area->second.base, alignment)
                                                : dmem_area->second.base;
        const auto alignment_size = aligned_base - dmem_area->second.base;
        const auto remaining_size =
            dmem_area->second.size >= alignment_size ? dmem_area->second.size - alignment_size : 0;
        return dmem_area->second.is_free && remaining_size >= size;
    };
    while (!is_suitable() && dmem_area->second.GetEnd() <= search_end) {
        dmem_area++;
    }
    ASSERT_MSG(is_suitable(), "Unable to find free direct memory area: size = {:#x}", size);

    // Align free position
    PAddr free_addr = dmem_area->second.base;
    free_addr = alignment > 0 ? Common::AlignUp(free_addr, alignment) : free_addr;

    // Add the allocated region to the list and commit its pages.
    auto& area = CarveDmemArea(free_addr, size)->second;
    area.is_free = false;
    area.is_pooled = true;
    return free_addr;
}

PAddr MemoryManager::Allocate(PAddr search_start, PAddr search_end, size_t size, u64 alignment,
                              int memory_type) {
    std::scoped_lock lk{mutex};

    auto dmem_area = FindDmemArea(search_start);

    const auto is_suitable = [&] {
        const auto aligned_base = alignment > 0 ? Common::AlignUp(dmem_area->second.base, alignment)
                                                : dmem_area->second.base;
        const auto alignment_size = aligned_base - dmem_area->second.base;
        const auto remaining_size =
            dmem_area->second.size >= alignment_size ? dmem_area->second.size - alignment_size : 0;
        return dmem_area->second.is_free && remaining_size >= size;
    };
    while (!is_suitable() && dmem_area->second.GetEnd() <= search_end) {
        dmem_area++;
    }
    ASSERT_MSG(is_suitable(), "Unable to find free direct memory area: size = {:#x}", size);

    // Align free position
    PAddr free_addr = dmem_area->second.base;
    free_addr = alignment > 0 ? Common::AlignUp(free_addr, alignment) : free_addr;

    // Add the allocated region to the list and commit its pages.
    auto& area = CarveDmemArea(free_addr, size)->second;
    area.memory_type = memory_type;
    area.is_free = false;
    return free_addr;
}

void MemoryManager::Free(PAddr phys_addr, size_t size) {
    std::scoped_lock lk{mutex};

    auto dmem_area = CarveDmemArea(phys_addr, size);
    ASSERT(dmem_area != dmem_map.end() && dmem_area->second.size >= size);

    // Release any dmem mappings that reference this physical block.
    std::vector<std::pair<VAddr, u64>> remove_list;
    for (const auto& [addr, mapping] : vma_map) {
        if (mapping.type != VMAType::Direct) {
            continue;
        }
        if (mapping.phys_base <= phys_addr && phys_addr < mapping.phys_base + mapping.size) {
            auto vma_segment_start_addr = phys_addr - mapping.phys_base + addr;
            LOG_INFO(Kernel_Vmm, "Unmaping direct mapping {:#x} with size {:#x}",
                     vma_segment_start_addr, size);
            // Unmaping might erase from vma_map. We can't do it here.
            remove_list.emplace_back(vma_segment_start_addr, size);
        }
    }
    for (const auto& [addr, size] : remove_list) {
        UnmapMemoryImpl(addr, size);
    }

    // Mark region as free and attempt to coalesce it with neighbours.
    auto& area = dmem_area->second;
    area.is_free = true;
    area.memory_type = 0;
    MergeAdjacent(dmem_map, dmem_area);
}

int MemoryManager::PoolReserve(void** out_addr, VAddr virtual_addr, size_t size,
                               MemoryMapFlags flags, u64 alignment) {
    std::scoped_lock lk{mutex};

    virtual_addr = (virtual_addr == 0) ? impl.SystemManagedVirtualBase() : virtual_addr;
    alignment = alignment > 0 ? alignment : 2_MB;
    VAddr mapped_addr = alignment > 0 ? Common::AlignUp(virtual_addr, alignment) : virtual_addr;

    // Fixed mapping means the virtual address must exactly match the provided one.
    if (True(flags & MemoryMapFlags::Fixed)) {
        const auto& vma = FindVMA(mapped_addr)->second;
        // If the VMA is mapped, unmap the region first.
        if (vma.IsMapped()) {
            UnmapMemoryImpl(mapped_addr, size);
        }
        const size_t remaining_size = vma.base + vma.size - mapped_addr;
        ASSERT_MSG(vma.type == VMAType::Free && remaining_size >= size);
    }

    // Find the first free area starting with provided virtual address.
    if (False(flags & MemoryMapFlags::Fixed)) {
        mapped_addr = SearchFree(mapped_addr, size, alignment);
    }

    // Add virtual memory area
    const auto new_vma_handle = CarveVMA(mapped_addr, size);
    auto& new_vma = new_vma_handle->second;
    new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
    new_vma.prot = MemoryProt::NoAccess;
    new_vma.name = "";
    new_vma.type = VMAType::PoolReserved;
    MergeAdjacent(vma_map, new_vma_handle);

    *out_addr = std::bit_cast<void*>(mapped_addr);
    return ORBIS_OK;
}

int MemoryManager::Reserve(void** out_addr, VAddr virtual_addr, size_t size, MemoryMapFlags flags,
                           u64 alignment) {
    std::scoped_lock lk{mutex};

    virtual_addr = (virtual_addr == 0) ? impl.SystemManagedVirtualBase() : virtual_addr;
    alignment = alignment > 0 ? alignment : 16_KB;
    VAddr mapped_addr = alignment > 0 ? Common::AlignUp(virtual_addr, alignment) : virtual_addr;

    // Fixed mapping means the virtual address must exactly match the provided one.
    if (True(flags & MemoryMapFlags::Fixed)) {
        const auto& vma = FindVMA(mapped_addr)->second;
        // If the VMA is mapped, unmap the region first.
        if (vma.IsMapped()) {
            UnmapMemoryImpl(mapped_addr, size);
        }
        const size_t remaining_size = vma.base + vma.size - mapped_addr;
        ASSERT_MSG(vma.type == VMAType::Free && remaining_size >= size);
    }

    // Find the first free area starting with provided virtual address.
    if (False(flags & MemoryMapFlags::Fixed)) {
        mapped_addr = SearchFree(mapped_addr, size, alignment);
    }

    // Add virtual memory area
    const auto new_vma_handle = CarveVMA(mapped_addr, size);
    auto& new_vma = new_vma_handle->second;
    new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
    new_vma.prot = MemoryProt::NoAccess;
    new_vma.name = "";
    new_vma.type = VMAType::Reserved;
    MergeAdjacent(vma_map, new_vma_handle);

    *out_addr = std::bit_cast<void*>(mapped_addr);
    return ORBIS_OK;
}

int MemoryManager::PoolCommit(VAddr virtual_addr, size_t size, MemoryProt prot) {
    std::scoped_lock lk{mutex};

    const u64 alignment = 64_KB;

    // When virtual addr is zero, force it to virtual_base. The guest cannot pass Fixed
    // flag so we will take the branch that searches for free (or reserved) mappings.
    virtual_addr = (virtual_addr == 0) ? impl.SystemManagedVirtualBase() : virtual_addr;
    VAddr mapped_addr = Common::AlignUp(virtual_addr, alignment);

    // This should return SCE_KERNEL_ERROR_ENOMEM but shouldn't normally happen.
    const auto& vma = FindVMA(mapped_addr)->second;
    const size_t remaining_size = vma.base + vma.size - mapped_addr;
    ASSERT_MSG(!vma.IsMapped() && remaining_size >= size);

    // Perform the mapping.
    void* out_addr = impl.Map(mapped_addr, size, alignment, -1, false);
    TRACK_ALLOC(out_addr, size, "VMEM");

    auto& new_vma = CarveVMA(mapped_addr, size)->second;
    new_vma.disallow_merge = false;
    new_vma.prot = prot;
    new_vma.name = "";
    new_vma.type = Core::VMAType::Pooled;
    new_vma.is_exec = false;
    new_vma.phys_base = 0;

    rasterizer->MapMemory(mapped_addr, size);
    return ORBIS_OK;
}

int MemoryManager::MapMemory(void** out_addr, VAddr virtual_addr, size_t size, MemoryProt prot,
                             MemoryMapFlags flags, VMAType type, std::string_view name,
                             bool is_exec, PAddr phys_addr, u64 alignment) {
    std::scoped_lock lk{mutex};

    // Certain games perform flexible mappings on loop to determine
    // the available flexible memory size. Questionable but we need to handle this.
    if (type == VMAType::Flexible && flexible_usage + size > total_flexible_size) {
        return SCE_KERNEL_ERROR_ENOMEM;
    }

    // When virtual addr is zero, force it to virtual_base. The guest cannot pass Fixed
    // flag so we will take the branch that searches for free (or reserved) mappings.
    virtual_addr = (virtual_addr == 0) ? impl.SystemManagedVirtualBase() : virtual_addr;
    alignment = alignment > 0 ? alignment : 16_KB;
    VAddr mapped_addr = alignment > 0 ? Common::AlignUp(virtual_addr, alignment) : virtual_addr;

    // Fixed mapping means the virtual address must exactly match the provided one.
    if (True(flags & MemoryMapFlags::Fixed)) {
        // This should return SCE_KERNEL_ERROR_ENOMEM but shouldn't normally happen.
        const auto& vma = FindVMA(mapped_addr)->second;
        const size_t remaining_size = vma.base + vma.size - mapped_addr;
        ASSERT_MSG(!vma.IsMapped() && remaining_size >= size);
    }

    // Find the first free area starting with provided virtual address.
    if (False(flags & MemoryMapFlags::Fixed)) {
        mapped_addr = SearchFree(mapped_addr, size, alignment);
    }

    // Perform the mapping.
    *out_addr = impl.Map(mapped_addr, size, alignment, phys_addr, is_exec);
    TRACK_ALLOC(*out_addr, size, "VMEM");

    auto& new_vma = CarveVMA(mapped_addr, size)->second;
    new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
    new_vma.prot = prot;
    new_vma.name = name;
    new_vma.type = type;
    new_vma.is_exec = is_exec;

    if (type == VMAType::Direct) {
        new_vma.phys_base = phys_addr;
        rasterizer->MapMemory(mapped_addr, size);
    }
    if (type == VMAType::Flexible) {
        flexible_usage += size;
    }

    return ORBIS_OK;
}

int MemoryManager::MapFile(void** out_addr, VAddr virtual_addr, size_t size, MemoryProt prot,
                           MemoryMapFlags flags, uintptr_t fd, size_t offset) {
    VAddr mapped_addr = (virtual_addr == 0) ? impl.SystemManagedVirtualBase() : virtual_addr;
    const size_t size_aligned = Common::AlignUp(size, 16_KB);

    // Find first free area to map the file.
    if (False(flags & MemoryMapFlags::Fixed)) {
        mapped_addr = SearchFree(mapped_addr, size_aligned, 1);
    }

    if (True(flags & MemoryMapFlags::Fixed)) {
        const auto& vma = FindVMA(virtual_addr)->second;
        const size_t remaining_size = vma.base + vma.size - virtual_addr;
        ASSERT_MSG(!vma.IsMapped() && remaining_size >= size);
    }

    // Map the file.
    impl.MapFile(mapped_addr, size, offset, std::bit_cast<u32>(prot), fd);

    // Add virtual memory area
    auto& new_vma = CarveVMA(mapped_addr, size_aligned)->second;
    new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
    new_vma.prot = prot;
    new_vma.name = "File";
    new_vma.fd = fd;
    new_vma.type = VMAType::File;

    *out_addr = std::bit_cast<void*>(mapped_addr);
    return ORBIS_OK;
}

void MemoryManager::PoolDecommit(VAddr virtual_addr, size_t size) {
    std::scoped_lock lk{mutex};

    const auto it = FindVMA(virtual_addr);
    const auto& vma_base = it->second;
    ASSERT_MSG(vma_base.Contains(virtual_addr, size),
               "Existing mapping does not contain requested unmap range");

    const auto vma_base_addr = vma_base.base;
    const auto vma_base_size = vma_base.size;
    const auto phys_base = vma_base.phys_base;
    const bool is_exec = vma_base.is_exec;
    const auto start_in_vma = virtual_addr - vma_base_addr;
    const auto type = vma_base.type;

    rasterizer->UnmapMemory(virtual_addr, size);

    // Mark region as free and attempt to coalesce it with neighbours.
    const auto new_it = CarveVMA(virtual_addr, size);
    auto& vma = new_it->second;
    vma.type = VMAType::PoolReserved;
    vma.prot = MemoryProt::NoAccess;
    vma.phys_base = 0;
    vma.disallow_merge = false;
    vma.name = "";
    MergeAdjacent(vma_map, new_it);

    // Unmap the memory region.
    impl.Unmap(vma_base_addr, vma_base_size, start_in_vma, start_in_vma + size, phys_base, is_exec,
               false, false);
    TRACK_FREE(virtual_addr, "VMEM");
}

void MemoryManager::UnmapMemory(VAddr virtual_addr, size_t size) {
    std::scoped_lock lk{mutex};
    UnmapMemoryImpl(virtual_addr, size);
}

void MemoryManager::UnmapMemoryImpl(VAddr virtual_addr, size_t size) {
    const auto it = FindVMA(virtual_addr);
    const auto& vma_base = it->second;
    ASSERT_MSG(vma_base.Contains(virtual_addr, size),
               "Existing mapping does not contain requested unmap range");

    const auto vma_base_addr = vma_base.base;
    const auto vma_base_size = vma_base.size;
    const auto phys_base = vma_base.phys_base;
    const bool is_exec = vma_base.is_exec;
    const auto start_in_vma = virtual_addr - vma_base_addr;
    const auto type = vma_base.type;
    const bool has_backing = type == VMAType::Direct || type == VMAType::File;
    if (type == VMAType::Direct) {
        rasterizer->UnmapMemory(virtual_addr, size);
    }
    if (type == VMAType::Flexible) {
        flexible_usage -= size;
    }

    // Mark region as free and attempt to coalesce it with neighbours.
    const auto new_it = CarveVMA(virtual_addr, size);
    auto& vma = new_it->second;
    vma.type = VMAType::Free;
    vma.prot = MemoryProt::NoAccess;
    vma.phys_base = 0;
    vma.disallow_merge = false;
    vma.name = "";
    MergeAdjacent(vma_map, new_it);
    bool readonly_file = vma.prot == MemoryProt::CpuRead && type == VMAType::File;

    // Unmap the memory region.
    impl.Unmap(vma_base_addr, vma_base_size, start_in_vma, start_in_vma + size, phys_base, is_exec,
               has_backing, readonly_file);
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

int MemoryManager::Protect(VAddr addr, size_t size, MemoryProt prot) {
    std::scoped_lock lk{mutex};

    // Find the virtual memory area that contains the specified address range.
    auto it = FindVMA(addr);
    if (it == vma_map.end() || !it->second.Contains(addr, size)) {
        LOG_ERROR(Core, "Address range not mapped");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    VirtualMemoryArea& vma = it->second;
    if (vma.type == VMAType::Free) {
        LOG_ERROR(Core, "Cannot change protection on free memory region");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    // Validate protection flags
    constexpr static MemoryProt valid_flags = MemoryProt::NoAccess | MemoryProt::CpuRead |
                                              MemoryProt::CpuReadWrite | MemoryProt::GpuRead |
                                              MemoryProt::GpuWrite | MemoryProt::GpuReadWrite;

    MemoryProt invalid_flags = prot & ~valid_flags;
    if (u32(invalid_flags) != 0 && u32(invalid_flags) != u32(MemoryProt::NoAccess)) {
        LOG_ERROR(Core, "Invalid protection flags: prot = {:#x}, invalid flags = {:#x}", u32(prot),
                  u32(invalid_flags));
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    // Change protection
    vma.prot = prot;

    // Set permissions
    Core::MemoryPermission perms{};

    if (True(prot & MemoryProt::CpuRead)) {
        perms |= Core::MemoryPermission::Read;
    }
    if (True(prot & MemoryProt::CpuReadWrite)) {
        perms |= Core::MemoryPermission::ReadWrite;
    }
    if (True(prot & MemoryProt::GpuRead)) {
        perms |= Core::MemoryPermission::Read;
    }
    if (True(prot & MemoryProt::GpuWrite)) {
        perms |= Core::MemoryPermission::Write;
    }
    if (True(prot & MemoryProt::GpuReadWrite)) {
        perms |= Core::MemoryPermission::ReadWrite;
    }

    impl.Protect(addr, size, perms);

    return ORBIS_OK;
}

int MemoryManager::VirtualQuery(VAddr addr, int flags,
                                ::Libraries::Kernel::OrbisVirtualQueryInfo* info) {
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
    info->offset = vma.phys_base;
    info->protection = static_cast<s32>(vma.prot);
    info->is_flexible.Assign(vma.type == VMAType::Flexible);
    info->is_direct.Assign(vma.type == VMAType::Direct);
    info->is_stack.Assign(vma.type == VMAType::Stack);
    info->is_pooled.Assign(vma.type == VMAType::Pooled);
    info->is_committed.Assign(vma.type != VMAType::Free && vma.type != VMAType::Reserved);
    vma.name.copy(info->name.data(), std::min(info->name.size(), vma.name.size()));
    if (vma.type == VMAType::Direct) {
        const auto dmem_it = FindDmemArea(vma.phys_base);
        ASSERT(dmem_it != dmem_map.end());
        info->memory_type = dmem_it->second.memory_type;
    } else {
        info->memory_type = ::Libraries::Kernel::SCE_KERNEL_WB_ONION;
    }

    return ORBIS_OK;
}

int MemoryManager::DirectMemoryQuery(PAddr addr, bool find_next,
                                     ::Libraries::Kernel::OrbisQueryInfo* out_info) {
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
        if (!dmem_area->second.is_free) {
            dmem_area++;
            continue;
        }

        const auto aligned_base = alignment > 0 ? Common::AlignUp(dmem_area->second.base, alignment)
                                                : dmem_area->second.base;
        const auto alignment_size = aligned_base - dmem_area->second.base;
        const auto remaining_size =
            dmem_area->second.size >= alignment_size ? dmem_area->second.size - alignment_size : 0;
        if (remaining_size > max_size) {
            paddr = aligned_base;
            max_size = remaining_size;
        }
        dmem_area++;
    }

    *phys_addr_out = paddr;
    *size_out = max_size;
    return ORBIS_OK;
}

void MemoryManager::NameVirtualRange(VAddr virtual_addr, size_t size, std::string_view name) {
    auto it = FindVMA(virtual_addr);

    ASSERT_MSG(it->second.Contains(virtual_addr, size),
               "Range provided is not fully contained in vma");
    it->second.name = name;
}
VAddr MemoryManager::SearchFree(VAddr virtual_addr, size_t size, u32 alignment) {
    // If the requested address is below the mapped range, start search from the lowest address
    auto min_search_address = impl.SystemManagedVirtualBase();
    if (virtual_addr < min_search_address) {
        virtual_addr = min_search_address;
    }

    auto it = FindVMA(virtual_addr);
    ASSERT_MSG(it != vma_map.end(), "Specified mapping address was not found!");

    // If the VMA is free and contains the requested mapping we are done.
    if (it->second.IsFree() && it->second.Contains(virtual_addr, size)) {
        return virtual_addr;
    }
    // Search for the first free VMA that fits our mapping.
    const auto is_suitable = [&] {
        if (!it->second.IsFree()) {
            return false;
        }
        const auto& vma = it->second;
        virtual_addr = Common::AlignUp(vma.base, alignment);
        // Sometimes the alignment itself might be larger than the VMA.
        if (virtual_addr > vma.base + vma.size) {
            return false;
        }
        const size_t remaining_size = vma.base + vma.size - virtual_addr;
        return remaining_size >= size;
    };
    while (!is_suitable()) {
        it++;
    }
    return virtual_addr;
}

MemoryManager::VMAHandle MemoryManager::CarveVMA(VAddr virtual_addr, size_t size) {
    auto vma_handle = FindVMA(virtual_addr);
    ASSERT_MSG(vma_handle != vma_map.end(), "Virtual address not in vm_map");

    const VirtualMemoryArea& vma = vma_handle->second;
    ASSERT_MSG(vma.base <= virtual_addr, "Adding a mapping to already mapped region");

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

    return vma_handle;
}

MemoryManager::DMemHandle MemoryManager::CarveDmemArea(PAddr addr, size_t size) {
    auto dmem_handle = FindDmemArea(addr);
    ASSERT_MSG(dmem_handle != dmem_map.end(), "Physical address not in dmem_map");

    const DirectMemoryArea& area = dmem_handle->second;
    ASSERT_MSG(area.base <= addr, "Adding an allocation to already allocated region");

    const PAddr start_in_area = addr - area.base;
    const PAddr end_in_vma = start_in_area + size;
    ASSERT_MSG(end_in_vma <= area.size, "Mapping cannot fit inside free region: size = {:#x}",
               size);

    if (end_in_vma != area.size) {
        // Split VMA at the end of the allocated region
        Split(dmem_handle, end_in_vma);
    }
    if (start_in_area != 0) {
        // Split VMA at the start of the allocated region
        dmem_handle = Split(dmem_handle, start_in_area);
    }

    return dmem_handle;
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

int MemoryManager::GetDirectMemoryType(PAddr addr, int* directMemoryTypeOut,
                                       void** directMemoryStartOut, void** directMemoryEndOut) {
    std::scoped_lock lk{mutex};

    auto dmem_area = FindDmemArea(addr);

    if (dmem_area == dmem_map.end() || dmem_area->second.is_free) {
        LOG_ERROR(Core, "Unable to find allocated direct memory region to check type!");
        return ORBIS_KERNEL_ERROR_ENOENT;
    }

    const auto& area = dmem_area->second;
    *directMemoryStartOut = reinterpret_cast<void*>(area.base);
    *directMemoryEndOut = reinterpret_cast<void*>(area.GetEnd());
    *directMemoryTypeOut = area.memory_type;
    return ORBIS_OK;
}

} // namespace Core
