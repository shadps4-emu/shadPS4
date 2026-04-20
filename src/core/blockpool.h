// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bit>
#include <map>
#include <mutex>
#include "common/assert.h"
#include "common/types.h"

namespace Libraries::Kernel {
struct OrbisVirtualQueryInfo;
}

namespace Core {

union DmemBlock {
    u32 raw{};
    struct {
        u32 block : 21;
        u32 unk1 : 1;
        u32 onion : 1;
        u32 writeback : 1;
        u32 prot_cpu : 2;
        u32 prot_gpu : 2;
        u32 valid : 2;
    };

    u32 Props() const {
        return raw & 0x3fc00000;
    }
};

struct BlockStats {
    u32 available_flushed_blocks;
    u32 available_cached_blocks;
    u32 allocated_flushed_blocks;
    u32 allocated_cached_blocks;
};

class Blockpool {
public:
    static constexpr u32 BLOCK_SHIFT = 16;
    static constexpr u32 BLOCK_SIZE = 1U << BLOCK_SHIFT;

public:
    explicit Blockpool() = default;
    ~Blockpool() = default;

    constexpr static u64 ToBlocks(VAddr addr) {
        return addr >> BLOCK_SHIFT;
    }

    constexpr static u32 ToBytes(u64 blocks) {
        return blocks << BLOCK_SHIFT;
    }

    BlockStats GetBlockStats() noexcept {
        std::scoped_lock lk{mutex};
        return BlockStats{
            .available_flushed_blocks = flushed.available_blocks,
            .available_cached_blocks = cached.available_blocks,
            .allocated_flushed_blocks = flushed.allocated_blocks,
            .allocated_cached_blocks = cached.allocated_blocks,
        };
    }

    void Expand(PAddr dmem_addr, u64 size) {
        std::scoped_lock lk{mutex};
        flushed.Insert(dmem_addr >> BLOCK_SHIFT, size >> BLOCK_SHIFT);
    }

    bool Commit(u32 count, bool onion, bool writeback, u32* out_blocks);

    void Decommit(DmemBlock* blocks, u32 count);

    void Flush();

    void Query(VAddr addr, VAddr base, u64 size, const DmemBlock* blocks,
               ::Libraries::Kernel::OrbisVirtualQueryInfo* info);

private:
    struct Bitmap {
        static constexpr u32 NUM_ENTRIES = 4096;
        static constexpr u32 LEVEL_SHIFT = 6;
        static constexpr u32 LEVEL_MASK = (1u << LEVEL_SHIFT) - 1;
        static_assert(NUM_ENTRIES >> (LEVEL_SHIFT * 2) == 1);

        constexpr void Allocate(u32 count, u32* out_blocks) {
            while (count && bits_l2) {
                const u32 l1_index = std::countr_zero(bits_l2);
                const u32 l0_index =
                    std::countr_zero(bits_l1[l1_index]) | (l1_index << LEVEL_SHIFT);

                u64 entry = bits_l0[l0_index];
                do {
                    *(out_blocks++) = std::countr_zero(entry) | (l0_index << LEVEL_SHIFT);
                    --available_blocks;
                    entry = entry - 1 & entry;
                    --count;
                } while (entry && count != 0);
                bits_l0[l0_index] = entry;

                if (entry == 0) {
                    bits_l1[l1_index] &= ~(u64{1} << (l0_index & LEVEL_MASK));
                }
                if (bits_l1[l1_index] == 0) {
                    bits_l2 &= ~(u64{1} << l1_index);
                }
            }
            ASSERT(count == 0);
        }

        constexpr void Insert(u32 start, u32 count) {
            available_blocks += count;
            for (auto* bits : {bits_l0.data(), bits_l1.data(), &bits_l2}) {
                InsertLevel(start, count, bits);
                start >>= LEVEL_SHIFT;
                count = (count + LEVEL_MASK) >> LEVEL_SHIFT;
            }
        }

        constexpr void InsertLevel(u32 start_bit, u32 num_bits, u64* bits) {
            const u32 end_bit = start_bit + num_bits - 1;
            const u32 start_index = start_bit >> LEVEL_SHIFT;
            const u32 end_index = end_bit >> LEVEL_SHIFT;
            bits[start_index] |= (~u64{0} >> (64 - std::min(num_bits, 64u)))
                                 << (start_bit & LEVEL_MASK);
            if (start_index == end_index) {
                return;
            }
            for (u32 index = start_index + 1; index < end_index; ++index) {
                bits[index] = ~u64{0};
            }
            bits[end_index] |= (~u64{0} >> (63 - (end_bit & LEVEL_MASK)));
        };

        u32 available_blocks;
        u32 allocated_blocks;
        std::array<u64, NUM_ENTRIES> bits_l0;
        std::array<u64, (NUM_ENTRIES >> LEVEL_SHIFT)> bits_l1;
        u64 bits_l2;
    };

    std::mutex mutex;
    Bitmap cached{};
    Bitmap flushed{};

    struct NameEntry {
        VAddr start;
        VAddr end;
        char name[32];
    };
    std::map<VAddr, NameEntry> name_tree;
};

} // namespace Core
