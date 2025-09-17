// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/libraries/avplayer/avplayer_error.h"
#include "core/libraries/avplayer/avplayer_source.h"
#include "core/libraries/avplayer/avplayer_state.h"
#include "core/tls.h"

#include <magic_enum/magic_enum.hpp>

namespace Libraries::AvPlayer {

void PS4_SYSV_ABI AvPlayerState::AutoPlayEventCallback(void* opaque, AvPlayerEvents event_id,
                                                       s32 source_id, void* event_data) {
    auto const self = reinterpret_cast<AvPlayerState*>(opaque);

    if (event_id == AvPlayerEvents::StateReady) {
        s32 video_stream_index = -1;
        s32 audio_stream_index = -1;
        s32 timedtext_stream_index = -1;
        const s32 stream_count = self->GetStreamCount();
        if (AVPLAYER_IS_ERROR(stream_count)) {
            self->Stop();
            return;
        }
        if (stream_count == 0) {
            self->Stop();
            return;
        }
        for (u32 stream_index = 0; stream_index < stream_count; ++stream_index) {
            AvPlayerStreamInfo info{};
            if (!self->GetStreamInfo(stream_index, info)) {
                self->Stop();
                return;
            }

            const std::string_view default_language{self->m_default_language};
            switch (info.type) {
            case AvPlayerStreamType::Video:
                if (video_stream_index == -1) {
                    video_stream_index = stream_index;
                }
                if (!default_language.empty() &&
                    default_language == info.details.video.language_code) {
                    video_stream_index = stream_index;
                }
                break;
            case AvPlayerStreamType::Audio:
                if (audio_stream_index == -1) {
                    audio_stream_index = stream_index;
                }
                if (!default_language.empty() &&
                    default_language == info.details.video.language_code) {
                    audio_stream_index = stream_index;
                }
                break;
            case AvPlayerStreamType::TimedText:
                if (default_language.empty()) {
                    timedtext_stream_index = stream_index;
                    break;
                }
                if (default_language == info.details.video.language_code) {
                    timedtext_stream_index = stream_index;
                }
                break;
            default:
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

    DefaultEventCallback(opaque, event_id, 0, event_data);
}

void AvPlayerState::DefaultEventCallback(void* opaque, AvPlayerEvents event_id, s32 source_id,
                                         void* event_data) {
    auto const self = reinterpret_cast<AvPlayerState*>(opaque);
    const auto callback = self->m_event_replacement.event_callback;
    const auto ptr = self->m_event_replacement.object_ptr;
    if (callback != nullptr) {
        Core::ExecuteGuest(callback, ptr, event_id, 0, event_data);
    }
}

// Called inside GAME thread
AvPlayerState::AvPlayerState(const AvPlayerInitData& init_data)
    : m_init_data(init_data), m_event_replacement(init_data.event_replacement) {
    if (m_event_replacement.event_callback == nullptr || init_data.auto_start) {
        m_auto_start = true;
        m_init_data.event_replacement.event_callback = &AvPlayerState::AutoPlayEventCallback;
    } else {
        m_init_data.event_replacement.event_callback = &AvPlayerState::DefaultEventCallback;
    }
    m_init_data.event_replacement.object_ptr = this;
    if (init_data.default_language != nullptr) {
        std::memcpy(m_default_language, init_data.default_language, sizeof(m_default_language));
    }
    SetState(AvState::Initial);
    StartControllerThread();
}

AvPlayerState::~AvPlayerState() {
    {
        std::unique_lock lock(m_source_mutex);
        m_up_source.reset();
    }
    m_controller_thread.Stop();
    m_event_queue.Clear();
}

void AvPlayerState::PostInit(const AvPlayerPostInitData& post_init_data) {
    m_post_init_data = post_init_data;
}

// Called inside GAME thread
bool AvPlayerState::AddSource(std::string_view path, AvPlayerSourceType source_type) {
    if (path.empty()) {
        LOG_ERROR(Lib_AvPlayer, "File path is empty.");
        return false;
    }

    {
        std::unique_lock lock(m_source_mutex);
        if (m_up_source != nullptr) {
            LOG_ERROR(Lib_AvPlayer, "Only one source is supported.");
            return false;
        }

        m_up_source = std::make_unique<AvPlayerSource>(
            *this, m_post_init_data.video_decoder_init.decoder_type.video_type ==
                       AvPlayerVideoDecoderType::Software2);
        if (!m_up_source->Init(m_init_data, path)) {
            SetState(AvState::Error);
            m_up_source.reset();
            return false;
        }
    }
    AddSourceEvent();
    return true;
}

// Called inside GAME thread
s32 AvPlayerState::GetStreamCount() {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr) {
        LOG_ERROR(Lib_AvPlayer, "Could not get stream count. No source.");
        return -1;
    }
    return m_up_source->GetStreamCount();
}

// Called inside GAME thread
bool AvPlayerState::GetStreamInfo(u32 stream_index, AvPlayerStreamInfo& info) {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr) {
        LOG_ERROR(Lib_AvPlayer, "Could not get stream {} info. No source.", stream_index);
        return false;
    }
    return m_up_source->GetStreamInfo(stream_index, info);
}

// Called inside GAME thread
bool AvPlayerState::Start() {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr || !m_up_source->Start()) {
        LOG_ERROR(Lib_AvPlayer, "Could not start playback.");
        return false;
    }
    SetState(AvState::Play);
    OnPlaybackStateChanged(AvState::Play);
    return true;
}

// Called inside GAME thread
bool AvPlayerState::Pause() {
    std::shared_lock lock(m_source_mutex);
    if (m_current_state == AvState::EndOfFile) {
        return true;
    }
    if (m_up_source == nullptr || m_current_state == AvState::Pause ||
        m_current_state == AvState::Ready || m_current_state == AvState::Initial ||
        m_current_state == AvState::Unknown || m_current_state == AvState::AddingSource) {
        LOG_ERROR(Lib_AvPlayer, "Could not pause playback.");
        return false;
    }
    m_up_source->Pause();
    SetState(AvState::Pause);
    OnPlaybackStateChanged(AvState::Pause);
    return true;
}

// Called inside GAME thread
bool AvPlayerState::Resume() {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr || m_current_state != AvState::Pause) {
        LOG_ERROR(Lib_AvPlayer, "Could not resume playback.");
        return false;
    }
    m_up_source->Resume();
    const auto state = m_previous_state.load();
    SetState(state);
    OnPlaybackStateChanged(state);
    return true;
}

void AvPlayerState::AvControllerThread(std::stop_token stop) {
    using std::chrono::milliseconds;
    Common::SetCurrentThreadName("shadPS4:AvController");

    while (!stop.stop_requested()) {
        if (m_event_queue.Size() != 0) {
            ProcessEvent();
            continue;
        }
        std::this_thread::sleep_for(milliseconds(5));
        UpdateBufferingState();
    }
}

// Called inside GAME thread
void AvPlayerState::AddSourceEvent() {
    SetState(AvState::AddingSource);
    m_event_queue.Push(AvPlayerEvent{
        .event = AvEventType::AddSource,
    });
}

void AvPlayerState::WarningEvent(s32 id) {
    m_event_queue.Push(AvPlayerEvent{
        .event = AvEventType::WarningId,
        .payload =
            {
                .error = id,
            },
    });
}

// Called inside GAME thread
void AvPlayerState::StartControllerThread() {
    m_controller_thread.Run([this](std::stop_token stop) { this->AvControllerThread(stop); });
}

// Called inside GAME thread
bool AvPlayerState::EnableStream(u32 stream_index) {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr) {
        return false;
    }
    return m_up_source->EnableStream(stream_index);
}

// Called inside GAME thread
bool AvPlayerState::Stop() {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr || m_current_state == AvState::Stop) {
        return false;
    }
    if (!m_up_source->Stop()) {
        return false;
    }
    if (!SetState(AvState::Stop)) {
        return false;
    }
    OnPlaybackStateChanged(AvState::Stop);
    return true;
}

bool AvPlayerState::GetVideoData(AvPlayerFrameInfo& video_info) {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr) {
        return false;
    }
    return m_up_source->GetVideoData(video_info);
}

bool AvPlayerState::GetVideoData(AvPlayerFrameInfoEx& video_info) {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr) {
        return false;
    }
    return m_up_source->GetVideoData(video_info);
}

bool AvPlayerState::GetAudioData(AvPlayerFrameInfo& audio_info) {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr) {
        return false;
    }
    return m_up_source->GetAudioData(audio_info);
}

bool AvPlayerState::IsActive() {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr) {
        return false;
    }
    return m_current_state != AvState::Stop && m_current_state != AvState::Error &&
           m_current_state != AvState::EndOfFile && m_up_source->IsActive();
}

u64 AvPlayerState::CurrentTime() {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr) {
        LOG_ERROR(Lib_AvPlayer, "Could not get current time. No source.");
        return 0;
    }
    return m_up_source->CurrentTime();
}

bool AvPlayerState::SetLooping(bool is_looping) {
    std::shared_lock lock(m_source_mutex);
    if (m_up_source == nullptr) {
        LOG_ERROR(Lib_AvPlayer, "Could not set loop flag. No source.");
        return false;
    }
    m_up_source->SetLooping(is_looping);
    return true;
}

// May be called from different threads
void AvPlayerState::OnWarning(u32 id) {
    // Forward to CONTROLLER thread
    WarningEvent(id);
}

void AvPlayerState::OnError() {
    SetState(AvState::Error);
    OnPlaybackStateChanged(AvState::Error);
}

void AvPlayerState::OnEOF() {
    SetState(AvState::EndOfFile);
}

// Called inside CONTROLLER thread
void AvPlayerState::OnPlaybackStateChanged(AvState state) {
    switch (state) {
    case AvState::Ready: {
        EmitEvent(AvPlayerEvents::StateReady);
        break;
    }
    case AvState::Play: {
        EmitEvent(AvPlayerEvents::StatePlay);
        break;
    }
    case AvState::Stop: {
        EmitEvent(AvPlayerEvents::StateStop);
        break;
    }
    case AvState::Pause: {
        EmitEvent(AvPlayerEvents::StatePause);
        break;
    }
    case AvState::Buffering: {
        EmitEvent(AvPlayerEvents::StateBuffering);
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
        LOG_ERROR(Lib_AvPlayer, "Invalid state transition: {} -> {}",
                  magic_enum::enum_name(m_current_state.load()), magic_enum::enum_name(state));
        return false;
    }
    m_previous_state.store(m_current_state);
    m_current_state.store(state);
    return true;
}

// Called inside CONTROLLER thread
std::optional<bool> AvPlayerState::OnBufferingCheckEvent(u32 num_frames) {
    std::shared_lock lock(m_source_mutex);
    if (!m_up_source) {
        return std::nullopt;
    }
    return m_up_source->HasFrames(num_frames);
}

// Called inside CONTROLLER thread
void AvPlayerState::EmitEvent(AvPlayerEvents event_id, void* event_data) {
    LOG_INFO(Lib_AvPlayer, "Sending event to the game: id = {}", magic_enum::enum_name(event_id));
    const auto callback = m_init_data.event_replacement.event_callback;
    if (callback) {
        const auto ptr = m_init_data.event_replacement.object_ptr;
        callback(ptr, event_id, 0, event_data);
    }
}

// Called inside CONTROLLER thread
void AvPlayerState::ProcessEvent() {
    if (m_current_state == AvState::Jump) {
        return;
    }

    std::lock_guard guard(m_event_handler_mutex);

    auto event = m_event_queue.Pop();
    if (!event.has_value()) {
        return;
    }
    switch (event->event) {
    case AvEventType::WarningId: {
        OnWarning(event->payload.error);
        break;
    }
    case AvEventType::RevertState: {
        SetState(m_previous_state.load());
        break;
    }
    case AvEventType::AddSource: {
        std::shared_lock lock(m_source_mutex);
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
}

// Called inside CONTROLLER thread
void AvPlayerState::UpdateBufferingState() {
    if (m_current_state == AvState::Buffering) {
        const auto has_frames = OnBufferingCheckEvent(10);
        if (!has_frames.has_value()) {
            return;
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
            return;
        }
        if (!has_frames.value()) {
            SetState(AvState::Buffering);
            OnPlaybackStateChanged(AvState::Buffering);
        }
    }
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
