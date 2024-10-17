// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "avplayer.h"
#include "avplayer_data_streamer.h"
#include "avplayer_source.h"

#include "common/polyfill_thread.h"
#include "core/libraries/kernel/thread_management.h"

#include <memory>
#include <mutex>
#include <shared_mutex>

namespace Libraries::AvPlayer {

class Stream;
class AvDecoder;

class AvPlayerState : public AvPlayerStateCallback {
public:
    AvPlayerState(const SceAvPlayerInitData& init_data);
    ~AvPlayerState();

    bool AddSource(std::string_view filename, SceAvPlayerSourceType source_type);
    s32 GetStreamCount();
    bool GetStreamInfo(u32 stream_index, SceAvPlayerStreamInfo& info);
    bool EnableStream(u32 stream_index);
    bool Start();
    bool Stop();
    bool GetAudioData(SceAvPlayerFrameInfo& audio_info);
    bool GetVideoData(SceAvPlayerFrameInfo& video_info);
    bool GetVideoData(SceAvPlayerFrameInfoEx& video_info);
    bool IsActive();
    u64 CurrentTime();
    bool SetLooping(bool is_looping);

private:
    // Event Replacement
    static void PS4_SYSV_ABI AutoPlayEventCallback(void* handle, SceAvPlayerEvents event_id,
                                                   s32 source_id, void* event_data);

    static void PS4_SYSV_ABI DefaultEventCallback(void* handle, SceAvPlayerEvents event_id,
                                                  s32 source_id, void* event_data);

    void OnWarning(u32 id) override;
    void OnError() override;
    void OnEOF() override;

    void OnPlaybackStateChanged(AvState state);
    std::optional<bool> OnBufferingCheckEvent(u32 num_frames);

    void EmitEvent(SceAvPlayerEvents event_id, void* event_data = nullptr);
    bool SetState(AvState state);

    void AvControllerThread(std::stop_token stop);

    void AddSourceEvent();
    void WarningEvent(s32 id);

    void StartControllerThread();
    void ProcessEvent();
    void UpdateBufferingState();
    bool IsStateTransitionValid(AvState state);

    std::unique_ptr<AvPlayerSource> m_up_source;

    SceAvPlayerInitData m_init_data{};
    SceAvPlayerEventReplacement m_event_replacement{};
    bool m_auto_start{};
    u8 m_default_language[4]{};

    std::atomic<AvState> m_current_state;
    std::atomic<AvState> m_previous_state;
    u32 m_thread_priority;
    u32 m_thread_affinity;
    std::atomic_uint32_t m_some_event_result{};

    std::shared_mutex m_source_mutex{};
    std::mutex m_state_machine_mutex{};
    std::mutex m_event_handler_mutex{};
    std::jthread m_controller_thread{};
    AvPlayerQueue<AvPlayerEvent> m_event_queue{};
};

} // namespace Libraries::AvPlayer
