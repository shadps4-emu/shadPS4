// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "avplayer_file_streamer.h"
#include "avplayer_source.h"
#include "avplayer_state.h"

#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/time_management.h"

#include <magic_enum.hpp>

namespace Libraries::AvPlayer {

using namespace Kernel;

void PS4_SYSV_ABI AvPlayerState::AutoPlayEventCallback(void* opaque, s32 event_id, s32 source_id,
                                                       void* event_data) {
    auto const self = reinterpret_cast<AvPlayerState*>(opaque);

    if (event_id == SCE_AVPLAYER_STATE_READY) {
        s32 video_stream_index = -1;
        s32 audio_stream_index = -1;
        s32 timedtext_stream_index = -1;
        const s32 stream_count = self->GetStreamCount();
        if (AVPLAYER_IS_ERROR(stream_count)) {
            return;
        }
        if (stream_count == 0) {
            self->Stop();
            return;
        }
        for (u32 stream_index = 0; stream_index < stream_count; ++stream_index) {
            SceAvPlayerStreamInfo info{};
            self->GetStreamInfo(stream_index, info);

            const std::string_view default_language(
                reinterpret_cast<char*>(self->m_default_language));
            switch (info.type) {
            case SCE_AVPLAYER_VIDEO:
                if (video_stream_index == -1) {
                    if (default_language.empty()) {
                        video_stream_index = stream_index;
                        break;
                    }
                    if (default_language ==
                        reinterpret_cast<char*>(info.details.video.language_code)) {
                        video_stream_index = stream_index;
                    }
                }
                break;
            case SCE_AVPLAYER_AUDIO:
                if (audio_stream_index == -1) {
                    if (default_language.empty()) {
                        audio_stream_index = stream_index;
                        break;
                    }
                    if (default_language ==
                        reinterpret_cast<char*>(info.details.video.language_code)) {
                        audio_stream_index = stream_index;
                    }
                }
                break;
            case SCE_AVPLAYER_TIMEDTEXT:
                if (timedtext_stream_index == -1) {
                    if (default_language.empty()) {
                        timedtext_stream_index = stream_index;
                        break;
                    }
                    if (default_language ==
                        reinterpret_cast<char*>(info.details.video.language_code)) {
                        timedtext_stream_index = stream_index;
                    }
                }
                break;
            }
        }

        if (video_stream_index != -1) {
            self->EnableStream(video_stream_index);
        }
        if (audio_stream_index != -1) {
            self->EnableStream(audio_stream_index);
        }
        if (timedtext_stream_index != -1) {
            self->EnableStream(timedtext_stream_index);
        }
        self->Start();
        return;
    }

    const auto callback = self->m_user_event_replacement.event_callback;
    const auto ptr = self->m_user_event_replacement.object_ptr;
    if (callback != nullptr) {
        callback(ptr, event_id, 0, event_data);
    }
}

// Called inside GAME thread
AvPlayerState::AvPlayerState(const SceAvPlayerInitData& init_data,
                             const ThreadPriorities& priorities)
    : m_event_handler_mutex(PTHREAD_MUTEX_ERRORCHECK, "SceAvPlayerEventHandler"),
      m_state_machine_mutex(PTHREAD_MUTEX_ERRORCHECK, "SceAvPlayerStateMachine") {
    if (init_data.event_replacement.event_callback == nullptr || init_data.auto_start) {
        m_auto_start = true;
        m_event_replacement.event_callback = &AvPlayerState::AutoPlayEventCallback;
        m_event_replacement.object_ptr = this;
    } else {
        m_event_replacement = init_data.event_replacement;
    }
    m_user_event_replacement = init_data.event_replacement;

    const auto& memory_replacement = init_data.memory_replacement;
    if (memory_replacement.allocate != nullptr && memory_replacement.deallocate != nullptr &&
        memory_replacement.allocate_texture != nullptr &&
        memory_replacement.deallocate_texture != nullptr) {
        m_memory_replacement = memory_replacement;
    }

    if (init_data.event_replacement.event_callback != nullptr) {
        m_event_replacement = init_data.event_replacement;
        m_auto_start = init_data.auto_start;
    } else {
        m_auto_start = true;
    }

    m_file_replacement = init_data.file_replacement;
    m_thread_priorities = priorities;

    if (init_data.default_language != nullptr) {
        std::memcpy(m_default_language, init_data.default_language, sizeof(m_default_language));
    }
    SetState(AvState::Initial);
    StartControllerThread();
}

AvPlayerState::~AvPlayerState() {
    m_event_queue.Clear();
    if (m_up_source && m_current_state == AvState::Play) {
        m_up_source->Stop();
        OnPlaybackStateChanged(AvState::Stop);
    }
}

// Called inside GAME thread
s32 AvPlayerState::AddSource(std::string_view path, SceAvPlayerSourceType source_type) {
    if (path.empty()) {
        return -1;
    }

    if (m_up_source) {
        return -1;
    }

    m_up_source = std::make_unique<AvPlayerSource>(*this);
    if (AVPLAYER_IS_ERROR(m_up_source->Init(path, m_memory_replacement, m_file_replacement,
                                            m_thread_priorities, source_type))) {
        return -1;
    }
    AddSourceEvent();

    return 0;
}

// Called inside GAME thread
s32 AvPlayerState::GetStreamCount() {
    return m_up_source->GetStreamCount();
}

// Called inside GAME thread
s32 AvPlayerState::GetStreamInfo(u32 stream_index, SceAvPlayerStreamInfo& info) {
    return m_up_source->GetStreamInfo(stream_index, info);
}

// Called inside GAME thread
s32 AvPlayerState::Start() {
    if (m_up_source == nullptr) {
        return -1;
    }
    return m_up_source->Start();
}

void* AvPlayerState::AvControllerThread(void* p_user_data) {
    AvPlayerState* self = reinterpret_cast<AvPlayerState*>(p_user_data);
    while (self->m_quit.load() == 0) {
        if (self->m_event_queue.Size() != 0) {
            self->ProcessEvent();
            continue;
        }
        sceKernelUsleep(5000);
        self->UpdateBufferingState();
    }
    scePthreadExit(0);
}

// Called inside GAME thread
void AvPlayerState::AddSourceEvent() {
    SetState(AvState::AddingSource);
    m_event_queue.Push(AvPlayerEvent{
        .event = AvEventType::AddSource,
    });
}

// Called inside GAME thread
int AvPlayerState::StartControllerThread() {
    m_quit.store(0);

    ThreadParameters params{
        .p_user_data = this,
        .thread_name = "AvPlayer_Controller",
        .stack_size = 0x4000,
        .priority = m_thread_priority,
        .affinity = m_thread_affinity,
    };
    m_controller_thread = CreateThread(&AvControllerThread, params);
    if (m_controller_thread == nullptr) {
        return -1;
    }
    return 0;
}

// Called inside GAME thread
s32 AvPlayerState::EnableStream(u32 stream_id) {
    if (m_up_source == nullptr) {
        return -1;
    }
    return m_up_source->EnableStream(stream_id);
}

// Called inside GAME thread
bool AvPlayerState::Stop() {
    if (m_up_source == nullptr) {
        return false;
    }
    SetState(AvState::Stop);
    OnPlaybackStateChanged(AvState::Stop);
    return m_up_source->Stop();
}

bool AvPlayerState::GetVideoData(SceAvPlayerFrameInfo& video_info) {
    if (m_up_source == nullptr) {
        return false;
    }
    return m_up_source->GetVideoData(video_info);
}

bool AvPlayerState::GetVideoData(SceAvPlayerFrameInfoEx& video_info) {
    if (m_up_source == nullptr) {
        return false;
    }
    return m_up_source->GetVideoData(video_info);
}

bool AvPlayerState::GetAudioData(SceAvPlayerFrameInfo& audio_info) {
    if (m_up_source == nullptr) {
        return false;
    }
    return m_up_source->GetAudioData(audio_info);
}

bool AvPlayerState::IsActive() {
    if (m_up_source == nullptr) {
        return false;
    }
    return m_current_state != AvState::Stop && m_current_state != AvState::Error &&
           m_up_source->IsActive();
}

u64 AvPlayerState::CurrentTime() {
    if (m_up_source == nullptr) {
        return 0;
    }
    return m_up_source->CurrentTime();
}

void AvPlayerState::OnWarning(u32 id) {
    EmitEvent(SCE_AVPLAYER_WARNING_ID, &id);
}

void AvPlayerState::OnError() {
    SetState(AvState::Error);
    OnPlaybackStateChanged(AvState::Error);
}

void AvPlayerState::OnEOF() {
    Stop();
}

// Called inside CONTROLLER thread
void AvPlayerState::OnPlaybackStateChanged(AvState state) {
    switch (state) {
    case AvState::Ready: {
        EmitEvent(SCE_AVPLAYER_STATE_READY);
        break;
    }
    case AvState::Play: {
        EmitEvent(SCE_AVPLAYER_STATE_PLAY);
        break;
    }
    case AvState::Stop: {
        EmitEvent(SCE_AVPLAYER_STATE_STOP);
        break;
    }
    case AvState::Pause: {
        EmitEvent(SCE_AVPLAYER_STATE_PAUSE);
        break;
    }
    case AvState::Buffering: {
        EmitEvent(SCE_AVPLAYER_STATE_BUFFERING);
        break;
    }
    default:
        break;
    }
}

// Called inside CONTROLLER and GAME threads
bool AvPlayerState::SetState(AvState state) {
    std::lock_guard guard(m_state_machine_mutex);

    if (!IsStateTransitionValid(state)) {
        return false;
    }
    m_previous_state.store(m_current_state);
    m_current_state.store(state);
    return true;
}

// Called inside CONTROLLER thread
std::optional<bool> AvPlayerState::OnBufferingCheckEvent(u32 num_frames) {
    if (!m_up_source) {
        return std::nullopt;
    }
    return m_up_source->HasFrames(num_frames);
}

// Called inside CONTROLLER thread
void AvPlayerState::EmitEvent(SceAvPlayerEvents event_id, void* event_data) {
    LOG_INFO(Lib_AvPlayer, "Sending event to the game: id = {}", magic_enum::enum_name(event_id));
    const auto callback = m_event_replacement.event_callback;
    if (callback) {
        const auto ptr = m_event_replacement.object_ptr;
        callback(ptr, event_id, 0, event_data);
    }
}

// Called inside CONTROLLER thread
int AvPlayerState::ProcessEvent() {
    if (m_current_state.load() == AvState::Jump) {
        return -2;
    }

    std::lock_guard guard(m_event_handler_mutex);

    auto event = m_event_queue.Pop();
    if (!event.has_value()) {
        return -1;
    }
    switch (event->event) {
    case AvEventType::RevertState: {
        SetState(m_previous_state.load());
        break;
    }
    case AvEventType::AddSource: {
        if (m_up_source->FindStreamInfo()) {
            SetState(AvState::Ready);
            OnPlaybackStateChanged(AvState::Ready);
        } else {
            OnWarning(ORBIS_AVPLAYER_ERROR_NOT_SUPPORTED);
            SetState(AvState::Error);
        }
        break;
    }
    case AvEventType::Error: {
        OnWarning(event->payload.error);
        SetState(AvState::Error);
        break;
    }
    default:
        break;
    }

    return 0;
}

// Called inside CONTROLLER thread
int AvPlayerState::UpdateBufferingState() {
    if (m_current_state == AvState::Buffering) {
        const auto has_frames = OnBufferingCheckEvent(10);
        if (!has_frames.has_value()) {
            return -1;
        }
        if (has_frames.value()) {
            const auto state =
                m_previous_state >= AvState::C0x0B ? m_previous_state.load() : AvState::Play;
            SetState(state);
            OnPlaybackStateChanged(state);
        }
    } else if (m_current_state == AvState::Play) {
        const auto has_frames = OnBufferingCheckEvent(0);
        if (!has_frames.has_value()) {
            return -1;
        }
        if (!has_frames.value()) {
            SetState(AvState::Buffering);
            OnPlaybackStateChanged(AvState::Buffering);
        }
    }
    return 0;
}

bool AvPlayerState::IsStateTransitionValid(AvState state) {
    switch (state) {
    case AvState::Play: {
        switch (m_current_state.load()) {
        case AvState::Stop:
        case AvState::EndOfFile:
        // case AvState::C0x08:
        case AvState::Error:
            return false;
        default:
            return true;
        }
    }
    case AvState::Pause: {
        switch (m_current_state.load()) {
        case AvState::Stop:
        case AvState::EndOfFile:
        // case AvState::C0x08:
        case AvState::Starting:
        case AvState::Error:
            return false;
        default:
            return true;
        }
    }
    case AvState::Jump: {
        switch (m_current_state.load()) {
        case AvState::Stop:
        case AvState::EndOfFile:
        // case AvState::C0x08:
        case AvState::TrickMode:
        case AvState::Starting:
        case AvState::Error:
            return false;
        default:
            return true;
        }
    }
    case AvState::TrickMode: {
        switch (m_current_state.load()) {
        case AvState::Stop:
        case AvState::EndOfFile:
        // case AvState::C0x08:
        case AvState::Jump:
        case AvState::Starting:
        case AvState::Error:
            return false;
        default:
            return true;
        }
    }
    case AvState::Buffering: {
        switch (m_current_state.load()) {
        case AvState::Stop:
        case AvState::EndOfFile:
        case AvState::Pause:
        // case AvState::C0x08:
        case AvState::Starting:
        case AvState::Error:
            return false;
        default:
            return true;
        }
    }
    default:
        return true;
    }
}

} // namespace Libraries::AvPlayer
