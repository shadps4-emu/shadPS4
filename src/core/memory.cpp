// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/debug.h"
#include "common/elf_info.h"
#include "core/emulator_settings.h"
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

    // Pre-initialize direct backing
    auto total_size = ORBIS_KERNEL_TOTAL_MEM_DEV_PRO;
    s32 extra_dmem = EmulatorSettings.GetExtraDmemInMBytes();
    if (extra_dmem != 0) {
        total_size += extra_dmem * 1_MB;
    }
    total_direct_size = total_size;
    dmem_map.clear();
    dmem_map.emplace(0, PhysicalMemoryArea{0, total_direct_size});

    // Pre-initialize flexible backing
    total_flexible_size = ORBIS_KERNEL_FLEXIBLE_MEMORY_SIZE;
    fmem_map.clear();
    fmem_map.emplace(total_size, PhysicalMemoryArea{total_size, total_flexible_size});

    ASSERT_MSG(Libraries::Kernel::sceKernelGetCompiledSdkVersion(&sdk_version) == 0,
               "Failed to get compiled SDK version");
}

MemoryManager::~MemoryManager() = default;

void MemoryManager::SetupMemoryRegions(u64 flexible_size, bool use_extended_mem1,
                                       bool use_extended_mem2) {
    // Calculate actual direct and flexible memory sizes
    const bool is_neo = ::Libraries::Kernel::sceKernelIsNeoMode();
    auto total_size = is_neo ? ORBIS_KERNEL_TOTAL_MEM_PRO : ORBIS_KERNEL_TOTAL_MEM;
    if (EmulatorSettings.IsDevKit()) {
        total_size = is_neo ? ORBIS_KERNEL_TOTAL_MEM_DEV_PRO : ORBIS_KERNEL_TOTAL_MEM_DEV;
    }
    s32 extra_dmem = EmulatorSettings.GetExtraDmemInMBytes();
    if (extra_dmem != 0) {
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

    // Update stored totals
    total_flexible_size = flexible_size - ORBIS_KERNEL_FLEXIBLE_MEMORY_BASE;
    ASSERT_MSG(total_flexible_size >= flexible_usage, "Unable to shrink flexible memory size");
    u64 old_direct_size = total_direct_size;
    total_direct_size = total_size - flexible_size;

    // Limit direct memory space to match actual limit
    auto last_dmem_area = FindDmemArea(total_direct_size);
    ASSERT_MSG(last_dmem_area->second.dma_type == PhysicalMemoryType::Free &&
                   last_dmem_area->second.size >= old_direct_size - total_direct_size,
               "Unable to shrink dmem map");
    last_dmem_area->second.size -= (old_direct_size - total_direct_size);

    LOG_INFO(Kernel_Vmm, "Configured memory regions: flexible size = {:#x}, direct size = {:#x}",
             total_flexible_size, total_direct_size);
}

bool MemoryManager::IsValidMapping(const VAddr virtual_addr, const u64 size) {
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

    return true;
}

u64 MemoryManager::ClampRangeSize(VAddr virtual_addr, u64 size) {
    static constexpr u64 MinSizeToClamp = 1_GB;
    // Dont bother with clamping if the size is small so we dont pay a map lookup on every buffer.
    if (size < MinSizeToClamp) {
        return size;
    }

    std::shared_lock lk{mutex};
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
    std::shared_lock lk{mutex};
    ASSERT_MSG(IsValidMapping(virtual_addr), "Attempted to access invalid address {:#x}",
               virtual_addr);

    auto vma = FindVMA(virtual_addr);
    while (size) {
        const u64 copy_size = std::min<u64>(vma->second.size - (virtual_addr - vma->first), size);
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

    std::scoped_lock lk{mutex};

    // Release any dmem mappings that reference this physical block.
    std::vector<std::pair<VAddr, u64>> remove_list;
    for (const auto& [addr, mapping] : vma_map) {
        if (mapping.type != VMAType::Direct) {
            continue;
        }
        if (mapping.phys_base <= phys_addr && phys_addr < mapping.phys_base + mapping.size) {
            const auto vma_start_offset = phys_addr - mapping.phys_base;
            const auto addr_in_vma = mapping.base + vma_start_offset;
            const auto size_in_vma =
                mapping.size - vma_start_offset > size ? size : mapping.size - vma_start_offset;

            LOG_INFO(Kernel_Vmm, "Unmaping direct mapping {:#x} with size {:#x}", addr_in_vma,
                     size_in_vma);
            // Unmaping might erase from vma_map. We can't do it here.
            remove_list.emplace_back(addr_in_vma, size_in_vma);
        }
    }
    for (const auto& [addr, size] : remove_list) {
        UnmapMemoryImpl(addr, size);
    }

    // Unmap all dmem areas within this area.
    auto phys_addr_to_search = phys_addr;
    auto remaining_size = size;
    auto dmem_area = FindDmemArea(phys_addr);
    while (dmem_area != dmem_map.end() && remaining_size > 0) {
        // Carve a free dmem area in place of this one.
        const auto start_phys_addr =
            phys_addr > dmem_area->second.base ? phys_addr : dmem_area->second.base;
        const auto offset_in_dma = start_phys_addr - dmem_area->second.base;
        const auto size_in_dma = dmem_area->second.size - offset_in_dma > remaining_size
                                     ? remaining_size
                                     : dmem_area->second.size - offset_in_dma;
        const auto dmem_handle = CarvePhysArea(dmem_map, start_phys_addr, size_in_dma);
        auto& new_dmem_area = dmem_handle->second;
        new_dmem_area.dma_type = PhysicalMemoryType::Free;
        new_dmem_area.memory_type = 0;

        // Merge the new dmem_area with dmem_map
        MergeAdjacent(dmem_map, dmem_handle);

        // Get the next relevant dmem area.
        phys_addr_to_search = phys_addr + size_in_dma;
        remaining_size -= size_in_dma;
        dmem_area = FindDmemArea(phys_addr_to_search);
    }

    return ORBIS_OK;
}

s32 MemoryManager::PoolCommit(VAddr virtual_addr, u64 size, MemoryProt prot, s32 mtype) {
    std::unique_lock lk{mutex};

    auto& vma = FindVMA(virtual_addr)->second;
    if (vma.type != VMAType::Pooled) {
        // If we're attempting to commit non-pooled memory, return EINVAL
        LOG_ERROR(Kernel_Vmm, "Attempting to commit non-pooled memory at {:#x}", virtual_addr);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (!vma.Contains(virtual_addr, size)) {
        // If there's not enough space to commit, return EINVAL
        LOG_ERROR(Kernel_Vmm,
                  "Pooled region {:#x} to {:#x} is not large enough to commit from {:#x} to {:#x}",
                  vma.base, vma.base + vma.size, virtual_addr, virtual_addr + size);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (size == 0) {
        return ORBIS_OK;
    }

    if (True(prot & MemoryProt::CpuWrite)) {
        // On PS4, read is appended to write mappings.
        prot |= MemoryProt::CpuRead;
    }

    // Allocate needed physical blocks from the pool
    const bool onion = mtype == 0;
    const bool writeback = mtype != 3;
    const u32 block_start = Blockpool::ToBlocks(virtual_addr - vma.base);
    const u32 num_blocks = Blockpool::ToBlocks(size);
    const u32 block_end = block_start + num_blocks;

    DmemBlock* vm_blocks = vma.blocks.data() + block_start;
    std::array<u32, 32> dmem_blocks;
    dmem_blocks.fill(std::numeric_limits<u32>::max());

    for (u32 chunk = 0; chunk < num_blocks; chunk += 32) {
        const u32 count = std::min(num_blocks - chunk, 32u);

        if (!blockpool.Commit(count, onion, writeback, dmem_blocks.data())) {
            // We run out of physical memory, revert our changes and error out
            blockpool.Decommit(vm_blocks, chunk);
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }

        // Assign physical blocks to virtual memory
        for (u32 i = 0; i < count; ++i) {
            ASSERT_MSG(dmem_blocks[i] != std::numeric_limits<u32>::max());
            vm_blocks[chunk + i] = DmemBlock{
                .block = dmem_blocks[i],
                .onion = onion,
                .writeback = writeback,
                .prot_cpu = u32(prot & MemoryProt::CpuReadWrite),
                .prot_gpu = u32(prot & MemoryProt::GpuReadWrite) >> 4,
                .valid = true,
            };
        }
    }

    u32 block{};
    u32 pending_blocks{};

    const auto map_memory = [&] {
        const u32 start_block = block - pending_blocks + 1;
        impl.Map(virtual_addr + Blockpool::ToBytes(start_block), Blockpool::ToBytes(pending_blocks),
                 Blockpool::ToBytes(vm_blocks[start_block].block));
        pending_blocks = 0;
    };

    // Defer mapping of physical pages to make failure case simpler
    for (; block < num_blocks; ++block) {
        ++pending_blocks;
        if (block != num_blocks - 1 && (vm_blocks[block + 1].block - vm_blocks[block].block) != 1) {
            map_memory();
        }
    }
    if (pending_blocks) {
        --block;
        map_memory();
    }

    lk.unlock();

    if (IsValidGpuMapping(virtual_addr, size)) {
        rasterizer->MapMemory(virtual_addr, size);
    }

    return ORBIS_OK;
}

s32 MemoryManager::PoolDecommit(VAddr virtual_addr, u64 size) {
    std::unique_lock lk{mutex};

    auto& vma = FindVMA(virtual_addr)->second;
    if (vma.type != VMAType::Pooled) {
        // If we're attempting to decommit non-pooled memory, return EINVAL
        LOG_ERROR(Kernel_Vmm, "Attempting to decommit non-pooled memory at {:#x}", virtual_addr);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (!vma.Contains(virtual_addr, size)) {
        // If there's not enough space to decommit, return EINVAL
        LOG_ERROR(
            Kernel_Vmm,
            "Pooled region {:#x} to {:#x} is not large enough to decommit from {:#x} to {:#x}",
            vma.base, vma.base + vma.size, virtual_addr, virtual_addr + size);
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (size == 0) {
        return ORBIS_OK;
    }

    lk.unlock();

    // Perform early GPU unmap to avoid potential deadlocks
    if (IsValidGpuMapping(virtual_addr, size)) {
        rasterizer->UnmapMemory(virtual_addr, size);
    }

    lk.lock();

    const u32 block_start = Blockpool::ToBlocks(virtual_addr - vma.base);
    const u32 num_blocks = Blockpool::ToBlocks(size);
    const u32 block_end = block_start + num_blocks;
    DmemBlock* vm_blocks = vma.blocks.data() + block_start;

    for (u32 chunk = 0; chunk < num_blocks; chunk += 2048) {
        const u32 count = std::min(num_blocks - chunk, 2048u);
        blockpool.Decommit(vm_blocks + chunk, count);
    }

    impl.Unmap(virtual_addr, size);

    return ORBIS_OK;
}

MemoryManager::VMAHandle MemoryManager::CreateArea(VAddr virtual_addr, u64 size, MemoryProt prot,
                                                   MemoryMapFlags flags, VMAType type,
                                                   std::string_view name, u64 alignment) {
    // Locate the VMA representing the requested region
    auto vma = FindVMA(virtual_addr)->second;
    if (True(flags & MemoryMapFlags::Fixed)) {
        // If fixed is specified, map directly to the region of virtual_addr + size.
        // Callers should check to ensure the NoOverwrite flag is handled appropriately beforehand.
        auto unmap_addr = virtual_addr;
        auto unmap_size = size;
        while (unmap_size > 0) {
            auto unmapped = UnmapBytesFromEntry(unmap_addr, vma, unmap_size);
            unmap_addr += unmapped;
            unmap_size -= unmapped;
            vma = FindVMA(unmap_addr)->second;
        }
    }
    vma = FindVMA(virtual_addr)->second;

    // By this point, vma should be free and ready to map.
    // Caller performs address searches for non-fixed mappings before this.
    ASSERT_MSG(vma.IsFree(), "VMA to map is not free");

    // Create a memory area representing this mapping.
    const auto new_vma_handle = CarveVMA(virtual_addr, size);
    auto& new_vma = new_vma_handle->second;
    const bool is_exec = True(prot & MemoryProt::CpuExec);
    if (True(prot & MemoryProt::CpuWrite)) {
        // On PS4, read is appended to write mappings.
        prot |= MemoryProt::CpuRead;
    }

    // Update VMA appropriately.
    new_vma.disallow_merge = True(flags & MemoryMapFlags::NoCoalesce);
    new_vma.prot = prot;
    new_vma.name = name;
    new_vma.type = type;
    new_vma.phys_base = 0;
    if (new_vma.type == VMAType::Pooled) {
        new_vma.blocks.resize(Blockpool::ToBlocks(size));
    }
    return new_vma_handle;
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

    std::unique_lock lk{mutex};

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

    if (True(flags & MemoryMapFlags::Fixed) && True(flags & MemoryMapFlags::NoOverwrite)) {
        // Perform necessary error checking for Fixed & NoOverwrite case
        ASSERT_MSG(IsValidMapping(virtual_addr, size), "Attempted to access invalid address {:#x}",
                   virtual_addr);
        auto vma = FindVMA(virtual_addr)->second;
        auto remaining_size = vma.base + vma.size - virtual_addr;
        if (!vma.IsFree() || remaining_size < size) {
            LOG_ERROR(Kernel_Vmm, "Unable to map {:#x} bytes at address {:#x}", size, virtual_addr);
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }
    } else if (False(flags & MemoryMapFlags::Fixed)) {
        // Find a free virtual addr to map
        alignment = alignment > 0 ? alignment : 16_KB;
        virtual_addr = virtual_addr == 0 ? DEFAULT_MAPPING_BASE : virtual_addr;
        virtual_addr = SearchFree(virtual_addr, size, alignment);
        if (virtual_addr == -1) {
            // No suitable memory areas to map to
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }
    }

    lk.unlock();

    // Perform early GPU unmap to avoid potential deadlocks
    if (IsValidGpuMapping(virtual_addr, size)) {
        rasterizer->UnmapMemory(virtual_addr, size);
    }

    lk.lock();

    // Create VMA representing this mapping.
    auto new_vma_handle = CreateArea(virtual_addr, size, prot, flags, type, name, alignment);
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
            new_vma.phys_base = handle->second.base;

            // Perform an address space mapping for each physical area
            void* out_addr = impl.Map(current_addr, size_to_map, new_fmem_area.base, is_exec);
            // Tracy memory tracking breaks from merging memory areas. Disabled for now.
            // TRACK_ALLOC(out_addr, size_to_map, "VMEM");

            // Merge this handle with adjacent areas
            handle = MergeAdjacent(fmem_map, new_fmem_handle);

            // Get the next flexible area.
            current_addr += size_to_map;
            remaining_size -= size_to_map;
            flexible_usage += size_to_map;
            handle++;
            ASSERT_MSG(remaining_size == 0, "Failed to map physical memory");
            break;
        }
    } else if (type == VMAType::Direct) {
        // Map the physical memory for this direct memory mapping.
        auto current_phys_addr = phys_addr;
        u64 remaining_size = size;
        auto dmem_area = FindDmemArea(phys_addr);
        // Carve a new dmem area in place of this one with the appropriate type.
        // Ensure the carved area only covers the current dmem area.
        const auto start_phys_addr = std::max<PAddr>(current_phys_addr, dmem_area->second.base);
        const auto offset_in_dma = start_phys_addr - dmem_area->second.base;
        const auto size_in_dma =
            std::min<u64>(dmem_area->second.size - offset_in_dma, remaining_size);
        const auto dmem_handle = CarvePhysArea(dmem_map, start_phys_addr, size_in_dma);
        auto& new_dmem_area = dmem_handle->second;
        new_dmem_area.dma_type = PhysicalMemoryType::Mapped;

        // Add the dmem area to this vma, merge it with any similar tracked areas.
        const u64 offset_in_vma = current_phys_addr - phys_addr;
        new_vma.phys_base = start_phys_addr;

        // Merge the new dmem_area with dmem_map
        MergeAdjacent(dmem_map, dmem_handle);

        // Get the next relevant dmem area.
        current_phys_addr += size_in_dma;
        remaining_size -= size_in_dma;
        dmem_area = FindDmemArea(current_phys_addr);
        ASSERT_MSG(remaining_size == 0, "Failed to map physical memory");
    }

    if (new_vma.type != VMAType::Direct || sdk_version >= Common::ElfInfo::FW_20) {
        // Merge this VMA with similar nearby areas
        // Direct memory mappings only coalesce on SDK version 2.00 or later.
        MergeAdjacent(vma_map, new_vma_handle);
    }

    *out_addr = std::bit_cast<void*>(mapped_addr);
    if (type != VMAType::Reserved && type != VMAType::Pooled) {
        // Flexible address space mappings were performed while finding direct memory areas.
        if (type != VMAType::Flexible) {
            impl.Map(mapped_addr, size, phys_addr, is_exec);
            // Tracy memory tracking breaks from merging memory areas. Disabled for now.
            // TRACK_ALLOC(mapped_addr, size, "VMEM");
        }

        lk.unlock();

        // If this is not a reservation, then map to GPU and address space
        if (IsValidGpuMapping(mapped_addr, size)) {
            rasterizer->MapMemory(mapped_addr, size);
        }
    }

    return ORBIS_OK;
}

s32 MemoryManager::MapFile(void** out_addr, VAddr virtual_addr, u64 size, MemoryProt prot,
                           MemoryMapFlags flags, s32 fd, s64 phys_addr) {
    uintptr_t handle = 0;

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

    handle = file->f.GetFileMapping();

    if (False(file->f.GetAccessMode() & Common::FS::FileAccessMode::Write) ||
        False(file->f.GetAccessMode() & Common::FS::FileAccessMode::Append)) {
        // If the file does not have write access, ensure prot does not contain write
        // permissions. On real hardware, these mappings succeed, but the memory cannot be
        // written to.
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

    if (True(flags & MemoryMapFlags::Fixed) && False(flags & MemoryMapFlags::NoOverwrite)) {
        std::shared_lock lk{mutex};
        ASSERT_MSG(IsValidMapping(virtual_addr, size), "Attempted to access invalid address {:#x}",
                   virtual_addr);
        auto vma = FindVMA(virtual_addr)->second;

        auto remaining_size = vma.base + vma.size - virtual_addr;
        if (!vma.IsFree() || remaining_size < size) {
            LOG_ERROR(Kernel_Vmm, "Unable to map {:#x} bytes at address {:#x}", size, virtual_addr);
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }
    } else if (False(flags & MemoryMapFlags::Fixed)) {
        std::shared_lock lk{mutex};
        virtual_addr = virtual_addr == 0 ? DEFAULT_MAPPING_BASE : virtual_addr;
        virtual_addr = SearchFree(virtual_addr, size, 16_KB);
        if (virtual_addr == -1) {
            // No suitable memory areas to map to
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }
    }

    // Perform early GPU unmap to avoid potential deadlocks
    if (IsValidGpuMapping(virtual_addr, size)) {
        rasterizer->UnmapMemory(virtual_addr, size);
    }

    // Aquire writer lock
    std::scoped_lock lk2{mutex};

    // Update VMA map and map to address space.
    auto new_vma_handle = CreateArea(virtual_addr, size, prot, flags, VMAType::File, "anon", 0);

    auto& new_vma = new_vma_handle->second;
    new_vma.fd = fd;
    auto mapped_addr = new_vma.base;
    bool is_exec = True(prot & MemoryProt::CpuExec);

    impl.MapFile(mapped_addr, size, phys_addr, std::bit_cast<u32>(prot), handle);

    *out_addr = std::bit_cast<void*>(mapped_addr);
    return ORBIS_OK;
}

s32 MemoryManager::UnmapMemory(VAddr virtual_addr, u64 size) {
    if (size == 0) {
        return ORBIS_OK;
    }

    // Align address and size appropriately
    virtual_addr = Common::AlignDown(virtual_addr, 16_KB);
    size = Common::AlignUp(size, 16_KB);
    ASSERT_MSG(IsValidMapping(virtual_addr, size), "Attempted to access invalid address {:#x}",
               virtual_addr);

    // If the requested range has GPU access, unmap from GPU.
    if (IsValidGpuMapping(virtual_addr, size)) {
        rasterizer->UnmapMemory(virtual_addr, size);
    }

    // Acquire writer lock.
    std::scoped_lock lk2{mutex};
    return UnmapMemoryImpl(virtual_addr, size);
}

u64 MemoryManager::UnmapBytesFromEntry(VAddr virtual_addr, const VirtualMemoryArea& vma_base,
                                       u64 size) {
    const auto type = vma_base.type;
    const VAddr start_in_vma = virtual_addr - vma_base.base;
    const u64 size_in_vma = std::min<u64>(vma_base.size - start_in_vma, size);

    if (type == VMAType::Free) {
        return size_in_vma;
    }

    if (type == VMAType::Direct) {
        // Unmap all direct memory areas covered by this unmap.
        auto phys_addr = vma_base.phys_base + start_in_vma;
        auto remaining_size = size_in_vma;
        PhysHandle dmem_handle = FindDmemArea(phys_addr);
        while (dmem_handle != dmem_map.end() && remaining_size > 0) {
            const auto start_in_dma = phys_addr - dmem_handle->second.base;
            const auto size_in_dma = dmem_handle->second.size - start_in_dma > remaining_size
                                         ? remaining_size
                                         : dmem_handle->second.size - start_in_dma;
            dmem_handle = CarvePhysArea(dmem_map, phys_addr, size_in_dma);
            auto& dmem_area = dmem_handle->second;
            dmem_area.dma_type = PhysicalMemoryType::Allocated;
            remaining_size -= dmem_area.size;
            phys_addr += dmem_area.size;

            // Check if we can coalesce any dmem areas.
            MergeAdjacent(dmem_map, dmem_handle);
            dmem_handle = FindDmemArea(phys_addr);
        }
    }

    if (type == VMAType::Flexible) {
        flexible_usage -= size_in_vma;

        // Now that there is a physical backing used for flexible memory,
        // manually erase the contents before unmapping to prevent possible issues.
        const auto unmap_hardware_address = impl.BackingBase() + vma_base.phys_base + start_in_vma;
        std::memset(unmap_hardware_address, 0, size_in_vma);

        // Address space unmap needs the physical_base from the start of the vma,
        // so calculate the phys_base to unmap from here.
        const auto unmap_phys_base = vma_base.phys_base + start_in_vma;
        const auto new_fmem_handle = CarvePhysArea(fmem_map, unmap_phys_base, size_in_vma);
        auto& new_fmem_area = new_fmem_handle->second;
        new_fmem_area.dma_type = PhysicalMemoryType::Free;
        MergeAdjacent(fmem_map, new_fmem_handle);
    }

    // Mark region as free and attempt to coalesce it with neighbours.
    const auto new_it = CarveVMA(virtual_addr, size_in_vma);
    auto& vma = new_it->second;
    vma.type = VMAType::Free;
    vma.prot = MemoryProt::NoAccess;
    vma.phys_base = 0;
    vma.disallow_merge = false;
    vma.name = "";
    MergeAdjacent(vma_map, new_it);

    if (type != VMAType::Reserved) {
        // If this mapping has GPU access, unmap from GPU.
        if (IsValidGpuMapping(virtual_addr, size)) {
            rasterizer->UnmapMemory(virtual_addr, size);
        }
        // Unmap the memory region.
        impl.Unmap(virtual_addr, size_in_vma);
        TRACK_FREE(virtual_addr, "VMEM");
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
    VAddr min_query_addr = impl.SystemManagedVirtualBase();
    if (addr < min_query_addr) {
        LOG_ERROR(Kernel_Vmm, "Address {:#x} is not mapped", addr);
        return ORBIS_KERNEL_ERROR_EACCES;
    }

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

    if (vma_base.type == VMAType::Free) {
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
        query_addr = it->first;
    }
    if (it == vma_map.end() || it->second.type == VMAType::Free) {
        LOG_WARNING(Kernel_Vmm, "VirtualQuery on free memory region");
        return ORBIS_KERNEL_ERROR_EACCES;
    }

    const auto& vma = it->second;

    memset(info, 0, sizeof(*info));
    strncpy(info->name, vma.name.data(), ::Libraries::Kernel::ORBIS_KERNEL_MAXIMUM_NAME_LENGTH);

    if (vma.type == VMAType::Pooled) {
        blockpool.Query(query_addr, vma.base, vma.size, vma.blocks.data(), info);
        return ORBIS_OK;
    }

    info->start = vma.base;
    info->end = vma.base + vma.size;
    info->offset = 0;
    info->protection = static_cast<s32>(vma.prot);
    info->is_flexible = vma.type == VMAType::Flexible ? 1 : 0;
    info->is_direct = vma.type == VMAType::Direct ? 1 : 0;
    info->is_stack = vma.type == VMAType::Stack ? 1 : 0;
    info->is_committed = vma.IsMapped() ? 1 : 0;
    info->memory_type = ::Libraries::Kernel::ORBIS_KERNEL_WB_ONION;
    if (vma.type == VMAType::Direct) {
        // Offset is only assigned for direct mappings.
        info->offset = vma.phys_base;
        const auto dmem_it = FindDmemArea(vma.phys_base);
        ASSERT_MSG(vma.phys_base <= dmem_it->second.GetEnd(), "vma.phys_base is not in dmem_map!");
        info->memory_type = dmem_it->second.memory_type;
    }
    if (vma.type == VMAType::Reserved) {
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
    auto remaining_size = size;
    auto vma_handle = FindVMA(addr);
    while (vma_handle != vma_map.end() && vma_handle->second.base < addr + size) {
        if (vma_handle->second.type == VMAType::Pooled) {
            LOG_WARNING(Kernel_Vmm, "Setting direct memory type on a pooled VMA is not supported");
            continue;
        }
        // Direct and Pooled mappings are the only ones with a memory type.
        if (vma_handle->second.type == VMAType::Direct) {
            // Calculate position in vma
            const auto start_in_vma = addr - vma_handle->second.base;
            const auto size_in_vma = vma_handle->second.size - start_in_vma;
            auto phys_addr = vma_handle->second.phys_base + start_in_vma;
            auto size_to_modify = remaining_size > size_in_vma ? size_in_vma : remaining_size;

            // Loop through remaining dmem areas until the physical addresses represented
            // are all adjusted.
            PhysHandle dmem_handle = FindDmemArea(phys_addr);
            while (dmem_handle != dmem_map.end() && size_in_vma >= size_to_modify &&
                   size_to_modify > 0) {
                const auto start_in_dma = phys_addr - dmem_handle->second.base;
                const auto size_in_dma = dmem_handle->second.size - start_in_dma > size_to_modify
                                             ? size_to_modify
                                             : dmem_handle->second.size - start_in_dma;
                dmem_handle = CarvePhysArea(dmem_map, phys_addr, size_in_dma);
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
        remaining_size -= size_in_vma;
        current_addr += size_in_vma;

        // Check if VMA can be merged with adjacent areas after modifications.
        it = MergeAdjacent(vma_map, it);
        if (it->second.base + it->second.size <= current_addr) {
            // If we're now in the next VMA, then go to the next handle.
            it++;
        }
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

    if (new_vma.HasPhysicalBacking()) {
        new_vma.phys_base += offset_in_vma;
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
