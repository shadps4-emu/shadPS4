// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include "common/types.h"

namespace Libraries::Np::NpUtility {

// sceNpLookupSetTimeout() values
struct LookupTimeouts {
    s32 resolveRetry = 0;
    u32 resolveTimeout = 0;
    u32 connTimeout = 0;
    u32 sendTimeout = 0;
    u32 recvTimeout = 0;
};

struct LookupRequestCtx {
    std::mutex mutex;
    std::condition_variable cv;
    std::optional<s32> result;

    s32 titleCtxId = 0;
    bool isAsync = false;
    bool used = false;
    bool aborted = false;
    std::optional<LookupTimeouts> timeouts;

    void SetResult(s32 r) {
        {
            std::lock_guard lock(mutex);
            if (result.has_value()) {
                return;
            }
            result = r;
        }
        cv.notify_all();
    }

    bool HasResult() {
        std::lock_guard lock(mutex);
        return result.has_value();
    }

    s32 Wait() {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this] { return result.has_value(); });
        return *result;
    }
};

} // namespace Libraries::Np::NpUtility
