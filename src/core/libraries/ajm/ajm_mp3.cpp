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

SwrContext* swr_context{};

AVFrame* ConvertAudioFrame(AVFrame* frame) {
    auto pcm16_frame = av_frame_clone(frame);
    pcm16_frame->format = AV_SAMPLE_FMT_S16;

    if (swr_context) {
        swr_free(&swr_context);
        swr_context = nullptr;
    }
    AVChannelLayout in_ch_layout = frame->ch_layout;
    AVChannelLayout out_ch_layout = pcm16_frame->ch_layout;
    swr_alloc_set_opts2(&swr_context, &out_ch_layout, AV_SAMPLE_FMT_S16, frame->sample_rate,
                        &in_ch_layout, AVSampleFormat(frame->format), frame->sample_rate, 0,
                        nullptr);
    swr_init(swr_context);
    const auto res = swr_convert_frame(swr_context, pcm16_frame, frame);
    if (res < 0) {
        LOG_ERROR(Lib_AvPlayer, "Could not convert to S16: {}", av_err2str(res));
        return nullptr;
    }
    av_frame_free(&frame);
    return pcm16_frame;
}

AjmMp3Decoder::AjmMp3Decoder() {
    m_codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
    ASSERT_MSG(m_codec, "MP3 m_codec not found");
    m_parser = av_parser_init(m_codec->id);
    ASSERT_MSG(m_parser, "Parser not found");
    AjmMp3Decoder::Reset();
}

AjmMp3Decoder::~AjmMp3Decoder() {
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
                                                u32 max_samples) {
    AVPacket* pkt = av_packet_alloc();

    int ret = av_parser_parse2(m_parser, m_codec_context, &pkt->data, &pkt->size, in_buf.data(),
                               in_buf.size(), AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    ASSERT_MSG(ret >= 0, "Error while parsing {}", ret);
    in_buf = in_buf.subspan(ret);

    u32 frames_decoded = 0;
    u32 samples_decoded = 0;

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
                break;
            } else if (ret < 0) {
                UNREACHABLE_MSG("Error during decoding");
            }
            if (frame->format != AV_SAMPLE_FMT_S16) {
                frame = ConvertAudioFrame(frame);
            }

            frames_decoded += 1;
            samples_decoded += frame->nb_samples;
            const auto size = frame->ch_layout.nb_channels * frame->nb_samples * sizeof(u16);
            std::span<s16> pcm_data(reinterpret_cast<s16*>(frame->data[0]), size >> 1);
            if (gapless.skipped_samples < gapless.skip_samples) {
                const auto skipped_samples = std::min(
                    u32(frame->nb_samples), u32(gapless.skip_samples - gapless.skipped_samples));
                gapless.skipped_samples += skipped_samples;
                pcm_data = pcm_data.subspan(skipped_samples * frame->ch_layout.nb_channels);
                samples_decoded -= skipped_samples;
            }

            const auto pcm_size = std::min(u32(pcm_data.size()), max_samples);
            output.Write(pcm_data.subspan(0, pcm_size));

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

} // namespace Libraries::Ajm
