// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <utility>
#include "core/libraries/kernel/memory.h"
#include "core/memory/address_space.h"
#include "core/memory/blockpool.h"
#include "core/memory/dmem_map.h"
#include "core/memory/vm_map.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

namespace Core {

bool Blockpool::Map(VmMapEntry& entry, VAddr addr, u64 size, MemoryProt prot, s32 mtype) {
    const bool onion = mtype == 0;
    const bool writeback = mtype != 3;
    const u32 block_start = ToBlocks(addr - entry.start);
    const u32 num_blocks = ToBlocks(size);
    const u32 block_end = block_start + num_blocks;

    DmemBlock* vm_blocks = entry.object->blockpool.blocks.data() + block_start;
    std::array<u32, 32> dmem_blocks;
    dmem_blocks.fill(std::numeric_limits<u32>::max());

    for (u32 chunk = 0; chunk < num_blocks; chunk += 32) {
        const u32 count = std::min(num_blocks - chunk, 32u);

        if (!Commit(count, onion, writeback, dmem_blocks.data())) {
            // We run out of physical memory, revert our changes and error out
            Decommit(vm_blocks, chunk);
            return false;
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
        impl.Map(addr + ToBytes(start_block), ToBytes(pending_blocks),
                 ToBytes(vm_blocks[start_block].block));
        if (rasterizer) {
            rasterizer->RegisterMemory(addr + ToBytes(start_block), ToBytes(pending_blocks));
        }
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

    if (entry.IsGpuMapping()) {
        rasterizer->MapMemory(addr, size);
    }

    return true;
}

void Blockpool::Unmap(VmMapEntry& entry, VAddr addr, u64 size) {
    const u32 block_start = ToBlocks(addr - entry.start);
    const u32 num_blocks = ToBlocks(size);
    DmemBlock* vm_blocks = entry.object->blockpool.blocks.data() + block_start;

    for (u32 chunk = 0; chunk < num_blocks; chunk += 2048) {
        const u32 count = std::min(num_blocks - chunk, 2048u);
        Decommit(vm_blocks + chunk, count);
    }

    if (entry.IsGpuMapping()) {
        rasterizer->UnmapMemory(addr, size);
    }

    u64 size_to_unmap = size;
    const VAddr unmapped_addr = impl.Unmap(addr, &size_to_unmap);
    if (rasterizer) {
        rasterizer->RegisterMemory(unmapped_addr, size_to_unmap);
    }
}

static void ProtectCommittedRuns(AddressSpace& impl, VAddr base, const DmemBlock* blocks,
                                 u64 start_block, u64 end_block, MemoryProt prot) {
    u64 block = start_block;
    while (block < end_block) {
        if (!blocks[block].valid) {
            ++block;
            continue;
        }
        const u64 run_start = block;
        while (block < end_block && blocks[block].valid) {
            ++block;
        }
        impl.Protect(base + Blockpool::ToBytes(run_start), Blockpool::ToBytes(block - run_start),
                     ToHostPerm(prot));
    }
}

static u32 BlockCpuProt(MemoryProt prot) {
    const u32 raw = static_cast<u32>(prot);
    return (raw & 3) | ((raw >> 1) & 1);
}

static u32 BlockGpuProt(MemoryProt prot) {
    return (static_cast<u32>(prot) & 0x30) >> 4;
}

s32 Blockpool::Protect(VmMapEntry& entry, u64 start_block, u64 end_block, MemoryProt prot) {
    DmemBlock* blocks = entry.object->blockpool.blocks.data();

    // WB Garlic blocks cannot be made writable.
    if (True(prot & (MemoryProt::CpuWrite | MemoryProt::GpuWrite))) {
        for (u64 block = start_block; block < end_block; ++block) {
            if (blocks[block].writeback && !blocks[block].onion) {
                return ORBIS_KERNEL_ERROR_EACCES;
            }
        }
    }

    for (u64 block = start_block; block < end_block; ++block) {
        if (blocks[block].valid) {
            blocks[block].prot_cpu = BlockCpuProt(prot);
            blocks[block].prot_gpu = BlockGpuProt(prot);
        }
    }
    ProtectCommittedRuns(impl, entry.start - entry.offset, blocks, start_block, end_block, prot);
    return ORBIS_OK;
}

void Blockpool::TypeProtect(VmMapEntry& entry, u64 start_block, u64 end_block, DmemMemoryType mtype,
                            MemoryProt prot) {
    DmemBlock* blocks = entry.object->blockpool.blocks.data();
    const bool writeback = mtype != DmemMemoryType::WcGarlic;
    const bool onion = writeback && mtype != DmemMemoryType::WbGarlic;

    s32 cached_delta = 0;
    for (u64 block = start_block; block < end_block; ++block) {
        if (blocks[block].valid && bool(blocks[block].writeback) != writeback) {
            cached_delta += writeback ? 1 : -1;
        }
    }
    {
        std::scoped_lock lk{mutex};
        cached.allocated_blocks += cached_delta;
        flushed.allocated_blocks -= cached_delta;
    }

    for (u64 block = start_block; block < end_block; ++block) {
        if (blocks[block].valid) {
            blocks[block].onion = onion;
            blocks[block].writeback = writeback;
            blocks[block].prot_cpu = BlockCpuProt(prot);
            blocks[block].prot_gpu = BlockGpuProt(prot);
        }
    }
    ProtectCommittedRuns(impl, entry.start - entry.offset, blocks, start_block, end_block, prot);
}

bool Blockpool::Commit(u32 count, bool onion, bool writeback, u32* out_blocks) {
    std::scoped_lock lk{mutex};

    if (count > (cached.available_blocks + flushed.available_blocks)) {
        return false;
    }

    while (count) {
        if (onion) {
            const u32 cached_available = cached.available_blocks;
            if (count <= cached_available) {
                cached.Allocate(count, out_blocks);
            } else {
                const u32 flushed_count = count - cached_available;
                cached.Allocate(cached_available, out_blocks);
                flushed.Allocate(flushed_count, out_blocks + cached_available);
            }
            break;
        } else {
            if (count <= flushed.available_blocks) {
                flushed.Allocate(count, out_blocks);
                break;
            }
            if (cached.available_blocks < 32) {
                const u32 cached_count = count - flushed.available_blocks;
                flushed.Allocate(flushed.allocated_blocks, out_blocks);
                cached.Allocate(cached_count, out_blocks + flushed.allocated_blocks);
                break;
            }
        }
        Flush();
    }

    if (writeback) {
        cached.allocated_blocks += count;
    } else {
        flushed.allocated_blocks += count;
    }

    return true;
}

void Blockpool::Decommit(DmemBlock* blocks, u32 count) {
    std::scoped_lock lk{mutex};

    while (count) {
        Bitmap* bitmap;
        if (blocks->writeback) {
            --cached.allocated_blocks;
            ++cached.available_blocks;
            bitmap = &cached;
        } else {
            --flushed.allocated_blocks;
            ++flushed.available_blocks;
            bitmap = &flushed;
        }
        u32 block = blocks->block;
        bitmap->bits_l0[block >> Bitmap::LEVEL_SHIFT] |= (u64{1} << (block & Bitmap::LEVEL_MASK));
        block >>= Bitmap::LEVEL_SHIFT;
        bitmap->bits_l1[block >> Bitmap::LEVEL_SHIFT] |= (u64{1} << (block & Bitmap::LEVEL_MASK));
        block >>= Bitmap::LEVEL_SHIFT;
        bitmap->bits_l2 |= (u64{1} << block);

        blocks->raw = 0u;
        ++blocks;
        --count;
    }
}

void Blockpool::Flush() {
    for (u32 i = 0; i < flushed.bits_l0.size(); i++) {
        flushed.bits_l0[i] |= std::exchange(cached.bits_l0[i], u64{0});
    }
    for (u32 i = 0; i < flushed.bits_l1.size(); i++) {
        flushed.bits_l1[i] |= std::exchange(cached.bits_l1[i], u64{0});
    }
    flushed.bits_l2 |= std::exchange(cached.bits_l2, u64{0});
}

void Blockpool::Query(VAddr addr, VAddr base, u64 size, const DmemBlock* blocks,
                      ::Libraries::Kernel::OrbisVirtualQueryInfo* info) {
    VAddr start = base;
    VAddr end = base + size;
    const auto next_name = blockpool_names.upper_bound(addr);
    bool in_name = false;
    if (next_name != blockpool_names.begin()) {
        const auto& prev = std::prev(next_name)->second;
        in_name = addr < prev.end;
        if (in_name) {
            start = std::max(start, prev.start);
            end = std::min(end, prev.end);
        } else {
            start = std::max(start, prev.end);
        }
    }
    if (!in_name && next_name != blockpool_names.end()) {
        end = std::min(end, next_name->second.start);
    }
    const u64 block_min = Blockpool::ToBlocks(start - base);
    const u64 block_max = Blockpool::ToBlocks(end - base);
    const u64 query_block = Blockpool::ToBlocks(addr - base);
    const auto vm_block = blocks[query_block];
    u64 start_block = query_block;
    u64 end_block = query_block + 1;
    while (start_block > block_min && blocks[start_block - 1].Props() == vm_block.Props()) {
        --start_block;
    }
    while (end_block < block_max && blocks[end_block].Props() == vm_block.Props()) {
        ++end_block;
    }
    info->start = base + Blockpool::ToBytes(start_block);
    info->end = base + Blockpool::ToBytes(end_block);
    const u32 prot_cpu = (vm_block.prot_cpu >> 1) ? 3 : vm_block.prot_cpu & 1;
    info->protection = (vm_block.prot_gpu << 4) | prot_cpu;
    if (vm_block.valid) {
        info->memory_type = 10;
        if (!vm_block.writeback || vm_block.onion) {
            info->memory_type = 0;
        }
        if (!vm_block.writeback && !vm_block.onion) {
            info->memory_type = 3;
        }
    }
    info->is_committed = vm_block.valid;
    info->is_pooled = 1;
    info->offset = info->start - base;
}

void Blockpool::SetName(VAddr start, VAddr end, const char* name) {
    NameSplit(start, end);
    if (name[0] == '\0') {
        return;
    }
    NameEntry entry{start, end, {}};
    std::strncpy(entry.name, name, 32);
    auto [it, _] = blockpool_names.emplace(start, entry);

    // Try merge with prev.
    if (it != blockpool_names.begin()) {
        auto prev = std::prev(it);
        if (prev->second.end == start && std::strcmp(prev->second.name, name) == 0) {
            prev->second.end = end;
            blockpool_names.erase(it);
            it = prev;
        }
    }
    // Try merge with next.
    auto next = std::next(it);
    if (next != blockpool_names.end() && next->second.start == it->second.end &&
        std::strcmp(next->second.name, it->second.name) == 0) {
        it->second.end = next->second.end;
        blockpool_names.erase(next);
    }
}

void Blockpool::NameSplit(VAddr start, VAddr end) {
    auto it = blockpool_names.lower_bound(start);
    if (it != blockpool_names.begin()) {
        --it;
    }
    while (it != blockpool_names.end() && it->second.start < end) {
        auto& entry = it->second;
        if (entry.end <= start) {
            ++it;
            continue;
        }
        if (entry.start < start && entry.end > end) {
            NameEntry right{end, entry.end, {}};
            std::memcpy(right.name, entry.name, sizeof(entry.name));
            entry.end = start;
            blockpool_names.emplace(end, right);
            return;
        }
        if (entry.start < start) {
            entry.end = start;
            ++it;
            continue;
        }
        if (entry.end > end) {
            entry.start = end;
            return;
        }
        it = blockpool_names.erase(it);
    }
}

} // namespace Core
