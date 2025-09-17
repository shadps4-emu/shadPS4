// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/ajm/ajm_at9.h"
#include "error_codes.h"

extern "C" {
#include <decoder.h>
#include <libatrac9.h>
}

#include <vector>

namespace Libraries::Ajm {

struct RIFFChunkHeader {
    char name[4];
    u32 length;
};
static_assert(sizeof(RIFFChunkHeader) == 8);

struct AudioFormat {
    RIFFChunkHeader header;
    u16 fmt_type;
    u16 num_channels;
    u32 avg_sample_rate;
    u32 avg_byte_rate;
    u16 block_align;
    u16 bits_per_sample;
    u16 ext_size;
    union {
        u16 valid_bits_per_sample;
        u16 samples_per_block;
        u16 reserved;
    };
    u32 channel_mask;
    u8 guid[16];
    u32 version;
    u8 config_data[4];
    u32 reserved2;
};
static_assert(sizeof(AudioFormat) == 60);

struct SampleData {
    RIFFChunkHeader header;
    u32 sample_length;
    u32 encoder_delay;
    u32 encoder_delay2;
};
static_assert(sizeof(SampleData) == 20);

struct RIFFHeader {
    char riff[4];
    u32 size;
    char wave[4];
    AudioFormat format;
    SampleData samples;
    RIFFChunkHeader data_header;
};
static_assert(sizeof(RIFFHeader) == 100);

AjmAt9Decoder::AjmAt9Decoder(AjmFormatEncoding format, AjmAt9CodecFlags flags)
    : m_format(format), m_flags(flags), m_handle(Atrac9GetHandle()) {
    ASSERT_MSG(m_handle, "Atrac9GetHandle failed");
    AjmAt9Decoder::Reset();
}

AjmAt9Decoder::~AjmAt9Decoder() {
    Atrac9ReleaseHandle(m_handle);
}

void AjmAt9Decoder::Reset() {
    Atrac9ReleaseHandle(m_handle);
    m_handle = Atrac9GetHandle();
    Atrac9InitDecoder(m_handle, m_config_data);
    Atrac9GetCodecInfo(m_handle, &m_codec_info);

    m_num_frames = 0;
    m_superframe_bytes_remain = m_codec_info.superframeSize;
}

void AjmAt9Decoder::Initialize(const void* buffer, u32 buffer_size) {
    ASSERT_MSG(buffer_size == sizeof(AjmDecAt9InitializeParameters),
               "Incorrect At9 initialization buffer size {}", buffer_size);
    const auto params = reinterpret_cast<const AjmDecAt9InitializeParameters*>(buffer);
    std::memcpy(m_config_data, params->config_data, ORBIS_AT9_CONFIG_DATA_SIZE);
    AjmAt9Decoder::Reset();
    m_pcm_buffer.resize(m_codec_info.frameSamples * m_codec_info.channels * GetPCMSize(m_format),
                        0);
}

void AjmAt9Decoder::GetInfo(void* out_info) const {
    auto* info = reinterpret_cast<AjmSidebandDecAt9CodecInfo*>(out_info);
    info->super_frame_size = m_codec_info.superframeSize;
    info->frames_in_super_frame = m_codec_info.framesInSuperframe;
    info->frame_samples = m_codec_info.frameSamples;
    info->next_frame_size = static_cast<Atrac9Handle*>(m_handle)->Config.FrameBytes;
}

std::tuple<u32, u32> AjmAt9Decoder::ProcessData(std::span<u8>& in_buf, SparseOutputBuffer& output,
                                                AjmInstanceGapless& gapless) {
    if (True(m_flags & AjmAt9CodecFlags::ParseRiffHeader) &&
        memcmp(in_buf.data(), "RIFF", 4) == 0) {
        auto* header = reinterpret_cast<RIFFHeader*>(in_buf.data());
        in_buf = in_buf.subspan(sizeof(RIFFHeader));

        ASSERT(memcmp(header->wave, "WAVE", 4) == 0);
        ASSERT(memcmp(header->format.header.name, "fmt ", 4) == 0);
        ASSERT(memcmp(header->samples.header.name, "fact", 4) == 0);
        ASSERT(memcmp(header->data_header.name, "data", 4) == 0);

        std::memcpy(m_config_data, header->format.config_data, ORBIS_AT9_CONFIG_DATA_SIZE);
        gapless.init.total_samples = header->samples.sample_length;
        gapless.init.skip_samples = header->samples.encoder_delay;

        Reset();
    }

    int ret = 0;
    int bytes_used = 0;
    switch (m_format) {
    case AjmFormatEncoding::S16:
        ret = Atrac9Decode(m_handle, in_buf.data(), reinterpret_cast<s16*>(m_pcm_buffer.data()),
                           &bytes_used, True(m_flags & AjmAt9CodecFlags::NonInterleavedOutput));
        break;
    case AjmFormatEncoding::S32:
        ret = Atrac9DecodeS32(m_handle, in_buf.data(), reinterpret_cast<s32*>(m_pcm_buffer.data()),
                              &bytes_used, True(m_flags & AjmAt9CodecFlags::NonInterleavedOutput));
        break;
    case AjmFormatEncoding::Float:
        ret =
            Atrac9DecodeF32(m_handle, in_buf.data(), reinterpret_cast<float*>(m_pcm_buffer.data()),
                            &bytes_used, True(m_flags & AjmAt9CodecFlags::NonInterleavedOutput));
        break;
    default:
        UNREACHABLE();
    }
    ASSERT_MSG(ret == At9Status::ERR_SUCCESS, "Atrac9Decode failed ret = {:#x}", ret);
    in_buf = in_buf.subspan(bytes_used);

    m_superframe_bytes_remain -= bytes_used;

    u32 skip_samples = 0;
    if (gapless.current.skip_samples > 0) {
        skip_samples = std::min(u16(m_codec_info.frameSamples), gapless.current.skip_samples);
        gapless.current.skip_samples -= skip_samples;
    }

    const auto max_pcm = gapless.init.total_samples != 0
                             ? gapless.current.total_samples * m_codec_info.channels
                             : std::numeric_limits<u32>::max();

    size_t pcm_written = 0;
    switch (m_format) {
    case AjmFormatEncoding::S16:
        pcm_written = WriteOutputSamples<s16>(output, skip_samples, max_pcm);
        break;
    case AjmFormatEncoding::S32:
        pcm_written = WriteOutputSamples<s32>(output, skip_samples, max_pcm);
        break;
    case AjmFormatEncoding::Float:
        pcm_written = WriteOutputSamples<float>(output, skip_samples, max_pcm);
        break;
    default:
        UNREACHABLE();
    }

    const auto samples_written = pcm_written / m_codec_info.channels;
    gapless.current.skipped_samples += m_codec_info.frameSamples - samples_written;
    if (gapless.init.total_samples != 0) {
        gapless.current.total_samples -= samples_written;
    }

    m_num_frames += 1;
    if ((m_num_frames % m_codec_info.framesInSuperframe) == 0) {
        if (m_superframe_bytes_remain) {
            in_buf = in_buf.subspan(m_superframe_bytes_remain);
        }
        m_superframe_bytes_remain = m_codec_info.superframeSize;
        m_num_frames = 0;
    }

    return {1, samples_written};
}

AjmSidebandFormat AjmAt9Decoder::GetFormat() const {
    return AjmSidebandFormat{
        .num_channels = u32(m_codec_info.channels),
        .channel_mask = GetChannelMask(u32(m_codec_info.channels)),
        .sampl_freq = u32(m_codec_info.samplingRate),
        .sample_encoding = m_format,
        .bitrate = u32((m_codec_info.samplingRate * m_codec_info.superframeSize * 8) /
                       (m_codec_info.framesInSuperframe * m_codec_info.frameSamples)),
        .reserved = 0,
    };
}

u32 AjmAt9Decoder::GetNextFrameSize(const AjmInstanceGapless& gapless) const {
    const auto max_samples =
        gapless.init.total_samples != 0
            ? std::min(gapless.current.total_samples, u32(m_codec_info.frameSamples))
            : m_codec_info.frameSamples;
    const auto skip_samples = std::min(u32(gapless.current.skip_samples), max_samples);
    return (max_samples - skip_samples) * m_codec_info.channels * GetPCMSize(m_format);
}

} // namespace Libraries::Ajm
