// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include "common/types.h"
#include "core/libraries/np/np_error.h"

namespace Libraries::Np::NpScore {

// Per-request state shared between np_score.cpp (creator/waiter) and the
// handler that fulfils the request (np_handler.cpp)
struct ScoreRequestCtx {
    std::mutex mutex;
    std::condition_variable cv;
    std::optional<s32> result; // nullopt = still pending

    s32 titleCtxId = 0;
    s32 userId = -1;
    s32 pcId = 0;
    u32* tmpRankOut = nullptr;
    u64 gameDataRecvOffset = 0;
    std::vector<u8> gameDataRecvCache;
    std::string gameDataRecvKey;
    std::vector<u8> gameDataSendBuf;
    u64 gameDataSendTotal = 0;

    // Start a new operation on this request ID. Request IDs are reused.
    bool BeginOp() {
        std::lock_guard lock(mutex);
        if (result.has_value() && *result == ORBIS_NP_COMMUNITY_ERROR_ABORTED) {
            return false;
        }
        result.reset();
        return true;
    }

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

    // Block until result is set. Used by the synced functions
    s32 Wait() {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this] { return result.has_value(); });
        return *result;
    }
};

} // namespace Libraries::Np::NpScore
