// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <unordered_map>
#include <queue>
#include <zlib.h>

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/libs.h"
#include "core/libraries/zlib/zlib_error.h"
#include "core/libraries/zlib/zlib_sce.h"

namespace Libraries::Zlib {

struct InflateTask {
    u64 request_id;
    const void* src;
    u32 src_length;
    void* dst;
    u32 dst_length;
};

struct InflateResult {
    u32 length;
    s32 status;
};

static Kernel::Thread task_thread;

static std::mutex mutex;
static std::queue<InflateTask> task_queue;
static std::condition_variable_any task_queue_cv;
static std::queue<u64> done_queue;
static std::condition_variable_any done_queue_cv;
static std::unordered_map<u64, InflateResult> results;
static u64 next_request_id;

void ZlibTaskThread(const std::stop_token& stop) {
    Common::SetCurrentThreadName("shadPS4:ZlibTaskThread");

    while (!stop.stop_requested()) {
        InflateTask task;
        {
            // Lock and pop from the task queue, unless stop has been requested.
            std::unique_lock lock(mutex);
            if (!task_queue_cv.wait(lock, stop, [&] { return !task_queue.empty(); })) {
                break;
            }
            task = task_queue.front();
            task_queue.pop();
        }

        uLongf decompressed_length = task.dst_length;
        const auto ret = uncompress(static_cast<Bytef*>(task.dst), &decompressed_length,
                                    static_cast<const Bytef*>(task.src), task.src_length);

        {
            // Lock, insert the new result, and push the finished request ID to the done queue.
            std::unique_lock lock(mutex);
            results[task.request_id] = InflateResult{
                .length = static_cast<u32>(decompressed_length),
                .status = ret == Z_BUF_ERROR ? ORBIS_ZLIB_ERROR_NOSPACE
                          : ret == Z_OK      ? ORBIS_OK
                                             : ORBIS_ZLIB_ERROR_FATAL,
            };
            done_queue.push(task.request_id);
        }
        done_queue_cv.notify_one();
    }
}

s32 PS4_SYSV_ABI sceZlibInitialize(const void* buffer, u32 length) {
    LOG_INFO(Lib_Zlib, "called");
    if (task_thread.Joinable()) {
        return ORBIS_ZLIB_ERROR_ALREADY_INITIALIZED;
    }

    // Initialize with empty task data
    task_queue = std::queue<InflateTask>();
    done_queue = std::queue<u64>();
    results.clear();
    next_request_id = 1;

    task_thread.Run([](const std::stop_token& stop) { ZlibTaskThread(stop); });
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceZlibInflate(const void* src, u32 src_len, void* dst, u32 dst_len,
                                u64* request_id) {
    LOG_DEBUG(Lib_Zlib, "(STUBBED) called");
    if (!task_thread.Joinable()) {
        return ORBIS_ZLIB_ERROR_NOT_INITIALIZED;
    }
    if (!src || !src_len || !dst || !dst_len || !request_id || dst_len > 64_KB ||
        dst_len % 2_KB != 0) {
        return ORBIS_ZLIB_ERROR_INVALID;
    }

    {
        std::unique_lock lock(mutex);
        *request_id = next_request_id++;
        task_queue.emplace(InflateTask{
            .request_id = *request_id,
            .src = src,
            .src_length = src_len,
            .dst = dst,
            .dst_length = dst_len,
        });
        task_queue_cv.notify_one();
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceZlibWaitForDone(u64* request_id, const u32* timeout) {
    LOG_DEBUG(Lib_Zlib, "(STUBBED) called");
    if (!task_thread.Joinable()) {
        return ORBIS_ZLIB_ERROR_NOT_INITIALIZED;
    }
    if (!request_id) {
        return ORBIS_ZLIB_ERROR_INVALID;
    }

    {
        // Pop from the done queue, unless the timeout is reached.
        std::unique_lock lock(mutex);
        const auto pred = [] { return !done_queue.empty(); };
        if (timeout) {
            if (!done_queue_cv.wait_for(lock, std::chrono::milliseconds(*timeout), pred)) {
                return ORBIS_ZLIB_ERROR_TIMEDOUT;
            }
        } else {
            done_queue_cv.wait(lock, pred);
        }
        *request_id = done_queue.front();
        done_queue.pop();
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceZlibGetResult(const u64 request_id, u32* dst_length, s32* status) {
    LOG_DEBUG(Lib_Zlib, "(STUBBED) called");
    if (!task_thread.Joinable()) {
        return ORBIS_ZLIB_ERROR_NOT_INITIALIZED;
    }
    if (!dst_length || !status) {
        return ORBIS_ZLIB_ERROR_INVALID;
    }

    {
        std::unique_lock lock(mutex);
        if (!results.contains(request_id)) {
            return ORBIS_ZLIB_ERROR_NOT_FOUND;
        }
        const auto result = results[request_id];
        *dst_length = result.length;
        *status = result.status;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceZlibFinalize() {
    LOG_INFO(Lib_Zlib, "called");
    if (!task_thread.Joinable()) {
        return ORBIS_ZLIB_ERROR_NOT_INITIALIZED;
    }
    task_thread.Stop();
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("m1YErdIXCp4", "libSceZlib", 1, "libSceZlib", sceZlibInitialize);
    LIB_FUNCTION("6na+Sa-B83w", "libSceZlib", 1, "libSceZlib", sceZlibFinalize);
    LIB_FUNCTION("TLar1HULv1Q", "libSceZlib", 1, "libSceZlib", sceZlibInflate);
    LIB_FUNCTION("uB8VlDD4e0s", "libSceZlib", 1, "libSceZlib", sceZlibWaitForDone);
    LIB_FUNCTION("2eDcGHC0YaM", "libSceZlib", 1, "libSceZlib", sceZlibGetResult);
};

} // namespace Libraries::Zlib
