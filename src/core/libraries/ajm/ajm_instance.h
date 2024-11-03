// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/ajm/ajm_batch.h"

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

enum class AjmCodecType : u32 {
    Mp3Dec = 0,
    At9Dec = 1,
    M4aacDec = 2,
    Max = 23,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmCodecType);

union AjmSidebandInitParameters {
    AjmDecAt9InitializeParameters at9;
    u8 reserved[8];
};

struct AjmInstance {
    AjmCodecType codec_type;
    AjmFormatEncoding fmt{};
    AjmInstanceFlags flags{.raw = 0};
    u32 num_channels{};
    u32 index{};
    u32 gapless_decoded_samples{};
    u32 total_decoded_samples{};
    AjmSidebandFormat format{};
    AjmSidebandGaplessDecode gapless{};
    AjmSidebandResampleParameters resample_parameters{};

    explicit AjmInstance() = default;
    virtual ~AjmInstance() = default;

    virtual void Reset() = 0;

    virtual void Initialize(const void* buffer, u32 buffer_size) = 0;

    virtual void GetCodecInfo(void* out_info) = 0;
    virtual u32 GetCodecInfoSize() = 0;

    virtual void Decode(const AjmJob::Input* input, AjmJob::Output* output) = 0;
};

} // namespace Libraries::Ajm
