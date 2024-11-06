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
    m_pcm_buffer.resize(m_codec_info.frameSamples * m_codec_info.channels * GetPointCodeSize(), 0);
}

u8 AjmAt9Decoder::GetPointCodeSize() {
    switch (m_format) {
    case AjmFormatEncoding::S16:
        return sizeof(s16);
    case AjmFormatEncoding::S32:
        return sizeof(s32);
    case AjmFormatEncoding::Float:
        return sizeof(float);
    default:
        UNREACHABLE();
    }
}

void AjmAt9Decoder::GetInfo(void* out_info) {
    auto* info = reinterpret_cast<AjmSidebandDecAt9CodecInfo*>(out_info);
    info->super_frame_size = m_codec_info.superframeSize;
    info->frames_in_super_frame = m_codec_info.framesInSuperframe;
    info->frame_samples = m_codec_info.frameSamples;
    info->next_frame_size = static_cast<Atrac9Handle*>(m_handle)->Config.FrameBytes;
}

std::tuple<u32, u32> AjmAt9Decoder::ProcessData(std::span<u8>& in_buf, SparseOutputBuffer& output,
                                                AjmSidebandGaplessDecode& gapless,
                                                std::optional<u32> max_samples_per_channel) {
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

    u32 skipped_samples = 0;
    if (gapless.skipped_samples < gapless.skip_samples) {
        skipped_samples = std::min(u32(m_codec_info.frameSamples),
                                   u32(gapless.skip_samples - gapless.skipped_samples));
        gapless.skipped_samples += skipped_samples;
    }

    const auto max_samples = max_samples_per_channel.has_value()
                                 ? max_samples_per_channel.value() * m_codec_info.channels
                                 : std::numeric_limits<u32>::max();

    size_t samples_written = 0;
    switch (m_format) {
    case AjmFormatEncoding::S16:
        samples_written = WriteOutputSamples<s16>(output, skipped_samples, max_samples);
        break;
    case AjmFormatEncoding::S32:
        samples_written = WriteOutputSamples<s32>(output, skipped_samples, max_samples);
        break;
    case AjmFormatEncoding::Float:
        samples_written = WriteOutputSamples<float>(output, skipped_samples, max_samples);
        break;
    default:
        UNREACHABLE();
    }

    m_num_frames += 1;
    if ((m_num_frames % m_codec_info.framesInSuperframe) == 0) {
        if (m_superframe_bytes_remain) {
            in_buf = in_buf.subspan(m_superframe_bytes_remain);
        }
        m_superframe_bytes_remain = m_codec_info.superframeSize;
        m_num_frames = 0;
    }

    return {1, samples_written / m_codec_info.channels};
}

AjmSidebandFormat AjmAt9Decoder::GetFormat() {
    return AjmSidebandFormat{
        .num_channels = u32(m_codec_info.channels),
        .channel_mask = GetChannelMask(u32(m_codec_info.channels)),
        .sampl_freq = u32(m_codec_info.samplingRate),
        .sample_encoding = m_format,
        .bitrate = u32(m_codec_info.samplingRate * GetPointCodeSize() * 8),
        .reserved = 0,
    };
}

} // namespace Libraries::Ajm
