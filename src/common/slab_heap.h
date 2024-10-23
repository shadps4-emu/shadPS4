// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include "common/assert.h"
#include "common/spin_lock.h"

namespace Common {

class SlabHeapImpl {
public:
    struct Node {
        Node* next{};
    };

public:
    constexpr SlabHeapImpl() = default;

    void Initialize() {
        ASSERT(m_head == nullptr);
    }

    Node* GetHead() const {
        return m_head;
    }

    void* Allocate() {
        m_lock.lock();

        Node* ret = m_head;
        if (ret != nullptr) {
            m_head = ret->next;
        }

        m_lock.unlock();
        return ret;
    }

    void Free(void* obj) {
        m_lock.lock();

        Node* node = static_cast<Node*>(obj);
        node->next = m_head;
        m_head = node;

        m_lock.unlock();
    }

private:
    std::atomic<Node*> m_head{};
    Common::SpinLock m_lock;
};

class SlabHeapBase : protected SlabHeapImpl {
private:
    size_t m_obj_size{};
    uintptr_t m_peak{};
    uintptr_t m_start{};
    uintptr_t m_end{};

public:
    constexpr SlabHeapBase() = default;

    bool Contains(uintptr_t address) const {
        return m_start <= address && address < m_end;
    }

    void Initialize(size_t obj_size, void* memory, size_t memory_size) {
        // Ensure we don't initialize a slab using null memory.
        ASSERT(memory != nullptr);

        // Set our object size.
        m_obj_size = obj_size;

        // Initialize the base allocator.
        SlabHeapImpl::Initialize();

        // Set our tracking variables.
        const size_t num_obj = (memory_size / obj_size);
        m_start = reinterpret_cast<uintptr_t>(memory);
        m_end = m_start + num_obj * obj_size;
        m_peak = m_start;

        // Free the objects.
        u8* cur = reinterpret_cast<u8*>(m_end);

        for (size_t i = 0; i < num_obj; i++) {
            cur -= obj_size;
            SlabHeapImpl::Free(cur);
        }
    }

    size_t GetSlabHeapSize() const {
        return (m_end - m_start) / this->GetObjectSize();
    }

    size_t GetObjectSize() const {
        return m_obj_size;
    }

    void* Allocate() {
        void* obj = SlabHeapImpl::Allocate();
        return obj;
    }

    void Free(void* obj) {
        // Don't allow freeing an object that wasn't allocated from this heap.
        const bool contained = this->Contains(reinterpret_cast<uintptr_t>(obj));
        ASSERT(contained);
        SlabHeapImpl::Free(obj);
    }

    size_t GetObjectIndex(const void* obj) const {
        return (reinterpret_cast<uintptr_t>(obj) - m_start) / this->GetObjectSize();
    }

    size_t GetPeakIndex() const {
        return this->GetObjectIndex(reinterpret_cast<const void*>(m_peak));
    }

    uintptr_t GetSlabHeapAddress() const {
        return m_start;
    }

    size_t GetNumRemaining() const {
        // Only calculate the number of remaining objects under debug configuration.
        return 0;
    }
};

template <typename T>
class SlabHeap final : public SlabHeapBase {
private:
    using BaseHeap = SlabHeapBase;

public:
    constexpr SlabHeap() = default;

    void Initialize(void* memory, size_t memory_size) {
        BaseHeap::Initialize(sizeof(T), memory, memory_size);
    }

    T* Allocate() {
        T* obj = static_cast<T*>(BaseHeap::Allocate());

        if (obj != nullptr) [[likely]] {
            std::construct_at(obj);
        }
        return obj;
    }

    void Free(T* obj) {
        BaseHeap::Free(obj);
    }

    size_t GetObjectIndex(const T* obj) const {
        return BaseHeap::GetObjectIndex(obj);
    }
};

} // namespace Common
