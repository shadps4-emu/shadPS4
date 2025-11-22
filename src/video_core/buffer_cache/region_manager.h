// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/config.h"
#include "common/types.h"
#include "video_core/buffer_cache/region_definitions.h"
#include "video_core/page_manager.h"

namespace VideoCore {

/**
 * Allows tracking CPU and GPU modification of pages in a contigious virtual address region.
 * Information is stored in bitsets for spacial locality and fast update of single pages.
 */
class RegionManager {
public:
    explicit RegionManager(PageManager* tracker_, VAddr cpu_addr_)
        : tracker{tracker_}, cpu_addr{cpu_addr_} {
        cpu.Fill(~0ULL);
        gpu.Fill(0ULL);
        writeable.Fill(~0ULL);
        readable.Fill(~0ULL);
    }
    explicit RegionManager() = default;

    void SetCpuAddress(VAddr new_cpu_addr) {
        cpu_addr = new_cpu_addr;
    }

    static constexpr Bounds GetBounds(VAddr address, u64 size) {
        const u64 end_address = address + size + BYTES_PER_PAGE - 1ULL;
        return Bounds{
            .start_word = address / BYTES_PER_WORD,
            .start_page = (address % BYTES_PER_WORD) / BYTES_PER_PAGE,
            .end_word = end_address / BYTES_PER_WORD,
            .end_page = (end_address % BYTES_PER_WORD) / BYTES_PER_PAGE,
        };
    }

    static constexpr std::pair<u64, u64> GetMasks(u64 start_page, u64 end_page) {
        const u64 start_mask = ~((1ULL << start_page) - 1);
        const u64 end_mask = (1ULL << end_page) - 1;
        return std::make_pair(start_mask, end_mask);
    }

    static constexpr void IterateWords(Bounds bounds, auto&& func) {
        const auto [start_word, start_page, end_word, end_page] = bounds;
        const auto [start_mask, end_mask] = GetMasks(start_page, end_page);
        if (start_word == end_word) [[likely]] {
            func(start_word, start_mask & end_mask);
        } else {
            func(start_word, start_mask);
            for (s64 i = start_word + 1; i < end_word; ++i) {
                func(i, ~0ULL);
            }
            if (end_mask) {
                func(end_word, end_mask);
            }
        }
    }

    static constexpr void IteratePages(u64 word, auto&& func) {
        u64 offset{};
        while (word != 0) {
            const u64 empty_bits = std::countr_zero(word);
            offset += empty_bits;
            word >>= empty_bits;
            const u64 set_bits = std::countr_one(word);
            func(offset, set_bits);
            word = set_bits < PAGES_PER_WORD ? (word >> set_bits) : 0;
            offset += set_bits;
        }
    }

    template <Type type, LockOp lock_op, bool enable>
    void ChangeRegionState(u64 offset, u64 size) {
        auto& state = GetRegionBits<type>();
        RegionBits prot;
        bool update_watchers{};
        auto bounds = GetBounds(offset, size);
        auto watcher_bounds = MIN_BOUNDS;
        IterateWords(bounds, [&](u64 index, u64 mask) {
            if constexpr (lock_op & LockOp::Lock) {
                LockWord(index, mask);
            }
            if constexpr (enable) {
                state[index] |= mask;
            } else {
                state[index] &= ~mask;
            }
            update_watchers |= UpdateProtection<type, !enable>(prot, watcher_bounds, index, mask);
        });
        constexpr bool is_gpu = type == Type::GPU;
        if (update_watchers && (Config::readbacks() || !is_gpu)) {
            constexpr bool track = is_gpu ? enable : !enable;
            tracker->UpdatePageWatchersForRegion<track, is_gpu>(cpu_addr, watcher_bounds, prot);
        }
        if constexpr (lock_op & LockOp::Unlock) {
            IterateWords(bounds, [&](u64 index, u64 mask) { UnlockWord(index, mask); });
        }
    }

    template <Type type, LockOp lock_op, bool clear>
    void ForEachModifiedRange(u64 offset, s64 size, auto&& func) {
        auto& state = GetRegionBits<type>();
        RegionBits prot;
        bool update_watchers{};
        u64 start_page{};
        u64 end_page{};
        auto bounds = GetBounds(offset, size);
        auto watcher_bounds = MIN_BOUNDS;
        IterateWords(bounds, [&](u64 index, u64 mask) {
            if constexpr (lock_op & LockOp::Lock) {
                LockWord(index, mask);
            }
            const u64 word = state[index] & mask;
            const u64 base_page = index * PAGES_PER_WORD;
            IteratePages(word, [&](u64 pages_offset, u64 pages_size) {
                if (end_page) {
                    if (end_page == base_page + pages_offset) {
                        end_page += pages_size;
                        return;
                    }
                    func(cpu_addr + start_page * BYTES_PER_PAGE,
                         (end_page - start_page) * BYTES_PER_PAGE);
                }
                start_page = base_page + pages_offset;
                end_page = start_page + pages_size;
            });
            if constexpr (clear) {
                state[index] &= ~mask;
                update_watchers |= UpdateProtection<type, clear>(prot, watcher_bounds, index, mask);
            }
        });
        if (end_page) {
            func(cpu_addr + start_page * BYTES_PER_PAGE, (end_page - start_page) * BYTES_PER_PAGE);
        }
        constexpr bool is_gpu = type == Type::GPU;
        if (update_watchers) {
            tracker->UpdatePageWatchersForRegion<true, is_gpu>(cpu_addr, watcher_bounds, prot);
        }
        if constexpr (lock_op & LockOp::Unlock) {
            IterateWords(bounds, [&](u64 index, u64 mask) { UnlockWord(index, mask); });
        }
    }

    template <Type type>
    bool IsRegionModified(u64 offset, u64 size) noexcept {
        auto& state = GetRegionBits<type>();
        const auto [start_word, start_page, end_word, end_page] = GetBounds(offset, size);
        const auto [start_mask, end_mask] = GetMasks(start_page, end_page);
        if (start_word == end_word) [[likely]] {
            return state[start_word] & (start_mask & end_mask);
        } else {
            if (state[start_word] & start_mask) {
                return true;
            }
            for (s64 i = start_word + 1; i < end_word; ++i) {
                if (state[i]) {
                    return true;
                }
            }
            if (state[end_word] & end_mask) {
                return true;
            }
            return false;
        }
    }

private:
    template <Type type, bool clear>
    bool UpdateProtection(RegionBits& prot, Bounds& bounds, u64 index, u64 mask) {
        if constexpr (type == Type::CPU) {
            const u64 perm = writeable[index];
            if constexpr (clear) {
                writeable[index] &= ~mask;
            } else {
                writeable[index] |= mask;
            }
            prot[index] = (cpu[index] ^ perm) & mask;
        } else {
            const u64 perm = readable[index];
            if constexpr (clear) {
                readable[index] |= mask;
            } else {
                readable[index] &= ~mask;
            }
            prot[index] = (~gpu[index] ^ perm) & mask;
        }
        const u64 prot_word = prot[index];
        if (prot_word) {
            if (index <= bounds.start_word) {
                bounds.start_word = index;
                bounds.start_page = std::countr_zero(prot_word);
            }
            if (index >= bounds.end_word) {
                bounds.end_word = index;
                bounds.end_page = PAGES_PER_WORD - std::countl_zero(prot_word) - 1;
            }
            return true;
        }
        return false;
    }

    void LockWord(u64 index, u64 mask) {
        auto& lock = locks[index];
        u64 current_lock = lock.load();
        u64 new_lock;
        do {
            while (current_lock & mask) {
                lock.wait(current_lock);
                current_lock = lock.load();
            }
            new_lock = current_lock | mask;
        } while (!lock.compare_exchange_weak(current_lock, new_lock));
    }

    void UnlockWord(u64 index, u64 mask) {
        auto& lock = locks[index];
        u64 current_lock = lock.load();
        u64 new_lock;
        do {
            new_lock = current_lock & ~mask;
        } while (!lock.compare_exchange_weak(current_lock, new_lock));
        lock.notify_all();
    }

    template <Type type>
    RegionBits& GetRegionBits() noexcept {
        if constexpr (type == Type::CPU) {
            return cpu;
        } else if constexpr (type == Type::GPU) {
            return gpu;
        }
    }

    RegionBits cpu;
    RegionBits gpu;
    RegionBits writeable;
    RegionBits readable;

    PageManager* tracker;
    std::array<std::atomic<u64>, NUM_REGION_WORDS> locks{};
    VAddr cpu_addr{};
};

} // namespace VideoCore
