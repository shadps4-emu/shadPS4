// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/ajm/ajm_instance.h"

extern "C" {
#include <libavcodec/avcodec.h>
struct SwrContext;
}

namespace Libraries::Ajm {

enum class AjmDecMp3OflType : u32 {
    None = 0,
    Lame = 1,
    Vbri = 2,
    Fgh = 3,
    VbriAndFgh = 4,
};

enum AjmMp3CodecFlags : u32 {
    IgnoreOfl = 1 << 0,
    VlcRewind = 1 << 8,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmMp3CodecFlags)

// 11-bit syncword if MPEG 2.5 extensions are enabled
static constexpr u8 SYNCWORDH = 0xff;
static constexpr u8 SYNCWORDL = 0xe0;

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

class AjmMp3Decoder : public AjmCodec {
public:
    explicit AjmMp3Decoder(AjmFormatEncoding format, AjmMp3CodecFlags flags);
    ~AjmMp3Decoder() override;

    void Reset() override;
    void Initialize(const void* buffer, u32 buffer_size) override {}
    void GetInfo(void* out_info) const override;
    AjmSidebandFormat GetFormat() const override;
    u32 GetNextFrameSize(const AjmInstanceGapless& gapless) const override;
    std::tuple<u32, u32, bool> ProcessData(std::span<u8>& input, SparseOutputBuffer& output,
                                           AjmInstanceGapless& gapless) override;

    static int ParseMp3Header(const u8* buf, u32 stream_size, int parse_ofl,
                              AjmDecMp3ParseFrame* frame);

private:
    template <class T>
    size_t WriteOutputPCM(AVFrame* frame, SparseOutputBuffer& output, u32 skipped_samples,
                          u32 max_pcm) {
        std::span<T> pcm_data(reinterpret_cast<T*>(frame->data[0]),
                              frame->nb_samples * frame->ch_layout.nb_channels);
        pcm_data = pcm_data.subspan(skipped_samples * frame->ch_layout.nb_channels);
        return output.Write(pcm_data.subspan(0, std::min(u32(pcm_data.size()), max_pcm)));
    }

    AVFrame* ConvertAudioFrame(AVFrame* frame);

    const AjmFormatEncoding m_format;
    const AjmMp3CodecFlags m_flags;
    const AVCodec* m_codec = nullptr;
    AVCodecContext* m_codec_context = nullptr;
    AVCodecParserContext* m_parser = nullptr;
    SwrContext* m_swr_context = nullptr;
    std::optional<u32> m_header;
    u32 m_frame_samples = 0;
};

} // namespace Libraries::Ajm
