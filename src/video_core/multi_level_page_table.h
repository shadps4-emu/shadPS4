// SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/object_pool.h"
#include "common/spin_lock.h"

namespace VideoCore {

template <class Traits>
class MultiLevelPageTable final {
    using Entry = typename Traits::Entry;

    static constexpr size_t FirstLevelBits = Traits::FirstLevelBits;
    static constexpr size_t SecondLevelBits =
        Traits::AddressSpaceBits - FirstLevelBits - Traits::PageBits;
    static constexpr size_t NumEntriesPerL1Page = 1ULL << SecondLevelBits;
    static constexpr bool NullCheck = Traits::NullCheck;

    using L1Page = std::array<Entry, NumEntriesPerL1Page>;

public:
    explicit MultiLevelPageTable(size_t chunk_size = 64)
        : top_level{1ULL << FirstLevelBits, nullptr}, top_locks{1ULL << FirstLevelBits},
          page_alloc{chunk_size} {}

    ~MultiLevelPageTable() noexcept = default;

    [[nodiscard]] Entry* find(size_t page) {
        const size_t l1_page = page >> SecondLevelBits;
        const size_t l2_page = page & (NumEntriesPerL1Page - 1);
        if (NullCheck && !top_level[l1_page]) [[unlikely]] {
            return nullptr;
        }
        return &(*top_level[l1_page])[l2_page];
    }

    [[nodiscard]] const Entry* find(size_t page) const {
        const size_t l1_page = page >> SecondLevelBits;
        const size_t l2_page = page & (NumEntriesPerL1Page - 1);
        if (!top_level[l1_page]) [[unlikely]] {
            return nullptr;
        }
        return &(*top_level[l1_page])[l2_page];
    }

    void reserve(size_t start_page, size_t end_page) {
        const size_t start_l1_page = start_page >> SecondLevelBits;
        const size_t end_l1_page = end_page >> SecondLevelBits;
        for (size_t l1_page = start_l1_page; l1_page <= end_l1_page; ++l1_page) {
            if (!top_level[l1_page]) {
                top_level[l1_page] = page_alloc.Create();
            }
        }
    }

    [[nodiscard]] const Entry& operator[](size_t page) const {
        const size_t l1_page = page >> SecondLevelBits;
        const size_t l2_page = page & (NumEntriesPerL1Page - 1);
        if (NullCheck && !top_level[l1_page]) [[unlikely]] {
            top_locks[l1_page].lock();
            if (!top_level[l1_page]) {
                top_level[l1_page] = page_alloc.Create();
            }
            top_locks[l1_page].unlock();
        }
        return (*top_level[l1_page])[l2_page];
    }

    [[nodiscard]] Entry& operator[](size_t page) {
        const size_t l1_page = page >> SecondLevelBits;
        const size_t l2_page = page & (NumEntriesPerL1Page - 1);
        if (NullCheck && !top_level[l1_page]) [[unlikely]] {
            top_locks[l1_page].lock();
            if (!top_level[l1_page]) {
                top_level[l1_page] = page_alloc.Create();
            }
            top_locks[l1_page].unlock();
        }
        return (*top_level[l1_page])[l2_page];
    }

private:
    std::vector<L1Page*> top_level{};
    mutable std::vector<Common::SpinLock> top_locks{};
    Common::ObjectPool<L1Page> page_alloc;
};

} // namespace VideoCore
