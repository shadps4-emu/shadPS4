// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <fstream>
#include "common/types.h"
#include "core/libraries/ajm/ajm_instance.h"

extern "C" {
struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;
}

namespace Libraries::Ajm {

enum class AjmDecMp3OflType : u32 {
    None = 0,
    Lame = 1,
    Vbri = 2,
    Fgh = 3,
    VbriAndFgh = 4
};

// 11-bit syncword if MPEG 2.5 extensions are enabled
static constexpr u8 SYNCWORDH = 0xff;
static constexpr u8	SYNCWORDL = 0xe0;

struct AjmDecMp3ParseFrame {
    u64 frame_size;
    u32 num_channels;
    u32 samples_per_channel;
    u32 bitrate;
    u32 sample_rate;
    u32 encoder_delay;
    u32 num_frames;
    u32 total_samples;
    AjmDecMp3OflType ofl_type;
};

enum class ChannelMode : u8 {
    Stereo = 0,
    JointStero = 1,
    Dual = 2,
    Mono = 3,
};

struct AjmSidebandDecMp3CodecInfo {
    u32 header;
    bool has_crc;
    ChannelMode channel_mode;
    u8 mode_extension;
    u8 copyright;
    u8 original;
    u8 emphasis;
    u16 reserved[3];
};

struct AjmDecMp3GetCodecInfoResult {
    AjmSidebandResult result;
    AjmSidebandDecMp3CodecInfo codec_info;
};

struct AjmMp3Decoder : public AjmInstance {
    const AVCodec* codec = nullptr;
    AVCodecContext* c = nullptr;
    AVCodecParserContext* parser = nullptr;
    u32 index;
    std::ofstream file;

    explicit AjmMp3Decoder();
    ~AjmMp3Decoder() override;

    void Reset() override;

    std::tuple<u32, u32, u32> Decode(const u8* in_buf, u32 in_size,
                                     u8* out_buf, u32 out_size) override;

    static int ParseMp3Header(const u8* buf, u32 stream_size, int parse_ofl,
                              AjmDecMp3ParseFrame* frame);
};

} // namespace Libraries::Ajm
