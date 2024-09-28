// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#include <stdarg.h> // va_list
#include <stddef.h> // size_t

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::AvPlayer {

class AvPlayer;

using SceAvPlayerHandle = AvPlayer*;

enum SceAvPlayerUriType { SCE_AVPLAYER_URI_TYPE_SOURCE = 0 };

struct SceAvPlayerUri {
    const char* name;
    u32 length;
};

enum SceAvPlayerSourceType {
    SCE_AVPLAYER_SOURCE_TYPE_UNKNOWN = 0,
    SCE_AVPLAYER_SOURCE_TYPE_FILE_MP4 = 1,
    SCE_AVPLAYER_SOURCE_TYPE_HLS = 8
};

struct SceAvPlayerSourceDetails {
    SceAvPlayerUri uri;
    u8 reserved1[64];
    SceAvPlayerSourceType source_type;
    u8 reserved2[44];
};

struct SceAvPlayerAudio {
    u16 channel_count;
    u8 reserved1[2];
    u32 sample_rate;
    u32 size;
    u8 language_code[4];
};

struct SceAvPlayerVideo {
    u32 width;
    u32 height;
    f32 aspect_ratio;
    u8 language_code[4];
};

struct SceAvPlayerTextPosition {
    u16 top;
    u16 left;
    u16 bottom;
    u16 right;
};

struct SceAvPlayerTimedText {
    u8 language_code[4];
    u16 text_size;
    u16 font_size;
    SceAvPlayerTextPosition position;
};

union SceAvPlayerStreamDetails {
    u8 reserved[16];
    SceAvPlayerAudio audio;
    SceAvPlayerVideo video;
    SceAvPlayerTimedText subs;
};

struct SceAvPlayerFrameInfo {
    u8* pData;
    u8 reserved[4];
    u64 timestamp;
    SceAvPlayerStreamDetails details;
};

struct SceAvPlayerStreamInfo {
    u32 type;
    u8 reserved[4];
    SceAvPlayerStreamDetails details;
    u64 duration;
    u64 start_time;
};

struct SceAvPlayerAudioEx {
    u16 channel_count;
    u8 reserved[2];
    u32 sample_rate;
    u32 size;
    u8 language_code[4];
    u8 reserved1[64];
};

struct SceAvPlayerVideoEx {
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

struct SceAvPlayerTimedTextEx {
    u8 language_code[4];
    u8 reserved[12];
    u8 reserved1[64];
};

union SceAvPlayerStreamDetailsEx {
    SceAvPlayerAudioEx audio;
    SceAvPlayerVideoEx video;
    SceAvPlayerTimedTextEx subs;
    u8 reserved1[80];
};

struct SceAvPlayerFrameInfoEx {
    void* pData;
    u8 reserved[4];
    u64 timestamp;
    SceAvPlayerStreamDetailsEx details;
};

typedef void* PS4_SYSV_ABI (*SceAvPlayerAllocate)(void* p, u32 align, u32 size);
typedef void PS4_SYSV_ABI (*SceAvPlayerDeallocate)(void* p, void* mem);
typedef void* PS4_SYSV_ABI (*SceAvPlayerAllocateTexture)(void* p, u32 align, u32 size);
typedef void PS4_SYSV_ABI (*SceAvPlayerDeallocateTexture)(void* p, void* mem);

struct SceAvPlayerMemAllocator {
    void* object_ptr;
    SceAvPlayerAllocate allocate;
    SceAvPlayerDeallocate deallocate;
    SceAvPlayerAllocateTexture allocate_texture;
    SceAvPlayerDeallocateTexture deallocate_texture;
};

typedef s32 PS4_SYSV_ABI (*SceAvPlayerOpenFile)(void* p, const char* name);
typedef s32 PS4_SYSV_ABI (*SceAvPlayerCloseFile)(void* p);
typedef s32 PS4_SYSV_ABI (*SceAvPlayerReadOffsetFile)(void* p, u8* buf, u64 pos, u32 len);
typedef u64 PS4_SYSV_ABI (*SceAvPlayerSizeFile)(void* p);

struct SceAvPlayerFileReplacement {
    void* object_ptr;
    SceAvPlayerOpenFile open;
    SceAvPlayerCloseFile close;
    SceAvPlayerReadOffsetFile readOffset;
    SceAvPlayerSizeFile size;
};

enum SceAvPlayerEvents {
    SCE_AVPLAYER_STATE_STOP = 0x01,
    SCE_AVPLAYER_STATE_READY = 0x02,
    SCE_AVPLAYER_STATE_PLAY = 0x03,
    SCE_AVPLAYER_STATE_PAUSE = 0x04,
    SCE_AVPLAYER_STATE_BUFFERING = 0x05,
    SCE_AVPLAYER_TIMED_TEXT_DELIVERY = 0x10,
    SCE_AVPLAYER_WARNING_ID = 0x20,
    SCE_AVPLAYER_ENCRYPTION = 0x30,
    SCE_AVPLAYER_DRM_ERROR = 0x40
};

typedef void PS4_SYSV_ABI (*SceAvPlayerEventCallback)(void* p, SceAvPlayerEvents event, s32 src_id,
                                                      void* data);

struct SceAvPlayerEventReplacement {
    void* object_ptr;
    SceAvPlayerEventCallback event_callback;
};

enum SceAvPlayerDebuglevels {
    SCE_AVPLAYER_DBG_NONE,
    SCE_AVPLAYER_DBG_INFO,
    SCE_AVPLAYER_DBG_WARNINGS,
    SCE_AVPLAYER_DBG_ALL
};

struct SceAvPlayerInitData {
    SceAvPlayerMemAllocator memory_replacement;
    SceAvPlayerFileReplacement file_replacement;
    SceAvPlayerEventReplacement event_replacement;
    SceAvPlayerDebuglevels debug_level;
    u32 base_priority;
    s32 num_output_video_framebuffers;
    bool auto_start;
    u8 reserved[3];
    const char* default_language;
};

struct SceAvPlayerInitDataEx {
    size_t this_size;
    SceAvPlayerMemAllocator memory_replacement;
    SceAvPlayerFileReplacement file_replacement;
    SceAvPlayerEventReplacement event_replacement;
    const char* default_language;
    SceAvPlayerDebuglevels debug_level;
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

enum SceAvPlayerStreamType {
    SCE_AVPLAYER_VIDEO,
    SCE_AVPLAYER_AUDIO,
    SCE_AVPLAYER_TIMEDTEXT,
    SCE_AVPLAYER_UNKNOWN
};

enum SceAvPlayerVideoDecoderType {
    SCE_AVPLAYER_VIDEO_DECODER_TYPE_DEFAULT = 0,
    SCE_AVPLAYER_VIDEO_DECODER_TYPE_RESERVED1,
    SCE_AVPLAYER_VIDEO_DECODER_TYPE_SOFTWARE,
    SCE_AVPLAYER_VIDEO_DECODER_TYPE_SOFTWARE2
};

enum SceAvPlayerAudioDecoderType {
    SCE_AVPLAYER_AUDIO_DECODER_TYPE_DEFAULT = 0,
    SCE_AVPLAYER_AUDIO_DECODER_TYPE_RESERVED1,
    SCE_AVPLAYER_AUDIO_DECODER_TYPE_RESERVED2
};

struct SceAvPlayerDecoderInit {
    union {
        SceAvPlayerVideoDecoderType video_type;
        SceAvPlayerAudioDecoderType audio_type;
        u8 reserved[4];
    } decoderType;
    union {
        struct {
            s32 cpu_affinity_mask;
            s32 cpu_thread_priority;
            u8 decode_pipeline_depth;
            u8 compute_pipe_id;
            u8 compute_queue_id;
            u8 enable_interlaced;
            u8 reserved[16];
        } avcSw2;
        struct {
            u8 audio_channel_order;
            u8 reserved[27];
        } aac;
        u8 reserved[28];
    } decoderParams;
};

struct SceAvPlayerHTTPCtx {
    u32 http_context_id;
    u32 ssl_context_id;
};

struct SceAvPlayerPostInitData {
    u32 demux_video_buffer_size;
    SceAvPlayerDecoderInit video_decoder_init;
    SceAvPlayerDecoderInit audio_decoder_init;
    SceAvPlayerHTTPCtx http_context;
    u8 reserved[56];
};

enum SceAvPlayerAvSyncMode {
    SCE_AVPLAYER_AV_SYNC_MODE_DEFAULT = 0,
    SCE_AVPLAYER_AV_SYNC_MODE_NONE
};

typedef int PS4_SYSV_ABI (*SceAvPlayerLogCallback)(void* p, const char* format, va_list args);

void RegisterlibSceAvPlayer(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::AvPlayer
