// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/icl/interval_set.hpp>

#include <limits>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

#include <memory>

namespace Common {

template <class Index, class T, Index MaxIndex = std::numeric_limits<Index>::max()>
class IndexedResources {
public:
    IndexedResources() {
        m_free_indices += boost::icl::interval<Index>::closed(0, MaxIndex);
    }

    template <class... Types>
    std::optional<Index> Create(Types&&... args) {
        std::unique_lock lock{m_mutex};
        if (m_free_indices.empty()) {
            return {};
        }
        auto index = first(*m_free_indices.begin());
        m_free_indices -= index;
        m_container.emplace(index, T(std::forward<Types>(args)...));
        return index;
    }

    void Destroy(Index index) {
        std::unique_lock lock{m_mutex};
        if (m_container.erase(index) > 0) {
            m_free_indices += index;
        }
    }

    std::optional<std::reference_wrapper<T>> Get(Index index) {
        std::shared_lock lock{m_mutex};
        auto it = m_container.find(index);
        if (it == m_container.end()) {
            return {};
        }
        return it->second;
    }

private:
    std::shared_mutex m_mutex;
    std::unordered_map<Index, T> m_container;
    boost::icl::interval_set<Index> m_free_indices;
};

} // namespace Common
