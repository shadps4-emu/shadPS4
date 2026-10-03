// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/types.h"

namespace Common {

struct SlotId {
    static constexpr u32 INVALID_INDEX = std::numeric_limits<u32>::max();

    SlotId() noexcept = default;
    constexpr SlotId(u32 index) noexcept : index(index) {}

    constexpr auto operator<=>(const SlotId&) const noexcept = default;

    constexpr explicit operator bool() const noexcept {
        return index != INVALID_INDEX;
    }

    u32 index = INVALID_INDEX;
};

void* ReserveMemoryPages(std::size_t size) noexcept;
void CommitMemoryPages(void* address, std::size_t size) noexcept;
void ReleaseMemoryPages(void* base, [[maybe_unused]] std::size_t size) noexcept;

template <class T>
class SlotVector {
    static constexpr std::size_t CHUNK_SIZE = 2_MB;
    static_assert(sizeof(T) <= CHUNK_SIZE);

public:
    explicit SlotVector() = default;
    explicit SlotVector(u32 max_entries) {
        Create(max_entries);
    }

    ~SlotVector() noexcept {
        Destroy();
    }

    SlotVector(const SlotVector&) = delete;
    SlotVector& operator=(const SlotVector&) = delete;
    SlotVector(SlotVector&&) = delete;
    SlotVector& operator=(SlotVector&&) = delete;

    [[nodiscard]] T& operator[](SlotId id) noexcept {
        ValidateIndex(id);
        return values[id.index];
    }

    [[nodiscard]] const T& operator[](SlotId id) const noexcept {
        ValidateIndex(id);
        return values[id.index];
    }

    void ResizeDestructive(u32 max_entries) {
        Destroy();
        Create(max_entries);
    }

    bool IsAllocated(SlotId id) const noexcept {
        return id.index < capacity && ReadStorageBit(id.index);
    }

    template <typename... Args>
    SlotId Insert(Args&&... args) noexcept {
        const u32 index = FreeValueIndex();
        new (&values[index]) T(std::forward<Args>(args)...);
        SetStorageBit(index);
        return SlotId{index};
    }

    void Erase(SlotId id) noexcept {
        ValidateIndex(id);
        values[id.index].~T();
        free_list.push_back(id.index);
        ResetStorageBit(id.index);
    }

    SlotId GetSlotId(const T& value) {
        const u32 index = std::addressof(value) - values;
        return SlotId{index};
    }

    std::size_t Size() const noexcept {
        return capacity - free_list.size();
    }

private:
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
        DEBUG_ASSERT(id.index < capacity);
        DEBUG_ASSERT(id.index / 64 < stored_bitset.size());
        DEBUG_ASSERT(((stored_bitset[id.index / 64] >> (id.index % 64)) & 1) != 0);
    }

    [[nodiscard]] u32 FreeValueIndex() noexcept {
        if (free_list.empty()) {
            AddChunk();
        }
        const u32 free_index = free_list.back();
        free_list.pop_back();
        return free_index;
    }

    void AddChunk() noexcept {
        ASSERT_MSG(!small_vector && num_chunks < max_chunks, "Run out of space");
        auto* const chunk_address = reinterpret_cast<u8*>(values) + num_chunks * CHUNK_SIZE;
        CommitMemoryPages(chunk_address, CHUNK_SIZE);
        ++num_chunks;
        const u32 new_capacity = num_chunks * CHUNK_SIZE / sizeof(T);
        const u32 new_entries = new_capacity - capacity;
        stored_bitset.resize((new_capacity + 63) / 64);
        for (u32 i = new_entries; i-- > 0;) {
            free_list.push_back(capacity + i);
        }
        capacity = new_capacity;
    }

    void Create(u32 max_entries) {
        const u64 max_bytes = max_entries * sizeof(T);
        const u64 reserved_bytes = Common::AlignUpPow2(max_bytes, CHUNK_SIZE);
        max_chunks = reserved_bytes / CHUNK_SIZE;
        small_vector = max_bytes <= CHUNK_SIZE;
        if (small_vector) {
            capacity = max_entries;
            values = static_cast<T*>(std::calloc(capacity, sizeof(T)));
            stored_bitset.resize((capacity + 63) / 64);
            free_list.resize(capacity);
            std::iota(free_list.rbegin(), free_list.rend(), 0u);
        } else {
            values = static_cast<T*>(ReserveMemoryPages(reserved_bytes));
        }
    }

    void Destroy() {
        std::size_t index = 0;
        for (u64 bits : stored_bitset) {
            for (std::size_t bit = 0; bits; ++bit, bits >>= 1) {
                if ((bits & 1) != 0) {
                    values[index + bit].~T();
                }
            }
            index += 64;
        }
        if (values) {
            if (small_vector) {
                std::free(values);
            } else {
                ReleaseMemoryPages(values, max_chunks * CHUNK_SIZE);
            }
            values = nullptr;
        }

        stored_bitset.clear();
        free_list.clear();
        max_chunks = 0;
        num_chunks = 0;
        capacity = 0;
        small_vector = false;
    }

    T* values{};
    std::vector<u64> stored_bitset;
    std::vector<u32> free_list;
    u32 max_chunks{};
    u32 num_chunks{};
    u32 capacity{};
    bool small_vector{};
};

} // namespace Common

template <>
struct std::hash<Common::SlotId> {
    std::size_t operator()(const Common::SlotId& id) const noexcept {
        return std::hash<u32>{}(id.index);
    }
};
