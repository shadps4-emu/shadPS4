// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// #include <boost/icl/interval_set.hpp>

#include <limits>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

#include <memory>
#include <numeric>

namespace Common {

template <class IndexType, class ResourceType,
          IndexType MaxIndex = std::numeric_limits<IndexType>::max(), IndexType MinIndex = 0>
class SlotArray {
public:
    SlotArray() {
        std::iota(m_free_indices.begin(), m_free_indices.end(), MinIndex);
    }

    template <class... Types>
    std::optional<IndexType> Create(Types&&... args) {
        if (!HasFreeSlots()) {
            return std::nullopt;
        }
        const auto index = m_free_indices[m_curr_cursor];
        m_resources[index - MinIndex] = ResourceType(std::forward<Types>(args)...);
        m_curr_cursor += 1;
        return index;
    }

    bool Destroy(IndexType index) {
        if (!m_resources[index - MinIndex].has_value()) {
            return false;
        }
        m_curr_cursor -= 1;
        m_free_indices[m_curr_cursor] = index;
        m_resources[index - MinIndex] = std::nullopt;
        return true;
    }

    ResourceType* Get(IndexType index) {
        auto& resource = m_resources[index - MinIndex];
        if (!resource.has_value()) {
            return nullptr;
        }
        return &resource.value();
    }

    bool HasFreeSlots() {
        return m_curr_cursor < m_free_indices.size();
    }

private:
    size_t m_curr_cursor = 0;
    std::array<IndexType, MaxIndex - MinIndex> m_free_indices;
    std::array<std::optional<ResourceType>, MaxIndex - MinIndex> m_resources;
};

} // namespace Common
