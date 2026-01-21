// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ajm.h"
#include "ajm_aac.h"
#include "ajm_result.h"

#include <aacdecoder_lib.h>
// using this internal header to manually configure the decoder in RAW mode
#include "externals/aacdec/fdk-aac/libAACdec/src/aacdecoder.h"

#include <algorithm> // std::transform
#include <iterator>  // std::back_inserter
#include <limits>

namespace Libraries::Ajm {

std::span<const s16> AjmAacDecoder::GetOuputPcm(u32 skipped_pcm, u32 max_pcm) const {
    const auto pcm_data = std::span(m_pcm_buffer).subspan(skipped_pcm);
    return pcm_data.subspan(0, std::min<u32>(pcm_data.size(), max_pcm));
}

template <>
size_t AjmAacDecoder::WriteOutputSamples<float>(SparseOutputBuffer& out, std::span<const s16> pcm) {
    if (pcm.empty()) {
        return 0;
    }

    m_resample_buffer.clear();
    constexpr float inv_scale = 1.0f / std::numeric_limits<s16>::max();
    std::transform(pcm.begin(), pcm.end(), std::back_inserter(m_resample_buffer),
                   [](auto sample) { return float(sample) * inv_scale; });

    return out.Write(std::span(m_resample_buffer));
}

AjmAacDecoder::AjmAacDecoder(AjmFormatEncoding format, AjmAacCodecFlags flags, u32 channels)
    : m_format(format), m_flags(flags), m_channels(channels), m_pcm_buffer(1024 * 8),
      m_skip_frames(True(flags & AjmAacCodecFlags::EnableNondelayOutput) ? 0 : 2) {
    m_resample_buffer.reserve(m_pcm_buffer.size());
}

AjmAacDecoder::~AjmAacDecoder() {
    if (m_decoder) {
        aacDecoder_Close(m_decoder);
    }
}

TRANSPORT_TYPE TransportTypeFromConfigType(ConfigType config_type) {
    switch (config_type) {
    case ConfigType::ADTS:
        return TT_MP4_ADTS;
    case ConfigType::RAW:
        return TT_MP4_RAW;
    default:
        UNREACHABLE();
    }
}

static UINT g_freq[] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000,
};

void AjmAacDecoder::Reset() {
    if (m_decoder) {
        aacDecoder_Close(m_decoder);
    }

    m_decoder = aacDecoder_Open(TransportTypeFromConfigType(m_init_params.config_type), 1);
    if (m_init_params.config_type == ConfigType::RAW) {
        // Manually configure the decoder
        // Things may be incorrect due to limited documentation
        CSAudioSpecificConfig asc{};
        asc.m_aot = AOT_AAC_LC;
        asc.m_samplingFrequency = g_freq[m_init_params.sampling_freq_type];
        asc.m_samplingFrequencyIndex = m_init_params.sampling_freq_type;
        asc.m_samplesPerFrame = 1024;
        asc.m_epConfig = -1;
        switch (m_channels) {
        case 0:
            asc.m_channelConfiguration = 2;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            asc.m_channelConfiguration = m_channels;
            break;
        case 7:
            asc.m_channelConfiguration = 11;
            break;
        case 8:
            asc.m_channelConfiguration = 12; // 7, 12 or 14 ?
            break;
        default:
            UNREACHABLE();
        }

        UCHAR changed = 1;
        CAacDecoder_Init(m_decoder, &asc, AC_CM_ALLOC_MEM, &changed);
    }
    m_skip_frames = True(m_flags & AjmAacCodecFlags::EnableNondelayOutput) ? 0 : 2;
}

void AjmAacDecoder::Initialize(const void* buffer, u32 buffer_size) {
    ASSERT(buffer_size == 8);
    m_init_params = *reinterpret_cast<const InitializeParameters*>(buffer);
    Reset();
}

void AjmAacDecoder::GetInfo(void* out_info) const {
    auto* codec_info = reinterpret_cast<AjmSidebandDecM4aacCodecInfo*>(out_info);
    *codec_info = {
        .heaac = True(m_flags & AjmAacCodecFlags::EnableSbrDecode),
    };
}

AjmSidebandFormat AjmAacDecoder::GetFormat() const {
    const auto* const info = aacDecoder_GetStreamInfo(m_decoder);
    return {
        .num_channels = static_cast<u32>(info->numChannels),
        .channel_mask = GetChannelMask(info->numChannels),
        .sampl_freq = static_cast<u32>(info->sampleRate),
        .sample_encoding = m_format,
        .bitrate = static_cast<u32>(info->bitRate),
    };
}

u32 AjmAacDecoder::GetMinimumInputSize() const {
    return 0;
}

u32 AjmAacDecoder::GetNextFrameSize(const AjmInstanceGapless& gapless) const {
    const auto* const info = aacDecoder_GetStreamInfo(m_decoder);
    if (info->aacSamplesPerFrame <= 0) {
        return 0;
    }
    const auto skip_samples = std::min<u32>(gapless.current.skip_samples, info->frameSize);
    const auto samples =
        gapless.init.total_samples != 0
            ? std::min<u32>(gapless.current.total_samples, info->frameSize - skip_samples)
            : info->frameSize - skip_samples;
    return samples * info->numChannels * GetPCMSize(m_format);
}

DecoderResult AjmAacDecoder::ProcessData(std::span<u8>& input, SparseOutputBuffer& output,
                                         AjmInstanceGapless& gapless) {
    DecoderResult result{};

    // Discard the previous contents of the internal buffer and replace them with new ones
    aacDecoder_SetParam(m_decoder, AAC_TPDEC_CLEAR_BUFFER, 1);
    UCHAR* buffers[] = {input.data()};
    const UINT sizes[] = {static_cast<UINT>(input.size())};
    UINT valid = sizes[0];
    aacDecoder_Fill(m_decoder, buffers, sizes, &valid);
    auto ret = aacDecoder_DecodeFrame(m_decoder, m_pcm_buffer.data(), m_pcm_buffer.size(), 0);

    switch (ret) {
    case AAC_DEC_OK:
        break;
    case AAC_DEC_NOT_ENOUGH_BITS:
        result.result = ORBIS_AJM_RESULT_PARTIAL_INPUT;
        return result;
    default:
        LOG_ERROR(Lib_Ajm, "aacDecoder_DecodeFrame failed ret = {:#x}", static_cast<u32>(ret));
        result.result = ORBIS_AJM_RESULT_CODEC_ERROR | ORBIS_AJM_RESULT_FATAL;
        result.internal_result = ret;
        return result;
    }

    const auto* const info = aacDecoder_GetStreamInfo(m_decoder);
    auto bytes_used = info->numTotalBytes;

    result.frames_decoded += 1;
    input = input.subspan(bytes_used);

    if (m_skip_frames > 0) {
        --m_skip_frames;
        return result;
    }

    u32 skip_samples = 0;
    if (gapless.current.skip_samples > 0) {
        skip_samples = std::min<u16>(info->frameSize, gapless.current.skip_samples);
        gapless.current.skip_samples -= skip_samples;
    }

    const auto max_samples =
        gapless.init.total_samples != 0 ? gapless.current.total_samples : info->aacSamplesPerFrame;

    size_t pcm_written = 0;
    auto pcm = GetOuputPcm(skip_samples * info->numChannels, max_samples * info->numChannels);
    switch (m_format) {
    case AjmFormatEncoding::S16:
        pcm_written = output.Write(pcm);
        break;
    case AjmFormatEncoding::S32:
        UNREACHABLE_MSG("NOT IMPLEMENTED");
        break;
    case AjmFormatEncoding::Float:
        pcm_written = WriteOutputSamples<float>(output, pcm);
        break;
    default:
        UNREACHABLE();
    }

    result.samples_written = pcm_written / info->numChannels;
    gapless.current.skipped_samples += info->frameSize - result.samples_written;
    if (gapless.init.total_samples != 0) {
        gapless.current.total_samples -= result.samples_written;
    }

    return result;
}

} // namespace Libraries::Ajm