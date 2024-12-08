// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <numeric>
#include <utility>
#include <vector>
#include "common/assert.h"
#include "common/types.h"

namespace Common {

struct SlotId {
    static constexpr u32 INVALID_INDEX = std::numeric_limits<u32>::max();

    constexpr auto operator<=>(const SlotId&) const noexcept = default;

    constexpr explicit operator bool() const noexcept {
        return index != INVALID_INDEX;
    }

    u32 index = INVALID_INDEX;
};

template <class T>
class SlotVector {
    constexpr static std::size_t InitialCapacity = 2048;

public:
    SlotVector() {
        Reserve(InitialCapacity);
    }

    ~SlotVector() noexcept {
        std::size_t index = 0;
        for (u64 bits : stored_bitset) {
            for (std::size_t bit = 0; bits; ++bit, bits >>= 1) {
                if ((bits & 1) != 0) {
                    values[index + bit].object.~T();
                }
            }
            index += 64;
        }
        delete[] values;
    }

    [[nodiscard]] T& operator[](SlotId id) noexcept {
        ValidateIndex(id);
        return values[id.index].object;
    }

    [[nodiscard]] const T& operator[](SlotId id) const noexcept {
        ValidateIndex(id);
        return values[id.index].object;
    }

    bool is_allocated(SlotId id) const {
        return ReadStorageBit(id.index);
    }

    template <typename... Args>
    [[nodiscard]] SlotId insert(Args&&... args) noexcept {
        const u32 index = FreeValueIndex();
        new (&values[index].object) T(std::forward<Args>(args)...);
        SetStorageBit(index);

        return SlotId{index};
    }

    void erase(SlotId id) noexcept {
        values[id.index].object.~T();
        free_list.push_back(id.index);
        ResetStorageBit(id.index);
    }

    std::size_t size() const noexcept {
        return values_capacity - free_list.size();
    }

private:
    struct NonTrivialDummy {
        NonTrivialDummy() noexcept {}
    };

    union Entry {
        Entry() noexcept : dummy{} {}
        ~Entry() noexcept {}

        NonTrivialDummy dummy;
        T object;
    };

    void SetStorageBit(u32 index) noexcept {
        stored_bitset[index / 64] |= u64(1) << (index % 64);
    }

    void ResetStorageBit(u32 index) noexcept {
        stored_bitset[index / 64] &= ~(u64(1) << (index % 64));
    }

    bool ReadStorageBit(u32 index) const noexcept {
        return ((stored_bitset[index / 64] >> (index % 64)) & 1) != 0;
    }

    void ValidateIndex([[maybe_unused]] SlotId id) const noexcept {
        DEBUG_ASSERT(id);
        DEBUG_ASSERT(id.index / 64 < stored_bitset.size());
        DEBUG_ASSERT(((stored_bitset[id.index / 64] >> (id.index % 64)) & 1) != 0);
    }

    [[nodiscard]] u32 FreeValueIndex() noexcept {
        if (free_list.empty()) {
            Reserve(values_capacity ? (values_capacity << 1) : 1);
        }

        const u32 free_index = free_list.back();
        free_list.pop_back();
        return free_index;
    }

    void Reserve(std::size_t new_capacity) noexcept {
        Entry* const new_values = new Entry[new_capacity];
        std::size_t index = 0;
        for (u64 bits : stored_bitset) {
            for (std::size_t bit = 0; bits; ++bit, bits >>= 1) {
                const std::size_t i = index + bit;
                if ((bits & 1) == 0) {
                    continue;
                }
                T& old_value = values[i].object;
                new (&new_values[i].object) T(std::move(old_value));
                old_value.~T();
            }
            index += 64;
        }

        stored_bitset.resize((new_capacity + 63) / 64);

        const std::size_t old_free_size = free_list.size();
        free_list.resize(old_free_size + (new_capacity - values_capacity));
        const std::size_t new_free_size = free_list.size();
        std::iota(free_list.rbegin(), free_list.rbegin() + new_free_size - old_free_size,
                  static_cast<u32>(values_capacity));

        delete[] values;
        values = new_values;
        values_capacity = new_capacity;
    }

    Entry* values = nullptr;
    std::size_t values_capacity = 0;

    std::vector<u64> stored_bitset;
    std::vector<u32> free_list;
};

} // namespace Common

template <>
struct std::hash<Common::SlotId> {
    std::size_t operator()(const Common::SlotId& id) const noexcept {
        return std::hash<u32>{}(id.index);
    }
};
