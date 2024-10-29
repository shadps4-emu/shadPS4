// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

#include <boost/container/small_vector.hpp>

#include <optional>
#include <span>
#include <vector>

extern "C" {
struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;
}

namespace Libraries::Ajm {

constexpr int ORBIS_AJM_RESULT_NOT_INITIALIZED = 0x00000001;
constexpr int ORBIS_AJM_RESULT_INVALID_DATA = 0x00000002;
constexpr int ORBIS_AJM_RESULT_INVALID_PARAMETER = 0x00000004;
constexpr int ORBIS_AJM_RESULT_PARTIAL_INPUT = 0x00000008;
constexpr int ORBIS_AJM_RESULT_NOT_ENOUGH_ROOM = 0x00000010;
constexpr int ORBIS_AJM_RESULT_STREAM_CHANGE = 0x00000020;
constexpr int ORBIS_AJM_RESULT_TOO_MANY_CHANNELS = 0x00000040;
constexpr int ORBIS_AJM_RESULT_UNSUPPORTED_FLAG = 0x00000080;
constexpr int ORBIS_AJM_RESULT_SIDEBAND_TRUNCATED = 0x00000100;
constexpr int ORBIS_AJM_RESULT_PRIORITY_PASSED = 0x00000200;
constexpr int ORBIS_AJM_RESULT_CODEC_ERROR = 0x40000000;
constexpr int ORBIS_AJM_RESULT_FATAL = 0x80000000;

constexpr u32 ORBIS_AT9_CONFIG_DATA_SIZE = 4;

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

struct AjmSidebandGaplessDecode {
    u32 total_samples;
    u16 skip_samples;
    u16 skipped_samples;
};

struct AjmSidebandResampleParameters {
    float ratio;
    uint32_t flags;
};

struct AjmDecAt9InitializeParameters {
    u8 config_data[ORBIS_AT9_CONFIG_DATA_SIZE];
    u32 reserved;
};

union AjmSidebandInitParameters {
    AjmDecAt9InitializeParameters at9;
    u8 reserved[8];
};

struct AjmJobInput {
    std::optional<AjmDecAt9InitializeParameters> init_params;
    std::optional<AjmSidebandResampleParameters> resample_parameters;
    std::optional<AjmSidebandFormat> format;
    std::optional<AjmSidebandGaplessDecode> gapless_decode;
    boost::container::small_vector<std::vector<u8>, 4> buffers;
};

struct AjmJobOutput {
    boost::container::small_vector<std::span<u8>, 4> buffers;
    AjmSidebandResult* p_result = nullptr;
    AjmSidebandStream* p_stream = nullptr;
    AjmSidebandFormat* p_format = nullptr;
    AjmSidebandGaplessDecode* p_gapless_decode = nullptr;
    AjmSidebandMFrame* p_mframe = nullptr;
    u8* p_codec_info = nullptr;
};

union AjmInstanceFlags {
    u64 raw;
    struct {
        u64 version : 3;
        u64 channels : 4;
        u64 format : 3;
        u64 gapless_loop : 1;
        u64 pad : 21;
        u64 codec : 28;
    };
};

struct AjmInstance {
    AjmCodecType codec_type;
    AjmFormatEncoding fmt{};
    AjmInstanceFlags flags{.raw = 0};
    u32 num_channels{};
    u32 index{};
    u32 bytes_remain{};
    u32 num_frames{};
    u32 decoded_samples{};
    AjmSidebandFormat format{};
    AjmSidebandGaplessDecode gapless{};
    AjmSidebandResampleParameters resample_parameters{};

    explicit AjmInstance() = default;
    virtual ~AjmInstance() = default;

    virtual void Reset() = 0;

    virtual void Initialize(const void* buffer, u32 buffer_size) = 0;

    virtual void GetCodecInfo(void* out_info) = 0;
    virtual u32 GetCodecInfoSize() = 0;

    virtual std::tuple<u32, u32> Decode(const u8* in_buf, u32 in_size, u8* out_buf, u32 out_size,
                                        AjmJobOutput* output) = 0;
};

} // namespace Libraries::Ajm
