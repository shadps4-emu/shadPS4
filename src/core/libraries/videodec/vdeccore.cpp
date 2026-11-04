// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "vdeccore.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"
#include "video_utils.h"
#include "videodec_error.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Libraries::VdecCore {

struct SceVdecCoreFrameBufferInfo {
    u32 frame_buffer_size;
    u32 frame_buffer_alignment;
    u32 min_frame_buffer_count;
    u32 horizontal_alignment;
    u32 frame_width;
    u32 frame_height;
};
static_assert(sizeof(SceVdecCoreFrameBufferInfo) == 0x18);

struct SceVdecCoreFrameBuffer {
    void* frame_buffer;
    u64 frame_buffer_size;
};

struct SceVdecCoreInputData {
    void* au_data;
    u64 au_size;
    s64 pts;
    s64 dts;
    u64 user_data;
    u32 flags;
    u32 stream_id;
};

struct SceVdecCorePicture {
    u32 flags;
    u32 reserved0;
    u64 pts;
    u64 dts;
    u64 user_data;
    void* picture_info;
    void* frame_buffer;
    u32 frame_buffer_size;
    u32 frame_pitch;
    u64 reserved1;
};
static_assert(sizeof(SceVdecCorePicture) == 0x40);

struct SceVdecCoreHwPictureInfo {
    u32 codec_type;
    u32 reserved0;
    u64 user_data;
    u64 pts;
    u64 dts;
    u32 valid;
    u32 reserved1;
    u32 width;
    u32 height;
    u32 crop_left;
    u32 crop_right;
    u32 crop_top;
    u32 crop_bottom;
    u32 sar_width;
    u32 sar_height;
    u32 reserved2;
    u32 flags;
    u8 reserved3[0x400 - 0x50];
};
static_assert(sizeof(SceVdecCoreHwPictureInfo) == 0x400);

struct SceVdecCoreOutputInfo {
    u32 frame_width;
    u32 frame_pitch;
    u32 frame_height;
    u32 picture_count;
    void* frame_buffer;
    u32 frame_format;
    u32 reserved0;
    SceVdecCorePicture pictures[2];
    u32 aspect_ratio;
    u32 sar_width;
    u32 video_format;
    u32 colour_primaries;
    u32 transfer_characteristics;
    u32 frame_pitch_in_bytes;
    void* luma_buffer;
    void* chroma_buffer;
    u32 video_full_range_flag;
    u32 colour_description_present_flag;
};
static_assert(sizeof(SceVdecCoreOutputInfo) == 0xd0);

struct SceVdecCoreMemoryBlock {
    void* addr;
    u64 size;
    u32 align;
    u32 type;
};

struct VdecCoreCommand {
    enum class Type {
        Packet,
        Reset,
        Flush,
    };
    Type type = Type::Packet;
    AVPacket* packet = nullptr;
    std::shared_ptr<std::atomic<bool>> sync_done;
};

struct VdecCoreDecoder {
    AVCodecContext* codec_context = nullptr;
    SwsContext* sws_context = nullptr;
    AVFrame* decoded_frame = nullptr;

    AVCodecID codec_id = AV_CODEC_ID_NONE;
    u32 codec_type = 0;
    u32 max_width = 1920;
    u32 max_height = 1080;
    u32 max_dpb = 16;
    u32 frame_buffer_size = 0;

    std::vector<SceVdecCoreMemoryBlock> mapped_blocks;
    std::vector<void*> frame_buffer_slots;
    u32 current_slot_index = 0;

    std::vector<u8> fallback_frame_buffer;
    s64 last_pts = ORBIS_VDEC_CORE_PTS_INVALID;
    s64 last_dts = ORBIS_VDEC_CORE_PTS_INVALID;
    u64 last_user_data = 0;

    mutable std::mutex mutex;
    std::condition_variable cv_input;
    std::condition_variable cv_output;
    std::deque<VdecCoreCommand> command_queue;
    std::deque<AVFrame*> frame_queue;
    std::jthread worker_thread;
    bool is_busy = false;
};

static void DrainDecodedFrames(VdecCoreDecoder* decoder) {
    if (!decoder || !decoder->codec_context) {
        return;
    }
    while (true) {
        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            break;
        }
        int ret = avcodec_receive_frame(decoder->codec_context, frame);
        if (ret == 0) {
            std::lock_guard lock(decoder->mutex);
            decoder->frame_queue.push_back(frame);
            LOG_DEBUG(Lib_Videodec, "VdecCore: frame decoded, queue_size={}, frame={}x{}, pts={}",
                      decoder->frame_queue.size(), frame->width, frame->height, frame->pts);
        } else {
            if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                LOG_WARNING(Lib_Videodec, "VdecCore: avcodec_receive_frame ret={}", ret);
            }
            av_frame_free(&frame);
            break;
        }
    }
}

static void DecoderWorkerLoop(std::stop_token stop_token, VdecCoreDecoder* dec) {
    while (!stop_token.stop_requested()) {
        VdecCoreCommand cmd{};
        {
            std::unique_lock lock(dec->mutex);
            dec->cv_input.wait(
                lock, [&]() { return stop_token.stop_requested() || !dec->command_queue.empty(); });
            if (stop_token.stop_requested()) {
                break;
            }
            cmd = std::move(dec->command_queue.front());
            dec->command_queue.pop_front();
            dec->is_busy = true;
        }

        switch (cmd.type) {
        case VdecCoreCommand::Type::Packet: {
            if (cmd.packet) {
                int ret = avcodec_send_packet(dec->codec_context, cmd.packet);
                if (ret == AVERROR_EOF) {
                    avcodec_flush_buffers(dec->codec_context);
                    ret = avcodec_send_packet(dec->codec_context, cmd.packet);
                }
                if (ret == AVERROR(EAGAIN)) {
                    DrainDecodedFrames(dec);
                    ret = avcodec_send_packet(dec->codec_context, cmd.packet);
                }
                av_packet_free(&cmd.packet);
                if (ret < 0 && ret != AVERROR(EAGAIN)) {
                    LOG_WARNING(Lib_Videodec, "VdecCore: avcodec_send_packet warning: {}", ret);
                }
                DrainDecodedFrames(dec);
            }
            break;
        }
        case VdecCoreCommand::Type::Flush: {
            if (dec->codec_context) {
                avcodec_send_packet(dec->codec_context, nullptr);
                DrainDecodedFrames(dec);
                avcodec_flush_buffers(dec->codec_context);
            }
            if (cmd.sync_done) {
                cmd.sync_done->store(true);
            }
            break;
        }
        case VdecCoreCommand::Type::Reset: {
            if (dec->codec_context) {
                avcodec_flush_buffers(dec->codec_context);
            }
            {
                std::lock_guard lock(dec->mutex);
                for (auto* f : dec->frame_queue) {
                    av_frame_free(&f);
                }
                dec->frame_queue.clear();
                dec->current_slot_index = 0;
            }
            if (cmd.sync_done) {
                cmd.sync_done->store(true);
            }
            break;
        }
        }

        {
            std::lock_guard lock(dec->mutex);
            dec->is_busy = !dec->command_queue.empty();
            dec->cv_output.notify_all();
        }
    }
}

static AVFrame* ConvertNV12Frame(VdecCoreDecoder* dec, AVFrame& frame) {
    AVFrame* nv12_frame = av_frame_alloc();
    nv12_frame->format = AV_PIX_FMT_NV12;
    nv12_frame->width = frame.width;
    nv12_frame->height = frame.height;
    nv12_frame->pts = frame.pts;
    nv12_frame->pkt_dts = frame.pkt_dts;
    nv12_frame->opaque = frame.opaque;
    av_frame_get_buffer(nv12_frame, 0);

    dec->sws_context = sws_getCachedContext(
        dec->sws_context, frame.width, frame.height, static_cast<AVPixelFormat>(frame.format),
        frame.width, frame.height, AV_PIX_FMT_NV12, SWS_BILINEAR, nullptr, nullptr, nullptr);
    sws_scale(dec->sws_context, frame.data, frame.linesize, 0, frame.height, nv12_frame->data,
              nv12_frame->linesize);
    return nv12_frame;
}

static s32 PS4_SYSV_ABI sceVdecCoreQueryInstanceSize(const OrbisVdecCoreConfigInfo* config,
                                                     u64* instance_size, u64* compute_size,
                                                     u64* reserved_size) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreQueryInstanceSize called");
    if (!config) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    if (static_cast<u32>(config->codec_type) > 4) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_CODEC_TYPE;
    }
    if (instance_size) {
        *instance_size = 0x40000;
    }
    if (compute_size) {
        *compute_size = 0x10000;
    }
    if (reserved_size) {
        *reserved_size = 0;
    }
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreQueryFrameBufferInfo(const OrbisVdecCoreConfigInfo* config,
                                                        SceVdecCoreFrameBufferInfo* fb_info) {
    if (!fb_info || !config) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    if (static_cast<u32>(config->codec_type) > 4) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_CODEC_TYPE;
    }
    if (config->max_dpb_frame_count < -1 || config->max_dpb_frame_count > 16) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    const u32 width = config->max_frame_width ? static_cast<u32>(config->max_frame_width) : 1920;
    const u32 height = config->max_frame_height ? static_cast<u32>(config->max_frame_height) : 1080;
    const u32 dpb = (config->max_dpb_frame_count > 0 && config->max_dpb_frame_count <= 16)
                        ? static_cast<u32>(config->max_dpb_frame_count)
                        : 16;
    const u32 horizontal_align =
        config->horizontal_align > 0 ? static_cast<u32>(config->horizontal_align) : 256;

    const u32 pitch = Common::AlignUp<u32>(width, horizontal_align);
    const u32 aligned_height = Common::AlignUp<u32>(height, 16);
    const u32 size = (pitch * aligned_height * 3) / 2;

    fb_info->frame_buffer_size = Common::AlignUp<u32>(size + 0x1000, 0x10000);
    fb_info->frame_buffer_alignment = 256;
    fb_info->min_frame_buffer_count = dpb + 1;
    fb_info->horizontal_alignment = horizontal_align;
    fb_info->frame_width = width;
    fb_info->frame_height = height;

    LOG_INFO(Lib_Videodec,
             "sceVdecCoreQueryFrameBufferInfo: fb_size={:#x}, align={}, min_count={}, "
             "horiz_align={}, w={}, h={}",
             fb_info->frame_buffer_size, fb_info->frame_buffer_alignment,
             fb_info->min_frame_buffer_count, fb_info->horizontal_alignment, fb_info->frame_width,
             fb_info->frame_height);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreCreateDecoder(const OrbisVdecCoreConfigInfo* config,
                                                 const OrbisVdecCoreMemoryInfo* mem_info,
                                                 SceVdecCoreComputeQueue compute_queue,
                                                 VdecCoreDecoder** out_decoder) {
    LOG_INFO(Lib_Videodec, "sceVdecCoreCreateDecoder called");
    if (!config || !out_decoder) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    if (config->decoder_type > OrbisVdecCoreDecoderType::Vdec2Special2) {
        LOG_ERROR(Lib_Videodec, "VdecCore: invalid decoder_type={}",
                  static_cast<u32>(config->decoder_type));
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    if (static_cast<u32>(config->codec_type) > 4) {
        LOG_ERROR(Lib_Videodec, "VdecCore: unsupported codec_type={}",
                  static_cast<u32>(config->codec_type));
        return ORBIS_VDEC_CORE_ERROR_INVALID_CODEC_TYPE;
    }

    const u32 width = config->max_frame_width ? static_cast<u32>(config->max_frame_width) : 1920;
    const u32 height = config->max_frame_height ? static_cast<u32>(config->max_frame_height) : 1080;
    const u32 dpb = (config->max_dpb_frame_count > 0 && config->max_dpb_frame_count <= 16)
                        ? static_cast<u32>(config->max_dpb_frame_count)
                        : 16;
    const u32 codec_type = static_cast<u32>(config->codec_type);

    LOG_INFO(Lib_Videodec,
             "VdecCore: config decoder_type={}, codec_type={}, width={}, height={}, dpb={}",
             static_cast<u32>(config->decoder_type), codec_type, width, height, dpb);

    auto* dec = new VdecCoreDecoder();
    dec->codec_type = codec_type;
    dec->max_width = width;
    dec->max_height = height;
    dec->max_dpb = dpb;

    const AVCodecID codec_id =
        (config->codec_type == OrbisVdecCoreCodecType::Hevc) ? AV_CODEC_ID_HEVC : AV_CODEC_ID_H264;
    dec->codec_id = codec_id;
    LOG_INFO(Lib_Videodec, "VdecCore: creating decoder codec_type={}, ffmpeg_codec_id={}",
             codec_type, static_cast<int>(codec_id));
    const AVCodec* codec = avcodec_find_decoder(codec_id);
    ASSERT(codec);

    dec->codec_context = avcodec_alloc_context3(codec);
    ASSERT(dec->codec_context);
    dec->codec_context->width = dec->max_width;
    dec->codec_context->height = dec->max_height;
    dec->codec_context->flags |= AV_CODEC_FLAG_COPY_OPAQUE;

    int ret = avcodec_open2(dec->codec_context, codec, nullptr);
    if (ret < 0) {
        LOG_ERROR(Lib_Videodec, "VdecCore: avcodec_open2 failed: {}", ret);
    }

    dec->decoded_frame = av_frame_alloc();

    const u32 horizontal_align =
        config->horizontal_align > 0 ? static_cast<u32>(config->horizontal_align) : 256;
    u32 pitch = Common::AlignUp<u32>(dec->max_width, horizontal_align);
    u32 aligned_height = Common::AlignUp<u32>(dec->max_height, 16);
    dec->frame_buffer_size =
        Common::AlignUp<u32>((pitch * aligned_height * 3) / 2 + 0x1000, 0x10000);

    dec->worker_thread = std::jthread(DecoderWorkerLoop, dec);

    *out_decoder = dec;
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreDeleteDecoder(VdecCoreDecoder* decoder) {
    LOG_INFO(Lib_Videodec, "sceVdecCoreDeleteDecoder called");
    if (!decoder) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    decoder->worker_thread.request_stop();
    decoder->cv_input.notify_all();
    if (decoder->worker_thread.joinable()) {
        decoder->worker_thread.join();
    }
    for (auto& cmd : decoder->command_queue) {
        if (cmd.packet) {
            av_packet_free(&cmd.packet);
        }
    }
    decoder->command_queue.clear();
    for (auto* f : decoder->frame_queue) {
        av_frame_free(&f);
    }
    decoder->frame_queue.clear();
    if (decoder->decoded_frame) {
        av_frame_free(&decoder->decoded_frame);
    }
    if (decoder->codec_context) {
        avcodec_free_context(&decoder->codec_context);
    }
    if (decoder->sws_context) {
        sws_freeContext(decoder->sws_context);
    }
    delete decoder;
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreResetDecoder(VdecCoreDecoder* decoder, void* reset_param) {
    LOG_INFO(Lib_Videodec, "sceVdecCoreResetDecoder called");
    if (!decoder) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    auto sync_done = std::make_shared<std::atomic<bool>>(false);
    {
        std::unique_lock lock(decoder->mutex);
        decoder->command_queue.push_back({VdecCoreCommand::Type::Reset, nullptr, sync_done});
        decoder->cv_input.notify_one();
        decoder->cv_output.wait(lock, [&]() { return sync_done->load(); });
    }
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreSetDecodeInput(VdecCoreDecoder* decoder,
                                                  const SceVdecCoreInputData* input_data,
                                                  void* reserved, u32* out_status) {
    if (!decoder || !input_data) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    if (out_status) {
        out_status[0] = 1;
        out_status[1] = 1;
    }
    if (!input_data->au_data || input_data->au_size == 0) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }

    LOG_TRACE(Lib_Videodec, "VdecCore: SetDecodeInput size={}, pts={}, dts={}, user_data={:#x}",
              input_data->au_size, input_data->pts, input_data->dts, input_data->user_data);

    const s64 pts =
        (input_data->pts == ORBIS_VDEC_CORE_PTS_INVALID) ? AV_NOPTS_VALUE : input_data->pts;
    const s64 dts =
        (input_data->dts == ORBIS_VDEC_CORE_PTS_INVALID) ? AV_NOPTS_VALUE : input_data->dts;

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        return ORBIS_VDEC_CORE_ERROR_FATAL;
    }
    if (av_new_packet(packet, static_cast<int>(input_data->au_size)) < 0) {
        av_packet_free(&packet);
        return ORBIS_VDEC_CORE_ERROR_FATAL;
    }
    std::memcpy(packet->data, input_data->au_data, input_data->au_size);
    packet->pts = pts;
    packet->dts = dts;
    packet->opaque = reinterpret_cast<void*>(input_data->user_data);

    {
        std::lock_guard lock(decoder->mutex);
        decoder->last_pts = input_data->pts;
        decoder->last_dts = input_data->dts;
        decoder->last_user_data = input_data->user_data;
        decoder->command_queue.push_back({VdecCoreCommand::Type::Packet, packet, nullptr});
        decoder->is_busy = true;
    }
    decoder->cv_input.notify_one();

    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreSyncDecode(VdecCoreDecoder* decoder, u32* out_count) {
    if (!decoder) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    std::unique_lock lock(decoder->mutex);
    decoder->cv_output.wait(lock, [&]() {
        return !decoder->frame_queue.empty() ||
               (!decoder->is_busy && decoder->command_queue.empty());
    });
    if (out_count) {
        *out_count = static_cast<u32>(decoder->frame_queue.size());
    }
    LOG_DEBUG(Lib_Videodec, "VdecCore: sceVdecCoreSyncDecode returning out_count={}",
              out_count ? *out_count : 0);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreGetDecodeOutput(VdecCoreDecoder* decoder,
                                                   SceVdecCoreFrameBuffer* frame_buf, u32 flags,
                                                   SceVdecCoreOutputInfo* output_info,
                                                   void* reserved) {
    if (!decoder || !output_info) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }

    AVFrame* frame = nullptr;
    {
        std::lock_guard lock(decoder->mutex);
        if (decoder->frame_queue.empty()) {
            output_info->frame_width = decoder->max_width;
            output_info->frame_pitch = Common::AlignUp<u32>(decoder->max_width, 64);
            output_info->frame_height = decoder->max_height;
            output_info->picture_count = 0;
            output_info->frame_buffer = nullptr;
            output_info->frame_format = 0;
            output_info->reserved0 = 0;
            output_info->pictures[0].frame_buffer = nullptr;
            output_info->pictures[1].frame_buffer = nullptr;
            output_info->luma_buffer = nullptr;
            output_info->chroma_buffer = nullptr;
            return ORBIS_VDEC_CORE_ERROR_NO_PICTURE;
        }

        frame = decoder->frame_queue.front();
        decoder->frame_queue.pop_front();
    }

    if (frame->format != AV_PIX_FMT_NV12) {
        AVFrame* converted = ConvertNV12Frame(decoder, *frame);
        av_frame_free(&frame);
        frame = converted;
    }

    const u32 width = frame->width;
    const u32 height = frame->height;
    const u32 aligned_width = Common::AlignUp<u32>(width, 16);
    const u32 pitch = Common::AlignUp<u32>(width, 64);
    const u32 aligned_height = Common::AlignUp<u32>(height, 16);
    const u32 fb_size = (pitch * aligned_height * 3) / 2;
    const u32 sar_width = frame->sample_aspect_ratio.num > 0 ? frame->sample_aspect_ratio.num : 1;
    const u32 sar_height = frame->sample_aspect_ratio.den > 0 ? frame->sample_aspect_ratio.den : 1;
    const u64 allocated_fb_size = (frame_buf && frame_buf->frame_buffer_size > 0)
                                      ? frame_buf->frame_buffer_size
                                      : decoder->frame_buffer_size;

    void* dest_buf = nullptr;
    if (frame_buf && frame_buf->frame_buffer) {
        dest_buf = frame_buf->frame_buffer;
    } else if (!decoder->frame_buffer_slots.empty()) {
        dest_buf = decoder->frame_buffer_slots[decoder->current_slot_index %
                                               decoder->frame_buffer_slots.size()];
        decoder->current_slot_index++;
    } else {
        const u64 fallback_size = std::max<u64>(fb_size, allocated_fb_size);
        if (decoder->fallback_frame_buffer.size() < fallback_size) {
            decoder->fallback_frame_buffer.resize(fallback_size);
        }
        dest_buf = decoder->fallback_frame_buffer.data();
    }

    Videodec::CopyNV12Data(static_cast<u8*>(dest_buf), fb_size, *frame);

    if (allocated_fb_size >= sizeof(SceVdecCoreHwPictureInfo)) {
        auto* meta = reinterpret_cast<SceVdecCoreHwPictureInfo*>(
            static_cast<u8*>(dest_buf) + allocated_fb_size - sizeof(SceVdecCoreHwPictureInfo));
        std::memset(meta, 0, sizeof(*meta));
        meta->codec_type = (decoder->codec_id == AV_CODEC_ID_HEVC) ? 1 : 0;
        meta->user_data = reinterpret_cast<u64>(frame->opaque);
        meta->pts = (frame->pts != AV_NOPTS_VALUE) ? frame->pts : decoder->last_pts;
        meta->dts = (frame->pkt_dts != AV_NOPTS_VALUE) ? frame->pkt_dts : decoder->last_dts;
        meta->valid = 1;
        meta->width = aligned_width;
        meta->height = aligned_height;
        meta->crop_left = 0;
        meta->crop_right = (pitch > width) ? ((pitch - width) / 2) : 0;
        meta->crop_top = 0;
        meta->crop_bottom = (aligned_height > height) ? ((aligned_height - height) / 2) : 0;
        meta->sar_width = sar_width;
        meta->sar_height = sar_height;
        meta->flags = 0x01;
        if (meta->crop_right > 0 || meta->crop_bottom > 0) {
            meta->flags |= 0x02;
        }
    }

    output_info->frame_width = aligned_width;
    output_info->frame_pitch = pitch;
    output_info->frame_height = aligned_height;
    output_info->picture_count = 1;
    output_info->frame_buffer = dest_buf;
    output_info->frame_format = 0; // NV12
    output_info->reserved0 = 0;
    output_info->pictures[0].flags = static_cast<u32>(decoder->codec_type);
    output_info->pictures[0].reserved0 = 0;
    output_info->pictures[0].pts = (frame->pts != AV_NOPTS_VALUE) ? frame->pts : decoder->last_pts;
    output_info->pictures[0].dts =
        (frame->pkt_dts != AV_NOPTS_VALUE) ? frame->pkt_dts : decoder->last_dts;
    output_info->pictures[0].user_data = reinterpret_cast<u64>(frame->opaque);
    output_info->pictures[0].picture_info = nullptr;
    output_info->pictures[0].frame_buffer = dest_buf;
    output_info->pictures[0].frame_buffer_size = fb_size;
    output_info->pictures[0].frame_pitch = pitch;
    output_info->pictures[0].reserved1 = 0;
    output_info->pictures[1].frame_buffer = nullptr;

    output_info->aspect_ratio = 1;
    output_info->sar_width = sar_width;
    output_info->video_format = 5;
    output_info->colour_primaries = 1;
    output_info->transfer_characteristics = 1;
    output_info->frame_pitch_in_bytes = pitch;
    output_info->luma_buffer = dest_buf;
    output_info->chroma_buffer = static_cast<u8*>(dest_buf) + pitch * aligned_height;
    output_info->video_full_range_flag = (frame->color_range == AVCOL_RANGE_JPEG) ? 1 : 0;
    output_info->colour_description_present_flag = 1;

    av_frame_free(&frame);

    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreFlushDecodeOutput(VdecCoreDecoder* decoder, u32* out_count,
                                                     void* reserved) {
    LOG_INFO(Lib_Videodec, "sceVdecCoreFlushDecodeOutput called");
    if (!decoder) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    auto sync_done = std::make_shared<std::atomic<bool>>(false);
    {
        std::unique_lock lock(decoder->mutex);
        decoder->command_queue.push_back({VdecCoreCommand::Type::Flush, nullptr, sync_done});
        decoder->cv_input.notify_one();
        decoder->cv_output.wait(lock, [&]() { return sync_done->load(std::memory_order_acquire); });
        if (out_count) {
            *out_count = static_cast<u32>(decoder->frame_queue.size());
        }
    }
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreMapMemoryBlock(VdecCoreDecoder* decoder,
                                                  const SceVdecCoreMemoryBlock* blocks, u32 count) {
    LOG_INFO(Lib_Videodec, "sceVdecCoreMapMemoryBlock called with count={}", count);
    if (!decoder) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    std::lock_guard lock(decoder->mutex);
    if (blocks && count > 0) {
        for (u32 i = 0; i < count; ++i) {
            decoder->mapped_blocks.push_back(blocks[i]);
            u8* base = reinterpret_cast<u8*>(blocks[i].addr);
            u64 size = blocks[i].size;
            LOG_INFO(Lib_Videodec, "VdecCore: mapped block[{}] addr={:p}, size={:#x}", i,
                     static_cast<void*>(base), size);
            if (base && size >= decoder->frame_buffer_size) {
                const u32 num_slots = static_cast<u32>(size / decoder->frame_buffer_size);
                for (u32 s = 0; s < num_slots; ++s) {
                    void* slot_addr = base + s * decoder->frame_buffer_size;
                    decoder->frame_buffer_slots.push_back(slot_addr);
                    LOG_DEBUG(Lib_Videodec, "VdecCore: created frame buffer slot[{}] at {:p}", s,
                              slot_addr);
                }
            }
        }
    }
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreAnalyzeStream(void* reserved) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreAnalyzeStream stub called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI
sceVdecCoreQueryComputeResourceInfo(OrbisVdecCoreComputeResourceInfo* info) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreQueryComputeResourceInfo called");
    if (!info) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    info->memory_type = 0;
    info->size = 0x10000;
    info->alignment = 0x10000;
    info->reserved = 0;
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreInitializeComputeResource(
    const OrbisVdecCoreComputeResourceInfo* res, SceVdecCoreComputeQueue compute_queue) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreInitializeComputeResource called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreFinalizeComputeResource(void* res) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreFinalizeComputeResource stub called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreSetDecodeOutputSw(void* decoder, void* out) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreSetDecodeOutputSw stub called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreSyncDecodeOutputSw(void* decoder, void* out) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreSyncDecodeOutputSw stub called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreTrySyncDecodeOutputSw(void* decoder, void* out) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreTrySyncDecodeOutputSw stub called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreSyncDecodeWptr(VdecCoreDecoder* decoder, u32* out_count,
                                                  void* out_wptr, void* out_rptr) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreSyncDecodeWptr called");
    return sceVdecCoreSyncDecode(decoder, out_count);
}

static s32 PS4_SYSV_ABI sceVdecCoreTrySyncDecode(VdecCoreDecoder* decoder, u32* out_count) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreTrySyncDecode called");
    if (!decoder) {
        return ORBIS_VDEC_CORE_ERROR_INVALID_ARGUMENT;
    }
    std::lock_guard lock(decoder->mutex);
    if (!decoder->frame_queue.empty()) {
        if (out_count) {
            *out_count = static_cast<u32>(decoder->frame_queue.size());
        }
        return ORBIS_OK;
    }
    if (decoder->is_busy || !decoder->command_queue.empty()) {
        if (out_count) {
            *out_count = 0;
        }
        return ORBIS_VDEC_CORE_ERROR_DECODE_RUNNING;
    }
    if (out_count) {
        *out_count = 0;
    }
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceVdecCoreTrySyncDecodeWptr(VdecCoreDecoder* decoder, u32* out_count,
                                                     void* out_wptr, void* out_rptr) {
    LOG_DEBUG(Lib_Videodec, "sceVdecCoreTrySyncDecodeWptr called");
    return sceVdecCoreTrySyncDecode(decoder, out_count);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("Zsh1K8YTD1E", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreQueryInstanceSize);
    LIB_FUNCTION("UxV5adxfBFg", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreQueryFrameBufferInfo);
    LIB_FUNCTION("wnWMkCYv0m4", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreCreateDecoder);
    LIB_FUNCTION("D0RWslNmK9s", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreDeleteDecoder);
    LIB_FUNCTION("nFn-3CWQEyo", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreResetDecoder);
    LIB_FUNCTION("F4nrEfi-M84", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreSetDecodeInput);
    LIB_FUNCTION("OyxRC7GT4Es", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreSyncDecode);
    LIB_FUNCTION("K4sH-0WUfuc", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreGetDecodeOutput);
    LIB_FUNCTION("b0xbD0x+02M", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreFlushDecodeOutput);
    LIB_FUNCTION("7YW9rloMLYo", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreMapMemoryBlock);

    LIB_FUNCTION("bWPuW6TnMjk", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreAnalyzeStream);
    LIB_FUNCTION("zwcpjJ7WUgQ", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreQueryComputeResourceInfo);
    LIB_FUNCTION("HPpz-oyX4vM", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreInitializeComputeResource);
    LIB_FUNCTION("DHQXNKiY-Dw", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreFinalizeComputeResource);
    LIB_FUNCTION("hF8Pz0rtPLU", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreSetDecodeOutputSw);
    LIB_FUNCTION("1hE5Sdn4U3U", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreSyncDecodeOutputSw);
    LIB_FUNCTION("WMhw0IVNNC8", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreTrySyncDecodeOutputSw);
    LIB_FUNCTION("JFcSDC6AD+k", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreSyncDecodeWptr);
    LIB_FUNCTION("iQRVLJQ+OYs", "libSceVdecCore", 1, "libSceVdecCore", sceVdecCoreTrySyncDecode);
    LIB_FUNCTION("T9yCBYaGDew", "libSceVdecCore", 1, "libSceVdecCore",
                 sceVdecCoreTrySyncDecodeWptr);
}

} // namespace Libraries::VdecCore
