// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>

#include "core/libraries/avplayer/avplayer.h"
#include "core/libraries/avplayer/avplayer_source.h"
#include "core/libraries/kernel/threads.h"

namespace Libraries::AvPlayer {

class Stream;
class AvDecoder;

class AvPlayerState : public AvPlayerStateCallback {
public:
    AvPlayerState(const AvPlayerInitData& init_data);
    ~AvPlayerState();

    void PostInit(const AvPlayerPostInitData& post_init_data);
    bool AddSource(std::string_view filename, AvPlayerSourceType source_type);
    s32 GetStreamCount();
    bool GetStreamInfo(u32 stream_index, AvPlayerStreamInfo& info);
    bool EnableStream(u32 stream_index);
    bool Start();
    bool Stop();
    bool Pause();
    bool Resume();
    void SetAvSyncMode(AvPlayerAvSyncMode sync_mode);
    bool GetAudioData(AvPlayerFrameInfo& audio_info);
    bool GetVideoData(AvPlayerFrameInfo& video_info);
    bool GetVideoData(AvPlayerFrameInfoEx& video_info);
    bool IsActive();
    u64 CurrentTime();
    bool SetLooping(bool is_looping);

private:
    // Event Replacement
    static void PS4_SYSV_ABI AutoPlayEventCallback(void* handle, AvPlayerEvents event_id,
                                                   s32 source_id, void* event_data);

    static void PS4_SYSV_ABI DefaultEventCallback(void* handle, AvPlayerEvents event_id,
                                                  s32 source_id, void* event_data);

    AvPlayerAvSyncMode GetSyncMode() override;
    void OnWarning(u32 id) override;
    void OnError() override;
    void OnEOF() override;

    void OnPlaybackStateChanged(AvState state);
    std::optional<bool> OnBufferingCheckEvent(u32 num_frames);

    void EmitEvent(AvPlayerEvents event_id, void* event_data = nullptr);
    bool SetState(AvState state);

    void AvControllerThread(std::stop_token stop);

    void AddSourceEvent();
    void WarningEvent(s32 id);

    void StartControllerThread();
    void ProcessEvent();
    void UpdateBufferingState();
    bool IsStateTransitionValid(AvState state);

    std::unique_ptr<AvPlayerSource> m_up_source;

    AvPlayerInitData m_init_data{};
    AvPlayerPostInitData m_post_init_data{};
    AvPlayerEventReplacement m_event_replacement{};
    bool m_auto_start{};
    char m_default_language[4]{};
    AvPlayerAvSyncMode m_sync_mode = AvPlayerAvSyncMode::Default;

    std::atomic<AvState> m_current_state;
    std::atomic<AvState> m_previous_state;
    u32 m_thread_priority;
    u32 m_thread_affinity;
    std::atomic_uint32_t m_some_event_result{};

    std::shared_mutex m_source_mutex{};
    std::mutex m_state_machine_mutex{};
    std::mutex m_event_handler_mutex{};
    Kernel::Thread m_controller_thread{};
    AvPlayerQueue<AvPlayerEvent> m_event_queue{};
};

} // namespace Libraries::AvPlayer
