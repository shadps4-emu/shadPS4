// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/videodec/vdecsw.h"
#include "core/libraries/videodec/vdecsw_impl.h"
#include "core/libraries/videodec/videodec2.h"
#include "core/libraries/videodec/videodec_error.h"

namespace Libraries::Videodec2 {

using namespace Vdecsw;

static constexpr u64 kMinimumMemorySize = 16_MB; ///> Fake minimum memory size for querying

static u64 ComputeFrameSizeBytes(s32 width, s32 height) {
    if (width <= 0 || height <= 0) {
        return 0;
    }

    const u32 aligned_width = Common::AlignUp<u32>((u32)width, 64);
    const u32 aligned_height = Common::AlignUp<u32>((u32)height, 16);

    const u64 pixels = (u64)aligned_width * (u64)aligned_height;
    return (pixels * 3) / 2;
}

static s32 ComputeDpbCount(const OrbisVideodec2DecoderConfigInfo& cfg) {
    if (cfg.max_dpb_frame_count > 0) {
        return cfg.max_dpb_frame_count;
    }

    return 8;
}

static void ComputeWorstCaseDimensions(const OrbisVideodec2DecoderConfigInfo& cfg, s32& out_width,
                                       s32& out_height) {
    if (cfg.max_frame_width > 0 && cfg.max_frame_height > 0) {
        out_width = cfg.max_frame_width;
        out_height = cfg.max_frame_height;
        return;
    }

    out_width = 1920;
    out_height = 1080;

    if (cfg.max_level >= 150) {
        out_width = 3840;
        out_height = 2160;
    }
}

static OrbisVdecswCodecType ConvertCodecType(OrbisVideodec2CodecType codec_type) {
    return codec_type == OrbisVideodec2CodecType::Avc ? OrbisVdecswCodecType::Avc
                                                      : OrbisVdecswCodecType::Hevc;
}

static OrbisVideodec2CodecType ConvertCodecType(OrbisVdecswCodecType codec_type) {
    return codec_type == OrbisVdecswCodecType::Avc ? OrbisVideodec2CodecType::Avc
                                                   : OrbisVideodec2CodecType::Hevc;
}

static OrbisVdecswDecoderConfigInfo ConvertDecoderConfigInfo(
    const OrbisVideodec2DecoderConfigInfo& config_info) {
    OrbisVdecswDecoderConfigInfo result{};
    result.this_size = sizeof(OrbisVdecswDecoderConfigInfo);
    result.resource_type = config_info.resource_type;
    result.codec_type = ConvertCodecType(config_info.codec_type);
    result.profile = config_info.profile;
    result.max_level = config_info.max_level;
    result.max_frame_width = config_info.max_frame_width;
    result.max_frame_height = config_info.max_frame_height;
    result.max_dpb_frame_count = config_info.max_dpb_frame_count;
    result.decode_pipeline_depth = config_info.decode_pipeline_depth;
    result.compute_queue = config_info.compute_queue;
    result.cpu_affinity_mask = config_info.cpu_affinity_mask;
    result.cpu_thread_priority = config_info.cpu_thread_priority;
    result.optimize_progressive_video = config_info.optimize_progressive_video;
    result.check_memory_type = config_info.check_memory_type;
    return result;
}

static OrbisVdecswDecoderMemoryInfo ConvertDecoderMemoryInfo(
    const OrbisVideodec2DecoderMemoryInfo& memory_info) {
    OrbisVdecswDecoderMemoryInfo result{};
    result.this_size = sizeof(OrbisVdecswDecoderMemoryInfo);
    result.cpu_memory = memory_info.cpu_memory;
    result.cpu_memory_size = memory_info.cpu_memory_size;
    result.gpu_memory = memory_info.gpu_memory;
    result.gpu_memory_size = memory_info.gpu_memory_size;
    result.cpu_gpu_memory = memory_info.cpu_gpu_memory;
    result.cpu_gpu_memory_size = memory_info.cpu_gpu_memory_size;
    result.max_frame_buffer_size = memory_info.max_frame_buffer_size;
    result.frame_buffer_alignment = memory_info.frame_buffer_alignment;
    return result;
}

static OrbisVdecswInputData ConvertInputData(const OrbisVideodec2InputData& input_data) {
    OrbisVdecswInputData result{};
    result.this_size = sizeof(OrbisVdecswInputData);
    result.au_data = input_data.au_data;
    result.au_size = input_data.au_size;
    result.pts_data = input_data.pts_data;
    result.dts_data = input_data.dts_data;
    result.attached_data = input_data.attached_data;
    return result;
}

static OrbisVdecswFrameBuffer ConvertFrameBuffer(const OrbisVideodec2FrameBuffer& frame_buffer) {
    OrbisVdecswFrameBuffer result{};
    result.this_size = sizeof(OrbisVdecswFrameBuffer);
    result.frame_buffer = frame_buffer.frame_buffer;
    result.frame_buffer_size = frame_buffer.frame_buffer_size;
    return result;
}

static void ConvertOutputInfo(const OrbisVdecswOutputInfo& src, OrbisVideodec2OutputInfo& dst) {
    // this_size is preserved so the game keeps seeing its own struct version.
    dst.is_valid = src.is_valid;
    dst.is_error_frame = src.is_error_frame;
    dst.picture_count = src.picture_count;
    dst.codec_type = ConvertCodecType(src.codec_type);
    dst.frame_width = src.frame_width;
    dst.frame_pitch = src.frame_pitch;
    dst.frame_height = src.frame_height;
    dst.frame_buffer = src.frame_buffer;
    dst.frame_buffer_size = src.frame_buffer_size;
    dst.frame_format = src.frame_format;
    dst.frame_pitch_in_bytes = src.frame_pitch_in_bytes;
}

static s32 ConvertVdecswError(s32 error) {
    switch (error) {
    case ORBIS_VDECSW_ERROR_API_FAIL:
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    case ORBIS_VDECSW_ERROR_ACCESS_UNIT_POINTER:
        return ORBIS_VIDEODEC2_ERROR_ACCESS_UNIT_POINTER;
    case ORBIS_VDECSW_ERROR_ACCESS_UNIT_SIZE:
        return ORBIS_VIDEODEC2_ERROR_ACCESS_UNIT_SIZE;
    case ORBIS_VDECSW_ERROR_FRAME_BUFFER_POINTER:
        return ORBIS_VIDEODEC2_ERROR_FRAME_BUFFER_POINTER;
    case ORBIS_VDECSW_ERROR_FRAME_BUFFER_SIZE:
        return ORBIS_VIDEODEC2_ERROR_FRAME_BUFFER_SIZE;
    default:
        return ORBIS_VIDEODEC2_ERROR_API_FAIL;
    }
}

s32 PS4_SYSV_ABI
sceVideodec2QueryComputeMemoryInfo(OrbisVideodec2ComputeMemoryInfo* compute_mem_info) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!compute_mem_info) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (compute_mem_info->this_size != sizeof(OrbisVideodec2ComputeMemoryInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    compute_mem_info->cpu_gpu_memory = nullptr;
    compute_mem_info->cpu_gpu_memory_size = kMinimumMemorySize;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVideodec2AllocateComputeQueue(const OrbisVideodec2ComputeConfigInfo* compute_cfg_info,
                                 const OrbisVideodec2ComputeMemoryInfo* compute_mem_info,
                                 OrbisVideodec2ComputeQueue* compute_queue) {
    LOG_WARNING(Lib_Vdec2, "called");
    if (!compute_cfg_info || !compute_mem_info || !compute_queue) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (compute_cfg_info->this_size != sizeof(OrbisVideodec2ComputeConfigInfo) ||
        compute_mem_info->this_size != sizeof(OrbisVideodec2ComputeMemoryInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    if (compute_cfg_info->reserved0 != 0 || compute_cfg_info->reserved1 != 0) {
        LOG_ERROR(Lib_Vdec2, "Invalid compute config");
        return ORBIS_VIDEODEC2_ERROR_CONFIG_INFO;
    }
    if (compute_cfg_info->compute_pipe_id > 4) {
        LOG_ERROR(Lib_Vdec2, "Invalid compute pipe id");
        return ORBIS_VIDEODEC2_ERROR_COMPUTE_PIPE_ID;
    }
    if (compute_cfg_info->compute_queue_id > 7) {
        LOG_ERROR(Lib_Vdec2, "Invalid compute queue id");
        return ORBIS_VIDEODEC2_ERROR_COMPUTE_QUEUE_ID;
    }
    if (!compute_mem_info->cpu_gpu_memory) {
        LOG_ERROR(Lib_Vdec2, "Invalid memory pointer");
        return ORBIS_VIDEODEC2_ERROR_MEMORY_POINTER;
    }

    // The real library returns a pointer to memory inside cpuGpuMemory
    *compute_queue = compute_mem_info->cpu_gpu_memory;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2ReleaseComputeQueue(OrbisVideodec2ComputeQueue compute_queue) {
    LOG_INFO(Lib_Vdec2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVideodec2QueryDecoderMemoryInfo(const OrbisVideodec2DecoderConfigInfo* decoder_cfg_info,
                                   OrbisVideodec2DecoderMemoryInfo* decoder_mem_info) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!decoder_cfg_info || !decoder_mem_info) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (decoder_cfg_info->this_size != sizeof(OrbisVideodec2DecoderConfigInfo) ||
        decoder_mem_info->this_size != sizeof(OrbisVideodec2DecoderMemoryInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    s32 width = 0;
    s32 height = 0;
    ComputeWorstCaseDimensions(*decoder_cfg_info, width, height);

    const u64 frame_size = ComputeFrameSizeBytes(width, height);
    u64 max_frame_buffer = 0;
    if (frame_size == 0) {
        max_frame_buffer = kMinimumMemorySize;
    } else {
        max_frame_buffer = Common::AlignUp<u64>(frame_size, 256) + 0x4000;
    }

    decoder_mem_info->cpu_memory = nullptr;
    decoder_mem_info->gpu_memory = nullptr;
    decoder_mem_info->cpu_gpu_memory = nullptr;

    decoder_mem_info->cpu_gpu_memory_size = kMinimumMemorySize;
    decoder_mem_info->cpu_memory_size = kMinimumMemorySize;
    decoder_mem_info->gpu_memory_size = kMinimumMemorySize;

    decoder_mem_info->max_frame_buffer_size = max_frame_buffer;
    decoder_mem_info->frame_buffer_alignment = 0x100;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2CreateDecoder(const OrbisVideodec2DecoderConfigInfo* decoder_cfg_info,
                                           const OrbisVideodec2DecoderMemoryInfo* decoder_mem_info,
                                           OrbisVideodec2Decoder* decoder) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!decoder_cfg_info || !decoder_mem_info || !decoder) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (decoder_cfg_info->this_size != sizeof(OrbisVideodec2DecoderConfigInfo) ||
        decoder_mem_info->this_size != sizeof(OrbisVideodec2DecoderMemoryInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    *decoder = new Vdecsw::VdecDecoder(ConvertDecoderConfigInfo(*decoder_cfg_info),
                                       ConvertDecoderMemoryInfo(*decoder_mem_info));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2DeleteDecoder(OrbisVideodec2Decoder decoder) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!decoder) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }

    delete decoder;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2Decode(OrbisVideodec2Decoder decoder,
                                    const OrbisVideodec2InputData* input_data,
                                    OrbisVideodec2FrameBuffer* frame_buffer,
                                    OrbisVideodec2OutputInfo* output_info) {
    LOG_TRACE(Lib_Vdec2, "called");

    if (!decoder) {
        LOG_ERROR(Lib_Vdec2, "Invalid decoder instance");
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }
    if (!input_data || !frame_buffer || !output_info) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (input_data->this_size != sizeof(OrbisVideodec2InputData) ||
        frame_buffer->this_size != sizeof(OrbisVideodec2FrameBuffer)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    s32 ret = decoder->SetDecodeInput(ConvertInputData(*input_data));
    if (ret != ORBIS_OK) {
        return ConvertVdecswError(ret);
    }

    OrbisVdecswInputResult input_result{};
    input_result.this_size = sizeof(OrbisVdecswInputResult);
    ret = decoder->SyncDecodeInput(input_result);
    if (ret != ORBIS_OK) {
        return ConvertVdecswError(ret);
    }
    if (input_result.output_frame_count == 0) {
        output_info->is_valid = false;
        return ORBIS_OK;
    }

    ret = decoder->SetDecodeOutput(ConvertFrameBuffer(*frame_buffer));
    if (ret != ORBIS_OK) {
        return ConvertVdecswError(ret);
    }

    OrbisVdecswOutputInfo vdecsw_output{};
    vdecsw_output.this_size = output_info->this_size;
    ret = decoder->SyncDecodeOutput(vdecsw_output);
    if (ret != ORBIS_OK && ret != ORBIS_VDECSW_ERROR_DECODE_PENDING) {
        return ConvertVdecswError(ret);
    }

    frame_buffer->is_accepted = vdecsw_output.is_valid;
    ConvertOutputInfo(vdecsw_output, *output_info);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2Flush(OrbisVideodec2Decoder decoder,
                                   OrbisVideodec2FrameBuffer* frame_buffer,
                                   OrbisVideodec2OutputInfo* output_info) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!decoder) {
        LOG_ERROR(Lib_Vdec2, "Invalid decoder instance");
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }
    if (!frame_buffer || !output_info) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (frame_buffer->this_size != sizeof(OrbisVideodec2FrameBuffer) ||
        (output_info->this_size | 8) != sizeof(OrbisVideodec2OutputInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    // Drain the frames that are still held for reordering purposes and
    // return the next one.
    decoder->FinalizeDecodeSequence();

    s32 ret = decoder->SetDecodeOutput(ConvertFrameBuffer(*frame_buffer));
    if (ret != ORBIS_OK) {
        return ConvertVdecswError(ret);
    }

    OrbisVdecswOutputInfo vdecsw_output{};
    vdecsw_output.this_size = output_info->this_size;
    ret = decoder->SyncDecodeOutput(vdecsw_output);
    if (ret != ORBIS_OK && ret != ORBIS_VDECSW_ERROR_DECODE_PENDING) {
        return ConvertVdecswError(ret);
    }

    frame_buffer->is_accepted = vdecsw_output.is_valid;
    ConvertOutputInfo(vdecsw_output, *output_info);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2Reset(OrbisVideodec2Decoder decoder) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!decoder) {
        LOG_ERROR(Lib_Vdec2, "Invalid decoder instance");
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }

    return decoder->Reset();
}

s32 PS4_SYSV_ABI sceVideodec2GetPictureInfo(const OrbisVideodec2OutputInfo* output_info,
                                            void* p_1st_picture_info, void* p_2nd_picture_info) {
    LOG_TRACE(Lib_Vdec2, "called");

    if (!output_info) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if ((output_info->this_size | 8) != sizeof(OrbisVideodec2OutputInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    if (output_info->picture_count == 0) {
        LOG_ERROR(Lib_Vdec2, "No picture info available");
        return ORBIS_OK;
    }

    if (p_1st_picture_info) {
        auto size = *reinterpret_cast<u64*>(p_1st_picture_info);
        auto* pictureInfo = (u8*)output_info->frame_buffer + output_info->frame_buffer_size;
        // Copy the requested picture data to the output omitting picture size field.
        memcpy((u8*)p_1st_picture_info + sizeof(u64), pictureInfo + sizeof(u64),
               size - sizeof(u64));
    }

    if (output_info->picture_count > 1) {
        UNREACHABLE();
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2GetAvcPictureInfo(const OrbisVideodec2OutputInfo* output_info,
                                               void* p_1st_picture_info, void* p_2nd_picture_info) {
    LOG_TRACE(Lib_Vdec2, "called");
    return sceVideodec2GetPictureInfo(output_info, p_1st_picture_info, p_2nd_picture_info);
}

s32 PS4_SYSV_ABI sceVideodec2GetHevcPictureInfo(const OrbisVideodec2OutputInfo* output_info,
                                                OrbisVideodec2HevcPictureInfo* picture_info) {
    LOG_TRACE(Lib_Vdec2, "called");
    return sceVideodec2GetPictureInfo(output_info, picture_info, nullptr);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("RnDibcGCPKw", "libSceVideodec2", 1, "libSceVideodec2",
                 sceVideodec2QueryComputeMemoryInfo);
    LIB_FUNCTION("eD+X2SmxUt4", "libSceVideodec2", 1, "libSceVideodec2",
                 sceVideodec2AllocateComputeQueue);
    LIB_FUNCTION("UvtA3FAiF4Y", "libSceVideodec2", 1, "libSceVideodec2",
                 sceVideodec2ReleaseComputeQueue);

    LIB_FUNCTION("qqMCwlULR+E", "libSceVideodec2", 1, "libSceVideodec2",
                 sceVideodec2QueryDecoderMemoryInfo);
    LIB_FUNCTION("CNNRoRYd8XI", "libSceVideodec2", 1, "libSceVideodec2", sceVideodec2CreateDecoder);
    LIB_FUNCTION("jwImxXRGSKA", "libSceVideodec2", 1, "libSceVideodec2", sceVideodec2DeleteDecoder);
    LIB_FUNCTION("852F5+q6+iM", "libSceVideodec2", 1, "libSceVideodec2", sceVideodec2Decode);
    LIB_FUNCTION("l1hXwscLuCY", "libSceVideodec2", 1, "libSceVideodec2", sceVideodec2Flush);
    LIB_FUNCTION("wJXikG6QFN8", "libSceVideodec2", 1, "libSceVideodec2", sceVideodec2Reset);
    LIB_FUNCTION("NtXRa3dRzU0", "libSceVideodec2", 1, "libSceVideodec2",
                 sceVideodec2GetPictureInfo);
    LIB_FUNCTION("kjrLbcyhEiw", "libSceVideodec2", 1, "libSceVideodec2",
                 sceVideodec2GetAvcPictureInfo);
    LIB_FUNCTION("7M+1UFqWOAI", "libSceVideodec2", 1, "libSceVideodec2",
                 sceVideodec2GetHevcPictureInfo);
}

} // namespace Libraries::Videodec2
