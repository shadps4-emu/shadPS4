// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <utility>
#include "common/div_ceil.h"

#ifdef __linux__
#include "common/adaptive_mutex.h"
#else
#include "common/spin_lock.h"
#endif
#include "common/debug.h"
#include "common/types.h"
#include "video_core/buffer_cache/region_definitions.h"
#include "video_core/page_manager.h"

namespace VideoCore {

/**
 * Allows tracking CPU and GPU modification of pages in a contigious 4MB virtual address region.
 * Information is stored in bitsets for spacial locality and fast update of single pages.
 */
class RegionManager {
public:
    explicit RegionManager(PageManager* tracker_, VAddr cpu_addr_)
        : tracker{tracker_}, cpu_addr{cpu_addr_} {
        cpu.Fill();
        gpu.Clear();
        writeable.Fill();
    }
    explicit RegionManager() = default;

    void SetCpuAddress(VAddr new_cpu_addr) {
        cpu_addr = new_cpu_addr;
    }

    VAddr GetCpuAddr() const {
        return cpu_addr;
    }

    static constexpr size_t SanitizeAddress(size_t address) {
        return static_cast<size_t>(std::max<s64>(static_cast<s64>(address), 0LL));
    }

    template <Type type>
    RegionBits& GetRegionBits() noexcept {
        static_assert(type != Type::Writeable);
        if constexpr (type == Type::CPU) {
            return cpu;
        } else if constexpr (type == Type::GPU) {
            return gpu;
        } else if constexpr (type == Type::Writeable) {
            return writeable;
        } else {
            static_assert(false, "Invalid type");
        }
    }

    template <Type type>
    const RegionBits& GetRegionBits() const noexcept {
        static_assert(type != Type::Writeable);
        if constexpr (type == Type::CPU) {
            return cpu;
        } else if constexpr (type == Type::GPU) {
            return gpu;
        } else if constexpr (type == Type::Writeable) {
            return writeable;
        } else {
            static_assert(false, "Invalid type");
        }
    }

    /**
     * Change the state of a range of pages
     *
     * @param dirty_addr    Base address to mark or unmark as modified
     * @param size          Size in bytes to mark or unmark as modified
     */
    template <Type type, bool enable>
    void ChangeRegionState(u64 dirty_addr, u64 size) noexcept(type == Type::GPU) {
        RENDERER_TRACE;
        const size_t offset = dirty_addr - cpu_addr;
        const size_t start_page = SanitizeAddress(offset) / BYTES_PER_PAGE;
        const size_t end_page = Common::DivCeil(SanitizeAddress(offset + size), BYTES_PER_PAGE);
        if (start_page >= NUM_REGION_PAGES || end_page <= start_page) {
            return;
        }
        std::scoped_lock lk{lock};
        static_assert(type != Type::Writeable);

        RegionBits& bits = GetRegionBits<type>();
        if constexpr (enable) {
            bits.SetRange(start_page, end_page);
        } else {
            bits.UnsetRange(start_page, end_page);
        }
        if constexpr (type == Type::CPU) {
            UpdateProtection<!enable>();
        }
    }

    /**
     * Loop over each page in the given range, turn off those bits and notify the tracker if
     * needed. Call the given function on each turned off range.
     *
     * @param query_cpu_range Base CPU address to loop over
     * @param size            Size in bytes of the CPU range to loop over
     * @param func            Function to call for each turned off region
     */
    template <Type type, bool clear>
    void ForEachModifiedRange(VAddr query_cpu_range, s64 size, auto&& func) {
        RENDERER_TRACE;
        const size_t offset = query_cpu_range - cpu_addr;
        const size_t start_page = SanitizeAddress(offset) / BYTES_PER_PAGE;
        const size_t end_page = Common::DivCeil(SanitizeAddress(offset + size), BYTES_PER_PAGE);
        if (start_page >= NUM_REGION_PAGES || end_page <= start_page) {
            return;
        }
        std::scoped_lock lk{lock};
        static_assert(type != Type::Writeable);

        RegionBits& bits = GetRegionBits<type>();
        RegionBits mask(bits, start_page, end_page);

        // TODO: this will not be needed once we handle readbacks
        if constexpr (type == Type::GPU) {
            mask &= ~writeable;
        }

        for (const auto& [start, end] : mask) {
            func(cpu_addr + start * BYTES_PER_PAGE, (end - start) * BYTES_PER_PAGE);
        }

        if constexpr (clear) {
            bits.UnsetRange(start_page, end_page);
            if constexpr (type == Type::CPU) {
                UpdateProtection<true>();
            }
        }
    }

    /**
     * Returns true when a region has been modified
     *
     * @param offset Offset in bytes from the start of the buffer
     * @param size   Size in bytes of the region to query for modifications
     */
    template <Type type>
    [[nodiscard]] bool IsRegionModified(u64 offset, u64 size) const noexcept {
        RENDERER_TRACE;
        const size_t start_page = SanitizeAddress(offset) / BYTES_PER_PAGE;
        const size_t end_page = Common::DivCeil(SanitizeAddress(offset + size), BYTES_PER_PAGE);
        if (start_page >= NUM_REGION_PAGES || end_page <= start_page) {
            return false;
        }
        // std::scoped_lock lk{lock}; // Is this needed?
        static_assert(type != Type::Writeable);

        const RegionBits& bits = GetRegionBits<type>();
        RegionBits test(bits, start_page, end_page);

        // TODO: this will not be needed once we handle readbacks
        if constexpr (type == Type::GPU) {
            test &= ~writeable;
        }

        return test.Any();
    }

private:
    /**
     * Notify tracker about changes in the CPU tracking state of a word in the buffer
     *
     * @param word_index   Index to the word to notify to the tracker
     * @param current_bits Current state of the word
     * @param new_bits     New state of the word
     *
     * @tparam add_to_tracker True when the tracker should start tracking the new pages
     */
    template <bool add_to_tracker>
    void UpdateProtection() {
        RENDERER_TRACE;
        RegionBits mask = cpu ^ writeable;

        if (mask.None()) {
            return; // No changes to the CPU tracking state
        }

        writeable = cpu;
        tracker->UpdatePageWatchersForRegion<add_to_tracker>(cpu_addr, mask);
    }

#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
    Common::AdaptiveMutex lock;
#else
    Common::SpinLock lock;
#endif
    PageManager* tracker;
    VAddr cpu_addr = 0;
    RegionBits cpu;
    RegionBits gpu;
    RegionBits writeable;
};

} // namespace VideoCore
