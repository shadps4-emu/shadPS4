// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/types.h"

template <class T>
class RingBufferQueue {
public:
    RingBufferQueue(u64 size) : m_storage(size) {}

    void Push(T item) {
        const u64 index = (m_begin + m_size) % m_storage.size();
        m_storage[index] = std::move(item);
        if (m_size < m_storage.size()) {
            m_size += 1;
        } else {
            m_begin = (m_begin + 1) % m_storage.size();
        }
    }

    std::optional<T> Pop() {
        if (m_size == 0) {
            return {};
        }
        const u64 index = m_begin;
        m_begin = (m_begin + 1) % m_storage.size();
        m_size -= 1;
        return std::move(m_storage[index]);
    }

    std::optional<T> Peek() {
        if (m_size == 0) {
            return {};
        }
        return m_storage[m_begin];
    }

private:
    u64 m_begin = 0;
    u64 m_size = 0;
    std::vector<T> m_storage;
};