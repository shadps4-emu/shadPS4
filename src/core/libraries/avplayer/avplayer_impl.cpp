// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "avplayer_common.h"
#include "avplayer_file_streamer.h"
#include "avplayer_impl.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/libkernel.h"

using namespace Libraries::Kernel;

namespace Libraries::AvPlayer {

void* PS4_SYSV_ABI AvPlayer::Allocate(void* handle, u32 alignment, u32 size) {
    const auto* const self = reinterpret_cast<AvPlayer*>(handle);
    const auto allocate = self->m_init_data_original.memory_replacement.allocate;
    const auto ptr = self->m_init_data_original.memory_replacement.object_ptr;
    return allocate(ptr, alignment, size);
}

void PS4_SYSV_ABI AvPlayer::Deallocate(void* handle, void* memory) {
    const auto* const self = reinterpret_cast<AvPlayer*>(handle);
    const auto deallocate = self->m_init_data_original.memory_replacement.deallocate;
    const auto ptr = self->m_init_data_original.memory_replacement.object_ptr;
    return deallocate(ptr, memory);
}

void* PS4_SYSV_ABI AvPlayer::AllocateTexture(void* handle, u32 alignment, u32 size) {
    const auto* const self = reinterpret_cast<AvPlayer*>(handle);
    const auto allocate = self->m_init_data_original.memory_replacement.allocate_texture;
    const auto ptr = self->m_init_data_original.memory_replacement.object_ptr;
    return allocate(ptr, alignment, size);
}

void PS4_SYSV_ABI AvPlayer::DeallocateTexture(void* handle, void* memory) {
    const auto* const self = reinterpret_cast<AvPlayer*>(handle);
    const auto deallocate = self->m_init_data_original.memory_replacement.deallocate_texture;
    const auto ptr = self->m_init_data_original.memory_replacement.object_ptr;
    return deallocate(ptr, memory);
}

int PS4_SYSV_ABI AvPlayer::OpenFile(void* handle, const char* filename) {
    auto const self = reinterpret_cast<AvPlayer*>(handle);
    std::lock_guard guard(self->m_file_io_mutex);

    const auto open = self->m_init_data_original.file_replacement.open;
    const auto ptr = self->m_init_data_original.file_replacement.object_ptr;
    return open(ptr, filename);
}

int PS4_SYSV_ABI AvPlayer::CloseFile(void* handle) {
    auto const self = reinterpret_cast<AvPlayer*>(handle);
    std::lock_guard guard(self->m_file_io_mutex);

    const auto close = self->m_init_data_original.file_replacement.close;
    const auto ptr = self->m_init_data_original.file_replacement.object_ptr;
    return close(ptr);
}

int PS4_SYSV_ABI AvPlayer::ReadOffsetFile(void* handle, u8* buffer, u64 position, u32 length) {
    auto const self = reinterpret_cast<AvPlayer*>(handle);
    std::lock_guard guard(self->m_file_io_mutex);

    const auto read_offset = self->m_init_data_original.file_replacement.readOffset;
    const auto ptr = self->m_init_data_original.file_replacement.object_ptr;
    return read_offset(ptr, buffer, position, length);
}

u64 PS4_SYSV_ABI AvPlayer::SizeFile(void* handle) {
    auto const self = reinterpret_cast<AvPlayer*>(handle);
    std::lock_guard guard(self->m_file_io_mutex);

    const auto size = self->m_init_data_original.file_replacement.size;
    const auto ptr = self->m_init_data_original.file_replacement.object_ptr;
    return size(ptr);
}

AvPlayer::AvPlayer() : m_file_io_mutex(PTHREAD_MUTEX_ERRORCHECK, "SceAvPlayerFileIOLock") {}

void AvPlayer::Init(const SceAvPlayerInitData& data, const ThreadPriorities& priorities) {
    m_init_data = data;
    m_init_data_original = data;

    m_init_data.memory_replacement.object_ptr = this;
    m_init_data.memory_replacement.allocate = &AvPlayer::Allocate;
    m_init_data.memory_replacement.deallocate = &AvPlayer::Deallocate;
    m_init_data.memory_replacement.allocate_texture = &AvPlayer::AllocateTexture;
    m_init_data.memory_replacement.deallocate_texture = &AvPlayer::DeallocateTexture;
    if (data.file_replacement.open == nullptr || data.file_replacement.close == nullptr ||
        data.file_replacement.readOffset == nullptr || data.file_replacement.size == nullptr) {
        m_init_data.file_replacement = {};
    } else {
        m_init_data.file_replacement.object_ptr = this;
        m_init_data.file_replacement.open = &AvPlayer::OpenFile;
        m_init_data.file_replacement.close = &AvPlayer::CloseFile;
        m_init_data.file_replacement.readOffset = &AvPlayer::ReadOffsetFile;
        m_init_data.file_replacement.size = &AvPlayer::SizeFile;
    }

    m_state = std::make_unique<AvPlayerState>(m_init_data, priorities);
}

s32 AvPlayer::PostInit(const SceAvPlayerPostInitData& data) {
    m_post_init_data = data;
    return ORBIS_OK;
}

s32 AvPlayer::AddSource(std::string_view path) {
    if (path.empty()) {
        return ORBIS_AVPLAYER_ERROR_INVALID_PARAMS;
    }

    if (AVPLAYER_IS_ERROR(m_state->AddSource(path, GetSourceType(path)))) {
        return ORBIS_AVPLAYER_ERROR_OPERATION_FAILED;
    }

    return ORBIS_OK;
}

s32 AvPlayer::GetStreamCount() {
    return m_state->GetStreamCount();
}

s32 AvPlayer::GetStreamInfo(u32 stream_index, SceAvPlayerStreamInfo& info) {
    if (AVPLAYER_IS_ERROR(m_state->GetStreamInfo(stream_index, info))) {
        return ORBIS_AVPLAYER_ERROR_OPERATION_FAILED;
    }
    return ORBIS_OK;
}

s32 AvPlayer::EnableStream(u32 stream_id) {
    if (m_state == nullptr) {
        return ORBIS_AVPLAYER_ERROR_OPERATION_FAILED;
    }
    return m_state->EnableStream(stream_id);
}

s32 AvPlayer::Start() {
    return m_state->Start();
}

bool AvPlayer::GetVideoData(SceAvPlayerFrameInfo& video_info) {
    if (m_state == nullptr) {
        return false;
    }
    return m_state->GetVideoData(video_info);
}

bool AvPlayer::GetVideoData(SceAvPlayerFrameInfoEx& video_info) {
    if (m_state == nullptr) {
        return false;
    }
    return m_state->GetVideoData(video_info);
}

bool AvPlayer::GetAudioData(SceAvPlayerFrameInfo& audio_info) {
    if (m_state == nullptr) {
        return false;
    }
    return m_state->GetAudioData(audio_info);
}

bool AvPlayer::IsActive() {
    if (m_state == nullptr) {
        return false;
    }
    return m_state->IsActive();
}

u64 AvPlayer::CurrentTime() {
    if (m_state == nullptr) {
        return 0;
    }
    return m_state->CurrentTime();
}

s32 AvPlayer::Stop() {
    if (m_state == nullptr || !m_state->Stop()) {
        return ORBIS_AVPLAYER_ERROR_OPERATION_FAILED;
    }
    return ORBIS_OK;
}

} // namespace Libraries::AvPlayer
