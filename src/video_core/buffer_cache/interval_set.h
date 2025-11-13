// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <vector>

#include "common/types.h"

struct Interval {
    u64 lo;
    u64 hi;

    constexpr bool CanMergeWith(const Interval&) const noexcept {
        return true;
    }
};

template <class IV>
concept IntervalOf = std::derived_from<IV, Interval> && requires(const IV& a, const IV& b) {
    { a.CanMergeWith(b) } -> std::same_as<bool>;
};

template <class IV = Interval>
    requires IntervalOf<IV>
class IntervalSet {
public:
    using Storage = std::vector<IV>;
    using iterator = Storage::iterator;
    using const_iterator = Storage::const_iterator;

    void reserve(std::size_t n) {
        v_.reserve(n);
    }
    void clear() {
        v_.clear();
    }
    bool empty() const {
        return v_.empty();
    }
    std::size_t size() const {
        return v_.size();
    }

    const_iterator begin() const {
        return v_.begin();
    }
    const_iterator end() const {
        return v_.end();
    }

    void add(IV n) {
        if (n.lo >= n.hi) {
            return;
        }

        // First overlapping or following interval after intertion range
        auto first = std::ranges::upper_bound(v_, n.lo, {}, &IV::hi);
        // First non-overlapping interval after insertion range
        auto last = std::ranges::lower_bound(v_, n.hi, {}, &IV::lo);

        IV left_piece, right_piece;
        bool have_left{}, have_right{};

        if (first != last) {
            const IV& left = *first;
            if (left.lo < n.lo) {
                if (left.CanMergeWith(n)) {
                    n.lo = left.lo;
                } else {
                    left_piece = left;
                    left_piece.hi = n.lo;
                    have_left = true;
                }
            }
            const IV& right = *std::prev(last);
            if (right.hi > n.hi) {
                if (right.CanMergeWith(n))
                    n.hi = right.hi;
                else {
                    right_piece = right;
                    right_piece.lo = n.hi;
                    have_right = true;
                }
            }
        }

        auto erase_from = first, erase_to = last;

        // Merge with non-overlapping mergeable neighbours
        if (!have_left && erase_from != v_.begin()) {
            auto p = std::prev(erase_from);
            if (p->hi == n.lo && p->CanMergeWith(n)) {
                n.lo = p->lo;
                erase_from = p;
            }
        }
        if (!have_right && erase_to != v_.end()) {
            if (erase_to->lo == n.hi && erase_to->CanMergeWith(n)) {
                n.hi = erase_to->hi;
                ++erase_to;
            }
        }

        IV buf[3];
        std::size_t num_insert = 0;
        if (have_left) {
            buf[num_insert++] = left_piece;
        }
        buf[num_insert++] = n;
        if (have_right) {
            buf[num_insert++] = right_piece;
        }

        splice(erase_from, erase_to, buf, num_insert);
    }

    const iterator find(u64 point) const {
        const auto it = std::ranges::upper_bound(v_, point, {}, &IV::lo);
        if (it == v_.begin()) {
            return v_.end();
        }
        --it;
        return (it->lo <= point && point < it->hi) ? it : v_.end();
    }

    bool contains(u64 point) const {
        return find(point) != v_.end();
    }

    bool contains(u64 lo, u64 hi) const {
        if (lo >= hi) {
            return false;
        }
        auto it = std::ranges::upper_bound(v_, lo, {}, &IV::lo);
        if (it == v_.begin()) {
            return false;
        }
        --it;
        return it->lo <= lo && hi <= it->hi;
    }

    bool intersects(u64 lo, u64 hi) const {
        if (lo >= hi) {
            return false;
        }
        const auto i = std::ranges::upper_bound(v_, lo, {}, &IV::hi);
        return i != v_.end() && i->lo < hi;
    }

private:
    void splice(iterator first, iterator last, const IV* src, std::size_t num_insert) noexcept {
        if (first == last) {
            v_.insert(first, src, src + num_insert);
            return;
        }
        const std::size_t num_slots = std::distance(first, last);
        if (num_insert <= num_slots) {
            std::copy(src, src + num_insert, first);
            if (num_insert < num_slots) {
                v_.erase(first + num_insert, last);
            }
        } else {
            std::copy(src, src + num_slots, first);
            v_.insert(last, src + num_slots, src + num_insert);
        }
    }

    Storage v_;
};
