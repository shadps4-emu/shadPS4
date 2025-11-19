// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#include <cstdarg> // va_list
#include <cstddef> // size_t

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::AvPlayer {

class AvPlayer;

using AvPlayerHandle = AvPlayer*;

enum class AvPlayerUriType : u32 {
    Source = 0,
};

struct AvPlayerUri {
    const char* name;
    u32 length;
};

enum class AvPlayerSourceType {
    Unknown = 0,
    FileMp4 = 1,
    Hls = 8,
};

enum class AvPlayerStreamType : u32 {
    Video,
    Audio,
    TimedText,
    Unknown,
};

struct AvPlayerSourceDetails {
    AvPlayerUri uri;
    u8 reserved1[64];
    AvPlayerSourceType source_type;
    u8 reserved2[44];
};

struct AvPlayerAudio {
    u16 channel_count;
    u8 reserved1[2];
    u32 sample_rate;
    u32 size;
    u8 language_code[4];
};

struct AvPlayerVideo {
    u32 width;
    u32 height;
    f32 aspect_ratio;
    char language_code[4];
};

struct AvPlayerTextPosition {
    u16 top;
    u16 left;
    u16 bottom;
    u16 right;
};

struct AvPlayerTimedText {
    u8 language_code[4];
    u16 text_size;
    u16 font_size;
    AvPlayerTextPosition position;
};

union AvPlayerStreamDetails {
    u8 reserved[16];
    AvPlayerAudio audio;
    AvPlayerVideo video;
    AvPlayerTimedText subs;
};

struct AvPlayerFrameInfo {
    u8* p_data;
    u8 reserved[4];
    u64 timestamp;
    AvPlayerStreamDetails details;
};

struct AvPlayerStreamInfo {
    AvPlayerStreamType type;
    u8 reserved[4];
    AvPlayerStreamDetails details;
    u64 duration;
    u64 start_time;
};

struct AvPlayerAudioEx {
    u16 channel_count;
    u8 reserved[2];
    u32 sample_rate;
    u32 size;
    u8 language_code[4];
    u8 reserved1[64];
};

struct AvPlayerVideoEx {
    u32 width;
    u32 height;
    f32 aspect_ratio;
    u8 language_code[4];
    u32 framerate;
    u32 crop_left_offset;
    u32 crop_right_offset;
    u32 crop_top_offset;
    u32 crop_bottom_offset;
    u32 pitch;
    u8 luma_bit_depth;
    u8 chroma_bit_depth;
    bool video_full_range_flag;
    u8 reserved1[37];
};

struct AvPlayerTimedTextEx {
    u8 language_code[4];
    u8 reserved[12];
    u8 reserved1[64];
};

union AvPlayerStreamDetailsEx {
    AvPlayerAudioEx audio;
    AvPlayerVideoEx video;
    AvPlayerTimedTextEx subs;
    u8 reserved1[80];
};

struct AvPlayerFrameInfoEx {
    void* p_data;
    u8 reserved[4];
    u64 timestamp;
    AvPlayerStreamDetailsEx details;
};

using AvPlayerAllocate = void* PS4_SYSV_ABI (*)(void* p, u32 align, u32 size);
using AvPlayerDeallocate = void PS4_SYSV_ABI (*)(void* p, void* mem);
using AvPlayerAllocateTexture = void* PS4_SYSV_ABI (*)(void* p, u32 align, u32 size);
using AvPlayerDeallocateTexture = void PS4_SYSV_ABI (*)(void* p, void* mem);

struct AvPlayerMemAllocator {
    void* object_ptr;
    AvPlayerAllocate allocate;
    AvPlayerDeallocate deallocate;
    AvPlayerAllocateTexture allocate_texture;
    AvPlayerDeallocateTexture deallocate_texture;
};

using AvPlayerOpenFile = s32 PS4_SYSV_ABI (*)(void* p, const char* name);
using AvPlayerCloseFile = s32 PS4_SYSV_ABI (*)(void* p);
using AvPlayerReadOffsetFile = s32 PS4_SYSV_ABI (*)(void* p, u8* buf, u64 pos, u32 len);
using AvPlayerSizeFile = u64 PS4_SYSV_ABI (*)(void* p);

struct AvPlayerFileReplacement {
    void* object_ptr;
    AvPlayerOpenFile open;
    AvPlayerCloseFile close;
    AvPlayerReadOffsetFile read_offset;
    AvPlayerSizeFile size;
};

enum class AvPlayerEvents {
    StateStop = 0x01,
    StateReady = 0x02,
    StatePlay = 0x03,
    StatePause = 0x04,
    StateBuffering = 0x05,
    TimedTextDelivery = 0x10,
    WarningId = 0x20,
    Encryption = 0x30,
    DrmError = 0x40,
};

using AvPlayerEventCallback = void PS4_SYSV_ABI (*)(void* p, AvPlayerEvents event, s32 src_id,
                                                    void* data);

struct AvPlayerEventReplacement {
    void* object_ptr;
    AvPlayerEventCallback event_callback;
};

enum class AvPlayerDebuglevels {
    None,
    Info,
    Warnings,
    All,
};

struct AvPlayerInitData {
    AvPlayerMemAllocator memory_replacement;
    AvPlayerFileReplacement file_replacement;
    AvPlayerEventReplacement event_replacement;
    AvPlayerDebuglevels debug_level;
    u32 base_priority;
    s32 num_output_video_framebuffers;
    bool auto_start;
    u8 reserved[3];
    const char* default_language;
};

struct AvPlayerInitDataEx {
    size_t this_size;
    AvPlayerMemAllocator memory_replacement;
    AvPlayerFileReplacement file_replacement;
    AvPlayerEventReplacement event_replacement;
    const char* default_language;
    AvPlayerDebuglevels debug_level;
    u32 audio_decoder_priority;
    u32 audio_decoder_affinity;
    u32 video_decoder_priority;
    u32 video_decoder_affinity;
    u32 demuxer_priority;
    u32 demuxer_affinity;
    u32 controller_priority;
    u32 controller_affinity;
    u32 http_streaming_priority;
    u32 http_streaming_affinity;
    u32 file_streaming_priority;
    u32 file_streaming_affinity;
    s32 num_output_video_framebuffers;
    bool auto_start;
    u8 reserved[3];
};

enum class AvPlayerVideoDecoderType {
    Default = 0,
    Reserved1,
    Software,
    Software2,
};

enum class AvPlayerAudioDecoderType {
    Default = 0,
    Reserved1,
    Reserved2,
};

struct AvPlayerDecoderInit {
    union {
        AvPlayerVideoDecoderType video_type;
        AvPlayerAudioDecoderType audio_type;
        u8 reserved[4];
    } decoder_type;
    union {
        struct {
            s32 cpu_affinity_mask;
            s32 cpu_thread_priority;
            u8 decode_pipeline_depth;
            u8 compute_pipe_id;
            u8 compute_queue_id;
            u8 enable_interlaced;
            u8 reserved[16];
        } avc_sw2;
        struct {
            u8 audio_channel_order;
            u8 reserved[27];
        } aac;
        u8 reserved[28];
    } decoder_params;
};

struct AvPlayerHTTPCtx {
    u32 http_context_id;
    u32 ssl_context_id;
};

struct AvPlayerPostInitData {
    u32 demux_video_buffer_size;
    AvPlayerDecoderInit video_decoder_init;
    AvPlayerDecoderInit audio_decoder_init;
    AvPlayerHTTPCtx http_context;
    u8 reserved[56];
};

enum class AvPlayerAvSyncMode {
    Default = 0,
    None,
};

using AvPlayerLogCallback = int PS4_SYSV_ABI (*)(void* p, const char* format, va_list args);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::AvPlayer
