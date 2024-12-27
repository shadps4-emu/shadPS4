// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <deque>
#include <type_traits>
#include <vector>
#include "common/types.h"
#include "video_core/buffer_cache/word_manager.h"

namespace VideoCore {

class MemoryTracker {
public:
    static constexpr size_t MAX_CPU_PAGE_BITS = 40;
    static constexpr size_t HIGHER_PAGE_BITS = 22;
    static constexpr size_t HIGHER_PAGE_SIZE = 1ULL << HIGHER_PAGE_BITS;
    static constexpr size_t HIGHER_PAGE_MASK = HIGHER_PAGE_SIZE - 1ULL;
    static constexpr size_t NUM_HIGH_PAGES = 1ULL << (MAX_CPU_PAGE_BITS - HIGHER_PAGE_BITS);
    static constexpr size_t MANAGER_POOL_SIZE = 32;
    static constexpr size_t WORDS_STACK_NEEDED = HIGHER_PAGE_SIZE / BYTES_PER_WORD;
    using Manager = WordManager<WORDS_STACK_NEEDED>;

public:
    explicit MemoryTracker(PageManager* tracker_) : tracker{tracker_} {}
    ~MemoryTracker() = default;

    /// Returns true if a region has been modified from the CPU
    [[nodiscard]] bool IsRegionCpuModified(VAddr query_cpu_addr, u64 query_size) noexcept {
        return IteratePages<true>(
            query_cpu_addr, query_size, [](Manager* manager, u64 offset, size_t size) {
                return manager->template IsRegionModified<Type::CPU>(offset, size);
            });
    }

    /// Returns true if a region has been modified from the GPU
    [[nodiscard]] bool IsRegionGpuModified(VAddr query_cpu_addr, u64 query_size) noexcept {
        return IteratePages<false>(
            query_cpu_addr, query_size, [](Manager* manager, u64 offset, size_t size) {
                return manager->template IsRegionModified<Type::GPU>(offset, size);
            });
    }

    /// Mark region as CPU modified, notifying the device_tracker about this change
    void MarkRegionAsCpuModified(VAddr dirty_cpu_addr, u64 query_size) {
        IteratePages<true>(dirty_cpu_addr, query_size,
                           [](Manager* manager, u64 offset, size_t size) {
                               manager->template ChangeRegionState<Type::CPU, true>(
                                   manager->GetCpuAddr() + offset, size);
                           });
    }

    /// Unmark region as CPU modified, notifying the device_tracker about this change
    void UnmarkRegionAsCpuModified(VAddr dirty_cpu_addr, u64 query_size) {
        IteratePages<true>(dirty_cpu_addr, query_size,
                           [](Manager* manager, u64 offset, size_t size) {
                               manager->template ChangeRegionState<Type::CPU, false>(
                                   manager->GetCpuAddr() + offset, size);
                           });
    }

    /// Mark region as modified from the host GPU
    void MarkRegionAsGpuModified(VAddr dirty_cpu_addr, u64 query_size) noexcept {
        IteratePages<true>(dirty_cpu_addr, query_size,
                           [](Manager* manager, u64 offset, size_t size) {
                               manager->template ChangeRegionState<Type::GPU, true>(
                                   manager->GetCpuAddr() + offset, size);
                           });
    }

    /// Unmark region as modified from the host GPU
    void UnmarkRegionAsGpuModified(VAddr dirty_cpu_addr, u64 query_size) noexcept {
        IteratePages<true>(dirty_cpu_addr, query_size,
                           [](Manager* manager, u64 offset, size_t size) {
                               manager->template ChangeRegionState<Type::GPU, false>(
                                   manager->GetCpuAddr() + offset, size);
                           });
    }

    /// Call 'func' for each CPU modified range and unmark those pages as CPU modified
    template <typename Func>
    void ForEachUploadRange(VAddr query_cpu_range, u64 query_size, Func&& func) {
        IteratePages<true>(query_cpu_range, query_size,
                           [&func](Manager* manager, u64 offset, size_t size) {
                               manager->template ForEachModifiedRange<Type::CPU, true>(
                                   manager->GetCpuAddr() + offset, size, func);
                           });
    }

    /// Call 'func' for each GPU modified range and unmark those pages as GPU modified
    template <bool clear, typename Func>
    void ForEachDownloadRange(VAddr query_cpu_range, u64 query_size, Func&& func) {
        IteratePages<false>(query_cpu_range, query_size,
                            [&func](Manager* manager, u64 offset, size_t size) {
                                if constexpr (clear) {
                                    manager->template ForEachModifiedRange<Type::GPU, true>(
                                        manager->GetCpuAddr() + offset, size, func);
                                } else {
                                    manager->template ForEachModifiedRange<Type::GPU, false>(
                                        manager->GetCpuAddr() + offset, size, func);
                                }
                            });
    }

private:
    /**
     * @brief IteratePages Iterates L2 word manager page table.
     * @param cpu_address Start byte cpu address
     * @param size Size in bytes of the region of iterate.
     * @param func Callback for each word manager.
     * @return
     */
    template <bool create_region_on_fail, typename Func>
    bool IteratePages(VAddr cpu_address, size_t size, Func&& func) {
        using FuncReturn = typename std::invoke_result<Func, Manager*, u64, size_t>::type;
        static constexpr bool BOOL_BREAK = std::is_same_v<FuncReturn, bool>;
        std::size_t remaining_size{size};
        std::size_t page_index{cpu_address >> HIGHER_PAGE_BITS};
        u64 page_offset{cpu_address & HIGHER_PAGE_MASK};
        while (remaining_size > 0) {
            const std::size_t copy_amount{
                std::min<std::size_t>(HIGHER_PAGE_SIZE - page_offset, remaining_size)};
            auto* manager{top_tier[page_index]};
            if (manager) {
                if constexpr (BOOL_BREAK) {
                    if (func(manager, page_offset, copy_amount)) {
                        return true;
                    }
                } else {
                    func(manager, page_offset, copy_amount);
                }
            } else if constexpr (create_region_on_fail) {
                CreateRegion(page_index);
                manager = top_tier[page_index];
                if constexpr (BOOL_BREAK) {
                    if (func(manager, page_offset, copy_amount)) {
                        return true;
                    }
                } else {
                    func(manager, page_offset, copy_amount);
                }
            }
            page_index++;
            page_offset = 0;
            remaining_size -= copy_amount;
        }
        return false;
    }

    void CreateRegion(std::size_t page_index) {
        const VAddr base_cpu_addr = page_index << HIGHER_PAGE_BITS;
        if (free_managers.empty()) {
            manager_pool.emplace_back();
            auto& last_pool = manager_pool.back();
            for (size_t i = 0; i < MANAGER_POOL_SIZE; i++) {
                std::construct_at(&last_pool[i], tracker, 0, HIGHER_PAGE_SIZE);
                free_managers.push_back(&last_pool[i]);
            }
        }
        // Each manager tracks a 4_MB virtual address space.
        auto* new_manager = free_managers.back();
        new_manager->SetCpuAddress(base_cpu_addr);
        free_managers.pop_back();
        top_tier[page_index] = new_manager;
    }

    PageManager* tracker;
    std::deque<std::array<Manager, MANAGER_POOL_SIZE>> manager_pool;
    std::vector<Manager*> free_managers;
    std::array<Manager*, NUM_HIGH_PAGES> top_tier{};
};

} // namespace VideoCore
