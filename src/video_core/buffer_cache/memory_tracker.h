// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <deque>
#include <functional>
#include <limits>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <boost/container/small_vector.hpp>

#include "common/debug.h"
#include "common/types.h"
#include "core/emulator_settings.h"
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

    void EnableAdaptiveCpuTracking() noexcept {
        adaptive_cpu_tracking.store(true, std::memory_order_release);
    }

    void BeginUploadBatch() noexcept {
        DEBUG_ASSERT(adaptive_cpu_tracking.load(std::memory_order_relaxed));
        DEBUG_ASSERT_MSG(!upload_batch_active, "Nested adaptive upload batch");
        upload_batch_active = true;
    }

    void EndUploadBatch() {
        DEBUG_ASSERT(adaptive_cpu_tracking.load(std::memory_order_relaxed));
        DEBUG_ASSERT_MSG(upload_batch_active, "Adaptive upload batch was not started");
        upload_batch_active = false;
        RestoreDeferredHotPages();
    }

    void NotifyCpuWriteFault(VAddr fault_address) noexcept {
        if (!adaptive_cpu_tracking.load(std::memory_order_acquire)) {
            return;
        }
        const VAddr page_addr = fault_address & ~(TRACKER_BYTES_PER_PAGE - 1);
        auto& candidate = candidate_pages[CandidateIndex(page_addr)];
        u64 observed = candidate.state.load(std::memory_order_relaxed);
        while (true) {
            const u8 count = static_cast<u8>(observed & CandidateCountMask);
            const VAddr observed_page = (observed >> CandidateCountBits) << TRACKER_PAGE_BITS;
            const bool matches = count != 0 && observed_page == page_addr;
            if (matches && count == CandidateCountMask) {
                return;
            }
            const u8 new_count = matches ? static_cast<u8>(count + 1) : 1;
            const u64 desired =
                ((page_addr >> TRACKER_PAGE_BITS) << CandidateCountBits) | new_count;
            if (candidate.state.compare_exchange_weak(observed, desired,
                                                      std::memory_order_relaxed)) {
                return;
            }
        }
    }

    /// Returns true if a region has been modified from the CPU
    bool IsRegionCpuModified(VAddr query_cpu_addr, u64 query_size) noexcept {
        return IteratePages<true>(
            query_cpu_addr, query_size, [](RegionManager* manager, u64 offset, size_t size) {
                std::scoped_lock lk{manager->lock};
                return manager->template IsRegionModified<Type::CPU>(offset, size);
            });
    }

    /// Returns true if a region has been modified from the GPU
    bool IsRegionGpuModified(VAddr query_cpu_addr, u64 query_size) noexcept {
        return IteratePages<false>(
            query_cpu_addr, query_size, [](RegionManager* manager, u64 offset, size_t size) {
                std::scoped_lock lk{manager->lock};
                return manager->template IsRegionModified<Type::GPU>(offset, size);
            });
    }

    /// Mark region as CPU modified, notifying the device_tracker about this change
    void MarkRegionAsCpuModified(VAddr dirty_cpu_addr, u64 query_size) {
        IteratePages<false>(dirty_cpu_addr, query_size,
                            [](RegionManager* manager, u64 offset, size_t size) {
                                std::scoped_lock lk{manager->lock};
                                manager->template ChangeRegionState<Type::CPU, true>(
                                    manager->GetCpuAddr() + offset, size);
                            });
    }

    /// Unmark region as modified from the host GPU
    void UnmarkRegionAsGpuModified(VAddr dirty_cpu_addr, u64 query_size) noexcept {
        IteratePages<false>(dirty_cpu_addr, query_size,
                            [](RegionManager* manager, u64 offset, size_t size) {
                                std::scoped_lock lk{manager->lock};
                                manager->template ChangeRegionState<Type::GPU, false>(
                                    manager->GetCpuAddr() + offset, size);
                            });
    }

    /// Removes all protection from a page and ensures GPU data has been flushed if requested
    void InvalidateRegion(VAddr cpu_addr, u64 size, auto&& on_flush) noexcept {
        IteratePages<false>(
            cpu_addr, size, [&on_flush](RegionManager* manager, u64 offset, size_t size) {
                const bool should_flush = [&] {
                    // Perform both the GPU modification check and CPU state change with the lock
                    // in case we are racing with GPU thread trying to mark the page as GPU
                    // modified. If we need to flush the flush function is going to perform CPU
                    // state change.
                    std::scoped_lock lk{manager->lock};
                    if (EmulatorSettings.GetReadbacksMode() != GpuReadbacksMode::Disabled &&
                        manager->template IsRegionModified<Type::GPU>(offset, size)) {
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
    void ForEachUploadRange(VAddr query_cpu_range, u64 query_size, bool is_written, auto&& func,
                            auto&& on_upload, u64 buffer_tracking_id = 0) {
        if (!adaptive_cpu_tracking.load(std::memory_order_acquire)) {
            IteratePages<true>(
                query_cpu_range, query_size,
                [&func, is_written](RegionManager* manager, u64 offset, size_t size) {
                    manager->lock.lock();
                    manager->template ForEachModifiedRange<Type::CPU, true>(
                        manager->GetCpuAddr() + offset, size, func);
                    if (!is_written) {
                        manager->lock.unlock();
                    }
                });
            on_upload();
            if (!is_written) {
                return;
            }
            IteratePages<false>(query_cpu_range, query_size,
                                [&func](RegionManager* manager, u64 offset, size_t size) {
                                    manager->template ChangeRegionState<Type::GPU, true>(
                                        manager->GetCpuAddr() + offset, size);
                                    manager->lock.unlock();
                                });
            return;
        }

        boost::container::small_vector<PendingHotPage, 8> pending_hot_pages;
        boost::container::small_vector<RegionManager*, 4> locked_managers;
        IteratePages<true>(
            query_cpu_range, query_size, [&](RegionManager* manager, u64 offset, size_t size) {
                // Clearing the CPU state below write-protects the page while this lock is held.
                // Keep the lock through staging and snapshot creation for changed hot pages, so a
                // concurrent guest write faults and waits instead of racing either copy.
                manager->lock.lock();
                bool keep_locked = is_written;
                boost::container::small_vector<VAddr, 8> restore_writable_pages;
                if (is_written) {
                    const VAddr range_begin =
                        manager->GetCpuAddr() + (offset & ~(TRACKER_BYTES_PER_PAGE - 1));
                    const VAddr range_end =
                        manager->GetCpuAddr() + ((offset + size + TRACKER_BYTES_PER_PAGE - 1) &
                                                 ~(TRACKER_BYTES_PER_PAGE - 1));
                    for (VAddr page_addr = range_begin; page_addr < range_end;
                         page_addr += TRACKER_BYTES_PER_PAGE) {
                        if (hot_pages.erase(page_addr) != 0) {
                            ResetCandidate(page_addr);
                        }
                    }
                }
                manager->template ForEachModifiedRange<Type::CPU, true>(
                    manager->GetCpuAddr() + offset, size, [&](VAddr dirty_addr, u64 dirty_size) {
                        const VAddr dirty_end = dirty_addr + dirty_size;
                        for (VAddr page_addr = dirty_addr; page_addr < dirty_end;
                             page_addr += TRACKER_BYTES_PER_PAGE) {
                            auto hot_it = hot_pages.find(page_addr);
                            if (hot_it != hot_pages.end() &&
                                manager->template IsRegionModified<Type::GPU>(
                                    page_addr - manager->GetCpuAddr(), TRACKER_BYTES_PER_PAGE)) {
                                hot_pages.erase(hot_it);
                                hot_it = hot_pages.end();
                                ResetCandidate(page_addr);
                            }
                            if (hot_it == hot_pages.end() && !is_written &&
                                hot_pages.size() < MaxHotPages &&
                                ConsumeFaultCandidate(page_addr)) {
                                hot_it = hot_pages.try_emplace(page_addr).first;
                            }

                            if (hot_it == hot_pages.end()) {
                                func(page_addr, TRACKER_BYTES_PER_PAGE);
                                continue;
                            }

                            HotPage& hot_page = hot_it->second;
                            const bool owner_changed =
                                hot_page.buffer_tracking_id != buffer_tracking_id;
                            const bool content_changed =
                                !hot_page.snapshot_valid || owner_changed ||
                                std::memcmp(hot_page.snapshot.data(),
                                            reinterpret_cast<const void*>(page_addr),
                                            TRACKER_BYTES_PER_PAGE) != 0;
                            if (content_changed) {
                                func(page_addr, TRACKER_BYTES_PER_PAGE);
                                pending_hot_pages.push_back(PendingHotPage{
                                    .manager = manager,
                                    .address = page_addr,
                                });
                                keep_locked = true;
                                continue;
                            }

                            if (++hot_page.unchanged_checks >= HotUnchangedCheckLimit) {
                                hot_pages.erase(hot_it);
                                ResetCandidate(page_addr);
                            } else {
                                QueueWritablePage(manager, page_addr, restore_writable_pages);
                            }
                        }
                    });
                RestoreWritablePages(manager, restore_writable_pages);
                if (keep_locked) {
                    if (!is_written) {
                        locked_managers.push_back(manager);
                    }
                } else {
                    manager->lock.unlock();
                }
            });
        on_upload();

        if (is_written) {
            IteratePages<false>(query_cpu_range, query_size,
                                [](RegionManager* manager, u64 offset, size_t size) {
                                    manager->template ChangeRegionState<Type::GPU, true>(
                                        manager->GetCpuAddr() + offset, size);
                                    manager->lock.unlock();
                                });
            return;
        }

        for (const PendingHotPage& pending : pending_hot_pages) {
            auto hot_it = hot_pages.find(pending.address);
            if (hot_it == hot_pages.end()) {
                continue;
            }
            HotPage& hot_page = hot_it->second;
            std::memcpy(hot_page.snapshot.data(), reinterpret_cast<const void*>(pending.address),
                        TRACKER_BYTES_PER_PAGE);
            hot_page.buffer_tracking_id = buffer_tracking_id;
            hot_page.unchanged_checks = 0;
            hot_page.snapshot_valid = true;
        }

        if (upload_batch_active) {
            deferred_hot_pages.insert(deferred_hot_pages.end(), pending_hot_pages.begin(),
                                      pending_hot_pages.end());
        } else {
            RestorePendingHotPages(pending_hot_pages);
        }
        for (RegionManager* manager : locked_managers) {
            manager->lock.unlock();
        }
    }

    /// Call 'func' for each GPU modified range and unmark those pages as GPU modified
    template <bool clear>
    void ForEachDownloadRange(VAddr query_cpu_range, u64 query_size, auto&& func) {
        IteratePages<false>(query_cpu_range, query_size,
                            [&func](RegionManager* manager, u64 offset, size_t size) {
                                std::scoped_lock lk{manager->lock};
                                manager->template ForEachModifiedRange<Type::GPU, clear>(
                                    manager->GetCpuAddr() + offset, size, func);
                            });
    }

private:
    // Promote only pages that repeatedly fault, then validate their exact contents at GPU use.
    // The fixed candidate table makes fault notification allocation-free; table collisions only
    // delay promotion because the complete page number is stored alongside the counter.
    static constexpr u64 CandidateCountBits = std::numeric_limits<u8>::digits;
    static constexpr u64 CandidateCountMask = std::numeric_limits<u8>::max();
    static constexpr size_t CandidateTableBits = 16;
    static constexpr size_t CandidateTableSize = 1ULL << CandidateTableBits;
    static constexpr size_t CandidateTableMask = CandidateTableSize - 1;
    static_assert(MAX_CPU_PAGE_BITS - TRACKER_PAGE_BITS + CandidateCountBits <= 64);
    static constexpr u8 HotFaultThreshold = 16;
    // Bound both comparison work and retained snapshots. A page that remains unchanged is returned
    // to fault tracking so cold memory never pays permanent comparison overhead.
    static constexpr u32 HotUnchangedCheckLimit = 32;
    static constexpr size_t MaxHotPages = 512;

    struct CandidatePage {
        std::atomic<u64> state{};
    };

    struct HotPage {
        std::array<u8, TRACKER_BYTES_PER_PAGE> snapshot{};
        u64 buffer_tracking_id{};
        u32 unchanged_checks{};
        bool snapshot_valid{};
    };

    struct PendingHotPage {
        RegionManager* manager{};
        VAddr address{};
    };

    static size_t CandidateIndex(VAddr page_addr) noexcept {
        const u64 page = page_addr >> TRACKER_PAGE_BITS;
        return static_cast<size_t>(page ^ (page >> CandidateTableBits)) & CandidateTableMask;
    }

    bool ConsumeFaultCandidate(VAddr page_addr) noexcept {
        auto& candidate = candidate_pages[CandidateIndex(page_addr)];
        u64 observed = candidate.state.load(std::memory_order_acquire);
        while (true) {
            const u8 count = static_cast<u8>(observed & CandidateCountMask);
            const VAddr observed_page = (observed >> CandidateCountBits) << TRACKER_PAGE_BITS;
            if (count < HotFaultThreshold || observed_page != page_addr) {
                return false;
            }
            if (candidate.state.compare_exchange_weak(observed, 0, std::memory_order_acq_rel,
                                                      std::memory_order_acquire)) {
                return true;
            }
        }
    }

    void ResetCandidate(VAddr page_addr) noexcept {
        auto& candidate = candidate_pages[CandidateIndex(page_addr)];
        u64 observed = candidate.state.load(std::memory_order_acquire);
        while (true) {
            const u8 count = static_cast<u8>(observed & CandidateCountMask);
            const VAddr observed_page = (observed >> CandidateCountBits) << TRACKER_PAGE_BITS;
            if (count == 0 || observed_page != page_addr) {
                return;
            }
            if (candidate.state.compare_exchange_weak(observed, 0, std::memory_order_acq_rel,
                                                      std::memory_order_acquire)) {
                return;
            }
        }
    }

    static void RestorePendingHotPages(const auto& pages) {
        for (size_t index = 0; index < pages.size();) {
            RegionManager* manager = pages[index].manager;
            const VAddr range_begin = pages[index].address;
            VAddr range_end = range_begin + TRACKER_BYTES_PER_PAGE;
            ++index;
            while (index < pages.size() && pages[index].manager == manager &&
                   pages[index].address == range_end) {
                range_end += TRACKER_BYTES_PER_PAGE;
                ++index;
            }
            manager->template ChangeRegionState<Type::CPU, true>(range_begin,
                                                                 range_end - range_begin);
        }
    }

    static void RestoreWritablePages(RegionManager* manager, const auto& pages) {
        for (size_t index = 0; index < pages.size();) {
            const VAddr range_begin = pages[index];
            VAddr range_end = range_begin + TRACKER_BYTES_PER_PAGE;
            ++index;
            while (index < pages.size() && pages[index] == range_end) {
                range_end += TRACKER_BYTES_PER_PAGE;
                ++index;
            }
            manager->template ChangeRegionState<Type::CPU, true>(range_begin,
                                                                 range_end - range_begin);
        }
    }

    void QueueWritablePage(RegionManager* manager, VAddr page_addr, auto& immediate_pages) {
        if (upload_batch_active) {
            // Keep already validated pages protected until this draw/dispatch finishes. This avoids
            // repeatedly validating the same hot page for several bindings in one operation.
            deferred_hot_pages.push_back(PendingHotPage{
                .manager = manager,
                .address = page_addr,
            });
        } else {
            immediate_pages.push_back(page_addr);
        }
    }

    void RestoreDeferredHotPages() {
        std::ranges::sort(deferred_hot_pages,
                          [](const PendingHotPage& left, const PendingHotPage& right) {
                              if (left.manager != right.manager) {
                                  return std::less<RegionManager*>{}(left.manager, right.manager);
                              }
                              return left.address < right.address;
                          });

        for (size_t index = 0; index < deferred_hot_pages.size();) {
            RegionManager* manager = deferred_hot_pages[index].manager;
            std::scoped_lock lock{manager->lock};
            bool has_range = false;
            VAddr range_begin{};
            VAddr range_end{};
            while (index < deferred_hot_pages.size() &&
                   deferred_hot_pages[index].manager == manager) {
                const VAddr page_addr = deferred_hot_pages[index++].address;
                if (!hot_pages.contains(page_addr)) {
                    continue;
                }
                const VAddr page_end = page_addr + static_cast<VAddr>(TRACKER_BYTES_PER_PAGE);
                if (!has_range) {
                    has_range = true;
                    range_begin = page_addr;
                    range_end = page_end;
                } else if (page_addr <= range_end) {
                    range_end = std::max(range_end, page_end);
                } else {
                    manager->template ChangeRegionState<Type::CPU, true>(range_begin,
                                                                         range_end - range_begin);
                    range_begin = page_addr;
                    range_end = page_end;
                }
            }
            if (has_range) {
                manager->template ChangeRegionState<Type::CPU, true>(range_begin,
                                                                     range_end - range_begin);
            }
        }
        deferred_hot_pages.clear();
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
        RENDERER_TRACE;
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
    std::atomic_bool adaptive_cpu_tracking{};
    bool upload_batch_active{};
    std::array<CandidatePage, CandidateTableSize> candidate_pages{};
    std::unordered_map<VAddr, HotPage> hot_pages;
    std::vector<PendingHotPage> deferred_hot_pages;
    std::deque<std::array<RegionManager, MANAGER_POOL_SIZE>> manager_pool;
    std::vector<RegionManager*> free_managers;
    std::array<RegionManager*, NUM_HIGH_PAGES> top_tier{};
};

} // namespace VideoCore
