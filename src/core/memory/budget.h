// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include "common/types.h"
#include "core/libraries/kernel/orbis_error.h"

namespace Core {

enum class BudgetPtype : u8 {
    Invalid = 0,
    BigApp = 1,
    System = 2,
};

enum class VmContainer : u8 {
    System = 0,
    Game = 1,
};

struct BudgetState {
    std::mutex mutex;
    std::array<u64, 3> mlock_used{};
    std::array<u64, 3> mlock_limit{};

    s32 ReserveWire(BudgetPtype ptype, u64 size) {
        if (ptype == BudgetPtype::Invalid) {
            return 0;
        }
        std::scoped_lock lock{mutex};
        const u8 idx = static_cast<u8>(ptype);
        if (mlock_limit[idx] < mlock_used[idx] || mlock_limit[idx] - mlock_used[idx] < size) {
            return ORBIS_KERNEL_ERROR_ENOMEM;
        }
        mlock_used[idx] += size;
        return 0;
    }

    void ReleaseWire(BudgetPtype ptype, u64 size) {
        if (ptype == BudgetPtype::Invalid) {
            return;
        }
        std::scoped_lock lock{mutex};
        u8 idx = static_cast<u8>(ptype);
        mlock_used[idx] = (size <= mlock_used[idx]) ? mlock_used[idx] - size : 0;
    }
};

} // namespace Core