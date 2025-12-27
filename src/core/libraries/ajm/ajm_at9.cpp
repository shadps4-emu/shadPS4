// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ajm_result.h"
#include "common/assert.h"
#include "core/libraries/ajm/ajm_at9.h"
#include "error_codes.h"

extern "C" {
#include <decoder.h>
#include <libatrac9.h>
}

namespace Libraries::Ajm {

struct ChunkHeader {
    u32 tag;
    u32 length;
};
static_assert(sizeof(ChunkHeader) == 8);

struct AudioFormat {
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
static_assert(sizeof(AudioFormat) == 52);

struct SampleData {
    u32 sample_length;
    u32 encoder_delay;
    u32 encoder_delay2;
};
static_assert(sizeof(SampleData) == 12);

struct RIFFHeader {
    u32 riff;
    u32 size;
    u32 wave;
};
static_assert(sizeof(RIFFHeader) == 12);

AjmAt9Decoder::AjmAt9Decoder(AjmFormatEncoding format, AjmAt9CodecFlags flags, u32)
    : m_format(format), m_flags(flags), m_handle(Atrac9GetHandle()) {}

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
    m_is_initialized = true;
}

void AjmAt9Decoder::GetInfo(void* out_info) const {
    auto* info = reinterpret_cast<AjmSidebandDecAt9CodecInfo*>(out_info);
    info->super_frame_size = m_codec_info.superframeSize;
    info->frames_in_super_frame = m_codec_info.framesInSuperframe;
    info->next_frame_size = m_superframe_bytes_remain;
    info->frame_samples = m_codec_info.frameSamples;
}

u8 g_at9_guid[] = {0xD2, 0x42, 0xE1, 0x47, 0xBA, 0x36, 0x8D, 0x4D,
                   0x88, 0xFC, 0x61, 0x65, 0x4F, 0x8C, 0x83, 0x6C};

void AjmAt9Decoder::ParseRIFFHeader(std::span<u8>& in_buf, AjmInstanceGapless& gapless) {
    auto* header = reinterpret_cast<RIFFHeader*>(in_buf.data());
    in_buf = in_buf.subspan(sizeof(RIFFHeader));

    ASSERT(header->riff == 'FFIR');
    ASSERT(header->wave == 'EVAW');

    auto* chunk = reinterpret_cast<ChunkHeader*>(in_buf.data());
    in_buf = in_buf.subspan(sizeof(ChunkHeader));
    while (chunk->tag != 'atad') {
        switch (chunk->tag) {
        case ' tmf': {
            ASSERT(chunk->length == sizeof(AudioFormat));
            auto* fmt = reinterpret_cast<AudioFormat*>(in_buf.data());

            ASSERT(fmt->fmt_type == 0xFFFE);
            ASSERT(memcmp(fmt->guid, g_at9_guid, 16) == 0);
            AjmDecAt9InitializeParameters init_params = {};
            std::memcpy(init_params.config_data, fmt->config_data, ORBIS_AT9_CONFIG_DATA_SIZE);
            Initialize(&init_params, sizeof(init_params));
            break;
        }
        case 'tcaf': {
            ASSERT(chunk->length == sizeof(SampleData));
            auto* samples = reinterpret_cast<SampleData*>(in_buf.data());

            gapless.init.total_samples = samples->sample_length;
            gapless.init.skip_samples = samples->encoder_delay;
            gapless.Reset();
            break;
        }
        default:
            break;
        }
        in_buf = in_buf.subspan(chunk->length);

        chunk = reinterpret_cast<ChunkHeader*>(in_buf.data());
        in_buf = in_buf.subspan(sizeof(ChunkHeader));
    }
}

u32 AjmAt9Decoder::GetMinimumInputSize() const {
    return m_superframe_bytes_remain;
}

DecoderResult AjmAt9Decoder::ProcessData(std::span<u8>& in_buf, SparseOutputBuffer& output,
                                         AjmInstanceGapless& gapless) {
    DecoderResult result{};
    if (True(m_flags & AjmAt9CodecFlags::ParseRiffHeader) &&
        *reinterpret_cast<u32*>(in_buf.data()) == 'FFIR') {
        ParseRIFFHeader(in_buf, gapless);
        result.is_reset = true;
    }

    if (!m_is_initialized) {
        result.result = ORBIS_AJM_RESULT_NOT_INITIALIZED;
        return result;
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
    if (ret != At9Status::ERR_SUCCESS) {
        LOG_ERROR(Lib_Ajm, "Atrac9Decode failed ret = {:#x}", ret);
        result.result = ORBIS_AJM_RESULT_CODEC_ERROR | ORBIS_AJM_RESULT_FATAL;
        result.internal_result = ret;
        return result;
    }

    result.frames_decoded += 1;
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

    result.samples_written = pcm_written / m_codec_info.channels;
    gapless.current.skipped_samples += m_codec_info.frameSamples - result.samples_written;
    if (gapless.init.total_samples != 0) {
        gapless.current.total_samples -= result.samples_written;
    }

    m_num_frames += 1;
    if ((m_num_frames % m_codec_info.framesInSuperframe) == 0) {
        if (m_superframe_bytes_remain) {
            in_buf = in_buf.subspan(m_superframe_bytes_remain);
        }
        m_superframe_bytes_remain = m_codec_info.superframeSize;
        m_num_frames = 0;
    } else if (gapless.IsEnd()) {
        // Drain the remaining superframe
        std::vector<s16> buf(m_codec_info.frameSamples * m_codec_info.channels, 0);
        while ((m_num_frames % m_codec_info.framesInSuperframe) != 0) {
            ret = Atrac9Decode(m_handle, in_buf.data(), buf.data(), &bytes_used,
                               True(m_flags & AjmAt9CodecFlags::NonInterleavedOutput));
            in_buf = in_buf.subspan(bytes_used);
            m_superframe_bytes_remain -= bytes_used;
            result.frames_decoded += 1;
            m_num_frames += 1;
        }
        in_buf = in_buf.subspan(m_superframe_bytes_remain);
        m_superframe_bytes_remain = m_codec_info.superframeSize;
        m_num_frames = 0;
    }

    return result;
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
    const auto skip_samples =
        std::min<u32>(gapless.current.skip_samples, m_codec_info.frameSamples);
    const auto samples =
        gapless.init.total_samples != 0
            ? std::min<u32>(gapless.current.total_samples, m_codec_info.frameSamples - skip_samples)
            : m_codec_info.frameSamples - skip_samples;
    return samples * m_codec_info.channels * GetPCMSize(m_format);
}

} // namespace Libraries::Ajm
