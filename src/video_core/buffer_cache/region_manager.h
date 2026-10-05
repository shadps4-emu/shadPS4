// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <utility>

#include "common/adaptive_mutex.h"
#include "common/types.h"
#include "core/emulator_settings.h"
#include "video_core/buffer_cache/region_definitions.h"
#include "video_core/page_manager.h"

namespace VideoCore {

#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
using LockType = Common::AdaptiveMutex;
#else
using LockType = std::mutex;
#endif

/**
 * Allows tracking CPU and GPU modification of pages in a contigious virtual address region.
 * Information is stored in bitsets for spacial locality and fast update of single pages.
 */
class RegionManager {
public:
    explicit RegionManager(PageManager* tracker_, VAddr cpu_addr_)
        : tracker{tracker_}, cpu_addr{cpu_addr_},
          readbacks_mode{EmulatorSettings.GetReadbacksMode()} {
        cpu.Fill(true);
        gpu.Fill(false);
    }
    explicit RegionManager() = default;

    void SetCpuAddress(VAddr new_cpu_addr) {
        cpu_addr = new_cpu_addr;
    }

    static constexpr Bounds GetBounds(u64 offset, u64 size) {
        const u64 end_address = offset + size - 1;
        return Bounds{
            .start_word = offset / BYTES_PER_WORD,
            .start_page = (offset) / BYTES_PER_PAGE,
            .end_word = end_address / BYTES_PER_WORD,
            .end_page = (end_address) / BYTES_PER_PAGE,
        };
    }

    static constexpr std::pair<u64, u64> GetMasks(u64 start_page, u64 end_page) {
        const u64 start_mask = ~u64{0} << (start_page & (PAGES_PER_WORD - 1));
        const u64 end_mask = ~u64{0} >> (63 - (end_page & (PAGES_PER_WORD - 1)));
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
            func(end_word, end_mask);
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

    template <StateOp cpu_op, StateOp gpu_op, bool locked = true>
    void ChangeRegionState(u64 offset, u64 size) {
        auto bounds = GetBounds(offset, size);
        if (!HasEffect<cpu_op, gpu_op>(bounds)) {
            return;
        }
        bool update_watchers{};
        RegionBits write_prot;
        RegionBits read_prot;
        Bounds watcher_bounds = MIN_BOUNDS;
        if constexpr (locked) {
            mutex.lock();
        }
        IterateWords(bounds, [&](u64 index, u64 mask) {
            update_watchers |= UpdateProtection<cpu_op, gpu_op>(write_prot, read_prot,
                                                                watcher_bounds, index, mask);
        });
        const auto write_op = GetPageOp<Type::CPU>(cpu_op);
        const auto read_op = GetPageOp<Type::GPU>(gpu_op);
        update_watchers &= write_op != PageOp::None || read_op != PageOp::None;
        if (update_watchers) {
            tracker->UpdatePageWatchersForRegion(cpu_addr, watcher_bounds, write_prot, read_prot,
                                                 write_op, read_op);
        }
        IterateWords(bounds,
                     [&](u64 index, u64 mask) { UpdateState<cpu_op, gpu_op>(index, mask); });
        if constexpr (locked) {
            mutex.unlock();
        }
    }

    template <Type type, StateOp cpu_op, StateOp gpu_op, bool locked = true>
    void ForEachModifiedRange(u64 offset, s64 size, auto&& func) {
        auto bounds = GetBounds(offset, size);
        if (!HasEffect<cpu_op, gpu_op, type>(bounds)) {
            return;
        }
        auto& state = GetRegionBits<type>();
        bool update_watchers{};
        RegionBits write_prot;
        RegionBits read_prot;
        u64 start_page{};
        u64 end_page{};
        Bounds watcher_bounds = MIN_BOUNDS;
        if constexpr (locked) {
            mutex.lock();
        }
        IterateWords(bounds, [&](u64 index, u64 mask) {
            const u64 base_page = index * PAGES_PER_WORD;
            const u64 word = state[index] & mask;
            update_watchers |= UpdateProtection<cpu_op, gpu_op>(write_prot, read_prot,
                                                                watcher_bounds, index, mask);
            IteratePages(word, [&](u64 pages_offset, u64 pages_size) {
                if (end_page == base_page + pages_offset) {
                    end_page += pages_size;
                    return;
                }
                if (end_page) {
                    func(cpu_addr + start_page * BYTES_PER_PAGE,
                         (end_page - start_page) * BYTES_PER_PAGE);
                }
                start_page = base_page + pages_offset;
                end_page = start_page + pages_size;
            });
        });
        if (end_page) {
            func(cpu_addr + start_page * BYTES_PER_PAGE, (end_page - start_page) * BYTES_PER_PAGE);
        }
        const auto write_op = GetPageOp<Type::CPU>(cpu_op);
        const auto read_op = GetPageOp<Type::GPU>(gpu_op);
        update_watchers &= write_op != PageOp::None || read_op != PageOp::None;
        if (update_watchers) {
            tracker->UpdatePageWatchersForRegion(cpu_addr, watcher_bounds, write_prot, read_prot,
                                                 write_op, read_op);
        }
        IterateWords(bounds,
                     [&](u64 index, u64 mask) { UpdateState<cpu_op, gpu_op>(index, mask); });
        if constexpr (locked) {
            mutex.unlock();
        }
    }

    template <Type type>
    bool IsRegionModified(u64 offset, u64 size) noexcept {
        const auto& state = GetRegionBits<type>();
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
            return state[end_word] & end_mask;
        }
    }

    void Lock(const Bounds& bounds) noexcept {
        mutex.lock();
    }

    void Unlock(const Bounds& bounds) noexcept {
        mutex.unlock();
    }

private:
    template <StateOp op>
    constexpr static u64 Apply(u64 word, u64 mask) {
        if constexpr (op == StateOp::Clear) {
            return word &= ~mask;
        } else if constexpr (op == StateOp::Set) {
            return word |= mask;
        } else {
            static_assert(false);
        }
    }

    template <StateOp cpu_op, StateOp gpu_op>
    constexpr bool UpdateProtection(RegionBits& write_prot, RegionBits& read_prot, Bounds& bounds,
                                    u64 index, u64 mask) {
        u64 prot_word = 0;
        if constexpr (cpu_op != StateOp::None) {
            const u64 prev = cpu[index];
            const u64 next = Apply<cpu_op>(prev, mask);
            write_prot[index] = (next ^ prev) & mask;
            prot_word |= write_prot[index];
        }
        if constexpr (gpu_op != StateOp::None) {
            const u64 prev = gpu[index];
            const u64 next = Apply<gpu_op>(prev, mask);
            read_prot[index] = (next ^ prev) & mask;
            prot_word |= read_prot[index];
        }
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

    template <StateOp cpu_op, StateOp gpu_op>
    constexpr void UpdateState(u64 index, u64 mask) {
        const u64 summary_bit = u64{1} << index;
        if constexpr (cpu_op != StateOp::None) {
            cpu[index] = Apply<cpu_op>(cpu[index], mask);
            cpu.set_summary =
                cpu[index] ? (cpu.set_summary | summary_bit) : (cpu.set_summary & ~summary_bit);
            cpu.clear_summary = cpu[index] != ~u64{0} ? (cpu.clear_summary | summary_bit)
                                                      : (cpu.clear_summary & ~summary_bit);
        }
        if constexpr (gpu_op != StateOp::None) {
            gpu[index] = Apply<gpu_op>(gpu[index], mask);
            gpu.set_summary =
                gpu[index] ? (gpu.set_summary | summary_bit) : (gpu.set_summary & ~summary_bit);
            gpu.clear_summary = gpu[index] != ~u64{0} ? (gpu.clear_summary | summary_bit)
                                                      : (gpu.clear_summary & ~summary_bit);
        }
    }

    template <StateOp cpu_op, StateOp gpu_op, Type iter_type = Type::None>
    constexpr bool HasEffect(const Bounds& bounds) const noexcept {
        const auto [start_mask, end_mask] = GetMasks(bounds.start_page, bounds.end_page);
        const u64 interior_mask = (u64{1} << bounds.end_word) - (u64{2} << bounds.start_word);

        const auto any_set = [&](const RegionBits& state) -> bool {
            if (bounds.start_word == bounds.end_word) {
                const u64 mask = start_mask & end_mask;
                return state[bounds.start_word] & mask;
            }
            return (state[bounds.start_word] & start_mask) || (state[bounds.end_word] & end_mask) ||
                   (state.set_summary & interior_mask);
        };

        const auto any_clear = [&](const RegionBits& state) -> bool {
            if (bounds.start_word == bounds.end_word) {
                const u64 mask = start_mask & end_mask;
                return (state[bounds.start_word] & mask) != mask;
            }
            return (state[bounds.start_word] & start_mask) != start_mask ||
                   (state[bounds.end_word] & end_mask) != end_mask ||
                   (state.clear_summary & interior_mask);
        };

        if constexpr (iter_type == Type::CPU) {
            if (any_set(cpu)) {
                return true;
            }
        } else if constexpr (cpu_op == StateOp::Set) {
            if (any_clear(cpu)) {
                return true;
            }
        } else if constexpr (cpu_op == StateOp::Clear) {
            if (any_set(cpu)) {
                return true;
            }
        }

        if constexpr (iter_type == Type::GPU) {
            return any_set(gpu);
        } else if constexpr (gpu_op == StateOp::Set) {
            return any_clear(gpu);
        } else if constexpr (gpu_op == StateOp::Clear) {
            return any_set(gpu);
        } else {
            return false;
        }
    }

    template <Type type>
        requires(std::popcount(std::to_underlying(type)) == 1)
    constexpr PageOp GetPageOp(StateOp state_op) {
        if constexpr (type == Type::CPU) {
            if (state_op == StateOp::Set) {
                return PageOp::Untrack;
            } else if (state_op == StateOp::Clear) {
                return PageOp::Track;
            }
        } else if (type == Type::GPU && readbacks_mode == GpuReadbacksMode::Precise) {
            if (state_op == StateOp::Set) {
                return PageOp::Track;
            } else if (state_op == StateOp::Clear) {
                return PageOp::Untrack;
            }
        }
        return PageOp::None;
    }

    template <Type type>
        requires(std::popcount(std::to_underlying(type)) == 1)
    RegionBits& GetRegionBits() noexcept {
        if constexpr (type == Type::CPU) {
            return cpu;
        } else if constexpr (type == Type::GPU) {
            return gpu;
        }
    }

    PageManager* tracker;
    VAddr cpu_addr{};
    u32 readbacks_mode;
    RegionBits cpu;
    RegionBits gpu;
    LockType mutex;
};

} // namespace VideoCore
