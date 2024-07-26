// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "avplayer.h"

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/thread_management.h"

#include <optional>
#include <utility>
#include <queue>

#define AVPLAYER_IS_ERROR(x) ((x) < 0)

namespace Libraries::AvPlayer {

enum class AvState {
    Initial,
    AddingSource,
    Ready,
    Play,
    Stop,
    EndOfFile,
    Pause,
    C0x08,
    Jump,
    TrickMode,
    C0x0B,
    Buffering,
    Starting,
    C0x0E,
    C0x0F,
    C0x10,
    Error,
};

enum class AvEventType {
    ChangeFlowState = 21,
    WarningId = 22,
    RevertState = 30,
    AddSource = 40,
    Error = 255,
};

struct ThreadPriorities {
    u32 audio_decoder_priority;
    u32 audio_decoder_affinity;
    u32 video_decoder_priority;
    u32 video_decoder_affinity;
    u32 demuxer_priority;
    u32 demuxer_affinity;
    u32 controller_priority;
    u32 controller_affinity;
    // u32 http_streaming_priority;
    // u32 http_streaming_affinity;
    // u32 file_streaming_priority;
    // u32 file_streaming_affinity;
    // u32 maxPriority;
    // u32 maxAffinity;
};

union AvPlayerEventData {
    u32 num_frames; // 20
    AvState state;  // AvEventType::ChangeFlowState
    s32 error;      // AvEventType::WarningId
    u32 attempt;    // AvEventType::AddSource
};

struct AvPlayerEvent {
    AvEventType event;
    AvPlayerEventData payload;
};

Kernel::ScePthreadMutex CreateMutex(int type, const char* name);

class PthreadMutex {
public:
    using ScePthreadMutex = Kernel::ScePthreadMutex;

    PthreadMutex() = default;

    PthreadMutex(const PthreadMutex&) = delete;
    PthreadMutex& operator=(const PthreadMutex&) = delete;

    PthreadMutex(PthreadMutex&& r) : m_mutex(r.m_mutex) {
        r.m_mutex = nullptr;
    }
    PthreadMutex& operator=(PthreadMutex&& r) {
        std::swap(m_mutex, r.m_mutex);
        return *this;
    }

    PthreadMutex(int type, const char* name) : m_mutex(CreateMutex(type, name)) {}
    ~PthreadMutex() {
        if (m_mutex != nullptr) {
            Kernel::scePthreadMutexDestroy(&m_mutex);
        }
    }

    operator ScePthreadMutex() {
        return m_mutex;
    }

    int Lock() {
        return Kernel::scePthreadMutexLock(&m_mutex);
    }

    int Unlock() {
        return Kernel::scePthreadMutexUnlock(&m_mutex);
    }

    // implement BasicLockable to use std::lock_guard
    // NOLINTNEXTLINE(readability-identifier-naming)
    void lock() {
        ASSERT_MSG(Lock() >= 0, "Could not lock the mutex");
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    void unlock() {
        ASSERT_MSG(Unlock() >= 0, "Could not unlock the mutex");
    }

    operator bool() {
        return m_mutex != nullptr;
    }

private:
    ScePthreadMutex m_mutex{};
};

template <class T>
class AvPlayerQueue {
public:
    AvPlayerQueue() : m_mutex(PTHREAD_MUTEX_ERRORCHECK, "SceAvPlayer0StlHandler") {}

    size_t Size() {
        return m_queue.size();
    }

    void Push(T&& value) {
        std::lock_guard guard(m_mutex);
        m_queue.emplace(std::forward<T>(value));
    }

    std::optional<T> Pop() {
        if (Size() == 0) {
            return std::nullopt;
        }
        std::lock_guard guard(m_mutex);
        auto result = std::move(m_queue.front());
        m_queue.pop();
        return result;
    }

    void Clear() {
        std::lock_guard guard(m_mutex);
        m_queue = {};
    }

private:
    PthreadMutex m_mutex{};
    std::queue<T> m_queue{};
};

struct ThreadParameters {
    void* p_user_data;
    const char* thread_name;
    u32 stack_size;
    u32 priority;
    u32 affinity;
};

Kernel::ScePthread CreateThread(Kernel::PthreadEntryFunc func, const ThreadParameters& params);
SceAvPlayerSourceType GetSourceType(std::string_view path);

} // namespace Libraries::AvPlayer
