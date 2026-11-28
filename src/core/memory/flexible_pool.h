// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <bit>
#include <mutex>
#include <vector>

#include "common/assert.h"
#include "common/types.h"

namespace Core {

struct PhysRange {
    u64 phys_offset;
    u64 size;
};

class FlexibleMemoryPool {
public:
    static constexpr u64 PAGE_SIZE = 0x4000;
    static constexpr u32 PAGE_SHIFT = 14;
    static constexpr u32 BITS_PER_WORD = 64;

    FlexibleMemoryPool() = default;

    void Init(u64 base_offset, u64 size) {
        m_base_offset = base_offset;
        m_total_size = size;
        m_total_pages = size >> PAGE_SHIFT;
        m_free_pages = m_total_pages;

        ASSERT(m_total_pages % BITS_PER_WORD == 0);
        const u64 num_words0 = m_total_pages / BITS_PER_WORD;
        m_l0.resize(num_words0, ~0ULL);

        const u64 num_words1 = (num_words0 + BITS_PER_WORD - 1) / BITS_PER_WORD;
        m_l1.resize(num_words1, 0);
        for (u64 i = 0; i < num_words0; ++i) {
            m_l1[i / BITS_PER_WORD] |= 1ULL << (i & (BITS_PER_WORD - 1));
        }
    }

    bool Allocate(u64 size, std::vector<PhysRange>& out) {
        std::scoped_lock lock{m_mutex};
        u64 pages_needed = size >> PAGE_SHIFT;
        if (m_free_pages < pages_needed) {
            return false;
        }

        u64 remaining = pages_needed;
        s64 search_from = 0;

        while (remaining > 0) {
            const s64 page = FindFreeFrom(search_from);
            const u64 run_start = page;
            u64 run_len = 0;
            u64 p = run_start;
            while (run_len < remaining && p < m_total_pages && IsFree(p)) {
                run_len++;
                p++;
            }

            MarkAllocatedRange(run_start, run_len);
            out.push_back({m_base_offset + (run_start << PAGE_SHIFT), run_len << PAGE_SHIFT});
            remaining -= run_len;
            search_from = p;
        }
        return true;
    }

    void Free(const std::vector<PhysRange>& ranges) {
        std::scoped_lock lock{m_mutex};
        for (const auto& [phys_offset, size] : ranges) {
            ASSERT(phys_offset >= m_base_offset);
            const u64 start = (phys_offset - m_base_offset) >> PAGE_SHIFT;
            const u64 count = size >> PAGE_SHIFT;
            ASSERT(start + count <= m_total_pages);
            MarkFreeRange(start, count);
        }
    }

    u64 GetTotalSize() const {
        return m_total_size;
    }
    u64 GetUsedSize() const {
        return (m_total_pages - m_free_pages) << PAGE_SHIFT;
    }
    u64 GetAvailableSize() const {
        return m_free_pages << PAGE_SHIFT;
    }
    u64 GetBaseOffset() const {
        return m_base_offset;
    }

private:
    bool IsFree(u64 page) const {
        u64 w = page / BITS_PER_WORD;
        u64 b = page & (BITS_PER_WORD - 1);
        return (m_l0[w] & (1ULL << b)) != 0;
    }

    s64 FindFreeFrom(s64 start) const {
        if (start < 0 || start >= m_total_pages) {
            return -1;
        }

        u64 w0 = start / BITS_PER_WORD;
        u64 b0 = start & (BITS_PER_WORD - 1);
        u64 masked = m_l0[w0] & (~0ULL << b0);
        if (masked) {
            u64 page = w0 * BITS_PER_WORD + std::countr_zero(masked);
            if (page < m_total_pages) {
                return page;
            }
        }

        u64 w1_start = (w0 + 1) / BITS_PER_WORD;
        u64 b1_start = (w0 + 1) & (BITS_PER_WORD - 1);

        for (u64 w1 = w1_start; w1 < m_l1.size(); ++w1) {
            u64 l1_word = m_l1[w1];
            if (w1 == w1_start && b1_start > 0) {
                l1_word &= ~0ULL << b1_start;
            }
            if (!l1_word) {
                continue;
            }
            u64 l0_idx = w1 * BITS_PER_WORD + std::countr_zero(l1_word);
            if (l0_idx >= m_l0.size()) {
                return -1;
            }
            u64 page = l0_idx * BITS_PER_WORD + std::countr_zero(m_l0[l0_idx]);
            if (page < m_total_pages) {
                return page;
            }
        }
        return -1;
    }

    void MarkAllocatedRange(u64 start, u64 count) {
        ApplyRange<false>(start, count);
    }

    void MarkFreeRange(u64 start, u64 count) {
        ApplyRange<true>(start, count);
    }

    template <bool set_free>
    void ApplyRange(u64 start, u64 count) {
        if (count == 0) {
            return;
        }
        const u64 end = start + count;
        const u64 start_word = start / BITS_PER_WORD;
        const u64 end_word = end / BITS_PER_WORD;
        const u64 start_bit = start & (BITS_PER_WORD - 1);
        const u64 end_bit = end & (BITS_PER_WORD - 1);

        if (start_word == end_word) {
            u64 mask = ~0ULL << start_bit;
            if (end_bit) {
                mask &= (1ULL << end_bit) - 1;
            }
            ApplyWord<set_free>(start_word, mask);
        } else {
            ApplyWord<set_free>(start_word, ~0ULL << start_bit);
            for (u64 w = start_word + 1; w < end_word; ++w) {
                ApplyWord<set_free>(w, ~0ULL);
            }
            if (end_bit) {
                ApplyWord<set_free>(end_word, (1ULL << end_bit) - 1);
            }
        }
    }

    template <bool set_free>
    void ApplyWord(u64 l0_idx, u64 mask) {
        const u64 old = m_l0[l0_idx];
        m_l0[l0_idx] = set_free ? (old | mask) : (old & ~mask);
        const u64 changed = std::popcount(old ^ m_l0[l0_idx]);
        if (changed) {
            if constexpr (set_free) {
                m_free_pages += changed;
            } else {
                m_free_pages -= changed;
            }
            const u64 l1_idx = l0_idx / BITS_PER_WORD;
            const u64 l1_bit = l0_idx & (BITS_PER_WORD - 1);
            if (m_l0[l0_idx]) {
                m_l1[l1_idx] |= 1ULL << l1_bit;
            } else {
                m_l1[l1_idx] &= ~(1ULL << l1_bit);
            }
        }
    }

    std::vector<u64> m_l0;
    std::vector<u64> m_l1;
    std::mutex m_mutex;
    u64 m_base_offset{};
    u64 m_total_size{};
    u64 m_total_pages{};
    u64 m_free_pages{};
};

} // namespace Core