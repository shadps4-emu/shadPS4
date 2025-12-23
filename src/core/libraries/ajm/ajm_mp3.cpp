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

AjmMp3Decoder::AjmMp3Decoder(AjmFormatEncoding format, AjmMp3CodecFlags flags)
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
    m_frame_size = 0;
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
    return std::max<u32>(m_frame_size, 4);
}

DecoderResult AjmMp3Decoder::ProcessData(std::span<u8>& in_buf, SparseOutputBuffer& output,
                                         AjmInstanceGapless& gapless) {
    DecoderResult result{};
    AVPacket* pkt = av_packet_alloc();

    if ((!m_header.has_value() || m_frame_samples == 0) && in_buf.size() >= 4) {
        m_header = std::byteswap(*reinterpret_cast<u32*>(in_buf.data()));
        AjmDecMp3ParseFrame info{};
        ParseMp3Header(in_buf.data(), in_buf.size(), true, &info);
        m_frame_samples = info.samples_per_channel;
        m_frame_size = info.frame_size;
        gapless.init = {
            .total_samples = info.total_samples,
            .skip_samples = static_cast<u16>(info.encoder_delay),
            .skipped_samples = 0,
        };
        gapless.current = gapless.init;
    }

    int ret = av_parser_parse2(m_parser, m_codec_context, &pkt->data, &pkt->size, in_buf.data(),
                               in_buf.size(), AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    ASSERT_MSG(ret >= 0, "Error while parsing {}", ret);
    in_buf = in_buf.subspan(ret);

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
    BitReader(const u8* data) : m_data(data) {}

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

    size_t GetCurrentOffset() {
        return m_bit_offset;
    }

private:
    u8 GetBit() {
        const auto bit = (m_data[m_bit_offset / 8] >> (7 - (m_bit_offset % 8))) & 1;
        m_bit_offset += 1;
        return bit;
    }

    const u8* m_data;
    size_t m_bit_offset = 0;
};

int AjmMp3Decoder::ParseMp3Header(const u8* p_begin, u32 stream_size, int parse_ofl,
                                  AjmDecMp3ParseFrame* frame) {
    LOG_TRACE(Lib_Ajm, "called stream_size = {} parse_ofl = {}", stream_size, parse_ofl);

    if (p_begin == nullptr || stream_size < 4 || frame == nullptr) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }

    const auto* p_current = p_begin;

    auto bytes = std::byteswap(*reinterpret_cast<const u32*>(p_current));
    p_current += 4;
    auto header = reinterpret_cast<const Mp3Header*>(&bytes);
    if (header->sync != 0x7FF) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }

    frame->sample_rate = Mp3SampleRateTable[u32(header->version)][header->sampling_rate_idx];
    frame->bitrate = Mp3BitRateTable[u32(header->version)][header->bitrate_idx] * 1000;
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

    if (!parse_ofl) {
        return ORBIS_OK;
    }

    BitReader reader(p_current);
    if (header->protection_type == 0) {
        reader.Skip(16); // crc = reader.Read<u16>(16);
    }

    if (header->version == Mp3AudioVersion::V1) {
        // main_data_begin = reader.Read<u16>(9);
        // if (header->channel_mode == Mp3ChannelMode::SingleChannel) {
        //     private_bits = reader.Read<u8>(5);
        // } else {
        //     private_bits = reader.Read<u8>(3);
        // }
        // for (u32 ch = 0; ch < frame->num_channels; ++ch) {
        //     for (u8 band = 0; band < 4; ++band) {
        //         scfsi[ch][band] = reader.Read<bool>(1);
        //     }
        // }
        if (header->channel_mode == Mp3ChannelMode::SingleChannel) {
            reader.Skip(18);
        } else {
            reader.Skip(20);
        }
    } else {
        // main_data_begin = reader.Read<u16>(8);
        // if (header->channel_mode == Mp3ChannelMode::SingleChannel) {
        //     private_bits = reader.Read<u8>(1);
        // } else {
        //     private_bits = reader.Read<u8>(2);
        // }
        if (header->channel_mode == Mp3ChannelMode::SingleChannel) {
            reader.Skip(9);
        } else {
            reader.Skip(10);
        }
    }

    u32 part2_3_length = 0;
    // Number of granules (18x32 sub-band samples)
    const u8 ngr = header->version == Mp3AudioVersion::V1 ? 2 : 1;
    for (u8 gr = 0; gr < ngr; ++gr) {
        for (u32 ch = 0; ch < frame->num_channels; ++ch) {
            // part2_3_length[gr][ch] = reader.Read<u16>(12);
            part2_3_length += reader.Read<u16>(12);
            // big_values[gr][ch] = reader.Read<u16>(9);
            // global_main[gr][ch] = reader.Read<u8>(8);
            // if (header->version == Mp3AudioVersion::V1) {
            //     scalefac_compress[gr][ch] = reader.Read<u16>(4);
            // } else {
            //     scalefac_compress[gr][ch] = reader.Read<u16>(9);
            // }
            // window_switching_flag = reader.Read<bool>(1);
            // if (window_switching_flag) {
            //     block_type[gr][ch] = reader.Read<u8>(2);
            //     mixed_block_flag[gr][ch] = reader.Read<bool>(1);
            //     for (u8 region = 0; region < 2; ++region) {
            //         table_select[gr][ch][region] = reader.Read<u8>(5);
            //     }
            //     for (u8 window = 0; window < 3; ++window) {
            //         subblock_gain[gr][ch][window] = reader.Read<u8>(3);
            //     }
            // } else {
            //     for (u8 region = 0; region < 3; ++region) {
            //         table_select[gr][ch][region] = reader.Read<u8>(5);
            //     }
            //     region0_count[gr][ch] = reader.Read<u8>(4);
            //     region1_count[gr][ch] = reader.Read<u8>(3);
            // }
            // if (header->version == Mp3AudioVersion::V1) {
            //     preflag[gr][ch] = reader.Read<bool>(1);
            // }
            // scalefac_scale[gr][ch] = reader.Read<bool>(1);
            // count1table_select[gr][ch] = reader.Read<bool>(1);
            if (header->version == Mp3AudioVersion::V1) {
                reader.Skip(47);
            } else {
                reader.Skip(51);
            }
        }
    }
    reader.Skip(part2_3_length);

    p_current += ((reader.GetCurrentOffset() + 7) / 8);

    const auto* p_end = p_begin + frame->frame_size;
    if (memcmp(p_current, "Xing", 4) == 0 || memcmp(p_current, "Info", 4) == 0) {
        // TODO: Parse Xing/Lame header
        LOG_ERROR(Lib_Ajm, "Xing/Lame header is not implemented.");
    } else if (memcmp(p_current, "VBRI", 4) == 0) {
        // TODO: Parse VBRI header
        LOG_ERROR(Lib_Ajm, "VBRI header is not implemented.");
    } else {
        // Parse FGH header
        constexpr auto fgh_indicator = 0xB4;
        while ((p_current + 9) < p_end && *p_current != fgh_indicator) {
            ++p_current;
        }
        auto p_fgh = p_current;
        if ((p_current + 9) < p_end && *p_current == fgh_indicator) {
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
                frame->encoder_delay = std::byteswap(*reinterpret_cast<const u16*>(p_fgh + 1));
                frame->total_samples = std::byteswap(*reinterpret_cast<const u32*>(p_fgh + 3));
                frame->ofl_type = AjmDecMp3OflType::Fgh;
            } else {
                LOG_ERROR(Lib_Ajm, "FGH header CRC is incorrect.");
            }
        } else {
            LOG_ERROR(Lib_Ajm, "Could not find vendor header.");
        }
    }

    return ORBIS_OK;
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
