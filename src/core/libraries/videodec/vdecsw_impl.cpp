// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "vdecsw_impl.h"

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/thread.h"
#include "video_utils.h"
#include "videodec_error.h"

#include "common/support/avdec.h"

namespace Libraries::Vdecsw {

VdecDecoder::VdecDecoder(const OrbisVdecswDecoderConfigInfo& config_info,
                         const OrbisVdecswDecoderMemoryInfo& memory_info)
    : m_is_avc(config_info.codec_type == OrbisVdecswCodecType::Avc) {
    const AVCodec* codec =
        avcodec_find_decoder(m_is_avc ? AV_CODEC_ID_H264 : AV_CODEC_ID_HEVC);
    ASSERT(codec);

    m_codec_context = avcodec_alloc_context3(codec);
    ASSERT(m_codec_context);
    m_codec_context->width = config_info.max_frame_width;
    m_codec_context->height = config_info.max_frame_height;
    m_codec_context->flags |= AV_CODEC_FLAG_COPY_OPAQUE;

    avcodec_open2(m_codec_context, codec, nullptr);

    m_worker_thread = std::jthread(
        [this](std::stop_token stop_token) { WorkerLoop(std::move(stop_token)); });
}

VdecDecoder::~VdecDecoder() {
    m_worker_thread.request_stop();
    m_input_cv.notify_all();
    m_output_cv.notify_all();
    m_worker_thread.join();

    ClearFrameQueue();
    avcodec_free_context(&m_codec_context);
    sws_freeContext(m_sws_context);
}

s32 VdecDecoder::SetDecodeInput(const OrbisVdecswInputData& input_data) {
    if (!input_data.au_data) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_ACCESS_UNIT_POINTER");
        return ORBIS_VDECSW_ERROR_ACCESS_UNIT_POINTER;
    }
    if (input_data.au_size == 0) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_ACCESS_UNIT_SIZE");
        return ORBIS_VDECSW_ERROR_ACCESS_UNIT_SIZE;
    }

    // The access unit data is copied because the game is allowed to reuse the
    // buffer as soon as this call returns.
    QueueItem item{};
    item.type = QueueItem::Type::Input;
    item.au_data.assign((const u8*)input_data.au_data,
                        (const u8*)input_data.au_data + input_data.au_size);
    item.original_au_data = input_data.au_data;
    item.pts_data = input_data.pts_data;
    item.dts_data = input_data.dts_data;
    item.attached_data = input_data.attached_data;

    {
        std::scoped_lock lock{m_mutex};
        m_queue.push_back(std::move(item));
        ++m_unsynced_inputs;
    }
    m_input_cv.notify_one();
    return ORBIS_OK;
}

s32 VdecDecoder::SyncDecodeInput(OrbisVdecswInputResult& input_result) {
    std::unique_lock lock{m_mutex};
    m_input_cv.wait(lock, [&] {
        return !m_completed_inputs.empty() || m_unsynced_inputs == 0 ||
               m_worker_thread.get_stop_token().stop_requested();
    });

    if (m_completed_inputs.empty()) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_INPUT_QUEUE_EMPTY");
        return ORBIS_VDECSW_ERROR_INPUT_QUEUE_EMPTY;
    }

    const CompletedInput entry = std::move(m_completed_inputs.front());
    m_completed_inputs.pop_front();
    --m_unsynced_inputs;

    input_result.decoded_au = entry.au_data;
    input_result.output_frame_count = entry.output_frame_count;
    input_result.reserved0 = 0;
    return entry.result;
}

s32 VdecDecoder::TrySyncDecodeInput(OrbisVdecswInputResult& input_result) {
    std::scoped_lock lock{m_mutex};
    if (m_completed_inputs.empty()) {
        if (m_unsynced_inputs == 0) {
            LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_INPUT_QUEUE_EMPTY");
            return ORBIS_VDECSW_ERROR_INPUT_QUEUE_EMPTY;
        }
        LOG_TRACE(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_DECODE_PENDING");
        return ORBIS_VDECSW_ERROR_DECODE_PENDING;
    }

    const CompletedInput entry = std::move(m_completed_inputs.front());
    m_completed_inputs.pop_front();
    --m_unsynced_inputs;

    input_result.decoded_au = entry.au_data;
    input_result.output_frame_count = entry.output_frame_count;
    input_result.reserved0 = 0;
    return entry.result;
}

s32 VdecDecoder::SetDecodeOutput(const OrbisVdecswFrameBuffer& frame_buffer) {
    if (!frame_buffer.frame_buffer) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_FRAME_BUFFER_POINTER");
        return ORBIS_VDECSW_ERROR_FRAME_BUFFER_POINTER;
    }
    if (frame_buffer.frame_buffer_size == 0) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_FRAME_BUFFER_SIZE");
        return ORBIS_VDECSW_ERROR_FRAME_BUFFER_SIZE;
    }

    std::scoped_lock lock{m_mutex};
    if (m_pending_output.has_value()) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_FULL");
        return ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_FULL;
    }

    m_pending_output = frame_buffer;
    return ORBIS_OK;
}

s32 VdecDecoder::SyncDecodeOutput(OrbisVdecswOutputInfo& output_info) {
    output_info.is_valid = false;
    output_info.is_last_frame = false;
    output_info.is_error_frame = false;
    output_info.is_discarded_frame = false;
    output_info.picture_count = 0;

    std::unique_lock lock{m_mutex};
    if (!m_pending_output.has_value()) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_EMPTY");
        return ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_EMPTY;
    }

    const OrbisVdecswFrameBuffer frame_buffer = *m_pending_output;
    m_pending_output.reset();

    m_output_cv.wait(lock, [&] {
        return !m_frame_queue.empty() || m_finalized ||
               m_worker_thread.get_stop_token().stop_requested();
    });

    if (m_frame_queue.empty()) {
        // The sequence was finalized and all frames have been returned.
        output_info.is_last_frame = m_finalized;
        return ORBIS_OK;
    }

    AVFrame* frame = m_frame_queue.front();
    m_frame_queue.pop_front();
    const bool is_last_frame = m_finalized && m_frame_queue.empty();
    lock.unlock();

    const s32 result = WriteFrame(output_info, *frame, frame_buffer, is_last_frame);
    av_frame_free(&frame);
    return result;
}

s32 VdecDecoder::TrySyncDecodeOutput(OrbisVdecswOutputInfo& output_info) {
    output_info.is_valid = false;
    output_info.is_last_frame = false;
    output_info.is_error_frame = false;
    output_info.is_discarded_frame = false;
    output_info.picture_count = 0;

    std::unique_lock lock{m_mutex};
    if (!m_pending_output.has_value()) {
        LOG_ERROR(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_EMPTY");
        return ORBIS_VDECSW_ERROR_OUTPUT_BUFFER_EMPTY;
    }

    if (m_frame_queue.empty()) {
        if (m_finalized) {
            output_info.is_last_frame = true;
            m_pending_output.reset();
            return ORBIS_OK;
        }
        LOG_TRACE(Lib_Vdecsw, "ORBIS_VDECSW_ERROR_DECODE_PENDING");
        return ORBIS_VDECSW_ERROR_DECODE_PENDING;
    }

    AVFrame* frame = m_frame_queue.front();
    m_frame_queue.pop_front();
    const bool is_last_frame = m_finalized && m_frame_queue.empty();
    const OrbisVdecswFrameBuffer frame_buffer = *m_pending_output;
    m_pending_output.reset();
    lock.unlock();

    const s32 result = WriteFrame(output_info, *frame, frame_buffer, is_last_frame);
    av_frame_free(&frame);
    return result;
}

s32 VdecDecoder::FinalizeDecodeSequence() {
    QueueItem item{};
    item.type = QueueItem::Type::Flush;
    {
        std::scoped_lock lock{m_mutex};
        m_queue.push_back(std::move(item));
    }
    m_input_cv.notify_one();
    return ORBIS_OK;
}

s32 VdecDecoder::Reset() {
    QueueItem item{};
    item.type = QueueItem::Type::Reset;
    {
        std::scoped_lock lock{m_mutex};
        m_pending_output.reset();
        m_queue.push_back(std::move(item));
    }
    m_input_cv.notify_one();
    return ORBIS_OK;
}

void VdecDecoder::WorkerLoop(std::stop_token stop_token) {
    Common::SetCurrentThreadName("shadPS4:VdecswWorker");

    std::unique_lock lock{m_mutex};
    while (!stop_token.stop_requested()) {
        m_input_cv.wait(lock, stop_token, [&] { return !m_queue.empty(); });
        if (stop_token.stop_requested() && m_queue.empty()) {
            break;
        }

        QueueItem item = std::move(m_queue.front());
        m_queue.pop_front();
        lock.unlock();

        switch (item.type) {
        case QueueItem::Type::Input:
            ProcessInput(item);
            break;
        case QueueItem::Type::Flush:
            ProcessFlush();
            break;
        case QueueItem::Type::Reset:
            ProcessReset();
            break;
        default:
            break;
        }

        lock.lock();
    }
}

void VdecDecoder::ProcessInput(QueueItem& item) {
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        LOG_ERROR(Lib_Vdecsw, "Failed to allocate packet");
        CompleteInput(item.original_au_data, 0, ORBIS_VDECSW_ERROR_API_FAIL);
        return;
    }

    packet->data = item.au_data.data();
    packet->size = (int)item.au_data.size();
    packet->pts = item.pts_data;
    packet->dts = item.dts_data;
    packet->opaque = reinterpret_cast<void*>(item.attached_data);

    int ret = avcodec_send_packet(m_codec_context, packet);
    if (ret == AVERROR_EOF) {
        // Attempt to flush buffers and try again.
        avcodec_flush_buffers(m_codec_context);
        ret = avcodec_send_packet(m_codec_context, packet);
    }
    av_packet_free(&packet);
    if (ret < 0) {
        LOG_ERROR(Lib_Vdecsw, "Error sending packet to decoder: {}", ret);
        CompleteInput(item.original_au_data, 0, ORBIS_VDECSW_ERROR_API_FAIL);
        return;
    }

    const u32 frame_count = ReceiveFrames();
    CompleteInput(item.original_au_data, frame_count, ORBIS_OK);
}

void VdecDecoder::ProcessFlush() {
    // Enter draining mode and gather the frames that are still held by the
    // decoder for reordering purposes.
    avcodec_send_packet(m_codec_context, nullptr);
    ReceiveFrames();

    {
        std::scoped_lock lock{m_mutex};
        m_finalized = true;
    }
    m_output_cv.notify_all();
}

void VdecDecoder::ProcessReset() {
    avcodec_flush_buffers(m_codec_context);

    std::scoped_lock lock{m_mutex};
    m_unsynced_inputs -= (s32)m_completed_inputs.size();
    m_completed_inputs.clear();
    ClearFrameQueue();
    m_finalized = false;
}

void VdecDecoder::CompleteInput(void* au_data, u32 frame_count, s32 result) {
    {
        std::scoped_lock lock{m_mutex};
        m_completed_inputs.push_back(CompletedInput{au_data, frame_count, result});
    }
    m_input_cv.notify_all();
}

u32 VdecDecoder::ReceiveFrames() {
    u32 frame_count = 0;

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        LOG_ERROR(Lib_Vdecsw, "Failed to allocate frame");
        return 0;
    }

    for (;;) {
        int ret = avcodec_receive_frame(m_codec_context, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            LOG_ERROR(Lib_Vdecsw, "Error receiving frame from decoder: {}", ret);
            break;
        }

        if (frame->flags & AV_FRAME_FLAG_INTERLACED) {
            LOG_ERROR(Lib_Vdecsw, "Interlaced video output is not suported.");
        }

        AVFrame* queued = nullptr;
        if (frame->format != AV_PIX_FMT_NV12) {
            AVFrame* nv12_frame = ConvertNV12Frame(*frame);
            ASSERT(nv12_frame);
            queued = nv12_frame;
        } else {
            queued = av_frame_clone(frame);
        }
        if (queued) {
            std::scoped_lock lock{m_mutex};
            m_frame_queue.push_back(queued);
            ++frame_count;
        }
        av_frame_unref(frame);
    }

    av_frame_free(&frame);

    if (frame_count > 0) {
        m_output_cv.notify_all();
    }
    return frame_count;
}

s32 VdecDecoder::WriteFrame(OrbisVdecswOutputInfo& output_info, AVFrame& frame,
                            const OrbisVdecswFrameBuffer& frame_buffer, bool is_last_frame) {
    const u64 info_size =
        m_is_avc ? sizeof(OrbisVdecswAvcPictureInfo) : sizeof(OrbisVdecswHevcPictureInfo);
    Videodec::CopyNV12Data((u8*)frame_buffer.frame_buffer,
                           frame_buffer.frame_buffer_size - info_size, frame);

    const auto width = Common::AlignUp<u32>(frame.width, 16);
    const auto pitch = Common::AlignUp<u32>(frame.width, 64);
    const auto height = Common::AlignUp<u32>(frame.height, 16);

    output_info.is_valid = true;
    output_info.is_last_frame = is_last_frame;
    output_info.picture_count = 1; // TODO: 2 pictures for interlaced video
    output_info.codec_type = m_is_avc ? OrbisVdecswCodecType::Avc : OrbisVdecswCodecType::Hevc;
    output_info.frame_width = width;
    output_info.frame_pitch = pitch;
    output_info.frame_height = height;
    output_info.frame_buffer = frame_buffer.frame_buffer;
    output_info.frame_buffer_size = (pitch * height * 3) / 2;

    // Only set frameFormat and framePitchInBytes if the game uses the newer struct version.
    if (output_info.this_size == sizeof(OrbisVdecswOutputInfo)) {
        output_info.frame_format = 0;
        output_info.frame_pitch_in_bytes = pitch;
    }

    if (m_is_avc) {
        auto& picture_info = *(OrbisVdecswAvcPictureInfo*)((u8*)output_info.frame_buffer +
                                                           output_info.frame_buffer_size);

        picture_info = {};
        picture_info.is_valid = true;

        picture_info.pts_data = frame.pts;
        picture_info.dts_data = frame.pkt_dts;
        picture_info.attached_data = reinterpret_cast<u64>(frame.opaque);

        picture_info.frame_crop_top_offset = 0;
        picture_info.frame_crop_left_offset = 0;
        picture_info.frame_crop_right_offset = pitch - frame.width;
        picture_info.frame_crop_bottom_offset = height - frame.height;
    } else {
        auto& picture_info = *(OrbisVdecswHevcPictureInfo*)((u8*)output_info.frame_buffer +
                                                            output_info.frame_buffer_size);

        picture_info = {};
        picture_info.is_valid = true;

        picture_info.pts_data = frame.pts;
        picture_info.dts_data = frame.pkt_dts;
        picture_info.attached_data = reinterpret_cast<u64>(frame.opaque);

        picture_info.pic_width_in_luma_samples = frame.width;
        picture_info.pic_height_in_luma_samples = frame.height;

        picture_info.frame_crop_top_offset = 0;
        picture_info.frame_crop_left_offset = 0;
        picture_info.frame_crop_right_offset = pitch - frame.width;
        picture_info.frame_crop_bottom_offset = height - frame.height;
    }

    return ORBIS_OK;
}

void VdecDecoder::ClearFrameQueue() {
    while (!m_frame_queue.empty()) {
        AVFrame* frame = m_frame_queue.front();
        m_frame_queue.pop_front();
        av_frame_free(&frame);
    }
}

AVFrame* VdecDecoder::ConvertNV12Frame(AVFrame& frame) {
    AVFrame* nv12_frame = av_frame_alloc();
    nv12_frame->pts = frame.pts;
    nv12_frame->pkt_dts = frame.pkt_dts < 0 ? 0 : frame.pkt_dts;
    nv12_frame->format = AV_PIX_FMT_NV12;
    nv12_frame->width = frame.width;
    nv12_frame->height = frame.height;
    nv12_frame->sample_aspect_ratio = frame.sample_aspect_ratio;
    nv12_frame->crop_top = frame.crop_top;
    nv12_frame->crop_bottom = frame.crop_bottom;
    nv12_frame->crop_left = frame.crop_left;
    nv12_frame->crop_right = frame.crop_right;
    nv12_frame->opaque = frame.opaque;

    av_frame_get_buffer(nv12_frame, 0);

    if (m_sws_context == nullptr) {
        m_sws_context = sws_getContext(frame.width, frame.height, AVPixelFormat(frame.format),
                                       nv12_frame->width, nv12_frame->height, AV_PIX_FMT_NV12,
                                       SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    }

    const auto res = sws_scale(m_sws_context, frame.data, frame.linesize, 0, frame.height,
                               nv12_frame->data, nv12_frame->linesize);
    if (res < 0) {
        LOG_ERROR(Lib_Vdecsw, "Could not convert to NV12: {}", av_err2str(res));
        av_frame_free(&nv12_frame);
        return nullptr;
    }

    return nv12_frame;
}

} // namespace Libraries::Vdecsw
