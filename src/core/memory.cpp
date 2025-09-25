// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/config.h"
#include "common/debug.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/process.h"
#include "core/memory.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

namespace Core {

MemoryManager::MemoryManager() {
    // Insert a virtual memory area that covers the entire area we manage.
    const VAddr system_managed_base = impl.SystemManagedVirtualBase();
    const u64 system_managed_size = impl.SystemManagedVirtualSize();
    const VAddr system_reserved_base = impl.SystemReservedVirtualBase();
    const u64 system_reserved_size = impl.SystemReservedVirtualSize();
    const VAddr user_base = impl.UserVirtualBase();
    const u64 user_size = impl.UserVirtualSize();
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

void MemoryManager::SetupMemoryRegions(u64 flexible_size, bool use_extended_mem1,
                                       bool use_extended_mem2) {
    const bool is_neo = ::Libraries::Kernel::sceKernelIsNeoMode();
    auto total_size = is_neo ? ORBIS_KERNEL_TOTAL_MEM_PRO : ORBIS_KERNEL_TOTAL_MEM;
    if (Config::isDevKitConsole()) {
        total_size = is_neo ? ORBIS_KERNEL_TOTAL_MEM_DEV_PRO : ORBIS_KERNEL_TOTAL_MEM_DEV;
    }
    if (!use_extended_mem1 && is_neo) {
        total_size -= 256_MB;
    }
    if (!use_extended_mem2 && !is_neo) {
        total_size -= 128_MB;
    }
    total_flexible_size = flexible_size - ORBIS_FLEXIBLE_MEMORY_BASE;
    total_direct_size = total_size - flexible_size;

    // Insert an area that covers the direct memory physical address block.
    // Note that this should never be called after direct memory allocations have been made.
    dmem_map.clear();
    dmem_map.emplace(0, DirectMemoryArea{0, total_direct_size});

    // Insert an area that covers the flexible memory physical address block.
    // Note that this should never be called after flexible memory allocations have been made.
    const auto remaining_physical_space = ORBIS_KERNEL_TOTAL_MEM_DEV_PRO - total_direct_size;
    fmem_map.clear();
    fmem_map.emplace(total_direct_size,
                     FlexibleMemoryArea{total_direct_size, remaining_physical_space});

    LOG_INFO(Kernel_Vmm, "Configured memory regions: flexible size = {:#x}, direct size = {:#x}",
             total_flexible_size, total_direct_size);
}

u64 MemoryManager::ClampRangeSize(VAddr virtual_addr, u64 size) {
    static constexpr u64 MinSizeToClamp = 3_GB;
    // Dont bother with clamping if the size is small so we dont pay a map lookup on every buffer.
    if (size < MinSizeToClamp) {
        return size;
    }

    ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(virtual_addr)),
               "Attempted to access invalid address {:#x}", virtual_addr);

    // Clamp size to the remaining size of the current VMA.
    auto vma = FindVMA(virtual_addr);
    u64 clamped_size = vma->second.base + vma->second.size - virtual_addr;
    ++vma;

    // Keep adding to the size while there is contigious virtual address space.
    while (vma != vma_map.end() && vma->second.IsMapped() && clamped_size < size) {
        clamped_size += vma->second.size;
        ++vma;
    }
    clamped_size = std::min(clamped_size, size);

    if (size != clamped_size) {
        LOG_WARNING(Kernel_Vmm, "Clamped requested buffer range addr={:#x}, size={:#x} to {:#x}",
                    virtual_addr, size, clamped_size);
    }
    return clamped_size;
}

void MemoryManager::SetPrtArea(u32 id, VAddr address, u64 size) {
    PrtArea& area = prt_areas[id];
    if (area.mapped) {
        rasterizer->UnmapMemory(area.start, area.end - area.start);
    }

    area.start = address;
    area.end = address + size;
    area.mapped = true;

    // Pretend the entire PRT area is mapped to avoid GPU tracking errors.
    // The caches will use CopySparseMemory to fetch data which avoids unmapped areas.
    rasterizer->MapMemory(address, size);
}

void MemoryManager::CopySparseMemory(VAddr virtual_addr, u8* dest, u64 size) {
    ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(virtual_addr)),
               "Attempted to access invalid address {:#x}", virtual_addr);

    auto vma = FindVMA(virtual_addr);
    while (size) {
        u64 copy_size = std::min<u64>(vma->second.size - (virtual_addr - vma->first), size);
        if (vma->second.IsMapped()) {
            std::memcpy(dest, std::bit_cast<const u8*>(virtual_addr), copy_size);
        } else {
            std::memset(dest, 0, copy_size);
        }
        size -= copy_size;
        virtual_addr += copy_size;
        dest += copy_size;
        ++vma;
    }
}

bool MemoryManager::TryWriteBacking(void* address, const void* data, u32 num_bytes) {
    ASSERT_MSG(IsValidAddress(address), "Attempted to access invalid address {}",
               fmt::ptr(address));
    const VAddr virtual_addr = std::bit_cast<VAddr>(address);
    const auto& vma = FindVMA(virtual_addr)->second;
    if (!HasPhysicalBacking(vma)) {
        return false;
    }
    u8* backing = impl.BackingBase() + vma.phys_base + (virtual_addr - vma.base);
    memcpy(backing, data, num_bytes);
    return true;
}

PAddr MemoryManager::PoolExpand(PAddr search_start, PAddr search_end, u64 size, u64 alignment) {
    std::scoped_lock lk{mutex};
    alignment = alignment > 0 ? alignment : 64_KB;

    auto dmem_area = FindDmemArea(search_start);
    auto mapping_start = search_start > dmem_area->second.base
                             ? Common::AlignUp(search_start, alignment)
                             : Common::AlignUp(dmem_area->second.base, alignment);
    auto mapping_end = mapping_start + size;

    // Find the first free, large enough dmem area in the range.
    while (dmem_area->second.dma_type != DMAType::Free ||
           dmem_area->second.GetEnd() < mapping_end) {
        // The current dmem_area isn't suitable, move to the next one.
        dmem_area++;
        if (dmem_area == dmem_map.end()) {
            break;
        }

        // Update local variables based on the new dmem_area
        mapping_start = Common::AlignUp(dmem_area->second.base, alignment);
        mapping_end = mapping_start + size;
    }

    if (dmem_area == dmem_map.end()) {
        // There are no suitable mappings in this range
        LOG_ERROR(Kernel_Vmm, "Unable to find free direct memory area: size = {:#x}", size);
        return -1;
    }

    // Add the allocated region to the list and commit its pages.
    auto& area = CarveDmemArea(mapping_start, size)->second;
    area.dma_type = DMAType::Pooled;
    area.memory_type = 3;

    // Track how much dmem was allocated for pools.
    pool_budget += size;

    return mapping_start;
}

PAddr MemoryManager::Allocate(PAddr search_start, PAddr search_end, u64 size, u64 alignment,
                              s32 memory_type) {
    std::scoped_lock lk{mutex};
    alignment = alignment > 0 ? alignment : 16_KB;

    auto dmem_area = FindDmemArea(search_start);
    auto mapping_start = search_start > dmem_area->second.base
                             ? Common::AlignUp(search_start, alignment)
                             : Common::AlignUp(dmem_area->second.base, alignment);
    auto mapping_end = mapping_start + size;

    // Find the first free, large enough dmem area in the range.
    while (dmem_area->second.dma_type != DMAType::Free ||
           dmem_area->second.GetEnd() < mapping_end) {
        // The current dmem_area isn't suitable, move to the next one.
        dmem_area++;
        if (dmem_area == dmem_map.end()) {
            break;
        }

        // Update local variables based on the new dmem_area
        mapping_start = Common::AlignUp(dmem_area->second.base, alignment);
        mapping_end = mapping_start + size;
    }

    if (dmem_area == dmem_map.end()) {
        // There are no suitable mappings in this range
        LOG_ERROR(Kernel_Vmm, "Unable to find free direct memory area: size = {:#x}", size);
        return -1;
    }

    // Add the allocated region to the list and commit its pages.
    auto& area = CarveDmemArea(mapping_start, size)->second;
    area.memory_type = memory_type;
    area.dma_type = DMAType::Allocated;
    MergeAdjacent(dmem_map, dmem_area);
    return mapping_start;
}

void MemoryManager::Free(PAddr phys_addr, u64 size) {
    std::scoped_lock lk{mutex};

    auto dmem_area = CarveDmemArea(phys_addr, size);

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
    area.dma_type = DMAType::Free;
    area.memory_type = 0;
    MergeAdjacent(dmem_map, dmem_area);
}

s32 MemoryManager::PoolCommit(VAddr virtual_addr, u64 size, MemoryProt prot, s32 mtype) {
    ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(virtual_addr)),
               "Attempted to access invalid address {:#x}", virtual_addr);
    std::scoped_lock lk{mutex};

    // Input addresses to PoolCommit are treated as fixed, and have a constant alignment.
    const u64 alignment = 64_KB;
    VAddr mapped_addr = Common::AlignUp(virtual_addr, alignment);

    auto& vma = FindVMA(mapped_addr)->second;
    if (vma.type != VMAType::PoolReserved) {
        // If we're attempting to commit non-pooled memory, return EINVAL
        LOG_ERROR(Kernel_Vmm, "Attempting to commit non-pooled memory at {:#x}", mapped_addr);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (!vma.Contains(mapped_addr, size)) {
        // If there's not enough space to commit, return EINVAL
        LOG_ERROR(Kernel_Vmm,
                  "Pooled region {:#x} to {:#x} is not large enough to commit from {:#x} to {:#x}",
                  vma.base, vma.base + vma.size, mapped_addr, mapped_addr + size);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (pool_budget <= size) {
        // If there isn't enough pooled memory to perform the mapping, return ENOMEM
        LOG_ERROR(Kernel_Vmm, "Not enough pooled memory to perform mapping");
        return ORBIS_KERNEL_ERROR_ENOMEM;
    } else {
        // Track how much pooled memory this commit will take
        pool_budget -= size;
    }

    // Carve out the new VMA representing this mapping
    const auto new_vma_handle = CarveVMA(mapped_addr, size);
    auto& new_vma = new_vma_handle->second;
    new_vma.disallow_merge = false;
    new_vma.prot = prot;
    new_vma.name = "anon";
    new_vma.type = Core::VMAType::Pooled;
    new_vma.is_exec = false;

    // Find a suitable physical address
    auto handle = dmem_map.begin();
    while (handle != dmem_map.end() &&
           (handle->second.dma_type != Core::DMAType::Pooled || handle->second.size < size)) {
        handle++;
    }
    ASSERT_MSG(handle != dmem_map.end() && handle->second.dma_type == Core::DMAType::Pooled,
               "No suitable physical memory areas to map");

    // Use the start of this area as the physical backing for this mapping.
    const auto new_dmem_handle = CarveDmemArea(handle->second.base, size);
    auto& new_dmem_area = new_dmem_handle->second;
    new_dmem_area.dma_type = DMAType::Committed;
    new_dmem_area.memory_type = mtype;
    new_vma.phys_base = new_dmem_area.base;
    MergeAdjacent(dmem_map, new_dmem_handle);

    // Perform the mapping
    void* out_addr = impl.Map(mapped_addr, size, alignment, new_vma.phys_base, false);
    TRACK_ALLOC(out_addr, size, "VMEM");

    if (IsValidGpuMapping(mapped_addr, size)) {
        rasterizer->MapMemory(mapped_addr, size);
    }

    return ORBIS_OK;
}

s32 MemoryManager::MapMemory(void** out_addr, VAddr virtual_addr, u64 size, MemoryProt prot,
                             MemoryMapFlags flags, VMAType type, std::string_view name,
                             bool validate_dmem, PAddr phys_addr, u64 alignment) {
    // Certain games perform flexible mappings on loop to determine
    // the available flexible memory size. Questionable but we need to handle this.
    if (type == VMAType::Flexible && flexible_usage + size > total_flexible_size) {
        LOG_ERROR(Kernel_Vmm,
                  "Out of flexible memory, available flexible memory = {:#x}"
                  " requested size = {:#x}",
                  total_flexible_size - flexible_usage, size);
        return ORBIS_KERNEL_ERROR_ENOMEM;
    }

    std::scoped_lock lk{mutex};

    // Validate the requested physical address range
    if (phys_addr != -1) {
        if (total_direct_size < phys_addr + size) {
            LOG_ERROR(Kernel_Vmm, "Unable to map {:#x} bytes at physical address {:#x}", size,
                      phys_addr);
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }

        // Validate direct memory areas involved in this call.
        auto dmem_area = FindDmemArea(phys_addr);
        while (dmem_area != dmem_map.end() && dmem_area->second.base < phys_addr + size) {
            // If any requested dmem area is not allocated, return an error.
            if (dmem_area->second.dma_type != DMAType::Allocated &&
                dmem_area->second.dma_type != DMAType::Mapped) {
                LOG_ERROR(Kernel_Vmm, "Unable to map {:#x} bytes at physical address {:#x}", size,
                          phys_addr);
                return ORBIS_KERNEL_ERROR_ENOMEM;
            }

            // If we need to perform extra validation, then check for Mapped dmem areas too.
            if (validate_dmem && dmem_area->second.dma_type == DMAType::Mapped) {
                LOG_ERROR(Kernel_Vmm, "Unable to map {:#x} bytes at physical address {:#x}", size,
                          phys_addr);
                return ORBIS_KERNEL_ERROR_EBUSY;
            }

            dmem_area++;
        }

        // If the prior loop succeeds, we need to loop through again and carve out mapped dmas.
        // This needs to be a separate loop to avoid modifying dmem map during failed calls.
        auto phys_addr_to_search = phys_addr;
        auto remaining_size = size;
        dmem_area = FindDmemArea(phys_addr);
        while (dmem_area != dmem_map.end() && remaining_size > 0) {
            // Carve a new dmem area in place of this one with the appropriate type.
            // Ensure the carved area only covers the current dmem area.
            const auto start_phys_addr =
                phys_addr > dmem_area->second.base ? phys_addr : dmem_area->second.base;
            const auto offset_in_dma = start_phys_addr - dmem_area->second.base;
            const auto size_in_dma = dmem_area->second.size - offset_in_dma > remaining_size
                                         ? remaining_size
                                         : dmem_area->second.size - offset_in_dma;
            const auto dmem_handle = CarveDmemArea(start_phys_addr, size_in_dma);
            auto& new_dmem_area = dmem_handle->second;
            new_dmem_area.dma_type = DMAType::Mapped;

            // Merge the new dmem_area with dmem_map
            MergeAdjacent(dmem_map, dmem_handle);

            // Get the next relevant dmem area.
            phys_addr_to_search = phys_addr + size_in_dma;
            remaining_size -= size_in_dma;
            dmem_area = FindDmemArea(phys_addr_to_search);
        }
    }

    // Limit the minimum address to SystemManagedVirtualBase to prevent hardware-specific issues.
    VAddr mapped_addr = (virtual_addr == 0) ? impl.SystemManagedVirtualBase() : virtual_addr;

    // Fixed mapping means the virtual address must exactly match the provided one.
    // On a PS4, the Fixed flag is ignored if address 0 is provided.
    if (True(flags & MemoryMapFlags::Fixed) && virtual_addr != 0) {
        ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(mapped_addr)),
                   "Attempted to access invalid address {:#x}", mapped_addr);
        auto vma = FindVMA(mapped_addr)->second;
        // There's a possible edge case where we're mapping to a partially reserved range.
        // To account for this, unmap any reserved areas within this mapping range first.
        auto unmap_addr = mapped_addr;
        auto unmap_size = size;

        // If flag NoOverwrite is provided, don't overwrite mapped VMAs.
        // When it isn't provided, VMAs can be overwritten regardless of if they're mapped.
        while ((False(flags & MemoryMapFlags::NoOverwrite) || !vma.IsMapped()) &&
               unmap_addr < mapped_addr + size) {
            auto unmapped = UnmapBytesFromEntry(unmap_addr, vma, unmap_size);
            unmap_addr += unmapped;
            unmap_size -= unmapped;
            vma = FindVMA(unmap_addr)->second;
        }

        vma = FindVMA(mapped_addr)->second;
        auto remaining_size = vma.base + vma.size - mapped_addr;
        if (vma.IsMapped() || remaining_size < size) {
            LOG_ERROR(Kernel_Vmm, "Unable to map {:#x} bytes at address {:#x}", size, mapped_addr);
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }
    } else {
        // When MemoryMapFlags::Fixed is not specified, and mapped_addr is 0,
        // search from address 0x200000000 instead.
        alignment = alignment > 0 ? alignment : 16_KB;
        mapped_addr = virtual_addr == 0 ? 0x200000000 : mapped_addr;
        mapped_addr = SearchFree(mapped_addr, size, alignment);
        if (mapped_addr == -1) {
            // No suitable memory areas to map to
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }
    }

    // Create a memory area representing this mapping.
    const auto new_vma_handle = CarveVMA(mapped_addr, size);
    auto& new_vma = new_vma_handle->second;

    // If type is Flexible, we need to track how much flexible memory is used here.
    // We also need to determine a reasonable physical base to perform this mapping at.
    if (type == VMAType::Flexible) {
        flexible_usage += size;

        // Find a suitable physical address
        auto handle = fmem_map.begin();
        while (handle != fmem_map.end() &&
               (!handle->second.is_free || handle->second.size < size)) {
            handle++;
        }

        // Some games will end up fragmenting the flexible address space.
        ASSERT_MSG(handle != fmem_map.end() && handle->second.is_free,
                   "No suitable physical memory areas to map");

        // We'll use the start of this area as the physical backing for this mapping.
        const auto new_fmem_handle = CarveFmemArea(handle->second.base, size);
        auto& new_fmem_area = new_fmem_handle->second;
        new_fmem_area.is_free = false;
        phys_addr = new_fmem_area.base;
        MergeAdjacent(fmem_map, new_fmem_handle);
    }

    const bool is_exec = True(prot & MemoryProt::CpuExec);

    new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
    new_vma.prot = prot;
    new_vma.name = name;
    new_vma.type = type;
    new_vma.phys_base = phys_addr == -1 ? 0 : phys_addr;
    new_vma.is_exec = is_exec;

    if (type == VMAType::Reserved) {
        // Technically this should be done for direct and flexible mappings too,
        // But some Windows-specific limitations make that hard to accomplish.
        MergeAdjacent(vma_map, new_vma_handle);
    }

    if (type == VMAType::Reserved || type == VMAType::PoolReserved) {
        // For Reserved/PoolReserved mappings, we don't perform any address space allocations.
        // Just set out_addr to mapped_addr instead.
        *out_addr = std::bit_cast<void*>(mapped_addr);
    } else {
        // If this is not a reservation, then map to GPU and address space
        if (IsValidGpuMapping(mapped_addr, size)) {
            rasterizer->MapMemory(mapped_addr, size);
        }
        *out_addr = impl.Map(mapped_addr, size, alignment, phys_addr, is_exec);

        TRACK_ALLOC(*out_addr, size, "VMEM");
    }

    return ORBIS_OK;
}

s32 MemoryManager::MapFile(void** out_addr, VAddr virtual_addr, u64 size, MemoryProt prot,
                           MemoryMapFlags flags, s32 fd, s64 phys_addr) {
    VAddr mapped_addr = (virtual_addr == 0) ? impl.SystemManagedVirtualBase() : virtual_addr;
    ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(mapped_addr)),
               "Attempted to access invalid address {:#x}", mapped_addr);

    std::scoped_lock lk{mutex};
    const u64 size_aligned = Common::AlignUp(size, 16_KB);

    // Find first free area to map the file.
    if (False(flags & MemoryMapFlags::Fixed)) {
        mapped_addr = SearchFree(mapped_addr, size_aligned, 1);
        if (mapped_addr == -1) {
            // No suitable memory areas to map to
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }
    }

    if (True(flags & MemoryMapFlags::Fixed)) {
        const auto& vma = FindVMA(mapped_addr)->second;
        const u64 remaining_size = vma.base + vma.size - virtual_addr;
        ASSERT_MSG(!vma.IsMapped() && remaining_size >= size,
                   "Memory region {:#x} to {:#x} isn't free enough to map region {:#x} to {:#x}",
                   vma.base, vma.base + vma.size, virtual_addr, virtual_addr + size);
    }

    // Get the file to map
    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto file = h->GetFile(fd);
    if (file == nullptr) {
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    const auto handle = file->f.GetFileMapping();

    impl.MapFile(mapped_addr, size_aligned, phys_addr, std::bit_cast<u32>(prot), handle);

    if (prot >= MemoryProt::GpuRead) {
        ASSERT_MSG(false, "Files cannot be mapped to GPU memory");
    }

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

s32 MemoryManager::PoolDecommit(VAddr virtual_addr, u64 size) {
    ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(virtual_addr)),
               "Attempted to access invalid address {:#x}", virtual_addr);
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

    if (type != VMAType::PoolReserved && type != VMAType::Pooled) {
        LOG_ERROR(Kernel_Vmm, "Attempting to decommit non-pooled memory!");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (type == VMAType::Pooled) {
        // We always map PoolCommitted memory to GPU, so unmap when decomitting.
        if (IsValidGpuMapping(virtual_addr, size)) {
            rasterizer->UnmapMemory(virtual_addr, size);
        }

        // Track how much pooled memory is decommitted
        pool_budget += size;

        // Re-pool the direct memory used by this mapping
        const auto unmap_phys_base = phys_base + start_in_vma;
        const auto new_dmem_handle = CarveDmemArea(unmap_phys_base, size);
        auto& new_dmem_area = new_dmem_handle->second;
        new_dmem_area.dma_type = DMAType::Pooled;

        // Coalesce with nearby direct memory areas.
        MergeAdjacent(dmem_map, new_dmem_handle);
    }

    // Mark region as pool reserved and attempt to coalesce it with neighbours.
    const auto new_it = CarveVMA(virtual_addr, size);
    auto& vma = new_it->second;
    vma.type = VMAType::PoolReserved;
    vma.prot = MemoryProt::NoAccess;
    vma.phys_base = 0;
    vma.disallow_merge = false;
    vma.name = "anon";
    MergeAdjacent(vma_map, new_it);

    if (type != VMAType::PoolReserved) {
        // Unmap the memory region.
        impl.Unmap(vma_base_addr, vma_base_size, start_in_vma, start_in_vma + size, phys_base,
                   is_exec, true, false);
        TRACK_FREE(virtual_addr, "VMEM");
    }

    return ORBIS_OK;
}

s32 MemoryManager::UnmapMemory(VAddr virtual_addr, u64 size) {
    std::scoped_lock lk{mutex};
    return UnmapMemoryImpl(virtual_addr, size);
}

u64 MemoryManager::UnmapBytesFromEntry(VAddr virtual_addr, VirtualMemoryArea vma_base, u64 size) {
    const auto vma_base_addr = vma_base.base;
    const auto vma_base_size = vma_base.size;
    const auto type = vma_base.type;
    const auto phys_base = vma_base.phys_base;
    const bool is_exec = vma_base.is_exec;
    const auto start_in_vma = virtual_addr - vma_base_addr;
    const auto adjusted_size =
        vma_base_size - start_in_vma < size ? vma_base_size - start_in_vma : size;
    const bool has_backing = HasPhysicalBacking(vma_base) || type == VMAType::File;
    const auto prot = vma_base.prot;
    const bool readonly_file = prot == MemoryProt::CpuRead && type == VMAType::File;

    if (type == VMAType::Free) {
        return adjusted_size;
    }

    if (type == VMAType::Direct) {
        // Unmap all direct memory areas covered by this unmap.
        auto phys_addr = phys_base + start_in_vma;
        auto remaining_size = adjusted_size;
        DMemHandle dmem_handle = FindDmemArea(phys_addr);
        while (dmem_handle != dmem_map.end() && remaining_size > 0) {
            const auto start_in_dma = phys_base - dmem_handle->second.base;
            const auto size_in_dma = dmem_handle->second.size - start_in_dma > remaining_size
                                         ? remaining_size
                                         : dmem_handle->second.size - start_in_dma;
            dmem_handle = CarveDmemArea(phys_addr, size_in_dma);
            auto& dmem_area = dmem_handle->second;
            dmem_area.dma_type = DMAType::Allocated;
            remaining_size -= dmem_area.size;
            phys_addr += dmem_area.size;

            // Check if we can coalesce any dmem areas.
            MergeAdjacent(dmem_map, dmem_handle);
            dmem_handle = FindDmemArea(phys_addr);
        }
    }

    if (type == VMAType::Flexible) {
        flexible_usage -= adjusted_size;

        // Now that there is a physical backing used for flexible memory,
        // manually erase the contents before unmapping to prevent possible issues.
        const auto unmap_hardware_address = impl.BackingBase() + phys_base + start_in_vma;
        std::memset(unmap_hardware_address, 0, adjusted_size);

        // Address space unmap needs the physical_base from the start of the vma,
        // so calculate the phys_base to unmap from here.
        const auto unmap_phys_base = phys_base + start_in_vma;
        const auto new_fmem_handle = CarveFmemArea(unmap_phys_base, adjusted_size);
        auto& new_fmem_area = new_fmem_handle->second;
        new_fmem_area.is_free = true;
        MergeAdjacent(fmem_map, new_fmem_handle);
    }

    // Mark region as free and attempt to coalesce it with neighbours.
    const auto new_it = CarveVMA(virtual_addr, adjusted_size);
    auto& vma = new_it->second;
    vma.type = VMAType::Free;
    vma.prot = MemoryProt::NoAccess;
    vma.phys_base = 0;
    vma.disallow_merge = false;
    vma.name = "";
    MergeAdjacent(vma_map, new_it);

    if (type != VMAType::Reserved && type != VMAType::PoolReserved) {
        // If this mapping has GPU access, unmap from GPU.
        if (IsValidGpuMapping(virtual_addr, size)) {
            rasterizer->UnmapMemory(virtual_addr, size);
        }

        // Unmap the memory region.
        impl.Unmap(vma_base_addr, vma_base_size, start_in_vma, start_in_vma + adjusted_size,
                   phys_base, is_exec, has_backing, readonly_file);
        TRACK_FREE(virtual_addr, "VMEM");
    }
    return adjusted_size;
}

s32 MemoryManager::UnmapMemoryImpl(VAddr virtual_addr, u64 size) {
    u64 unmapped_bytes = 0;
    virtual_addr = Common::AlignDown(virtual_addr, 16_KB);
    size = Common::AlignUp(size, 16_KB);
    do {
        ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(virtual_addr)),
                   "Attempted to access invalid address {:#x}", virtual_addr);
        auto it = FindVMA(virtual_addr + unmapped_bytes);
        auto& vma_base = it->second;
        auto unmapped =
            UnmapBytesFromEntry(virtual_addr + unmapped_bytes, vma_base, size - unmapped_bytes);
        ASSERT_MSG(unmapped > 0, "Failed to unmap memory, progress is impossible");
        unmapped_bytes += unmapped;
    } while (unmapped_bytes < size);

    return ORBIS_OK;
}

s32 MemoryManager::QueryProtection(VAddr addr, void** start, void** end, u32* prot) {
    ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(addr)),
               "Attempted to access invalid address {:#x}", addr);
    std::scoped_lock lk{mutex};

    const auto it = FindVMA(addr);
    const auto& vma = it->second;
    if (vma.IsFree()) {
        LOG_ERROR(Kernel_Vmm, "Address {:#x} is not mapped", addr);
        return ORBIS_KERNEL_ERROR_EACCES;
    }

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

s64 MemoryManager::ProtectBytes(VAddr addr, VirtualMemoryArea vma_base, u64 size, MemoryProt prot) {
    const auto start_in_vma = addr - vma_base.base;
    const auto adjusted_size =
        vma_base.size - start_in_vma < size ? vma_base.size - start_in_vma : size;

    if (vma_base.type == VMAType::Free) {
        // On PS4, protecting freed memory does nothing.
        return adjusted_size;
    }

    // Change protection
    vma_base.prot = prot;

    // Set permissions
    Core::MemoryPermission perms{};

    if (True(prot & MemoryProt::CpuRead)) {
        perms |= Core::MemoryPermission::Read;
    }
    if (True(prot & MemoryProt::CpuReadWrite)) {
        perms |= Core::MemoryPermission::ReadWrite;
    }
    if (True(prot & MemoryProt::CpuExec)) {
        perms |= Core::MemoryPermission::Execute;
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

    return adjusted_size;
}

s32 MemoryManager::Protect(VAddr addr, u64 size, MemoryProt prot) {
    std::scoped_lock lk{mutex};

    // Validate protection flags
    constexpr static MemoryProt valid_flags =
        MemoryProt::NoAccess | MemoryProt::CpuRead | MemoryProt::CpuReadWrite |
        MemoryProt::CpuExec | MemoryProt::GpuRead | MemoryProt::GpuWrite | MemoryProt::GpuReadWrite;

    MemoryProt invalid_flags = prot & ~valid_flags;
    if (invalid_flags != MemoryProt::NoAccess) {
        LOG_ERROR(Kernel_Vmm, "Invalid protection flags");
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    // Protect all VMAs between addr and addr + size.
    s64 protected_bytes = 0;
    while (protected_bytes < size) {
        ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(addr)),
                   "Attempted to access invalid address {:#x}", addr);
        auto it = FindVMA(addr + protected_bytes);
        auto& vma_base = it->second;
        auto result = ProtectBytes(addr + protected_bytes, vma_base, size - protected_bytes, prot);
        if (result < 0) {
            // ProtectBytes returned an error, return it
            return result;
        }
        protected_bytes += result;
    }

    return ORBIS_OK;
}

s32 MemoryManager::VirtualQuery(VAddr addr, s32 flags,
                                ::Libraries::Kernel::OrbisVirtualQueryInfo* info) {
    std::scoped_lock lk{mutex};

    // FindVMA on addresses before the vma_map return garbage data.
    auto query_addr =
        addr < impl.SystemManagedVirtualBase() ? impl.SystemManagedVirtualBase() : addr;
    if (addr < query_addr && flags == 0) {
        LOG_WARNING(Kernel_Vmm, "VirtualQuery on free memory region");
        return ORBIS_KERNEL_ERROR_EACCES;
    }
    auto it = FindVMA(query_addr);

    while (it != vma_map.end() && it->second.type == VMAType::Free && flags == 1) {
        ++it;
    }
    if (it == vma_map.end() || it->second.type == VMAType::Free) {
        LOG_WARNING(Kernel_Vmm, "VirtualQuery on free memory region");
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    const auto& vma = it->second;
    info->start = vma.base;
    info->end = vma.base + vma.size;
    info->offset = vma.type == VMAType::Flexible ? 0 : vma.phys_base;
    info->protection = static_cast<s32>(vma.prot);
    info->is_flexible = vma.type == VMAType::Flexible ? 1 : 0;
    info->is_direct = vma.type == VMAType::Direct ? 1 : 0;
    info->is_stack = vma.type == VMAType::Stack ? 1 : 0;
    info->is_pooled = vma.type == VMAType::PoolReserved || vma.type == VMAType::Pooled ? 1 : 0;
    info->is_committed = vma.IsMapped() ? 1 : 0;

    strncpy(info->name, vma.name.data(), ::Libraries::Kernel::ORBIS_KERNEL_MAXIMUM_NAME_LENGTH);

    if (vma.type == VMAType::Direct) {
        const auto dmem_it = FindDmemArea(vma.phys_base);
        ASSERT_MSG(vma.phys_base <= dmem_it->second.GetEnd(), "vma.phys_base is not in dmem_map!");
        info->memory_type = dmem_it->second.memory_type;
    } else {
        info->memory_type = ::Libraries::Kernel::ORBIS_KERNEL_WB_ONION;
    }

    return ORBIS_OK;
}

s32 MemoryManager::DirectMemoryQuery(PAddr addr, bool find_next,
                                     ::Libraries::Kernel::OrbisQueryInfo* out_info) {
    std::scoped_lock lk{mutex};

    if (addr >= total_direct_size) {
        LOG_WARNING(Kernel_Vmm, "Unable to find allocated direct memory region to query!");
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    auto dmem_area = FindDmemArea(addr);
    while (dmem_area != dmem_map.end() && dmem_area->second.dma_type == DMAType::Free &&
           find_next) {
        dmem_area++;
    }

    if (dmem_area == dmem_map.end() || dmem_area->second.dma_type == DMAType::Free) {
        LOG_WARNING(Kernel_Vmm, "Unable to find allocated direct memory region to query!");
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    out_info->start = dmem_area->second.base;
    out_info->memoryType = dmem_area->second.memory_type;

    // Loop through all sequential mapped or allocated dmem areas
    // to determine the hardware accurate end.
    while (dmem_area != dmem_map.end() && dmem_area->second.memory_type == out_info->memoryType &&
           (dmem_area->second.dma_type == DMAType::Mapped ||
            dmem_area->second.dma_type == DMAType::Allocated)) {
        out_info->end = dmem_area->second.GetEnd();
        dmem_area++;
    }

    return ORBIS_OK;
}

s32 MemoryManager::DirectQueryAvailable(PAddr search_start, PAddr search_end, u64 alignment,
                                        PAddr* phys_addr_out, u64* size_out) {
    std::scoped_lock lk{mutex};

    auto dmem_area = FindDmemArea(search_start);
    PAddr paddr{};
    u64 max_size{};

    while (dmem_area != dmem_map.end()) {
        if (dmem_area->second.dma_type != DMAType::Free) {
            dmem_area++;
            continue;
        }

        auto aligned_base = alignment > 0 ? Common::AlignUp(dmem_area->second.base, alignment)
                                          : dmem_area->second.base;
        const auto alignment_size = aligned_base - dmem_area->second.base;
        auto remaining_size =
            dmem_area->second.size >= alignment_size ? dmem_area->second.size - alignment_size : 0;

        if (dmem_area->second.base < search_start) {
            // We need to trim remaining_size to ignore addresses before search_start
            remaining_size = remaining_size > (search_start - dmem_area->second.base)
                                 ? remaining_size - (search_start - dmem_area->second.base)
                                 : 0;
            aligned_base = alignment > 0 ? Common::AlignUp(search_start, alignment) : search_start;
        }

        if (dmem_area->second.GetEnd() > search_end) {
            // We need to trim remaining_size to ignore addresses beyond search_end
            remaining_size = remaining_size > (dmem_area->second.GetEnd() - search_end)
                                 ? remaining_size - (dmem_area->second.GetEnd() - search_end)
                                 : 0;
        }

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

s32 MemoryManager::SetDirectMemoryType(VAddr addr, u64 size, s32 memory_type) {
    std::scoped_lock lk{mutex};

    // Search through all VMAs covered by the provided range.
    // We aren't modifying these VMAs, so it's safe to iterate through them.
    auto remaining_size = size;
    auto vma_handle = FindVMA(addr);
    while (vma_handle != vma_map.end() && vma_handle->second.base < addr + size) {
        // Direct and Pooled mappings are the only ones with a memory type.
        if (vma_handle->second.type == VMAType::Direct ||
            vma_handle->second.type == VMAType::Pooled) {
            // Calculate position in vma
            const auto start_in_vma = addr - vma_handle->second.base;
            const auto size_in_vma = vma_handle->second.size - start_in_vma;
            auto phys_addr = vma_handle->second.phys_base + start_in_vma;
            auto size_to_modify = remaining_size > size_in_vma ? size_in_vma : remaining_size;

            // Loop through remaining dmem areas until the physical addresses represented
            // are all adjusted.
            DMemHandle dmem_handle = FindDmemArea(phys_addr);
            while (dmem_handle != dmem_map.end() && size_in_vma >= size_to_modify &&
                   size_to_modify > 0) {
                const auto start_in_dma = phys_addr - dmem_handle->second.base;
                const auto size_in_dma = dmem_handle->second.size - start_in_dma > size_to_modify
                                             ? size_to_modify
                                             : dmem_handle->second.size - start_in_dma;
                dmem_handle = CarveDmemArea(phys_addr, size_in_dma);
                auto& dmem_area = dmem_handle->second;
                dmem_area.memory_type = memory_type;
                size_to_modify -= dmem_area.size;
                phys_addr += dmem_area.size;

                // Check if we can coalesce any dmem areas now that the types are different.
                MergeAdjacent(dmem_map, dmem_handle);
                dmem_handle = FindDmemArea(phys_addr);
            }
        }
        remaining_size -= vma_handle->second.size;
        vma_handle++;
    }

    return ORBIS_OK;
}

void MemoryManager::NameVirtualRange(VAddr virtual_addr, u64 size, std::string_view name) {
    std::scoped_lock lk{mutex};

    // Sizes are aligned up to the nearest 16_KB
    auto aligned_size = Common::AlignUp(size, 16_KB);
    // Addresses are aligned down to the nearest 16_KB
    auto aligned_addr = Common::AlignDown(virtual_addr, 16_KB);

    ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(aligned_addr)),
               "Attempted to access invalid address {:#x}", aligned_addr);
    auto it = FindVMA(aligned_addr);
    s64 remaining_size = aligned_size;
    auto current_addr = aligned_addr;
    while (remaining_size > 0) {
        // Nothing needs to be done to free VMAs
        if (!it->second.IsFree()) {
            if (remaining_size < it->second.size) {
                // We should split VMAs here, but this could cause trouble for Windows.
                // Instead log a warning and name the whole VMA.
                LOG_WARNING(Kernel_Vmm, "Trying to partially name a range");
            }
            auto& vma = it->second;
            vma.name = name;
        }
        remaining_size -= it->second.size;
        current_addr += it->second.size;
        it = FindVMA(current_addr);
    }
}

s32 MemoryManager::GetDirectMemoryType(PAddr addr, s32* directMemoryTypeOut,
                                       void** directMemoryStartOut, void** directMemoryEndOut) {
    if (addr >= total_direct_size) {
        LOG_ERROR(Kernel_Vmm, "Unable to find allocated direct memory region to check type!");
        return ORBIS_KERNEL_ERROR_ENOENT;
    }

    const auto& dmem_area = FindDmemArea(addr)->second;
    if (dmem_area.dma_type == DMAType::Free) {
        LOG_ERROR(Kernel_Vmm, "Unable to find allocated direct memory region to check type!");
        return ORBIS_KERNEL_ERROR_ENOENT;
    }

    *directMemoryStartOut = reinterpret_cast<void*>(dmem_area.base);
    *directMemoryEndOut = reinterpret_cast<void*>(dmem_area.GetEnd());
    *directMemoryTypeOut = dmem_area.memory_type;
    return ORBIS_OK;
}

s32 MemoryManager::IsStack(VAddr addr, void** start, void** end) {
    ASSERT_MSG(IsValidAddress(reinterpret_cast<void*>(addr)),
               "Attempted to access invalid address {:#x}", addr);
    const auto& vma = FindVMA(addr)->second;
    if (vma.IsFree()) {
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    u64 stack_start = 0;
    u64 stack_end = 0;
    if (vma.type == VMAType::Stack) {
        stack_start = vma.base;
        stack_end = vma.base + vma.size;
    }

    if (start != nullptr) {
        *start = reinterpret_cast<void*>(stack_start);
    }

    if (end != nullptr) {
        *end = reinterpret_cast<void*>(stack_end);
    }

    return ORBIS_OK;
}

s32 MemoryManager::GetMemoryPoolStats(::Libraries::Kernel::OrbisKernelMemoryPoolBlockStats* stats) {
    std::scoped_lock lk{mutex};

    // Run through dmem_map, determine how much physical memory is currently committed
    constexpr u64 block_size = 64_KB;
    u64 committed_size = 0;

    auto dma_handle = dmem_map.begin();
    while (dma_handle != dmem_map.end()) {
        if (dma_handle->second.dma_type == DMAType::Committed) {
            committed_size += dma_handle->second.size;
        }
        dma_handle++;
    }

    stats->allocated_flushed_blocks = committed_size / block_size;
    stats->available_flushed_blocks = committed_size / block_size;
    // TODO: Determine how "cached blocks" work
    stats->allocated_cached_blocks = 0;
    stats->available_cached_blocks = 0;
    return ORBIS_OK;
}

void MemoryManager::InvalidateMemory(const VAddr addr, const u64 size) const {
    if (rasterizer) {
        rasterizer->InvalidateMemory(addr, size);
    }
}

VAddr MemoryManager::SearchFree(VAddr virtual_addr, u64 size, u32 alignment) {
    // If the requested address is below the mapped range, start search from the lowest address
    auto min_search_address = impl.SystemManagedVirtualBase();
    if (virtual_addr < min_search_address) {
        virtual_addr = min_search_address;
    }

    // If the requested address is beyond the maximum our code can handle, throw an assert
    auto max_search_address = impl.UserVirtualBase() + impl.UserVirtualSize();
    ASSERT_MSG(virtual_addr <= max_search_address, "Input address {:#x} is out of bounds",
               virtual_addr);

    // Align up the virtual_addr first.
    virtual_addr = Common::AlignUp(virtual_addr, alignment);
    auto it = FindVMA(virtual_addr);

    // If the VMA is free and contains the requested mapping we are done.
    if (it->second.IsFree() && it->second.Contains(virtual_addr, size)) {
        return virtual_addr;
    }

    // Search for the first free VMA that fits our mapping.
    while (it != vma_map.end()) {
        if (!it->second.IsFree()) {
            it++;
            continue;
        }

        const auto& vma = it->second;
        virtual_addr = Common::AlignUp(vma.base, alignment);
        // Sometimes the alignment itself might be larger than the VMA.
        if (virtual_addr > vma.base + vma.size) {
            it++;
            continue;
        }

        // Make sure the address is within our defined bounds
        if (virtual_addr >= max_search_address) {
            // There are no free mappings within our safely usable address space.
            break;
        }

        // If there's enough space in the VMA, return the address.
        const u64 remaining_size = vma.base + vma.size - virtual_addr;
        if (remaining_size >= size) {
            return virtual_addr;
        }
        it++;
    }

    // Couldn't find a suitable VMA, return an error.
    LOG_ERROR(Kernel_Vmm, "Couldn't find a free mapping for address {:#x}, size {:#x}",
              virtual_addr, size);
    return -1;
}

MemoryManager::VMAHandle MemoryManager::CarveVMA(VAddr virtual_addr, u64 size) {
    auto vma_handle = FindVMA(virtual_addr);

    const VirtualMemoryArea& vma = vma_handle->second;
    ASSERT_MSG(vma.base <= virtual_addr, "Adding a mapping to already mapped region");

    const VAddr start_in_vma = virtual_addr - vma.base;
    const VAddr end_in_vma = start_in_vma + size;

    if (start_in_vma == 0 && size == vma.size) {
        // if requsting the whole VMA, return it
        return vma_handle;
    }

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

MemoryManager::DMemHandle MemoryManager::CarveDmemArea(PAddr addr, u64 size) {
    auto dmem_handle = FindDmemArea(addr);
    ASSERT_MSG(addr <= dmem_handle->second.GetEnd(), "Physical address not in dmem_map");

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

MemoryManager::FMemHandle MemoryManager::CarveFmemArea(PAddr addr, u64 size) {
    auto fmem_handle = FindFmemArea(addr);
    ASSERT_MSG(addr <= fmem_handle->second.GetEnd(), "Physical address not in fmem_map");

    const FlexibleMemoryArea& area = fmem_handle->second;
    ASSERT_MSG(area.base <= addr, "Adding an allocation to already allocated region");

    const PAddr start_in_area = addr - area.base;
    const PAddr end_in_vma = start_in_area + size;
    ASSERT_MSG(end_in_vma <= area.size, "Mapping cannot fit inside free region: size = {:#x}",
               size);

    if (end_in_vma != area.size) {
        // Split VMA at the end of the allocated region
        Split(fmem_handle, end_in_vma);
    }
    if (start_in_area != 0) {
        // Split VMA at the start of the allocated region
        fmem_handle = Split(fmem_handle, start_in_area);
    }

    return fmem_handle;
}

MemoryManager::VMAHandle MemoryManager::Split(VMAHandle vma_handle, u64 offset_in_vma) {
    auto& old_vma = vma_handle->second;
    ASSERT(offset_in_vma < old_vma.size && offset_in_vma > 0);

    auto new_vma = old_vma;
    old_vma.size = offset_in_vma;
    new_vma.base += offset_in_vma;
    new_vma.size -= offset_in_vma;

    if (HasPhysicalBacking(new_vma)) {
        new_vma.phys_base += offset_in_vma;
    }
    return vma_map.emplace_hint(std::next(vma_handle), new_vma.base, new_vma);
}

MemoryManager::DMemHandle MemoryManager::Split(DMemHandle dmem_handle, u64 offset_in_area) {
    auto& old_area = dmem_handle->second;
    ASSERT(offset_in_area < old_area.size && offset_in_area > 0);

    auto new_area = old_area;
    old_area.size = offset_in_area;
    new_area.memory_type = old_area.memory_type;
    new_area.base += offset_in_area;
    new_area.size -= offset_in_area;

    return dmem_map.emplace_hint(std::next(dmem_handle), new_area.base, new_area);
}

MemoryManager::FMemHandle MemoryManager::Split(FMemHandle fmem_handle, u64 offset_in_area) {
    auto& old_area = fmem_handle->second;
    ASSERT(offset_in_area < old_area.size && offset_in_area > 0);

    auto new_area = old_area;
    old_area.size = offset_in_area;
    new_area.base += offset_in_area;
    new_area.size -= offset_in_area;

    return fmem_map.emplace_hint(std::next(fmem_handle), new_area.base, new_area);
}

} // namespace Core
