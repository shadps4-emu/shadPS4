// SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <iterator>
#include <map>
#include <boost/icl/discrete_interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/split_interval_set.hpp>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/poolfwd.hpp>
#include "common/types.h"

namespace VideoCore {

template <class T>
using RangeSetsAllocator =
    boost::fast_pool_allocator<T, boost::default_user_allocator_new_delete,
                               boost::details::pool::default_mutex, 1024, 2048>;

struct RangeSet {
    explicit RangeSet() = default;
    ~RangeSet() = default;

    void Add(VAddr base_address, size_t size) {
        if (size == 0) {
            return;
        }
        VAddr start = base_address;
        VAddr end = base_address + size;
        auto it = m_ranges.upper_bound(start);
        if (it != m_ranges.begin()) {
            const auto prev = std::prev(it);
            if (prev->second >= start) {
                start = prev->first;
                end = std::max(end, prev->second);
                it = m_ranges.erase(prev);
            }
        }
        while (it != m_ranges.end() && it->first <= end) {
            end = std::max(end, it->second);
            it = m_ranges.erase(it);
        }
        m_ranges.emplace_hint(it, start, end);
    }

    void Subtract(VAddr base_address, size_t size) {
        if (size == 0) {
            return;
        }
        const VAddr start = base_address;
        const VAddr end = base_address + size;
        auto it = m_ranges.lower_bound(start);
        if (it != m_ranges.begin()) {
            const auto prev = std::prev(it);
            if (prev->second > start) {
                const VAddr prev_end = prev->second;
                prev->second = start;
                if (prev_end > end) {
                    m_ranges.emplace_hint(it, end, prev_end);
                    return;
                }
            }
        }
        while (it != m_ranges.end() && it->first < end) {
            if (it->second > end) {
                const VAddr range_end = it->second;
                it = m_ranges.erase(it);
                m_ranges.emplace_hint(it, end, range_end);
                return;
            }
            it = m_ranges.erase(it);
        }
    }

    void Clear() {
        m_ranges.clear();
    }

    bool Contains(VAddr base_address, size_t size) const {
        if (size == 0) {
            return false;
        }
        auto it = m_ranges.upper_bound(base_address);
        if (it == m_ranges.begin()) {
            return false;
        }
        --it;
        return it->first <= base_address && base_address + size <= it->second;
    }

    bool Intersects(VAddr base_address, size_t size) const {
        if (size == 0) {
            return false;
        }
        auto it = m_ranges.lower_bound(base_address);
        if (it != m_ranges.end() && it->first < base_address + size) {
            return true;
        }
        return it != m_ranges.begin() && std::prev(it)->second > base_address;
    }

    template <typename Func>
    void ForEach(Func&& func) const {
        for (const auto& [start, end] : m_ranges) {
            func(start, end);
        }
    }

    template <typename Func>
    void ForEachInRange(VAddr base_addr, size_t size, Func&& func) const {
        if (size == 0 || m_ranges.empty()) {
            return;
        }
        const VAddr start_address = base_addr;
        const VAddr end_address = start_address + size;
        auto it = m_ranges.upper_bound(start_address);
        if (it != m_ranges.begin() && std::prev(it)->second > start_address) {
            --it;
        }
        for (; it != m_ranges.end() && it->first < end_address; ++it) {
            func(std::max(it->first, start_address), std::min(it->second, end_address));
        }
    }

    template <typename Func>
    void ForEachNotInRange(VAddr base_addr, size_t size, Func&& func) const {
        const VAddr end_addr = base_addr + size;
        ForEachInRange(base_addr, size, [&](VAddr range_addr, VAddr range_end) {
            if (size_t gap_size = range_addr - base_addr; gap_size != 0) {
                func(base_addr, gap_size);
            }
            base_addr = range_end;
        });
        if (base_addr != end_addr) {
            func(base_addr, end_addr - base_addr);
        }
    }

private:
    std::map<VAddr, VAddr> m_ranges;
};

template <typename T>
class RangeMap {
public:
    using IntervalMap =
        boost::icl::interval_map<VAddr, T, boost::icl::total_absorber, std::less,
                                 boost::icl::inplace_identity, boost::icl::inter_section,
                                 ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, VAddr, std::less),
                                 RangeSetsAllocator>;
    using IntervalType = typename IntervalMap::interval_type;

public:
    RangeMap() = default;
    ~RangeMap() = default;

    RangeMap(RangeMap const&) = delete;
    RangeMap& operator=(RangeMap const&) = delete;

    RangeMap(RangeMap&& other);
    RangeMap& operator=(RangeMap&& other);

    void Add(VAddr base_address, size_t size, const T& value) {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        m_ranges_map.add({interval, value});
    }

    void Subtract(VAddr base_address, size_t size) {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        m_ranges_map -= interval;
    }

    void Clear() {
        m_ranges_map.clear();
    }

    bool Contains(VAddr base_address, size_t size) const {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        return boost::icl::contains(m_ranges_map, interval);
    }

    bool Intersects(VAddr base_address, size_t size) const {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        return boost::icl::intersects(m_ranges_map, interval);
    }

    template <typename Func>
    void ForEach(Func&& func) const {
        if (m_ranges_map.empty()) {
            return;
        }

        for (const auto& [interval, value] : m_ranges_map) {
            const VAddr inter_addr_end = interval.upper();
            const VAddr inter_addr = interval.lower();
            func(inter_addr, inter_addr_end, value);
        }
    }

    template <typename Func>
    void ForEachInRange(VAddr base_addr, size_t size, Func&& func) const {
        if (m_ranges_map.empty() || size == 0) {
            return;
        }
        const VAddr start_address = base_addr;
        const VAddr end_address = start_address + size;
        const IntervalType search_interval{start_address, end_address};
        auto it = m_ranges_map.lower_bound(search_interval);
        if (it == m_ranges_map.end()) {
            return;
        }
        auto end_it = m_ranges_map.upper_bound(search_interval);
        for (; it != end_it; it++) {
            VAddr inter_addr_end = it->first.upper();
            VAddr inter_addr = it->first.lower();
            if (inter_addr_end > end_address) {
                inter_addr_end = end_address;
            }
            if (inter_addr < start_address) {
                inter_addr = start_address;
            }
            func(inter_addr, inter_addr_end, it->second);
        }
    }

    template <typename Func>
    void ForEachNotInRange(VAddr base_addr, size_t size, Func&& func) const {
        const VAddr end_addr = base_addr + size;
        ForEachInRange(base_addr, size, [&](VAddr range_addr, VAddr range_end, const T&) {
            if (size_t gap_size = range_addr - base_addr; gap_size != 0) {
                func(base_addr, gap_size);
            }
            base_addr = range_end;
        });
        if (base_addr != end_addr) {
            func(base_addr, end_addr - base_addr);
        }
    }

private:
    IntervalMap m_ranges_map;
};

template <typename T>
class SplitRangeMap {
public:
    using IntervalMap = boost::icl::split_interval_map<
        VAddr, T, boost::icl::total_absorber, std::less, boost::icl::inplace_identity,
        boost::icl::inter_section, ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, VAddr, std::less),
        RangeSetsAllocator>;
    using IntervalType = typename IntervalMap::interval_type;

public:
    SplitRangeMap() = default;
    ~SplitRangeMap() = default;

    SplitRangeMap(SplitRangeMap const&) = delete;
    SplitRangeMap& operator=(SplitRangeMap const&) = delete;

    SplitRangeMap(SplitRangeMap&& other);
    SplitRangeMap& operator=(SplitRangeMap&& other);

    void Add(VAddr base_address, size_t size, const T& value) {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        m_ranges_map.add({interval, value});
    }

    void Subtract(VAddr base_address, size_t size) {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        m_ranges_map -= interval;
    }

    void Clear() {
        m_ranges_map.clear();
    }

    bool Contains(VAddr base_address, size_t size) const {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        return boost::icl::contains(m_ranges_map, interval);
    }

    bool Intersects(VAddr base_address, size_t size) const {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        return boost::icl::intersects(m_ranges_map, interval);
    }

    template <typename Func>
    void ForEach(Func&& func) const {
        if (m_ranges_map.empty()) {
            return;
        }

        for (const auto& [interval, value] : m_ranges_map) {
            const VAddr inter_addr_end = interval.upper();
            const VAddr inter_addr = interval.lower();
            func(inter_addr, inter_addr_end, value);
        }
    }

    template <typename Func>
    void ForEachInRange(VAddr base_addr, size_t size, Func&& func) const {
        if (m_ranges_map.empty() || size == 0) {
            return;
        }
        const VAddr start_address = base_addr;
        const VAddr end_address = start_address + size;
        const IntervalType search_interval{start_address, end_address};
        auto it = m_ranges_map.lower_bound(search_interval);
        if (it == m_ranges_map.end()) {
            return;
        }
        auto end_it = m_ranges_map.upper_bound(search_interval);
        for (; it != end_it; it++) {
            VAddr inter_addr_end = it->first.upper();
            VAddr inter_addr = it->first.lower();
            if (inter_addr_end > end_address) {
                inter_addr_end = end_address;
            }
            if (inter_addr < start_address) {
                inter_addr = start_address;
            }
            func(inter_addr, inter_addr_end, it->second);
        }
    }

    template <typename Func>
    void ForEachNotInRange(VAddr base_addr, size_t size, Func&& func) const {
        const VAddr end_addr = base_addr + size;
        ForEachInRange(base_addr, size, [&](VAddr range_addr, VAddr range_end, const T&) {
            if (size_t gap_size = range_addr - base_addr; gap_size != 0) {
                func(base_addr, gap_size);
            }
            base_addr = range_end;
        });
        if (base_addr != end_addr) {
            func(base_addr, end_addr - base_addr);
        }
    }

private:
    IntervalMap m_ranges_map;
};

} // namespace VideoCore
