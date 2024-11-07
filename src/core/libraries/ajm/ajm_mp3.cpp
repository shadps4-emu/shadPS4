// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/ajm/ajm_error.h"
#include "core/libraries/ajm/ajm_mp3.h"
#include "core/libraries/error_codes.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace Libraries::Ajm {

// Following tables have been reversed from AJM library
static constexpr std::array<std::array<s32, 3>, 3> SamplerateTable = {{
    {0x5622, 0x5DC0, 0x3E80},
    {0xAC44, 0xBB80, 0x7D00},
    {0x2B11, 0x2EE0, 0x1F40},
}};

static constexpr std::array<std::array<s32, 15>, 2> BitrateTable = {{
    {0, 0x20, 0x28, 0x30, 0x38, 0x40, 0x50, 0x60, 0x70, 0x80, 0xA0, 0xC0, 0xE0, 0x100, 0x140},
    {0, 0x8, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0},
}};

static constexpr std::array<s32, 2> UnkTable = {0x48, 0x90};

static AVSampleFormat AjmToAVSampleFormat(AjmFormatEncoding format) {
    switch (format) {
    case AjmFormatEncoding::S16:
        return AV_SAMPLE_FMT_S16;
    case AjmFormatEncoding::S32:
        return AV_SAMPLE_FMT_S32;
    case AjmFormatEncoding::Float:
        return AV_SAMPLE_FMT_FLT;
    default:
        UNREACHABLE();
    }
}

AVFrame* AjmMp3Decoder::ConvertAudioFrame(AVFrame* frame) {
    AVSampleFormat format = AjmToAVSampleFormat(m_format);
    if (frame->format == format) {
        return frame;
    }

    AVFrame* new_frame = av_frame_alloc();
    new_frame->pts = frame->pts;
    new_frame->pkt_dts = frame->pkt_dts < 0 ? 0 : frame->pkt_dts;
    new_frame->format = format;
    new_frame->ch_layout = frame->ch_layout;
    new_frame->sample_rate = frame->sample_rate;

    AVChannelLayout in_ch_layout = frame->ch_layout;
    AVChannelLayout out_ch_layout = new_frame->ch_layout;
    swr_alloc_set_opts2(&m_swr_context, &out_ch_layout, AVSampleFormat(new_frame->format),
                        frame->sample_rate, &in_ch_layout, AVSampleFormat(frame->format),
                        frame->sample_rate, 0, nullptr);
    swr_init(m_swr_context);
    const auto res = swr_convert_frame(m_swr_context, new_frame, frame);
    if (res < 0) {
        LOG_ERROR(Lib_AvPlayer, "Could not convert to S16: {}", av_err2str(res));
        av_frame_free(&new_frame);
        av_frame_free(&frame);
        return nullptr;
    }
    av_frame_free(&frame);
    return new_frame;
}

AjmMp3Decoder::AjmMp3Decoder(AjmFormatEncoding format)
    : m_format(format), m_codec(avcodec_find_decoder(AV_CODEC_ID_MP3)),
      m_parser(av_parser_init(m_codec->id)) {
    AjmMp3Decoder::Reset();
}

AjmMp3Decoder::~AjmMp3Decoder() {
    swr_free(&m_swr_context);
    avcodec_free_context(&m_codec_context);
}

void AjmMp3Decoder::Reset() {
    if (m_codec_context) {
        avcodec_free_context(&m_codec_context);
    }
    m_codec_context = avcodec_alloc_context3(m_codec);
    ASSERT_MSG(m_codec_context, "Could not allocate audio m_codec context");
    int ret = avcodec_open2(m_codec_context, m_codec, nullptr);
    ASSERT_MSG(ret >= 0, "Could not open m_codec");
}

void AjmMp3Decoder::GetInfo(void* out_info) {
    auto* info = reinterpret_cast<AjmSidebandDecMp3CodecInfo*>(out_info);
}

std::tuple<u32, u32> AjmMp3Decoder::ProcessData(std::span<u8>& in_buf, SparseOutputBuffer& output,
                                                AjmSidebandGaplessDecode& gapless,
                                                std::optional<u32> max_samples_per_channel) {
    AVPacket* pkt = av_packet_alloc();

    int ret = av_parser_parse2(m_parser, m_codec_context, &pkt->data, &pkt->size, in_buf.data(),
                               in_buf.size(), AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    ASSERT_MSG(ret >= 0, "Error while parsing {}", ret);
    in_buf = in_buf.subspan(ret);

    u32 frames_decoded = 0;
    u32 samples_decoded = 0;

    auto max_samples =
        max_samples_per_channel.has_value()
            ? max_samples_per_channel.value() * m_codec_context->ch_layout.nb_channels
            : std::numeric_limits<u32>::max();

    if (pkt->size) {
        // Send the packet with the compressed data to the decoder
        pkt->pts = m_parser->pts;
        pkt->dts = m_parser->dts;
        pkt->flags = (m_parser->key_frame == 1) ? AV_PKT_FLAG_KEY : 0;
        ret = avcodec_send_packet(m_codec_context, pkt);
        ASSERT_MSG(ret >= 0, "Error submitting the packet to the decoder {}", ret);

        // Read all the output frames (in general there may be any number of them
        while (ret >= 0) {
            AVFrame* frame = av_frame_alloc();
            ret = avcodec_receive_frame(m_codec_context, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_frame_free(&frame);
                break;
            } else if (ret < 0) {
                UNREACHABLE_MSG("Error during decoding");
            }
            frame = ConvertAudioFrame(frame);

            frames_decoded += 1;
            u32 skipped_samples = 0;
            if (gapless.skipped_samples < gapless.skip_samples) {
                skipped_samples = std::min(u32(frame->nb_samples),
                                           u32(gapless.skip_samples - gapless.skipped_samples));
                gapless.skipped_samples += skipped_samples;
            }

            switch (m_format) {
            case AjmFormatEncoding::S16:
                samples_decoded +=
                    WriteOutputSamples<s16>(frame, output, skipped_samples, max_samples);
                break;
            case AjmFormatEncoding::S32:
                samples_decoded +=
                    WriteOutputSamples<s32>(frame, output, skipped_samples, max_samples);
                break;
            case AjmFormatEncoding::Float:
                samples_decoded +=
                    WriteOutputSamples<float>(frame, output, skipped_samples, max_samples);
                break;
            default:
                UNREACHABLE();
            }

            max_samples -= samples_decoded;

            av_frame_free(&frame);
        }
    }

    av_packet_free(&pkt);

    return {frames_decoded, samples_decoded};
}

int AjmMp3Decoder::ParseMp3Header(const u8* buf, u32 stream_size, int parse_ofl,
                                  AjmDecMp3ParseFrame* frame) {
    LOG_INFO(Lib_Ajm, "called stream_size = {} parse_ofl = {}", stream_size, parse_ofl);
    if (buf == nullptr || stream_size < 4 || frame == nullptr) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    if ((buf[0] & SYNCWORDH) != SYNCWORDH || (buf[1] & SYNCWORDL) != SYNCWORDL) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }

    const u32 unk_idx = buf[1] >> 3 & 1;
    const s32 version_idx = (buf[1] >> 3 & 3) ^ 2;
    const s32 sr_idx = buf[2] >> 2 & 3;
    const s32 br_idx = (buf[2] >> 4) & 0xf;
    const s32 padding_bit = (buf[2] >> 1) & 0x1;

    frame->sample_rate = SamplerateTable[version_idx][sr_idx];
    frame->bitrate = BitrateTable[version_idx != 1][br_idx] * 1000;
    frame->num_channels = (buf[3] < 0xc0) + 1;
    frame->frame_size = (UnkTable[unk_idx] * frame->bitrate) / frame->sample_rate + padding_bit;
    frame->samples_per_channel = UnkTable[unk_idx] * 8;
    frame->encoder_delay = 0;

    return ORBIS_OK;
}

AjmSidebandFormat AjmMp3Decoder::GetFormat() {
    LOG_ERROR(Lib_Ajm, "Unimplemented");
    return AjmSidebandFormat{};
};

} // namespace Libraries::Ajm
