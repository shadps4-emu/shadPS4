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

AjmAt9Decoder::AjmAt9Decoder() {
    m_handle = Atrac9GetHandle();
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
    m_pcm_buffer.resize(m_codec_info.frameSamples * m_codec_info.channels, 0);
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
                                                u32 max_samples_per_channel) {
    int bytes_used = 0;
    u32 ret = Atrac9Decode(m_handle, in_buf.data(), m_pcm_buffer.data(), &bytes_used);
    ASSERT_MSG(ret == At9Status::ERR_SUCCESS, "Atrac9Decode failed ret = {:#x}", ret);
    in_buf = in_buf.subspan(bytes_used);

    m_superframe_bytes_remain -= bytes_used;
    std::span<s16> pcm_data{m_pcm_buffer};

    if (gapless.skipped_samples < gapless.skip_samples) {
        const auto skipped_samples = std::min(u32(m_codec_info.frameSamples),
                                              u32(gapless.skip_samples - gapless.skipped_samples));
        gapless.skipped_samples += skipped_samples;
        pcm_data = pcm_data.subspan(skipped_samples * m_codec_info.channels);
    }

    const auto max_samples = max_samples_per_channel == std::numeric_limits<u32>::max()
                                 ? max_samples_per_channel
                                 : max_samples_per_channel * m_codec_info.channels;

    const auto pcm_size = std::min(u32(pcm_data.size()), max_samples);
    const auto written = output.Write(pcm_data.subspan(0, pcm_size));

    m_num_frames += 1;
    if ((m_num_frames % m_codec_info.framesInSuperframe) == 0) {
        if (m_superframe_bytes_remain) {
            in_buf = in_buf.subspan(m_superframe_bytes_remain);
        }
        m_superframe_bytes_remain = m_codec_info.superframeSize;
        m_num_frames = 0;
    }

    return {1, (written / m_codec_info.channels) / sizeof(s16)};
}

} // namespace Libraries::Ajm
