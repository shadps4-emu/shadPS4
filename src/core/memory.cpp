// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/config.h"
#include "common/debug.h"
#include "common/elf_info.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/kernel/process.h"
#include "core/memory.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

namespace Core {

MemoryManager::MemoryManager() {
    LOG_INFO(Kernel_Vmm, "Virtual memory space initialized with regions:");

    // Construct vma_map using the regions reserved by the address space
    auto regions = impl.GetUsableRegions();
    u64 total_usable_space = 0;
    for (auto region : regions) {
        vma_map.emplace(region.lower(),
                        VirtualMemoryArea{region.lower(), region.upper() - region.lower()});
        LOG_INFO(Kernel_Vmm, "{:#x} - {:#x}", region.lower(), region.upper());
    }

    ASSERT_MSG(Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk_version) == 0,
               "Failed to get compiled SDK version");
}

MemoryManager::~MemoryManager() = default;

void MemoryManager::SetupMemoryRegions(u64 flexible_size, bool use_extended_mem1,
                                       bool use_extended_mem2) {
    const bool is_neo = ::Libraries::Kernel::sceKernelIsNeoMode();
    auto total_size = is_neo ? ORBIS_KERNEL_TOTAL_MEM_PRO : ORBIS_KERNEL_TOTAL_MEM;
    if (Config::isDevKitConsole()) {
        total_size = is_neo ? ORBIS_KERNEL_TOTAL_MEM_DEV_PRO : ORBIS_KERNEL_TOTAL_MEM_DEV;
    }
    s32 extra_dmem = Config::getExtraDmemInMbytes();
    if (Config::getExtraDmemInMbytes() != 0) {
        LOG_WARNING(Kernel_Vmm,
                    "extraDmemInMbytes is {} MB! Old Direct Size: {:#x} -> New Direct Size: {:#x}",
                    extra_dmem, total_size, total_size + extra_dmem * 1_MB);
        total_size += extra_dmem * 1_MB;
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
    dmem_map.emplace(0, PhysicalMemoryArea{0, total_direct_size});

    // Insert an area that covers the flexible memory physical address block.
    // Note that this should never be called after flexible memory allocations have been made.
    const auto remaining_physical_space = total_size - total_direct_size;
    fmem_map.clear();
    fmem_map.emplace(total_direct_size,
                     PhysicalMemoryArea{total_direct_size, remaining_physical_space});

    LOG_INFO(Kernel_Vmm, "Configured memory regions: flexible size = {:#x}, direct size = {:#x}",
             total_flexible_size, total_direct_size);
}

u64 MemoryManager::ClampRangeSize(VAddr virtual_addr, u64 size) {
    static constexpr u64 MinSizeToClamp = 3_GB;
    // Dont bother with clamping if the size is small so we dont pay a map lookup on every buffer.
    if (size < MinSizeToClamp) {
        return size;
    }

    ASSERT_MSG(IsValidMapping(virtual_addr), "Attempted to access invalid address {:#x}",
               virtual_addr);

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
    ASSERT_MSG(IsValidMapping(virtual_addr), "Attempted to access invalid address {:#x}",
               virtual_addr);

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

bool MemoryManager::TryWriteBacking(void* address, const void* data, u64 size) {
    const VAddr virtual_addr = std::bit_cast<VAddr>(address);
    ASSERT_MSG(IsValidMapping(virtual_addr, size), "Attempted to access invalid address {:#x}",
               virtual_addr);

    std::vector<VirtualMemoryArea> vmas_to_write;
    auto current_vma = FindVMA(virtual_addr);
    while (current_vma->second.Overlaps(virtual_addr, size)) {
        if (!HasPhysicalBacking(current_vma->second)) {
            break;
        }
        vmas_to_write.emplace_back(current_vma->second);
        current_vma++;
    }

    if (vmas_to_write.empty()) {
        return false;
    }

    for (auto& vma : vmas_to_write) {
        auto start_in_vma = std::max<VAddr>(virtual_addr, vma.base) - vma.base;
        auto phys_handle = std::prev(vma.phys_areas.upper_bound(start_in_vma));
        for (; phys_handle != vma.phys_areas.end(); phys_handle++) {
            if (!size) {
                break;
            }
            const u64 start_in_dma =
                std::max<u64>(start_in_vma, phys_handle->first) - phys_handle->first;
            u8* backing = impl.BackingBase() + phys_handle->second.base + start_in_dma;
            u64 copy_size = std::min<u64>(size, phys_handle->second.size - start_in_dma);
            memcpy(backing, data, copy_size);
            size -= copy_size;
        }
    }

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
    while (dmem_area->second.dma_type != PhysicalMemoryType::Free ||
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
    auto& area = CarvePhysArea(dmem_map, mapping_start, size)->second;
    area.dma_type = PhysicalMemoryType::Pooled;
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
    auto mapping_start =
        Common::AlignUp(std::max<PAddr>(search_start, dmem_area->second.base), alignment);
    auto mapping_end = mapping_start + size;

    // Find the first free, large enough dmem area in the range.
    while (dmem_area->second.dma_type != PhysicalMemoryType::Free ||
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
    auto& area = CarvePhysArea(dmem_map, mapping_start, size)->second;
    area.memory_type = memory_type;
    area.dma_type = PhysicalMemoryType::Allocated;
    MergeAdjacent(dmem_map, dmem_area);

    return mapping_start;
}

s32 MemoryManager::Free(PAddr phys_addr, u64 size, bool is_checked) {
    // Basic bounds checking
    if (phys_addr > total_direct_size || (is_checked && phys_addr + size > total_direct_size)) {
        LOG_ERROR(Kernel_Vmm, "phys_addr {:#x}, size {:#x} goes outside dmem map", phys_addr, size);
        if (is_checked) {
            return ORBIS_KERNEL_ERROR_ENOENT;
        }
        return ORBIS_OK;
    }

    // Lock mutex
    std::scoped_lock lk{mutex};

    // If this is a checked free, then all direct memory in range must be allocated.
    std::vector<std::pair<PAddr, u64>> free_list;
    u64 remaining_size = size;
    auto phys_handle = FindDmemArea(phys_addr);
    for (; phys_handle != dmem_map.end(); phys_handle++) {
        if (remaining_size == 0) {
            // Done searching
            break;
        }
        auto& dmem_area = phys_handle->second;
        if (dmem_area.dma_type == PhysicalMemoryType::Free) {
            if (is_checked) {
                // Checked frees will error if anything in the area isn't allocated.
                // Unchecked frees will just ignore free areas.
                LOG_ERROR(Kernel_Vmm, "Attempting to release a free dmem area");
                return ORBIS_KERNEL_ERROR_ENOENT;
            }
            continue;
        }

        // Store physical address and size to release
        const PAddr current_phys_addr = std::max<PAddr>(phys_addr, phys_handle->first);
        const u64 start_in_dma = current_phys_addr - phys_handle->first;
        const u64 size_in_dma =
            std::min<u64>(remaining_size, phys_handle->second.size - start_in_dma);
        free_list.emplace_back(current_phys_addr, size_in_dma);

        // Track remaining size to free
        remaining_size -= size_in_dma;
    }

    // Release any dmem mappings that reference this physical block.
    std::vector<std::pair<VAddr, u64>> remove_list;
    for (const auto& [addr, mapping] : vma_map) {
        if (mapping.type != VMAType::Direct) {
            continue;
        }
        for (auto& [offset_in_vma, phys_mapping] : mapping.phys_areas) {
            if (phys_addr + size > phys_mapping.base &&
                phys_addr < phys_mapping.base + phys_mapping.size) {
                const u64 phys_offset =
                    std::max<u64>(phys_mapping.base, phys_addr) - phys_mapping.base;
                const VAddr addr_in_vma = mapping.base + offset_in_vma + phys_offset;
                const u64 unmap_size = std::min<u64>(phys_mapping.size - phys_offset, size);

                // Unmapping might erase from vma_map. We can't do it here.
                remove_list.emplace_back(addr_in_vma, unmap_size);
            }
        }
    }
    for (const auto& [addr, size] : remove_list) {
        LOG_INFO(Kernel_Vmm, "Unmapping direct mapping {:#x} with size {:#x}", addr, size);
        UnmapMemoryImpl(addr, size);
    }

    // Unmap all dmem areas within this area.
    for (auto& [phys_addr, size] : free_list) {
        // Carve a free dmem area in place of this one.
        const auto dmem_handle = CarvePhysArea(dmem_map, phys_addr, size);
        auto& new_dmem_area = dmem_handle->second;
        new_dmem_area.dma_type = PhysicalMemoryType::Free;
        new_dmem_area.memory_type = 0;

        // Merge the new dmem_area with dmem_map
        MergeAdjacent(dmem_map, dmem_handle);
    }

    return ORBIS_OK;
}

s32 MemoryManager::PoolCommit(VAddr virtual_addr, u64 size, MemoryProt prot, s32 mtype) {
    std::scoped_lock lk{mutex};
    ASSERT_MSG(IsValidMapping(virtual_addr, size), "Attempted to access invalid address {:#x}",
               virtual_addr);

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

    if (True(prot & MemoryProt::CpuWrite)) {
        // On PS4, read is appended to write mappings.
        prot |= MemoryProt::CpuRead;
    }

    // Create the virtual mapping for the commit
    const auto new_vma_handle = CarveVMA(virtual_addr, size);
    auto& new_vma = new_vma_handle->second;
    new_vma.disallow_merge = false;
    new_vma.prot = prot;
    new_vma.name = "anon";
    new_vma.type = Core::VMAType::Pooled;
    new_vma.phys_areas.clear();

    // Find suitable physical addresses
    auto handle = dmem_map.begin();
    u64 remaining_size = size;
    VAddr current_addr = mapped_addr;
    while (handle != dmem_map.end() && remaining_size > 0) {
        if (handle->second.dma_type != PhysicalMemoryType::Pooled) {
            // Non-pooled means it's either not for pool use, or already committed.
            handle++;
            continue;
        }

        // On PS4, commits can make sparse physical mappings.
        u64 size_to_map = std::min<u64>(remaining_size, handle->second.size);

        // Use the start of this area as the physical backing for this mapping.
        const auto new_dmem_handle = CarvePhysArea(dmem_map, handle->second.base, size_to_map);
        auto& new_dmem_area = new_dmem_handle->second;
        new_dmem_area.dma_type = PhysicalMemoryType::Committed;
        new_dmem_area.memory_type = mtype;

        // Add the dmem area to this vma, merge it with any similar tracked areas.
        new_vma.phys_areas[current_addr - mapped_addr] = new_dmem_handle->second;
        MergeAdjacent(new_vma.phys_areas, new_vma.phys_areas.find(current_addr - mapped_addr));

        // Perform an address space mapping for each physical area
        void* out_addr = impl.Map(current_addr, size_to_map, new_dmem_area.base);
        // Tracy memory tracking breaks from merging memory areas. Disabled for now.
        // TRACK_ALLOC(out_addr, size_to_map, "VMEM");

        handle = MergeAdjacent(dmem_map, new_dmem_handle);
        current_addr += size_to_map;
        remaining_size -= size_to_map;
        handle++;
    }
    ASSERT_MSG(remaining_size == 0, "Failed to commit pooled memory");

    // Merge this VMA with similar nearby areas
    MergeAdjacent(vma_map, new_vma_handle);

    if (IsValidGpuMapping(mapped_addr, size)) {
        rasterizer->MapMemory(mapped_addr, size);
    }

    return ORBIS_OK;
}

std::pair<s32, MemoryManager::VMAHandle> MemoryManager::CreateArea(
    VAddr virtual_addr, u64 size, MemoryProt prot, MemoryMapFlags flags, VMAType type,
    std::string_view name, u64 alignment) {

    // Limit the minimum address to SystemManagedVirtualBase to prevent hardware-specific issues.
    VAddr mapped_addr = (virtual_addr == 0) ? impl.SystemManagedVirtualBase() : virtual_addr;

    // Fixed mapping means the virtual address must exactly match the provided one.
    // On a PS4, the Fixed flag is ignored if address 0 is provided.
    if (True(flags & MemoryMapFlags::Fixed) && virtual_addr != 0) {
        ASSERT_MSG(IsValidMapping(mapped_addr, size), "Attempted to access invalid address {:#x}",
                   mapped_addr);
        auto vma = FindVMA(mapped_addr)->second;
        // There's a possible edge case where we're mapping to a partially reserved range.
        // To account for this, unmap any reserved areas within this mapping range first.
        auto unmap_addr = mapped_addr;
        auto unmap_size = size;

        // If flag NoOverwrite is provided, don't overwrite mapped VMAs.
        // When it isn't provided, VMAs can be overwritten regardless of if they're mapped.
        while ((False(flags & MemoryMapFlags::NoOverwrite) || vma.IsFree()) &&
               unmap_addr < mapped_addr + size) {
            auto unmapped = UnmapBytesFromEntry(unmap_addr, vma, unmap_size);
            unmap_addr += unmapped;
            unmap_size -= unmapped;
            vma = FindVMA(unmap_addr)->second;
        }

        vma = FindVMA(mapped_addr)->second;
        auto remaining_size = vma.base + vma.size - mapped_addr;
        if (!vma.IsFree() || remaining_size < size) {
            LOG_ERROR(Kernel_Vmm, "Unable to map {:#x} bytes at address {:#x}", size, mapped_addr);
            return {ORBIS_KERNEL_ERROR_ENOMEM, vma_map.end()};
        }
    } else {
        // When MemoryMapFlags::Fixed is not specified, and mapped_addr is 0,
        // search from address 0x200000000 instead.
        alignment = alignment > 0 ? alignment : 16_KB;
        mapped_addr = virtual_addr == 0 ? 0x200000000 : mapped_addr;
        mapped_addr = SearchFree(mapped_addr, size, alignment);
        if (mapped_addr == -1) {
            // No suitable memory areas to map to
            return {ORBIS_KERNEL_ERROR_ENOMEM, vma_map.end()};
        }
    }

    // Create a memory area representing this mapping.
    const auto new_vma_handle = CarveVMA(mapped_addr, size);
    auto& new_vma = new_vma_handle->second;
    const bool is_exec = True(prot & MemoryProt::CpuExec);
    if (True(prot & MemoryProt::CpuWrite)) {
        // On PS4, read is appended to write mappings.
        prot |= MemoryProt::CpuRead;
    }

    new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
    new_vma.prot = prot;
    new_vma.name = name;
    new_vma.type = type;
    new_vma.phys_areas.clear();
    return {ORBIS_OK, new_vma_handle};
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
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::scoped_lock lk{mutex};

    PhysHandle dmem_area;
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
            if (dmem_area->second.dma_type != PhysicalMemoryType::Allocated &&
                dmem_area->second.dma_type != PhysicalMemoryType::Mapped) {
                LOG_ERROR(Kernel_Vmm, "Unable to map {:#x} bytes at physical address {:#x}", size,
                          phys_addr);
                return ORBIS_KERNEL_ERROR_ENOMEM;
            }

            // If we need to perform extra validation, then check for Mapped dmem areas too.
            if (validate_dmem && dmem_area->second.dma_type == PhysicalMemoryType::Mapped) {
                LOG_ERROR(Kernel_Vmm, "Unable to map {:#x} bytes at physical address {:#x}", size,
                          phys_addr);
                return ORBIS_KERNEL_ERROR_EBUSY;
            }

            dmem_area++;
        }
    }

    auto [result, new_vma_handle] =
        CreateArea(virtual_addr, size, prot, flags, type, name, alignment);
    if (result != ORBIS_OK) {
        return result;
    }

    auto& new_vma = new_vma_handle->second;
    auto mapped_addr = new_vma.base;
    bool is_exec = True(prot & MemoryProt::CpuExec);

    // If type is Flexible, we need to track how much flexible memory is used here.
    // We also need to determine a reasonable physical base to perform this mapping at.
    if (type == VMAType::Flexible) {
        // Find suitable physical addresses
        auto handle = fmem_map.begin();
        u64 remaining_size = size;
        VAddr current_addr = mapped_addr;
        while (handle != fmem_map.end() && remaining_size != 0) {
            if (handle->second.dma_type != PhysicalMemoryType::Free) {
                // If the handle isn't free, we cannot use it.
                handle++;
                continue;
            }

            // Determine the size we can map here.
            u64 size_to_map = std::min<u64>(remaining_size, handle->second.size);

            // Create a physical area
            const auto new_fmem_handle = CarvePhysArea(fmem_map, handle->second.base, size_to_map);
            auto& new_fmem_area = new_fmem_handle->second;
            new_fmem_area.dma_type = PhysicalMemoryType::Flexible;

            // Add the new area to the vma, merge it with any similar tracked areas.
            new_vma.phys_areas[current_addr - mapped_addr] = new_fmem_handle->second;
            MergeAdjacent(new_vma.phys_areas, new_vma.phys_areas.find(current_addr - mapped_addr));

            // Perform an address space mapping for each physical area
            void* out_addr = impl.Map(current_addr, size_to_map, new_fmem_area.base, is_exec);
            // Tracy memory tracking breaks from merging memory areas. Disabled for now.
            // TRACK_ALLOC(out_addr, size_to_map, "VMEM");

            handle = MergeAdjacent(fmem_map, new_fmem_handle);
            current_addr += size_to_map;
            remaining_size -= size_to_map;
            flexible_usage += size_to_map;
            handle++;
        }
        ASSERT_MSG(remaining_size == 0, "Failed to map physical memory");
    } else if (type == VMAType::Direct) {
        // Map the physical memory for this direct memory mapping.
        auto phys_addr_to_search = phys_addr;
        u64 remaining_size = size;
        dmem_area = FindDmemArea(phys_addr);
        while (dmem_area != dmem_map.end() && remaining_size > 0) {
            // Carve a new dmem area in place of this one with the appropriate type.
            // Ensure the carved area only covers the current dmem area.
            const auto start_phys_addr = std::max<PAddr>(phys_addr, dmem_area->second.base);
            const auto offset_in_dma = start_phys_addr - dmem_area->second.base;
            const auto size_in_dma =
                std::min<u64>(dmem_area->second.size - offset_in_dma, remaining_size);
            const auto dmem_handle = CarvePhysArea(dmem_map, start_phys_addr, size_in_dma);
            auto& new_dmem_area = dmem_handle->second;
            new_dmem_area.dma_type = PhysicalMemoryType::Mapped;

            // Add the dmem area to this vma, merge it with any similar tracked areas.
            new_vma.phys_areas[phys_addr_to_search - phys_addr] = dmem_handle->second;
            MergeAdjacent(new_vma.phys_areas,
                          new_vma.phys_areas.find(phys_addr_to_search - phys_addr));

            // Merge the new dmem_area with dmem_map
            MergeAdjacent(dmem_map, dmem_handle);

            // Get the next relevant dmem area.
            phys_addr_to_search = phys_addr + size_in_dma;
            remaining_size -= size_in_dma;
            dmem_area = FindDmemArea(phys_addr_to_search);
        }
        ASSERT_MSG(remaining_size == 0, "Failed to map physical memory");
    }

    if (new_vma.type != VMAType::Direct || sdk_version >= Common::ElfInfo::FW_20) {
        // Merge this VMA with similar nearby areas
        // Direct memory mappings only coalesce on SDK version 2.00 or later.
        MergeAdjacent(vma_map, new_vma_handle);
    }

    *out_addr = std::bit_cast<void*>(mapped_addr);
    if (type != VMAType::Reserved && type != VMAType::PoolReserved) {
        // Flexible address space mappings were performed while finding direct memory areas.
        if (type != VMAType::Flexible) {
            impl.Map(mapped_addr, size, phys_addr, is_exec);
            // Tracy memory tracking breaks from merging memory areas. Disabled for now.
            // TRACK_ALLOC(mapped_addr, size, "VMEM");
        }

        // If this is not a reservation, then map to GPU and address space
        if (IsValidGpuMapping(mapped_addr, size)) {
            rasterizer->MapMemory(mapped_addr, size);
        }
    }
    return ORBIS_OK;
}

s32 MemoryManager::MapFile(void** out_addr, VAddr virtual_addr, u64 size, MemoryProt prot,
                           MemoryMapFlags flags, s32 fd, s64 phys_addr) {
    std::scoped_lock lk{mutex};
    // Get the file to map

    auto* h = Common::Singleton<Core::FileSys::HandleTable>::Instance();
    auto file = h->GetFile(fd);
    if (file == nullptr) {
        LOG_WARNING(Kernel_Vmm, "Invalid file for mmap, fd {}", fd);
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    if (file->type != Core::FileSys::FileType::Regular) {
        LOG_WARNING(Kernel_Vmm, "Unsupported file type for mmap, fd {}", fd);
        return ORBIS_KERNEL_ERROR_EBADF;
    }

    if (True(prot & MemoryProt::CpuWrite)) {
        // On PS4, read is appended to write mappings.
        prot |= MemoryProt::CpuRead;
    }

    const auto handle = file->f.GetFileMapping();

    if (False(file->f.GetAccessMode() & Common::FS::FileAccessMode::Write) ||
        False(file->f.GetAccessMode() & Common::FS::FileAccessMode::Append)) {
        // If the file does not have write access, ensure prot does not contain write permissions.
        // On real hardware, these mappings succeed, but the memory cannot be written to.
        prot &= ~MemoryProt::CpuWrite;
    }

    if (prot >= MemoryProt::GpuRead) {
        // On real hardware, GPU file mmaps cause a full system crash due to an internal error.
        ASSERT_MSG(false, "Files cannot be mapped to GPU memory");
    }

    if (True(prot & MemoryProt::CpuExec)) {
        // On real hardware, execute permissions are silently removed.
        prot &= ~MemoryProt::CpuExec;
    }

    auto [result, new_vma_handle] =
        CreateArea(virtual_addr, size, prot, flags, VMAType::File, "anon", 0);
    if (result != ORBIS_OK) {
        return result;
    }

    auto& new_vma = new_vma_handle->second;
    auto mapped_addr = new_vma.base;
    bool is_exec = True(prot & MemoryProt::CpuExec);

    impl.MapFile(mapped_addr, size, phys_addr, std::bit_cast<u32>(prot), handle);

    *out_addr = std::bit_cast<void*>(mapped_addr);
    return ORBIS_OK;
}

s32 MemoryManager::PoolDecommit(VAddr virtual_addr, u64 size) {
    std::scoped_lock lk{mutex};
    ASSERT_MSG(IsValidMapping(virtual_addr, size), "Attempted to access invalid address {:#x}",
               virtual_addr);

    // Do an initial search to ensure this decommit is valid.
    auto it = FindVMA(virtual_addr);
    while (it != vma_map.end() && it->second.base + it->second.size <= virtual_addr + size) {
        if (it->second.type != VMAType::PoolReserved && it->second.type != VMAType::Pooled) {
            LOG_ERROR(Kernel_Vmm, "Attempting to decommit non-pooled memory!");
            return ORBIS_KERNEL_ERROR_EINVAL;
        }
        it++;
    }

    // Loop through all vmas in the area, unmap them.
    u64 remaining_size = size;
    VAddr current_addr = virtual_addr;
    while (remaining_size != 0) {
        const auto handle = FindVMA(current_addr);
        const auto& vma_base = handle->second;
        const auto start_in_vma = current_addr - vma_base.base;
        const auto size_in_vma = std::min<u64>(remaining_size, vma_base.size - start_in_vma);

        if (vma_base.type == VMAType::Pooled) {
            // We always map PoolCommitted memory to GPU, so unmap when decomitting.
            if (IsValidGpuMapping(current_addr, size_in_vma)) {
                rasterizer->UnmapMemory(current_addr, size_in_vma);
            }

            // Track how much pooled memory is decommitted
            pool_budget += size_in_vma;

            // Re-pool the direct memory used by this mapping
            u64 size_to_free = size_in_vma;
            auto phys_handle = std::prev(vma_base.phys_areas.upper_bound(start_in_vma));
            while (phys_handle != vma_base.phys_areas.end() && size_to_free > 0) {
                // Calculate physical memory offset, address, and size
                u64 dma_offset =
                    std::max<PAddr>(phys_handle->first, start_in_vma) - phys_handle->first;
                PAddr phys_addr = phys_handle->second.base + dma_offset;
                u64 size_in_dma =
                    std::min<u64>(size_to_free, phys_handle->second.size - dma_offset);

                // Create a new dmem area reflecting the pooled region
                const auto new_dmem_handle = CarvePhysArea(dmem_map, phys_addr, size_in_dma);
                auto& new_dmem_area = new_dmem_handle->second;
                new_dmem_area.dma_type = PhysicalMemoryType::Pooled;

                // Coalesce with nearby direct memory areas.
                MergeAdjacent(dmem_map, new_dmem_handle);

                // Increment loop
                size_to_free -= size_in_dma;
                phys_handle++;
            }
            ASSERT_MSG(size_to_free == 0, "Failed to decommit pooled memory");
        }

        // Mark region as pool reserved and attempt to coalesce it with neighbours.
        const auto new_it = CarveVMA(current_addr, size_in_vma);
        auto& vma = new_it->second;
        vma.type = VMAType::PoolReserved;
        vma.prot = MemoryProt::NoAccess;
        vma.disallow_merge = false;
        vma.name = "anon";
        vma.phys_areas.clear();
        MergeAdjacent(vma_map, new_it);

        current_addr += size_in_vma;
        remaining_size -= size_in_vma;
    }

    // Unmap from address space
    impl.Unmap(virtual_addr, size, true);
    // Tracy memory tracking breaks from merging memory areas. Disabled for now.
    // TRACK_FREE(virtual_addr, "VMEM");

    return ORBIS_OK;
}

s32 MemoryManager::UnmapMemory(VAddr virtual_addr, u64 size) {
    if (size == 0) {
        return ORBIS_OK;
    }
    std::scoped_lock lk{mutex};
    virtual_addr = Common::AlignDown(virtual_addr, 16_KB);
    size = Common::AlignUp(size, 16_KB);
    ASSERT_MSG(IsValidMapping(virtual_addr, size), "Attempted to access invalid address {:#x}",
               virtual_addr);
    u64 bytes_unmapped = UnmapMemoryImpl(virtual_addr, size);
    return bytes_unmapped;
}

u64 MemoryManager::UnmapBytesFromEntry(VAddr virtual_addr, VirtualMemoryArea vma_base, u64 size) {
    const auto start_in_vma = virtual_addr - vma_base.base;
    const auto size_in_vma = std::min<u64>(vma_base.size - start_in_vma, size);
    const auto vma_type = vma_base.type;
    const bool has_backing = HasPhysicalBacking(vma_base) || vma_base.type == VMAType::File;
    const bool readonly_file =
        vma_base.prot == MemoryProt::CpuRead && vma_base.type == VMAType::File;
    const bool is_exec = True(vma_base.prot & MemoryProt::CpuExec);

    if (vma_base.type == VMAType::Free || vma_base.type == VMAType::Pooled) {
        return size_in_vma;
    }

    PAddr phys_base = 0;
    VAddr current_addr = virtual_addr;
    if (vma_base.phys_areas.size() > 0) {
        u64 size_to_free = size_in_vma;
        auto phys_handle = std::prev(vma_base.phys_areas.upper_bound(start_in_vma));
        while (phys_handle != vma_base.phys_areas.end() && size_to_free > 0) {
            // Calculate physical memory offset, address, and size
            u64 dma_offset = std::max<PAddr>(phys_handle->first, start_in_vma) - phys_handle->first;
            PAddr phys_addr = phys_handle->second.base + dma_offset;
            u64 size_in_dma = std::min<u64>(size_to_free, phys_handle->second.size - dma_offset);

            // Create a new dmem area reflecting the pooled region
            if (vma_type == VMAType::Direct) {
                const auto new_dmem_handle = CarvePhysArea(dmem_map, phys_addr, size_in_dma);
                auto& new_dmem_area = new_dmem_handle->second;
                new_dmem_area.dma_type = PhysicalMemoryType::Allocated;

                // Coalesce with nearby direct memory areas.
                MergeAdjacent(dmem_map, new_dmem_handle);
            } else if (vma_type == VMAType::Flexible) {
                // Update fmem_map
                const auto new_fmem_handle = CarvePhysArea(fmem_map, phys_addr, size_in_dma);
                auto& new_fmem_area = new_fmem_handle->second;
                new_fmem_area.dma_type = PhysicalMemoryType::Free;

                // Coalesce with nearby flexible memory areas.
                MergeAdjacent(fmem_map, new_fmem_handle);

                // Zero out the old memory data
                const auto unmap_hardware_address = impl.BackingBase() + phys_addr;
                std::memset(unmap_hardware_address, 0, size_in_dma);

                // Update flexible usage
                flexible_usage -= size_in_dma;
            }

            // Increment through loop
            size_to_free -= size_in_dma;
            phys_handle++;
        }
        ASSERT_MSG(size_to_free == 0, "Failed to unmap physical memory");
    }

    // Mark region as free and attempt to coalesce it with neighbours.
    const auto new_it = CarveVMA(virtual_addr, size_in_vma);
    auto& vma = new_it->second;
    vma.type = VMAType::Free;
    vma.prot = MemoryProt::NoAccess;
    vma.phys_areas.clear();
    vma.disallow_merge = false;
    vma.name = "";
    MergeAdjacent(vma_map, new_it);

    if (vma_type != VMAType::Reserved && vma_type != VMAType::PoolReserved) {
        // Unmap the memory region.
        impl.Unmap(virtual_addr, size_in_vma, has_backing);
        // Tracy memory tracking breaks from merging memory areas. Disabled for now.
        // TRACK_FREE(virtual_addr, "VMEM");

        // If this mapping has GPU access, unmap from GPU.
        if (IsValidGpuMapping(virtual_addr, size)) {
            rasterizer->UnmapMemory(virtual_addr, size);
        }
    }
    return size_in_vma;
}

s32 MemoryManager::UnmapMemoryImpl(VAddr virtual_addr, u64 size) {
    u64 unmapped_bytes = 0;
    do {
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
    std::shared_lock lk{mutex};
    ASSERT_MSG(IsValidMapping(addr), "Attempted to access invalid address {:#x}", addr);

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

s64 MemoryManager::ProtectBytes(VAddr addr, VirtualMemoryArea& vma_base, u64 size,
                                MemoryProt prot) {
    const auto start_in_vma = addr - vma_base.base;
    const auto adjusted_size = std::min<u64>(vma_base.size - start_in_vma, size);
    const MemoryProt old_prot = vma_base.prot;
    const MemoryProt new_prot = prot;

    if (vma_base.type == VMAType::Free || vma_base.type == VMAType::PoolReserved) {
        // On PS4, protecting freed memory does nothing.
        return adjusted_size;
    }

    if (True(prot & MemoryProt::CpuWrite)) {
        // On PS4, read is appended to write mappings.
        prot |= MemoryProt::CpuRead;
    }

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

    if (vma_base.type == VMAType::Direct || vma_base.type == VMAType::Pooled ||
        vma_base.type == VMAType::File) {
        // On PS4, execute permissions are hidden from direct memory and file mappings.
        // Tests show that execute permissions still apply, so handle this after reading perms.
        prot &= ~MemoryProt::CpuExec;
    }

    // Split VMAs and apply protection change.
    const auto new_it = CarveVMA(addr, adjusted_size);
    auto& new_vma = new_it->second;
    new_vma.prot = prot;
    MergeAdjacent(vma_map, new_it);

    if (vma_base.type == VMAType::Reserved) {
        // On PS4, protections change vma_map, but don't apply.
        // Return early to avoid protecting memory that isn't mapped in address space.
        return adjusted_size;
    }

    // Perform address-space memory protections if needed.
    if (new_prot != old_prot) {
        impl.Protect(addr, adjusted_size, perms);
    }

    return adjusted_size;
}

s32 MemoryManager::Protect(VAddr addr, u64 size, MemoryProt prot) {
    // If size is zero, then there's nothing to protect
    if (size == 0) {
        return ORBIS_OK;
    }

    // Ensure the range to modify is valid
    std::scoped_lock lk{mutex};
    ASSERT_MSG(IsValidMapping(addr, size), "Attempted to access invalid address {:#x}", addr);

    // Appropriately restrict flags.
    constexpr static MemoryProt flag_mask =
        MemoryProt::CpuReadWrite | MemoryProt::CpuExec | MemoryProt::GpuReadWrite;
    MemoryProt valid_flags = prot & flag_mask;

    // Protect all VMAs between addr and addr + size.
    s64 protected_bytes = 0;
    while (protected_bytes < size) {
        auto it = FindVMA(addr + protected_bytes);
        auto& vma_base = it->second;
        if (vma_base.base > addr + protected_bytes) {
            // Account for potential gaps in memory map.
            protected_bytes += vma_base.base - (addr + protected_bytes);
        }
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
    // FindVMA on addresses before the vma_map return garbage data.
    auto query_addr =
        addr < impl.SystemManagedVirtualBase() ? impl.SystemManagedVirtualBase() : addr;
    if (addr < query_addr && flags == 0) {
        LOG_WARNING(Kernel_Vmm, "VirtualQuery on free memory region");
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    std::shared_lock lk{mutex};
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
    info->offset = 0;
    info->protection = static_cast<s32>(vma.prot);
    info->is_flexible = vma.type == VMAType::Flexible ? 1 : 0;
    info->is_direct = vma.type == VMAType::Direct ? 1 : 0;
    info->is_stack = vma.type == VMAType::Stack ? 1 : 0;
    info->is_pooled = vma.type == VMAType::PoolReserved || vma.type == VMAType::Pooled ? 1 : 0;
    info->is_committed = vma.IsMapped() ? 1 : 0;
    info->memory_type = 0;
    if (vma.type == VMAType::Direct) {
        // Offset is only assigned for direct mappings.
        ASSERT_MSG(vma.phys_areas.size() > 0, "No physical backing for direct mapping?");
        info->offset = vma.phys_areas.begin()->second.base;
        info->memory_type = vma.phys_areas.begin()->second.memory_type;
    }
    if (vma.type == VMAType::Reserved || vma.type == VMAType::PoolReserved) {
        // Protection is hidden from reserved mappings.
        info->protection = 0;
    }

    strncpy(info->name, vma.name.data(), ::Libraries::Kernel::ORBIS_KERNEL_MAXIMUM_NAME_LENGTH);

    return ORBIS_OK;
}

s32 MemoryManager::DirectMemoryQuery(PAddr addr, bool find_next,
                                     ::Libraries::Kernel::OrbisQueryInfo* out_info) {
    if (addr >= total_direct_size) {
        LOG_WARNING(Kernel_Vmm, "Unable to find allocated direct memory region to query!");
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    std::shared_lock lk{mutex};
    auto dmem_area = FindDmemArea(addr);
    while (dmem_area != dmem_map.end() && dmem_area->second.dma_type == PhysicalMemoryType::Free &&
           find_next) {
        dmem_area++;
    }

    if (dmem_area == dmem_map.end() || dmem_area->second.dma_type == PhysicalMemoryType::Free) {
        LOG_WARNING(Kernel_Vmm, "Unable to find allocated direct memory region to query!");
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    out_info->start = dmem_area->second.base;
    out_info->memoryType = dmem_area->second.memory_type;

    // Loop through all sequential mapped or allocated dmem areas
    // to determine the hardware accurate end.
    while (dmem_area != dmem_map.end() && dmem_area->second.memory_type == out_info->memoryType &&
           (dmem_area->second.dma_type == PhysicalMemoryType::Mapped ||
            dmem_area->second.dma_type == PhysicalMemoryType::Allocated)) {
        out_info->end = dmem_area->second.GetEnd();
        dmem_area++;
    }

    return ORBIS_OK;
}

s32 MemoryManager::DirectQueryAvailable(PAddr search_start, PAddr search_end, u64 alignment,
                                        PAddr* phys_addr_out, u64* size_out) {
    std::shared_lock lk{mutex};

    auto dmem_area = FindDmemArea(search_start);
    PAddr paddr{};
    u64 max_size{};

    while (dmem_area != dmem_map.end()) {
        if (dmem_area->second.dma_type != PhysicalMemoryType::Free) {
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

    ASSERT_MSG(IsValidMapping(addr, size), "Attempted to access invalid address {:#x}", addr);

    // Search through all VMAs covered by the provided range.
    // We aren't modifying these VMAs, so it's safe to iterate through them.
    VAddr current_addr = addr;
    u64 remaining_size = size;
    auto vma_handle = FindVMA(addr);
    while (vma_handle != vma_map.end() && remaining_size > 0) {
        // Calculate position in vma
        const VAddr start_in_vma = current_addr - vma_handle->second.base;
        const u64 size_in_vma =
            std::min<u64>(remaining_size, vma_handle->second.size - start_in_vma);

        // Direct and Pooled mappings are the only ones with a memory type.
        if (vma_handle->second.type == VMAType::Direct ||
            vma_handle->second.type == VMAType::Pooled) {
            // Split area to modify into a new VMA.
            vma_handle = CarveVMA(current_addr, size_in_vma);
            auto phys_handle = vma_handle->second.phys_areas.begin();
            while (phys_handle != vma_handle->second.phys_areas.end()) {
                // Update internal physical areas
                phys_handle->second.memory_type = memory_type;

                // Carve a new dmem area in dmem_map, update memory type there
                auto dmem_handle =
                    CarvePhysArea(dmem_map, phys_handle->second.base, phys_handle->second.size);
                auto& dmem_area = dmem_handle->second;
                dmem_area.memory_type = memory_type;

                // Increment phys_handle
                phys_handle++;
            }

            // Check if VMA can be merged with adjacent areas after physical area modifications.
            vma_handle = MergeAdjacent(vma_map, vma_handle);
        }
        current_addr += size_in_vma;
        remaining_size -= size_in_vma;
        vma_handle++;
    }

    return ORBIS_OK;
}

void MemoryManager::NameVirtualRange(VAddr virtual_addr, u64 size, std::string_view name) {
    std::scoped_lock lk{mutex};

    // Sizes are aligned up to the nearest 16_KB
    u64 aligned_size = Common::AlignUp(size, 16_KB);
    // Addresses are aligned down to the nearest 16_KB
    VAddr aligned_addr = Common::AlignDown(virtual_addr, 16_KB);

    ASSERT_MSG(IsValidMapping(aligned_addr, aligned_size),
               "Attempted to access invalid address {:#x}", aligned_addr);
    auto it = FindVMA(aligned_addr);
    u64 remaining_size = aligned_size;
    VAddr current_addr = aligned_addr;
    while (remaining_size > 0 && it != vma_map.end()) {
        const u64 start_in_vma = current_addr - it->second.base;
        const u64 size_in_vma = std::min<u64>(remaining_size, it->second.size - start_in_vma);
        // Nothing needs to be done to free VMAs
        if (!it->second.IsFree()) {
            if (size_in_vma < it->second.size) {
                it = CarveVMA(current_addr, size_in_vma);
                auto& new_vma = it->second;
                new_vma.name = name;
            } else {
                auto& vma = it->second;
                vma.name = name;
            }
        }
        it = MergeAdjacent(vma_map, it);
        remaining_size -= size_in_vma;
        current_addr += size_in_vma;
        it++;
    }
}

s32 MemoryManager::GetDirectMemoryType(PAddr addr, s32* directMemoryTypeOut,
                                       void** directMemoryStartOut, void** directMemoryEndOut) {
    if (addr >= total_direct_size) {
        LOG_ERROR(Kernel_Vmm, "Unable to find allocated direct memory region to check type!");
        return ORBIS_KERNEL_ERROR_ENOENT;
    }

    std::shared_lock lk{mutex};
    const auto& dmem_area = FindDmemArea(addr)->second;
    if (dmem_area.dma_type == PhysicalMemoryType::Free) {
        LOG_ERROR(Kernel_Vmm, "Unable to find allocated direct memory region to check type!");
        return ORBIS_KERNEL_ERROR_ENOENT;
    }

    *directMemoryStartOut = reinterpret_cast<void*>(dmem_area.base);
    *directMemoryEndOut = reinterpret_cast<void*>(dmem_area.GetEnd());
    *directMemoryTypeOut = dmem_area.memory_type;
    return ORBIS_OK;
}

s32 MemoryManager::IsStack(VAddr addr, void** start, void** end) {
    std::shared_lock lk{mutex};
    ASSERT_MSG(IsValidMapping(addr), "Attempted to access invalid address {:#x}", addr);
    const auto& vma = FindVMA(addr)->second;
    if (vma.IsFree()) {
        mutex.unlock_shared();
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
    std::shared_lock lk{mutex};

    // Run through dmem_map, determine how much physical memory is currently committed
    constexpr u64 block_size = 64_KB;
    u64 committed_size = 0;

    auto dma_handle = dmem_map.begin();
    while (dma_handle != dmem_map.end()) {
        if (dma_handle->second.dma_type == PhysicalMemoryType::Committed) {
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
    // Calculate the minimum and maximum addresses present in our address space.
    auto min_search_address = impl.SystemManagedVirtualBase();
    auto max_search_address = impl.UserVirtualBase() + impl.UserVirtualSize();

    // If the requested address is below the mapped range, start search from the lowest address
    if (virtual_addr < min_search_address) {
        virtual_addr = min_search_address;
    }

    // If the requested address is beyond the maximum our code can handle, throw an assert
    ASSERT_MSG(IsValidMapping(virtual_addr), "Input address {:#x} is out of bounds", virtual_addr);

    // Align up the virtual_addr first.
    virtual_addr = Common::AlignUp(virtual_addr, alignment);
    auto it = FindVMA(virtual_addr);

    // If the VMA is free and contains the requested mapping we are done.
    if (it->second.IsFree() && it->second.Contains(virtual_addr, size)) {
        return virtual_addr;
    }

    // If we didn't hit the return above, then we know the current VMA isn't suitable
    it++;

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

MemoryManager::VMAHandle MemoryManager::MergeAdjacent(VMAMap& handle_map, VMAHandle iter) {
    const auto next_vma = std::next(iter);
    if (next_vma != handle_map.end() && iter->second.CanMergeWith(next_vma->second)) {
        u64 base_offset = iter->second.size;
        iter->second.size += next_vma->second.size;
        for (auto& area : next_vma->second.phys_areas) {
            iter->second.phys_areas[base_offset + area.first] = area.second;
        }
        handle_map.erase(next_vma);
    }

    if (iter != handle_map.begin()) {
        auto prev_vma = std::prev(iter);
        if (prev_vma->second.CanMergeWith(iter->second)) {
            u64 base_offset = prev_vma->second.size;
            prev_vma->second.size += iter->second.size;
            for (auto& area : iter->second.phys_areas) {
                prev_vma->second.phys_areas[base_offset + area.first] = area.second;
            }
            handle_map.erase(iter);
            iter = prev_vma;
        }
    }

    return iter;
}

MemoryManager::PhysHandle MemoryManager::MergeAdjacent(PhysMap& handle_map, PhysHandle iter) {
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

MemoryManager::PhysHandle MemoryManager::CarvePhysArea(PhysMap& map, PAddr addr, u64 size) {
    auto pmem_handle = std::prev(map.upper_bound(addr));
    ASSERT_MSG(addr <= pmem_handle->second.GetEnd(), "Physical address not in map");

    const PhysicalMemoryArea& area = pmem_handle->second;
    ASSERT_MSG(area.base <= addr, "Adding an allocation to already allocated region");

    const PAddr start_in_area = addr - area.base;
    const PAddr end_in_vma = start_in_area + size;
    ASSERT_MSG(end_in_vma <= area.size, "Mapping cannot fit inside free region: size = {:#x}",
               size);

    if (end_in_vma != area.size) {
        // Split VMA at the end of the allocated region
        Split(map, pmem_handle, end_in_vma);
    }
    if (start_in_area != 0) {
        // Split VMA at the start of the allocated region
        pmem_handle = Split(map, pmem_handle, start_in_area);
    }

    return pmem_handle;
}

MemoryManager::VMAHandle MemoryManager::Split(VMAHandle vma_handle, u64 offset_in_vma) {
    auto& old_vma = vma_handle->second;
    ASSERT(offset_in_vma < old_vma.size && offset_in_vma > 0);

    auto new_vma = old_vma;
    old_vma.size = offset_in_vma;
    new_vma.base += offset_in_vma;
    new_vma.size -= offset_in_vma;

    if (HasPhysicalBacking(new_vma)) {
        // Update physical areas map for both areas
        new_vma.phys_areas.clear();

        std::map<uintptr_t, PhysicalMemoryArea> old_vma_phys_areas;
        for (auto& [offset, region] : old_vma.phys_areas) {
            // Fully contained in first VMA
            if (offset + region.size <= offset_in_vma) {
                old_vma_phys_areas[offset] = region;
            }
            // Split between both VMAs
            if (offset < offset_in_vma && offset + region.size > offset_in_vma) {
                // Create region in old VMA
                u64 size_in_old = offset_in_vma - offset;
                old_vma_phys_areas[offset] = PhysicalMemoryArea{
                    region.base, size_in_old, region.memory_type, region.dma_type};
                // Create region in new VMA
                PAddr new_base = region.base + size_in_old;
                u64 size_in_new = region.size - size_in_old;
                new_vma.phys_areas[0] =
                    PhysicalMemoryArea{new_base, size_in_new, region.memory_type, region.dma_type};
            }
            // Fully contained in new VMA
            if (offset >= offset_in_vma) {
                new_vma.phys_areas[offset - offset_in_vma] = region;
            }
        }

        // Set old_vma's physical areas map to the newly created map
        old_vma.phys_areas = old_vma_phys_areas;
    }

    return vma_map.emplace_hint(std::next(vma_handle), new_vma.base, new_vma);
}

MemoryManager::PhysHandle MemoryManager::Split(PhysMap& map, PhysHandle phys_handle,
                                               u64 offset_in_area) {
    auto& old_area = phys_handle->second;
    ASSERT(offset_in_area < old_area.size && offset_in_area > 0);

    auto new_area = old_area;
    old_area.size = offset_in_area;
    new_area.memory_type = old_area.memory_type;
    new_area.base += offset_in_area;
    new_area.size -= offset_in_area;

    return map.emplace_hint(std::next(phys_handle), new_area.base, new_area);
}

} // namespace Core
