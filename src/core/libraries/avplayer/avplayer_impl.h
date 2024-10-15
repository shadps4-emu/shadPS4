// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "avplayer.h"
#include "avplayer_data_streamer.h"
#include "avplayer_state.h"

#include "core/libraries/kernel/thread_management.h"

#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <memory>
#include <vector>

namespace Libraries::AvPlayer {

class AvPlayer {
public:
    AvPlayer(const SceAvPlayerInitData& data);

    s32 PostInit(const SceAvPlayerPostInitData& data);
    s32 AddSource(std::string_view filename);
    s32 GetStreamCount();
    s32 GetStreamInfo(u32 stream_index, SceAvPlayerStreamInfo& info);
    s32 EnableStream(u32 stream_index);
    s32 Start();
    bool GetAudioData(SceAvPlayerFrameInfo& audio_info);
    bool GetVideoData(SceAvPlayerFrameInfo& video_info);
    bool GetVideoData(SceAvPlayerFrameInfoEx& video_info);
    bool IsActive();
    u64 CurrentTime();
    s32 Stop();
    bool SetLooping(bool is_looping);

private:
    // Memory Replacement
    static void* PS4_SYSV_ABI Allocate(void* handle, u32 alignment, u32 size);
    static void PS4_SYSV_ABI Deallocate(void* handle, void* memory);
    static void* PS4_SYSV_ABI AllocateTexture(void* handle, u32 alignment, u32 size);
    static void PS4_SYSV_ABI DeallocateTexture(void* handle, void* memory);

    // File Replacement
    static int PS4_SYSV_ABI OpenFile(void* handle, const char* filename);
    static int PS4_SYSV_ABI CloseFile(void* handle);
    static int PS4_SYSV_ABI ReadOffsetFile(void* handle, u8* buffer, u64 position, u32 length);
    static u64 PS4_SYSV_ABI SizeFile(void* handle);

    SceAvPlayerInitData StubInitData(const SceAvPlayerInitData& data);

    SceAvPlayerInitData m_init_data{};
    SceAvPlayerInitData m_init_data_original{};
    std::mutex m_file_io_mutex{};

    std::atomic_bool m_has_source{};
    std::unique_ptr<AvPlayerState> m_state{};
};

} // namespace Libraries::AvPlayer
