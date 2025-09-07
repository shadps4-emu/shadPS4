// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include "common/assert.h"
#include "common/recursive_lock.h"

namespace Common::Detail {

struct RecursiveLockState {
    RecursiveLockType type;
    int count;
};

thread_local std::unordered_map<void*, RecursiveLockState> g_recursive_locks;

bool IncrementRecursiveLock(void* mutex, RecursiveLockType type) {
    auto& state = g_recursive_locks[mutex];
    if (state.count == 0) {
        ASSERT(state.type == RecursiveLockType::None);
        state.type = type;
    }
    ASSERT(state.type == type);
    return state.count++ == 0;
}

bool DecrementRecursiveLock(void* mutex, RecursiveLockType type) {
    auto& state = g_recursive_locks[mutex];
    ASSERT(state.type == type && state.count > 0);
    if (--state.count == 0) {
        g_recursive_locks.erase(mutex);
        return true;
    }
    return false;
}

} // namespace Common::Detail
