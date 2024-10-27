// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

extern "C" {
struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;
}

namespace Libraries::Ajm {

enum class AjmCodecType : u32 {
    Mp3Dec = 0,
    At9Dec = 1,
    M4aacDec = 2,
    Max = 23,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmCodecType);
static constexpr u32 NumAjmCodecs = u32(AjmCodecType::Max);

enum class AjmFormatEncoding : u32 {
    S16 = 0,
    S32 = 1,
    Float = 2,
};

struct AjmSidebandResult {
    s32 result;
    s32 internal_result;
};

struct AjmSidebandMFrame {
    u32 num_frames;
    u32 reserved;
};

struct AjmSidebandStream {
    s32 input_consumed;
    s32 output_written;
    u64 total_decoded_samples;
};

struct AjmSidebandFormat {
    u32 num_channels;
    u32 channel_mask;
    u32 sampl_freq;
    AjmFormatEncoding sample_encoding;
    u32 bitrate;
    u32 reserved;
};

struct AjmInstance {
    AjmCodecType codec_type;
    u32 decoded_samples{};
    AjmFormatEncoding fmt{};
    u32 num_channels{};
    u32 index{};

    explicit AjmInstance() = default;
    virtual ~AjmInstance() = default;

    virtual void Reset() = 0;

    virtual void Initialize(const void* buffer, u32 buffer_size) = 0;

    virtual void GetCodecInfo(void* out_info) = 0;

    virtual std::tuple<u32, u32, u32> Decode(const u8* in_buf, u32 in_size, u8* out_buf,
                                             u32 out_size) = 0;
};

} // namespace Libraries::Ajm
