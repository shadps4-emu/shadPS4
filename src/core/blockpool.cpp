// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <utility>
#include "common/assert.h"
#include "core/blockpool.h"
#include "core/libraries/kernel/memory.h"

namespace Core {

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
    u32 start = base;
    u32 end = base + size;
    // UNREACHABLE();
    //  get _start and __end from name splay tree
    u32 block_min = 0;
    if (base <= start) {
        block_min = Blockpool::ToBlocks(start - base);
    }
    u32 block_max = Blockpool::ToBlocks(size);
    if ((end - base) <= size) {
        block_max = Blockpool::ToBlocks(end - base);
    }
    const u32 query_block = Blockpool::ToBlocks(addr - base);
    const auto vm_block = blocks[query_block];
    u32 start_block = query_block;
    u32 end_block = query_block;
    if (!vm_block.valid) {
        while (start_block > block_min && !blocks[start_block - 1].valid) {
            --start_block;
        }
        while (end_block < block_max && !blocks[end_block + 1].valid) {
            ++end_block;
        }
    } else {
        while (start_block > block_min &&
               blocks[start_block].Props() == blocks[query_block].Props()) {
            --start_block;
        }
        while (end_block < block_max && blocks[end_block].Props() == blocks[query_block].Props()) {
            ++end_block;
        }
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

} // namespace Core
