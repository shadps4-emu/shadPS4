// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "avplayer.h"
#include "avplayer_data_streamer.h"
#include "avplayer_source.h"

#include "core/libraries/kernel/thread_management.h"

#include <memory>

namespace Libraries::AvPlayer {

class Stream;
class AvDecoder;

class AvPlayerState : public AvPlayerStateCallback {
public:
    AvPlayerState(const SceAvPlayerInitData& init_data, const ThreadPriorities& priorities);
    ~AvPlayerState();

    s32 AddSource(std::string_view filename, SceAvPlayerSourceType source_type);
    s32 GetStreamCount();
    s32 GetStreamInfo(u32 stream_index, SceAvPlayerStreamInfo& info);
    s32 EnableStream(u32 stream_id);
    s32 Start();
    bool Stop();
    bool GetAudioData(SceAvPlayerFrameInfo& audio_info);
    bool GetVideoData(SceAvPlayerFrameInfo& video_info);
    bool GetVideoData(SceAvPlayerFrameInfoEx& video_info);
    bool IsActive();
    u64 CurrentTime();

private:
    using ScePthreadMutex = Kernel::ScePthreadMutex;
    using ScePthread = Kernel::ScePthread;

    // Event Replacement
    static void PS4_SYSV_ABI AutoPlayEventCallback(void* handle, s32 event_id, s32 source_id,
                                                   void* event_data);

    void OnWarning(u32 id) override;
    void OnError() override;
    void OnEOF() override;

    void OnPlaybackStateChanged(AvState state);
    std::optional<bool> OnBufferingCheckEvent(u32 num_frames);

    void EmitEvent(SceAvPlayerEvents event_id, void* event_data = nullptr);
    bool SetState(AvState state);

    static void* PS4_SYSV_ABI AvControllerThread(void* p_user_data);

    void AddSourceEvent();
    int StartControllerThread();
    int ProcessEvent();
    int UpdateBufferingState();
    bool IsStateTransitionValid(AvState state);

    std::unique_ptr<AvPlayerSource> m_up_source;

    SceAvPlayerMemAllocator m_memory_replacement{};
    SceAvPlayerFileReplacement m_file_replacement{};
    SceAvPlayerEventReplacement m_event_replacement{};
    SceAvPlayerEventReplacement m_user_event_replacement{};
    ThreadPriorities m_thread_priorities{};
    bool m_auto_start{};
    u8 m_default_language[4]{};

    std::atomic_int32_t m_quit;
    std::atomic<AvState> m_current_state;
    std::atomic<AvState> m_previous_state;
    u32 m_thread_priority;
    u32 m_thread_affinity;
    std::atomic_uint32_t m_some_event_result{};

    PthreadMutex m_state_machine_mutex{};
    PthreadMutex m_event_handler_mutex{};
    ScePthread m_controller_thread{};
    AvPlayerQueue<AvPlayerEvent> m_event_queue{};
};

} // namespace Libraries::AvPlayer
