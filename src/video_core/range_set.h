// SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
    using IntervalSet =
        boost::icl::interval_set<VAddr, std::less,
                                 ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, VAddr, std::less),
                                 RangeSetsAllocator>;
    using IntervalType = typename IntervalSet::interval_type;

    explicit RangeSet() = default;
    ~RangeSet() = default;

    void Add(VAddr base_address, size_t size) {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        m_ranges_set.add(interval);
    }

    void Subtract(VAddr base_address, size_t size) {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        m_ranges_set.subtract(interval);
    }

    void Clear() {
        m_ranges_set.clear();
    }

    bool Contains(VAddr base_address, size_t size) const {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        return boost::icl::contains(m_ranges_set, interval);
    }

    bool Intersects(VAddr base_address, size_t size) const {
        const VAddr end_address = base_address + size;
        IntervalType interval{base_address, end_address};
        return boost::icl::intersects(m_ranges_set, interval);
    }

    template <typename Func>
    void ForEach(Func&& func) const {
        if (m_ranges_set.empty()) {
            return;
        }

        for (const auto& set : m_ranges_set) {
            const VAddr inter_addr_end = set.upper();
            const VAddr inter_addr = set.lower();
            func(inter_addr, inter_addr_end - inter_addr);
        }
    }

    template <typename Func>
    void ForEachInRange(VAddr base_addr, size_t size, Func&& func) const {
        if (m_ranges_set.empty()) {
            return;
        }
        const VAddr start_address = base_addr;
        const VAddr end_address = start_address + size;
        const IntervalType search_interval{start_address, end_address};
        auto it = m_ranges_set.lower_bound(search_interval);
        if (it == m_ranges_set.end()) {
            return;
        }
        auto end_it = m_ranges_set.upper_bound(search_interval);
        for (; it != end_it; it++) {
            VAddr inter_addr_end = it->upper();
            VAddr inter_addr = it->lower();
            if (inter_addr_end > end_address) {
                inter_addr_end = end_address;
            }
            if (inter_addr < start_address) {
                inter_addr = start_address;
            }
            func(inter_addr, inter_addr_end - inter_addr);
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

    IntervalSet m_ranges_set;
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
            func(inter_addr, inter_addr_end - inter_addr, value);
        }
    }

    template <typename Func>
    void ForEachInRange(VAddr base_addr, size_t size, Func&& func) const {
        if (m_ranges_map.empty()) {
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
            func(inter_addr, inter_addr_end - inter_addr, it->second);
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
            func(inter_addr, inter_addr_end - inter_addr, value);
        }
    }

    template <typename Func>
    void ForEachInRange(VAddr base_addr, size_t size, Func&& func) const {
        if (m_ranges_map.empty()) {
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
            func(inter_addr, inter_addr_end - inter_addr, it->second);
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
