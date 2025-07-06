// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <deque>
#include <shared_mutex>
#include <type_traits>
#include <vector>
#include "common/debug.h"
#include "common/types.h"
#include "video_core/buffer_cache/region_manager.h"

namespace VideoCore {

class MemoryTracker {
public:
    static constexpr size_t MAX_CPU_PAGE_BITS = 40;
    static constexpr size_t NUM_HIGH_PAGES = 1ULL << (MAX_CPU_PAGE_BITS - TRACKER_HIGHER_PAGE_BITS);
    static constexpr size_t MANAGER_POOL_SIZE = 32;

public:
    explicit MemoryTracker(PageManager& tracker_) : tracker{&tracker_} {}
    ~MemoryTracker() = default;

    /// Returns true if a region has been modified from the CPU
    template <bool locking = true>
    bool IsRegionCpuModified(VAddr query_cpu_addr, u64 query_size) noexcept {
        return IterateRegions<true, locking>(
            query_cpu_addr, query_size, [](RegionManager* manager, u64 offset, size_t size) {
                std::scoped_lock lk{manager->lock};
                return manager->template IsRegionModified<Type::CPU>(offset, size);
            });
    }

    /// Returns true if a region has been modified from the GPU
    template <bool locking = true>
    bool IsRegionGpuModified(VAddr query_cpu_addr, u64 query_size) noexcept {
        return IterateRegions<false, locking>(
            query_cpu_addr, query_size, [](RegionManager* manager, u64 offset, size_t size) {
                std::scoped_lock lk{manager->lock};
                return manager->template IsRegionModified<Type::GPU>(offset, size);
            });
    }

    /// Mark region as CPU modified, notifying the device_tracker about this change
    template <bool locking = true>
    void MarkRegionAsCpuModified(VAddr dirty_cpu_addr, u64 query_size) {
        IterateRegions<false, locking>(dirty_cpu_addr, query_size,
                                       [](RegionManager* manager, u64 offset, size_t size) {
                                           std::scoped_lock lk{manager->lock};
                                           manager->template ChangeRegionState<Type::CPU, true>(
                                               manager->GetCpuAddr() + offset, size);
                                       });
    }

    /// Unmark all regions as CPU modified, notifying the device_tracker about this change
    template <bool locking = true>
    void UnmarkAllRegionsAsCpuModified() noexcept {
        ForEachRegion<locking>([](RegionManager* manager) {
            std::scoped_lock lk{manager->lock};
            manager->template ChangeAllRegionState<Type::CPU, false>();
        });
    }

    /// Unmark region as modified from the host GPU
    template <bool locking = true>
    void UnmarkRegionAsGpuModified(VAddr dirty_cpu_addr, u64 query_size) noexcept {
        IterateRegions<false, locking>(dirty_cpu_addr, query_size,
                                       [](RegionManager* manager, u64 offset, size_t size) {
                                           std::scoped_lock lk{manager->lock};
                                           manager->template ChangeRegionState<Type::GPU, false>(
                                               manager->GetCpuAddr() + offset, size);
                                       });
    }

    /// Removes all protection from a page and ensures GPU data has been flushed if requested
    template <bool locking = true>
    void InvalidateRegion(VAddr cpu_addr, u64 size, bool try_flush, auto&& on_flush) noexcept {
        IterateRegions<false, locking>(
            cpu_addr, size,
            [try_flush, &on_flush](RegionManager* manager, u64 offset, size_t size) {
                const bool should_flush = [&] {
                    // Perform both the GPU modification check and CPU state change with the lock
                    // in case we are racing with GPU thread trying to mark the page as GPU
                    // modified. If we need to flush the flush function is going to perform CPU
                    // state change.
                    std::scoped_lock lk{manager->lock};
                    if (try_flush && manager->template IsRegionModified<Type::GPU>(offset, size)) {
                        return true;
                    }
                    manager->template ChangeRegionState<Type::CPU, true>(
                        manager->GetCpuAddr() + offset, size);
                    return false;
                }();
                if (should_flush) {
                    on_flush();
                }
            });
    }

    /// Call 'func' for each CPU modified range and unmark those pages as CPU modified
    template <bool clear, bool locking = true>
    void ForEachUploadRange(VAddr query_cpu_range, u64 query_size, bool is_written, auto&& func) {
        IterateRegions<true, locking>(
            query_cpu_range, query_size,
            [&func, is_written](RegionManager* manager, u64 offset, size_t size) {
                std::scoped_lock lk{manager->lock};
                manager->template ForEachModifiedRange<Type::CPU, clear>(
                    manager->GetCpuAddr() + offset, size, func);
                if (is_written && clear) {
                    manager->template ChangeRegionState<Type::GPU, true>(
                        manager->GetCpuAddr() + offset, size);
                }
            });
    }

    /// Call 'func' for each GPU modified range and unmark those pages as GPU modified
    template <bool clear, bool locking = true>
    void ForEachDownloadRange(VAddr query_cpu_range, u64 query_size, auto&& func) {
        IterateRegions<false, locking>(query_cpu_range, query_size,
                                       [&func](RegionManager* manager, u64 offset, size_t size) {
                                           std::scoped_lock lk{manager->lock};
                                           manager->template ForEachModifiedRange<Type::GPU, clear>(
                                               manager->GetCpuAddr() + offset, size, func);
                                       });
    }

    /// Lck the memory tracker.
    void Lock() {
        global_lock.lock();
    }

    /// Unlock the memory tracker.
    void Unlock() {
        global_lock.unlock();
    }

private:
    /**
     * @brief IteratePages Iterates L2 word manager page table.
     * @param cpu_address Start byte cpu address
     * @param size Size in bytes of the region of iterate.
     * @param func Callback for each word manager.
     * @return
     */
    template <bool create_region_on_fail, bool locking, typename Func>
    bool IterateRegions(VAddr cpu_address, size_t size, Func&& func) {
        RENDERER_TRACE;
        if constexpr (locking) {
            std::shared_lock lock{global_lock};
        }
        using FuncReturn = typename std::invoke_result<Func, RegionManager*, u64, size_t>::type;
        static constexpr bool BOOL_BREAK = std::is_same_v<FuncReturn, bool>;
        std::size_t remaining_size{size};
        std::size_t page_index{cpu_address >> TRACKER_HIGHER_PAGE_BITS};
        u64 page_offset{cpu_address & TRACKER_HIGHER_PAGE_MASK};
        while (remaining_size > 0) {
            const std::size_t copy_amount{
                std::min<std::size_t>(TRACKER_HIGHER_PAGE_SIZE - page_offset, remaining_size)};
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

    /**
     * @brief Iterate throw all regions in the memory tracker.
     * @param func Callback for each region manager.
     * @return
     */
    template <bool locking, typename Func>
    void ForEachRegion(Func&& func) {
        RENDERER_TRACE;
        if constexpr (locking) {
            std::shared_lock lock{global_lock};
        }
        for (auto& pool : manager_pool) {
            for (auto& manager : pool) {
                if (manager.GetCpuAddr() != 0) {
                    func(&manager);
                }
            }
        }
    }

    void CreateRegion(std::size_t page_index) {
        const VAddr base_cpu_addr = page_index << TRACKER_HIGHER_PAGE_BITS;
        if (free_managers.empty()) {
            manager_pool.emplace_back();
            auto& last_pool = manager_pool.back();
            for (size_t i = 0; i < MANAGER_POOL_SIZE; i++) {
                std::construct_at(&last_pool[i], tracker, 0);
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
    std::deque<std::array<RegionManager, MANAGER_POOL_SIZE>> manager_pool;
    std::vector<RegionManager*> free_managers;
    std::array<RegionManager*, NUM_HIGH_PAGES> top_tier{};
    std::shared_mutex global_lock;
};

} // namespace VideoCore
