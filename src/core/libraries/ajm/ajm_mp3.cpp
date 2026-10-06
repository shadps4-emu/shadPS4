// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#include <cstring>

#include "ajm_error.h"
#include "ajm_mp3.h"
#include "ajm_result.h"

#include "common/assert.h"
#include "core/libraries/error_codes.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include "common/support/avdec.h"

namespace Libraries::Ajm {

// Following tables have been reversed from AJM library
static constexpr std::array<std::array<s32, 4>, 4> Mp3SampleRateTable = {
    std::array<s32, 4>{11025, 12000, 8000, 0},
    std::array<s32, 4>{0, 0, 0, 0},
    std::array<s32, 4>{22050, 24000, 16000, 0},
    std::array<s32, 4>{44100, 48000, 32000, 0},
};

static constexpr std::array<std::array<s32, 16>, 4> Mp3BitRateTable = {
    std::array<s32, 16>{0, 8, 16, 24, 32, 40, 48, 56, 64, 0, 0, 0, 0, 0, 0, 0},
    std::array<s32, 16>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    std::array<s32, 16>{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
    std::array<s32, 16>{0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0},
};

enum class Mp3AudioVersion : u32 {
    V2_5 = 0,
    Reserved = 1,
    V2 = 2,
    V1 = 3,
};

enum class Mp3ChannelMode : u32 {
    Stereo = 0,
    JointStereo = 1,
    DualChannel = 2,
    SingleChannel = 3,
};

struct Mp3Header {
    u32 emphasis : 2;
    u32 original : 1;
    u32 copyright : 1;
    u32 mode_ext_idx : 2;
    Mp3ChannelMode channel_mode : 2;
    u32 : 1;
    u32 padding : 1;
    u32 sampling_rate_idx : 2;
    u32 bitrate_idx : 4;
    u32 protection_type : 1;
    u32 layer_type : 2;
    Mp3AudioVersion version : 2;
    u32 sync : 11;
};
static_assert(sizeof(Mp3Header) == sizeof(u32));

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
        LOG_ERROR(Lib_AvPlayer, "Could not convert frame: {}", av_err2str(res));
        av_frame_free(&new_frame);
        av_frame_free(&frame);
        return nullptr;
    }
    av_frame_free(&frame);
    return new_frame;
}

AjmMp3Decoder::AjmMp3Decoder(AjmFormatEncoding format, AjmMp3CodecFlags flags, u32)
    : m_format(format), m_flags(flags), m_codec(avcodec_find_decoder(AV_CODEC_ID_MP3)),
      m_codec_context(avcodec_alloc_context3(m_codec)), m_parser(av_parser_init(m_codec->id)) {
    int ret = avcodec_open2(m_codec_context, m_codec, nullptr);
    ASSERT_MSG(ret >= 0, "Could not open m_codec");
}

AjmMp3Decoder::~AjmMp3Decoder() {
    swr_free(&m_swr_context);
    av_parser_close(m_parser);
    avcodec_free_context(&m_codec_context);
}

void AjmMp3Decoder::Reset() {
    avcodec_flush_buffers(m_codec_context);
    m_header.reset();
    m_frame_samples = 0;
}

void AjmMp3Decoder::GetInfo(void* out_info) const {
    auto* info = reinterpret_cast<AjmSidebandDecMp3CodecInfo*>(out_info);
    if (m_header.has_value()) {
        auto* header = reinterpret_cast<const Mp3Header*>(&m_header.value());
        info->header = std::byteswap(m_header.value());
        info->has_crc = header->protection_type;
        info->channel_mode = static_cast<ChannelMode>(header->channel_mode);
        info->mode_extension = header->mode_ext_idx;
        info->copyright = header->copyright;
        info->original = header->original;
        info->emphasis = header->emphasis;
    }
}

u32 AjmMp3Decoder::GetMinimumInputSize() const {
    // 4 bytes is for mp3 header that contains frame_size
    return 4;
}

DecoderResult AjmMp3Decoder::ProcessData(std::span<u8>& in_buf, SparseOutputBuffer& output,
                                         AjmInstanceGapless& gapless) {
    DecoderResult result{};
    AVPacket* pkt = av_packet_alloc();

    int ret = av_parser_parse2(m_parser, m_codec_context, &pkt->data, &pkt->size, in_buf.data(),
                               in_buf.size(), AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    ASSERT_MSG(ret >= 0, "Error while parsing {}", ret);
    in_buf = in_buf.subspan(ret);

    if (pkt->size >= 4) {
        u32 raw_header = 0;
        std::memcpy(&raw_header, pkt->data, sizeof(raw_header));
        m_header = std::byteswap(raw_header);
        AjmDecMp3ParseFrame info{};
        auto res = ParseMp3Header(pkt->data, pkt->size, true, &info);
        ASSERT(res == ORBIS_OK);

        m_frame_samples = info.samples_per_channel;
        if (info.total_samples != 0 || info.encoder_delay != 0) {
            gapless.init = {
                .total_samples = info.total_samples,
                .skip_samples = static_cast<u16>(info.encoder_delay),
                .skipped_samples = 0,
            };
            gapless.current = gapless.init;
        }

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

            result.frames_decoded += 1;
            u32 skip_samples = 0;
            if (gapless.current.skip_samples > 0) {
                skip_samples = std::min(u16(frame->nb_samples), gapless.current.skip_samples);
                gapless.current.skip_samples -= skip_samples;
            }

            const auto max_pcm =
                gapless.init.total_samples != 0
                    ? gapless.current.total_samples * m_codec_context->ch_layout.nb_channels
                    : std::numeric_limits<u32>::max();

            u32 pcm_written = 0;
            switch (m_format) {
            case AjmFormatEncoding::S16:
                pcm_written = WriteOutputPCM<s16>(frame, output, skip_samples, max_pcm);
                break;
            case AjmFormatEncoding::S32:
                pcm_written = WriteOutputPCM<s32>(frame, output, skip_samples, max_pcm);
                break;
            case AjmFormatEncoding::Float:
                pcm_written = WriteOutputPCM<float>(frame, output, skip_samples, max_pcm);
                break;
            default:
                UNREACHABLE();
            }

            const auto samples = pcm_written / m_codec_context->ch_layout.nb_channels;
            gapless.current.skipped_samples += frame->nb_samples - samples;
            if (gapless.init.total_samples != 0) {
                gapless.current.total_samples -= samples;
            }
            result.samples_written += samples;

            av_frame_free(&frame);
        }
    } else {
        result.result |= ORBIS_AJM_RESULT_PARTIAL_INPUT;
    }

    av_packet_free(&pkt);

    return result;
}

u32 AjmMp3Decoder::GetNextFrameSize(const AjmInstanceGapless& gapless) const {
    const auto skip_samples = std::min<u32>(gapless.current.skip_samples, m_frame_samples);
    const auto samples =
        gapless.init.total_samples != 0
            ? std::min<u32>(gapless.current.total_samples, m_frame_samples - skip_samples)
            : m_frame_samples - skip_samples;
    return samples * m_codec_context->ch_layout.nb_channels * GetPCMSize(m_format);
}

class BitReader {
public:
    BitReader(const u8* data, size_t max_bytes) : m_data(data), m_max_bytes(max_bytes) {}

    template <class T>
    T Read(u32 const nbits) {
        T accumulator = 0;
        for (unsigned i = 0; i < nbits; ++i) {
            accumulator = (accumulator << 1) + GetBit();
        }
        return accumulator;
    }

    void Skip(size_t nbits) {
        m_bit_offset += nbits;
    }

    size_t GetCurrentOffset() const {
        return m_bit_offset;
    }

private:
    u8 GetBit() {
        if (m_bit_offset / 8 >= m_max_bytes) {
            m_bit_offset += 1;
            return 0;
        }
        const auto bit = (m_data[m_bit_offset / 8] >> (7 - (m_bit_offset % 8))) & 1;
        m_bit_offset += 1;
        return bit;
    }

    const u8* m_data;
    size_t m_max_bytes = 0;
    size_t m_bit_offset = 0;
};

static void ParseMp3ExtendedHeader(const u8* p_begin, u32 stream_size, const Mp3Header* header,
                                   AjmDecMp3ParseFrame* frame) {
    const auto* p_end = p_begin + std::min<size_t>(stream_size, frame->frame_size);
    const auto* p_current = p_begin + 4;
    if (p_current >= p_end) {
        return;
    }

    const size_t available_bytes = static_cast<size_t>(p_end - p_current);
    BitReader reader(p_current, available_bytes);
    if (header->protection_type == 0) {
        reader.Skip(16);
    }

    if (header->version == Mp3AudioVersion::V1) {
        if (header->channel_mode == Mp3ChannelMode::SingleChannel) {
            reader.Skip(18);
        } else {
            reader.Skip(20);
        }
    } else {
        if (header->channel_mode == Mp3ChannelMode::SingleChannel) {
            reader.Skip(9);
        } else {
            reader.Skip(10);
        }
    }

    u32 part2_3_length = 0;
    const u8 ngr = header->version == Mp3AudioVersion::V1 ? 2 : 1;
    for (u8 gr = 0; gr < ngr; ++gr) {
        for (u32 ch = 0; ch < frame->num_channels; ++ch) {
            part2_3_length += reader.Read<u16>(12);
            if (header->version == Mp3AudioVersion::V1) {
                reader.Skip(47);
            } else {
                reader.Skip(51);
            }
        }
    }
    reader.Skip(part2_3_length);

    const size_t byte_offset = (reader.GetCurrentOffset() + 7) / 8;
    if (byte_offset > available_bytes) {
        p_current = p_end;
    } else {
        p_current += byte_offset;
    }

    if (p_current + 8 <= p_end &&
        (std::memcmp(p_current, "Xing", 4) == 0 || std::memcmp(p_current, "Info", 4) == 0)) {
        const u8 xing_flags = p_current[7];
        const u8* p_field = p_current + 8;
        u32 num_frames_xing = 0;

        if (xing_flags & 0x01) {
            if (p_field + 4 > p_end) {
                return;
            }
            u32 raw = 0;
            std::memcpy(&raw, p_field, sizeof(raw));
            num_frames_xing = std::byteswap(raw);
            frame->num_frames = num_frames_xing;
            p_field += 4;
        }
        if (xing_flags & 0x02) {
            p_field += 4;
            if (p_field > p_end) {
                return;
            }
        }
        if (xing_flags & 0x04) {
            p_field += 100;
            if (p_field > p_end) {
                return;
            }
        }
        if (xing_flags & 0x08) {
            p_field += 4;
            if (p_field > p_end) {
                return;
            }
        }

        if (p_field + 0x18 <= p_end && p_field[0] == 'L' && p_field[1] == 'A' &&
            p_field[2] == 'M' && p_field[3] == 'E') {
            const u32 enc_delay = (u32(p_field[0x15]) << 4) | (u32(p_field[0x16]) >> 4);
            const u32 padding = ((u32(p_field[0x16]) & 0x0f) << 8) | u32(p_field[0x17]);

            if ((xing_flags & 0x01) && num_frames_xing > 0) {
                frame->total_samples =
                    (num_frames_xing * frame->samples_per_channel) - (enc_delay + padding);
            }

            frame->encoder_delay = frame->samples_per_channel + enc_delay + 529;
            frame->ofl_type = AjmDecMp3OflType::Lame;
        }
    } else if (p_current + 26 <= p_end && std::memcmp(p_current, "VBRI", 4) == 0) {
        u16 vbri_delay = 0;
        std::memcpy(&vbri_delay, p_current + 6, sizeof(vbri_delay));
        frame->encoder_delay = std::byteswap(vbri_delay);
        frame->ofl_type = AjmDecMp3OflType::Vbri;

        if (frame->frame_size <= stream_size) {
            const u8* next_frame = p_begin + frame->frame_size;
            const u32 remaining = stream_size - static_cast<u32>(frame->frame_size);
            AjmDecMp3ParseFrame next_info{};
            if (AjmMp3Decoder::ParseMp3Header(next_frame, remaining, 1, &next_info) == ORBIS_OK &&
                next_info.ofl_type == AjmDecMp3OflType::Fgh) {
                frame->encoder_delay += next_info.encoder_delay;
                frame->total_samples = next_info.total_samples;
                frame->ofl_type = AjmDecMp3OflType::VbriAndFgh;
            }
        }
    } else {
        const u8* p_fgh_end = p_begin + std::min<size_t>(stream_size, frame->frame_size);
        if (p_current + 10 <= p_fgh_end) {
            constexpr auto fgh_indicator = 0xB4;
            while ((p_current + 9) < p_fgh_end && *p_current != fgh_indicator) {
                ++p_current;
            }
            auto p_fgh = p_current;
            if ((p_current + 9) < p_fgh_end && *p_current == fgh_indicator) {
                u8 crc = 0xFF;
                auto crc_func = [](u8 c, u8 v, u8 s) {
                    if (((c >> 7) & 1) != ((v >> s) & 1)) {
                        return c * 2;
                    }
                    return (c * 2) ^ 0x45;
                };
                for (u8 i = 0; i < 9; ++i, ++p_current) {
                    for (u8 j = 0; j < 8; ++j) {
                        crc = crc_func(crc, *p_current, 7 - j);
                    }
                }
                if (p_fgh[9] == crc) {
                    u16 encoder_delay = 0;
                    u32 total_samples = 0;
                    std::memcpy(&encoder_delay, p_fgh + 1, sizeof(encoder_delay));
                    std::memcpy(&total_samples, p_fgh + 3, sizeof(total_samples));
                    frame->encoder_delay = std::byteswap(encoder_delay);
                    frame->total_samples = std::byteswap(total_samples);
                    frame->ofl_type = AjmDecMp3OflType::Fgh;
                }
            }
        }
    }
}

static int ParseMp3HeaderCommon(const u8* p_begin, u32 stream_size, int parse_ofl,
                                AjmDecMp3ParseFrame* frame, bool enforce_layer3) {
    if (p_begin == nullptr || stream_size < 4 || frame == nullptr) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }

    u32 raw_header = 0;
    std::memcpy(&raw_header, p_begin, sizeof(raw_header));
    auto bytes = std::byteswap(raw_header);
    auto header = reinterpret_cast<const Mp3Header*>(&bytes);
    if (header->sync != 0x7FF) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }

    if (enforce_layer3 && header->layer_type != 1) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }

    frame->sample_rate = Mp3SampleRateTable[u32(header->version)][header->sampling_rate_idx];
    if (frame->sample_rate == 0) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    frame->bitrate = Mp3BitRateTable[u32(header->version)][header->bitrate_idx] * 1000;
    if (frame->bitrate == 0) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    frame->num_channels = header->channel_mode == Mp3ChannelMode::SingleChannel ? 1 : 2;
    if (header->version == Mp3AudioVersion::V1) {
        frame->frame_size = (144 * frame->bitrate) / frame->sample_rate + header->padding;
        frame->samples_per_channel = 1152;
    } else {
        frame->frame_size = (72 * frame->bitrate) / frame->sample_rate + header->padding;
        frame->samples_per_channel = 576;
    }

    frame->encoder_delay = 0;
    frame->num_frames = 0;
    frame->total_samples = 0;
    frame->ofl_type = AjmDecMp3OflType::None;

    if (parse_ofl) {
        ParseMp3ExtendedHeader(p_begin, stream_size, header, frame);
    }

    return ORBIS_OK;
}

int AjmMp3Decoder::ParseMp3Header(const u8* p_begin, u32 stream_size, int parse_ofl,
                                  AjmDecMp3ParseFrame* frame) {
    LOG_TRACE(Lib_Ajm, "called stream_size = {} parse_ofl = {}", stream_size, parse_ofl);
    return ParseMp3HeaderCommon(p_begin, stream_size, parse_ofl, frame, false);
}

int AjmMp3Decoder::ParseMp3Frame(const u8* p_begin, u32 stream_size, int parse_ofl,
                                 AjmDecMp3ParseFrame* frame) {
    LOG_TRACE(Lib_Ajm, "called stream_size = {} parse_ofl = {}", stream_size, parse_ofl);
    return ParseMp3HeaderCommon(p_begin, stream_size, parse_ofl, frame, true);
}

AjmSidebandFormat AjmMp3Decoder::GetFormat() const {
    return AjmSidebandFormat{
        .num_channels = u32(m_codec_context->ch_layout.nb_channels),
        .channel_mask = GetChannelMask(u32(m_codec_context->ch_layout.nb_channels)),
        .sampl_freq = u32(m_codec_context->sample_rate),
        .sample_encoding = m_format,
        .bitrate = u32(m_codec_context->bit_rate),
        .reserved = 0,
    };
};

} // namespace Libraries::Ajm
