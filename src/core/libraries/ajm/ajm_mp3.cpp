// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/ajm/ajm_mp3.h"

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
    codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
    ASSERT_MSG(codec, "MP3 codec not found");
    parser = av_parser_init(codec->id);
    ASSERT_MSG(parser, "Parser not found");
    AjmMp3Decoder::Reset();
}

AjmMp3Decoder::~AjmMp3Decoder() {
    avcodec_free_context(&c);
    av_free(c);
}

void AjmMp3Decoder::Reset() {
    if (c) {
        avcodec_free_context(&c);
        av_free(c);
    }
    c = avcodec_alloc_context3(codec);
    ASSERT_MSG(c, "Could not allocate audio codec context");
    int ret = avcodec_open2(c, codec, nullptr);
    ASSERT_MSG(ret >= 0, "Could not open codec");
    total_decoded_samples = 0;
}

void AjmMp3Decoder::Decode(const AjmJobInput* input, AjmJobOutput* output) {
    AVPacket* pkt = av_packet_alloc();

    size_t out_buffer_index = 0;
    std::span<const u8> in_buf(input->buffer);
    std::span<u8> out_buf = output->buffers[out_buffer_index];
    const auto should_decode = [&] {
        if (in_buf.empty() || out_buf.empty()) {
            return false;
        }
        if (gapless.total_samples != 0 && gapless.total_samples < gapless_decoded_samples) {
            return false;
        }
        return true;
    };

    const auto write_output = [&](std::span<s16> pcm) {
        while (!pcm.empty()) {
            auto size = std::min(pcm.size() * sizeof(u16), out_buf.size());
            std::memcpy(out_buf.data(), pcm.data(), size);
            pcm = pcm.subspan(size >> 1);
            out_buf = out_buf.subspan(size);
            if (out_buf.empty()) {
                out_buffer_index += 1;
                if (out_buffer_index >= output->buffers.size()) {
                    return pcm.empty();
                }
                out_buf = output->buffers[out_buffer_index];
            }
        }
        return true;
    };

    while (should_decode()) {
        int ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, in_buf.data(), in_buf.size(),
                                   AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        ASSERT_MSG(ret >= 0, "Error while parsing {}", ret);
        in_buf = in_buf.subspan(ret);

        if (output->p_stream) {
            output->p_stream->input_consumed += ret;
        }
        if (pkt->size) {
            // Send the packet with the compressed data to the decoder
            pkt->pts = parser->pts;
            pkt->dts = parser->dts;
            pkt->flags = (parser->key_frame == 1) ? AV_PKT_FLAG_KEY : 0;
            ret = avcodec_send_packet(c, pkt);
            ASSERT_MSG(ret >= 0, "Error submitting the packet to the decoder {}", ret);

            // Read all the output frames (in general there may be any number of them
            while (ret >= 0) {
                AVFrame* frame = av_frame_alloc();
                ret = avcodec_receive_frame(c, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    UNREACHABLE_MSG("Error during decoding");
                }
                if (frame->format != AV_SAMPLE_FMT_S16) {
                    frame = ConvertAudioFrame(frame);
                }
                const auto frame_samples = frame->ch_layout.nb_channels * frame->nb_samples;
                const auto size = frame_samples * sizeof(u16);
                if (gapless.skipped_samples < gapless.skip_samples) {
                    gapless.skipped_samples += frame_samples;
                    if (gapless.skipped_samples > gapless.skip_samples) {
                        const u32 nsamples = gapless.skipped_samples - gapless.skip_samples;
                        const auto start = frame_samples - nsamples;
                        write_output({reinterpret_cast<s16*>(frame->data[0]), nsamples});
                        gapless.skipped_samples = gapless.skip_samples;
                        total_decoded_samples += nsamples;
                        if (gapless.total_samples != 0) {
                            gapless_decoded_samples += nsamples;
                        }
                    }
                } else {
                    write_output({reinterpret_cast<s16*>(frame->data[0]), size >> 1});
                    total_decoded_samples += frame_samples;
                    if (gapless.total_samples != 0) {
                        gapless_decoded_samples += frame_samples;
                    }
                }
                av_frame_free(&frame);
                if (output->p_stream) {
                    output->p_stream->output_written += size;
                }
                if (output->p_mframe) {
                    output->p_mframe->num_frames += 1;
                }
            }
        }
    }
    av_packet_free(&pkt);
    if (gapless.total_samples != 0 && gapless_decoded_samples >= gapless.total_samples) {
        if (flags.gapless_loop) {
            gapless.skipped_samples = 0;
            gapless_decoded_samples = 0;
        }
    }
    if (output->p_stream) {
        output->p_stream->total_decoded_samples = total_decoded_samples;
    }
}

int AjmMp3Decoder::ParseMp3Header(const u8* buf, u32 stream_size, int parse_ofl,
                                  AjmDecMp3ParseFrame* frame) {
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
    if (parse_ofl == 0) {
        return 0;
    }

    return 0;
}

} // namespace Libraries::Ajm
