// SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <concepts>
#include <iterator>
#include <vector>

#include "common/types.h"

struct Interval {
    u64 start;
    u64 end;

    constexpr bool CanMergeWith(const Interval&) const noexcept {
        return true;
    }
    constexpr Interval SubRange(u64 a, u64 b) const noexcept {
        return {a, b};
    }
};

template <class IV>
concept IsInterval = std::derived_from<IV, Interval> && std::is_trivially_copyable_v<IV> &&
                     requires(const IV& a, const IV& b, u64 x) {
                         { a.CanMergeWith(b) } -> std::same_as<bool>;
                         { a.SubRange(x, x) } -> std::same_as<IV>;
                     };

template <class IV = Interval>
    requires IsInterval<IV>
class IntervalList {
public:
    using List = std::vector<IV>;
    using const_iterator = typename List::const_iterator;
    using iterator = typename List::iterator;

    const_iterator begin() const {
        return intervals.begin();
    }
    const_iterator end() const {
        return intervals.end();
    }

    void Clear() {
        intervals.clear();
    }
    bool Empty() const {
        return intervals.empty();
    }
    std::size_t Size() const {
        return intervals.size();
    }
    void Reserve(std::size_t n) {
        intervals.reserve(n);
    }

    /// Adds a range to the interval set.
    void Add(IV value) {
        if (value.start >= value.end) [[unlikely]] {
            return;
        }

        IV out[3];
        std::size_t m = 0;

        auto [first, last] = OverlapRun(value.start, value.end);
        if (first != last) {
            if (first->start < value.start) {
                out[m++] = first->SubRange(first->start, value.start);
            }
            out[m++] = value;
            auto prev = std::prev(last);
            if (prev->end > value.end) {
                out[m++] = prev->SubRange(value.end, prev->end);
            }
        } else {
            out[m++] = value;
        }

        const auto at = std::distance(intervals.begin(), first);
        Splice(first, last, out, m);
        Coalesce(at, m);
    }

    /// Returns immutable iterator to an interval that contains provided address.
    const_iterator Find(u64 addr) const {
        auto it = std::ranges::upper_bound(intervals, addr, {}, &IV::start);
        if (it == intervals.begin()) {
            return intervals.end();
        }
        --it;
        return (it->start <= addr && addr < it->end) ? it : intervals.end();
    }

    /// Returns true if address if contained inside an interval.
    bool Contains(u64 addr) const {
        return Find(addr) != intervals.end();
    }

    /// Returns true if range is fully contained in the interval set.
    bool Contains(u64 start, u64 end) const {
        if (start >= end) [[unlikely]] {
            return false;
        }
        auto it = std::ranges::upper_bound(intervals, start, {}, &IV::end);
        u64 cur = start;
        while (it != intervals.end() && it->start <= cur && cur < end) {
            cur = it->end;
            ++it;
        }
        return cur >= end;
    }

    /// Returns true if range partially overlaps an interval.
    bool Overlaps(u64 start, u64 end) const {
        if (start >= end) [[unlikely]] {
            return false;
        }
        auto it = std::ranges::upper_bound(intervals, start, {}, &IV::end);
        return it != intervals.end() && it->start < end;
    }

    /// Calls 'func' for each gap in provided range.
    void ForEachGap(u64 start, u64 end, auto&& func) const {
        if (start >= end) [[unlikely]] {
            return;
        }
        auto it = std::ranges::upper_bound(intervals, start, {}, &IV::end);
        u64 cur = start;
        for (; it != intervals.end() && it->start < end; ++it) {
            if (it->start > cur) {
                func(cur, it->start);
            }
            cur = std::max(cur, it->end);
        }
        if (cur < end) {
            func(cur, end);
        }
    }

    /// Calls 'func' for each interval that partially overlaps provided range.
    void ForEachInRange(u64 start, u64 end, auto&& func) const {
        if (start >= end) [[unlikely]] {
            return;
        }
        auto it = std::ranges::upper_bound(intervals, start, {}, &IV::end);
        if (it == intervals.end()) {
            return;
        }
        while (it != intervals.end() && it->start < end) {
            func(*it);
            ++it;
        }
    }

protected:
    /// Returns first interval that overlaps range and first interval after range.
    std::pair<iterator, iterator> OverlapRun(u64 start, u64 end) noexcept {
        auto first = std::ranges::upper_bound(intervals, start, {}, &IV::end);
        auto last = first;
        while (last != intervals.end() && last->start <= end) {
            ++last;
        }
        return {first, last};
    }

    /// Inserts intervals from first until last
    void Splice(iterator first, iterator last, const IV* src, std::size_t m) noexcept {
        if (first == last) {
            intervals.insert(first, src, src + m);
            return;
        }
        const auto slots = std::distance(first, last);
        if (m <= slots) {
            std::copy(src, src + m, first);
            if (m < slots) {
                intervals.erase(first + m, last);
            }
        } else {
            std::copy(src, src + slots, first);
            intervals.insert(last, src + slots, src + m);
        }
    }

    /// Merges compatible adjacent intervals.
    void Coalesce(std::size_t start, std::size_t count) {
        auto stop = std::min(start + count + 1, intervals.size());
        for (auto i = start ? start - 1 : start; i + 1 < stop;) {
            IV& a = intervals[i];
            IV& b = intervals[i + 1];
            if (a.end == b.start && a.CanMergeWith(b)) {
                a.end = b.end;
                intervals.erase(intervals.begin() + i + 1);
                --stop;
            } else {
                ++i;
            }
        }
    }

    List intervals;
};
