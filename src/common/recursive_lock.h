// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <optional>
#include <shared_mutex>

namespace Common {

namespace Detail {

enum class RecursiveLockType { None, Shared, Exclusive };

bool IncrementRecursiveLock(void* mutex, RecursiveLockType type);
bool DecrementRecursiveLock(void* mutex, RecursiveLockType type);

} // namespace Detail

template <typename MutexType>
class RecursiveScopedLock {
public:
    explicit RecursiveScopedLock(MutexType& mutex) : m_mutex(mutex), m_locked(false) {
        if (Detail::IncrementRecursiveLock(&m_mutex, Detail::RecursiveLockType::Exclusive)) {
            m_locked = true;
            m_lock.emplace(m_mutex);
        }
    }

    ~RecursiveScopedLock() {
        Detail::DecrementRecursiveLock(&m_mutex, Detail::RecursiveLockType::Exclusive);
        if (m_locked) {
            m_lock.reset();
        }
    }

private:
    MutexType& m_mutex;
    std::optional<std::unique_lock<MutexType>> m_lock;
    bool m_locked = false;
};

template <typename MutexType>
class RecursiveSharedLock {
public:
    explicit RecursiveSharedLock(MutexType& mutex) : m_mutex(mutex), m_locked(false) {
        if (Detail::IncrementRecursiveLock(&m_mutex, Detail::RecursiveLockType::Shared)) {
            m_locked = true;
            m_lock.emplace(m_mutex);
        }
    }

    ~RecursiveSharedLock() {
        Detail::DecrementRecursiveLock(&m_mutex, Detail::RecursiveLockType::Shared);
        if (m_locked) {
            m_lock.reset();
        }
    }

private:
    MutexType& m_mutex;
    std::optional<std::shared_lock<MutexType>> m_lock;
    bool m_locked = false;
};

} // namespace Common