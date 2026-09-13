// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/videodec/vdecsw.h"
#include "core/libraries/videodec/vdecsw_impl.h"
#include "core/libraries/videodec/videodec_error.h"

namespace Libraries::Vdecsw {

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

static void ComputeWorstCaseDimensions(const OrbisVdecswDecoderConfigInfo& cfg, s32& out_width,
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

s32 PS4_SYSV_ABI
sceVdecswQueryComputeMemoryInfo(OrbisVdecswComputeMemoryInfo* compute_mem_info) {
    LOG_INFO(Lib_Vdecsw, "called");

    if (!compute_mem_info) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }
    if (compute_mem_info->this_size != sizeof(OrbisVdecswComputeMemoryInfo)) {
        LOG_ERROR(Lib_Vdecsw, "Invalid struct size");
        return ORBIS_VDECSW_ERROR_STRUCT_SIZE;
    }

    compute_mem_info->cpu_gpu_memory = nullptr;
    compute_mem_info->cpu_gpu_memory_size = kMinimumMemorySize;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVdecswAllocateComputeQueue(const OrbisVdecswComputeConfigInfo* compute_cfg_info_in,
                              const OrbisVdecswComputeMemoryInfo* compute_mem_info_in,
                              OrbisVdecswComputeQueue* compute_queue_out) {
    LOG_WARNING(Lib_Vdecsw, "called");
    if (!compute_cfg_info_in || !compute_mem_info_in || !compute_queue_out) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }
    if (compute_cfg_info_in->this_size != sizeof(OrbisVdecswComputeConfigInfo) ||
        compute_mem_info_in->this_size != sizeof(OrbisVdecswComputeMemoryInfo)) {
        LOG_ERROR(Lib_Vdecsw, "Invalid struct size");
        return ORBIS_VDECSW_ERROR_STRUCT_SIZE;
    }
    if (compute_cfg_info_in->reserved0 != 0 || compute_cfg_info_in->reserved1 != 0) {
        LOG_ERROR(Lib_Vdecsw, "Invalid compute config");
        return ORBIS_VDECSW_ERROR_CONFIG_INFO;
    }
    if (compute_cfg_info_in->compute_pipe_id > 4) {
        LOG_ERROR(Lib_Vdecsw, "Invalid compute pipe id");
        return ORBIS_VDECSW_ERROR_COMPUTE_PIPE_ID;
    }
    if (compute_cfg_info_in->compute_queue_id > 7) {
        LOG_ERROR(Lib_Vdecsw, "Invalid compute queue id");
        return ORBIS_VDECSW_ERROR_COMPUTE_QUEUE_ID;
    }
    if (!compute_mem_info_in->cpu_gpu_memory) {
        LOG_ERROR(Lib_Vdecsw, "Invalid memory pointer");
        return ORBIS_VDECSW_ERROR_MEMORY_POINTER;
    }

    // The real library returns a pointer to memory inside cpuGpuMemory
    *compute_queue_out = compute_mem_info_in->cpu_gpu_memory;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswReleaseComputeQueue(OrbisVdecswComputeQueue compute_queue_in) {
    LOG_INFO(Lib_Vdecsw, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVdecswQueryDecoderMemoryInfo(const OrbisVdecswDecoderConfigInfo* decoder_config_info_in,
                                OrbisVdecswDecoderMemoryInfo* decoder_memory_info_out) {
    LOG_INFO(Lib_Vdecsw, "called");

    if (!decoder_config_info_in || !decoder_memory_info_out) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }
    if (decoder_config_info_in->this_size != sizeof(OrbisVdecswDecoderConfigInfo) ||
        decoder_memory_info_out->this_size != sizeof(OrbisVdecswDecoderMemoryInfo)) {
        LOG_ERROR(Lib_Vdecsw, "Invalid struct size");
        return ORBIS_VDECSW_ERROR_STRUCT_SIZE;
    }

    s32 width = 0;
    s32 height = 0;
    ComputeWorstCaseDimensions(*decoder_config_info_in, width, height);

    const u64 frame_size = ComputeFrameSizeBytes(width, height);
    u64 max_frame_buffer = 0;
    if (frame_size == 0) {
        max_frame_buffer = kMinimumMemorySize;
    } else {
        max_frame_buffer = Common::AlignUp<u64>(frame_size, 256) + 0x4000;
    }

    decoder_memory_info_out->cpu_memory = nullptr;
    decoder_memory_info_out->gpu_memory = nullptr;
    decoder_memory_info_out->cpu_gpu_memory = nullptr;

    decoder_memory_info_out->cpu_gpu_memory_size = kMinimumMemorySize;
    decoder_memory_info_out->cpu_memory_size = kMinimumMemorySize;
    decoder_memory_info_out->gpu_memory_size = kMinimumMemorySize;

    decoder_memory_info_out->max_frame_buffer_size = max_frame_buffer;
    decoder_memory_info_out->frame_buffer_alignment = 0x100;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswCreateDecoder(const OrbisVdecswDecoderConfigInfo* decoder_config_info_in,
                                        const OrbisVdecswDecoderMemoryInfo* decoder_memory_info_in,
                                        OrbisVdecswDecoder* decoder_instance_out) {
    LOG_INFO(Lib_Vdecsw, "called");

    if (!decoder_config_info_in || !decoder_memory_info_in || !decoder_instance_out) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }
    if (decoder_config_info_in->this_size != sizeof(OrbisVdecswDecoderConfigInfo) ||
        decoder_memory_info_in->this_size != sizeof(OrbisVdecswDecoderMemoryInfo)) {
        LOG_ERROR(Lib_Vdecsw, "Invalid struct size");
        return ORBIS_VDECSW_ERROR_STRUCT_SIZE;
    }

    *decoder_instance_out = new VdecDecoder(*decoder_config_info_in, *decoder_memory_info_in);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswDeleteDecoder(OrbisVdecswDecoder decoder_instance_in) {
    LOG_INFO(Lib_Vdecsw, "called");

    if (!decoder_instance_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_DECODER_INSTANCE;
    }

    delete decoder_instance_in;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswResetDecoder(OrbisVdecswDecoder decoder_instance_in) {
    LOG_INFO(Lib_Vdecsw, "called");

    if (!decoder_instance_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid decoder instance");
        return ORBIS_VDECSW_ERROR_DECODER_INSTANCE;
    }

    return decoder_instance_in->Reset();
}

s32 PS4_SYSV_ABI sceVdecswSetDecodeInput(OrbisVdecswDecoder decoder_instance_in,
                                         const OrbisVdecswInputData* input_data_in) {
    LOG_TRACE(Lib_Vdecsw, "called");

    if (!decoder_instance_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid decoder instance");
        return ORBIS_VDECSW_ERROR_DECODER_INSTANCE;
    }
    if (!input_data_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }
    if (input_data_in->this_size != sizeof(OrbisVdecswInputData)) {
        LOG_ERROR(Lib_Vdecsw, "Invalid struct size");
        return ORBIS_VDECSW_ERROR_STRUCT_SIZE;
    }

    return decoder_instance_in->SetDecodeInput(*input_data_in);
}

s32 PS4_SYSV_ABI sceVdecswSyncDecodeInput(OrbisVdecswDecoder decoder_instance_in,
                                          OrbisVdecswInputResult* input_result_out) {
    LOG_TRACE(Lib_Vdecsw, "called");

    if (!decoder_instance_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid decoder instance");
        return ORBIS_VDECSW_ERROR_DECODER_INSTANCE;
    }
    if (!input_result_out) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }

    return decoder_instance_in->SyncDecodeInput(*input_result_out);
}

s32 PS4_SYSV_ABI sceVdecswTrySyncDecodeInput(OrbisVdecswDecoder decoder_instance_in,
                                             OrbisVdecswInputResult* input_result_out) {
    LOG_TRACE(Lib_Vdecsw, "called");

    if (!decoder_instance_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid decoder instance");
        return ORBIS_VDECSW_ERROR_DECODER_INSTANCE;
    }
    if (!input_result_out) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }

    // Decoding is performed synchronously, so the result is always ready.
    return decoder_instance_in->SyncDecodeInput(*input_result_out);
}

s32 PS4_SYSV_ABI sceVdecswSetDecodeOutput(OrbisVdecswDecoder decoder_instance_in,
                                          OrbisVdecswFrameBuffer* frame_buffer_in_out) {
    LOG_TRACE(Lib_Vdecsw, "called");

    if (!decoder_instance_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid decoder instance");
        return ORBIS_VDECSW_ERROR_DECODER_INSTANCE;
    }
    if (!frame_buffer_in_out) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }
    if (frame_buffer_in_out->this_size != sizeof(OrbisVdecswFrameBuffer)) {
        LOG_ERROR(Lib_Vdecsw, "Invalid struct size");
        return ORBIS_VDECSW_ERROR_STRUCT_SIZE;
    }

    return decoder_instance_in->SetDecodeOutput(*frame_buffer_in_out);
}

s32 PS4_SYSV_ABI sceVdecswSyncDecodeOutput(OrbisVdecswDecoder decoder_instance_in,
                                           OrbisVdecswOutputInfo* output_info_out) {
    LOG_TRACE(Lib_Vdecsw, "called");

    if (!decoder_instance_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid decoder instance");
        return ORBIS_VDECSW_ERROR_DECODER_INSTANCE;
    }
    if (!output_info_out) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }

    return decoder_instance_in->SyncDecodeOutput(*output_info_out);
}

s32 PS4_SYSV_ABI sceVdecswTrySyncDecodeOutput(OrbisVdecswDecoder decoder_instance_in,
                                              OrbisVdecswOutputInfo* output_info_out) {
    LOG_TRACE(Lib_Vdecsw, "called");

    if (!decoder_instance_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid decoder instance");
        return ORBIS_VDECSW_ERROR_DECODER_INSTANCE;
    }
    if (!output_info_out) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }

    // Decoding is performed synchronously, so the result is always ready.
    return decoder_instance_in->SyncDecodeOutput(*output_info_out);
}

s32 PS4_SYSV_ABI sceVdecswFinalizeDecodeSequence(OrbisVdecswDecoder decoder_instance_in) {
    LOG_INFO(Lib_Vdecsw, "called");

    if (!decoder_instance_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid decoder instance");
        return ORBIS_VDECSW_ERROR_DECODER_INSTANCE;
    }

    return decoder_instance_in->FinalizeDecodeSequence();
}

s32 PS4_SYSV_ABI sceVdecswGetPictureInfo(const OrbisVdecswOutputInfo* output_info_in,
                                         void* p_1st_picture_info_out,
                                         void* p_2nd_picture_info_out) {
    LOG_TRACE(Lib_Vdecsw, "called");

    if (!output_info_in) {
        LOG_ERROR(Lib_Vdecsw, "Invalid arguments");
        return ORBIS_VDECSW_ERROR_ARGUMENT_POINTER;
    }
    if ((output_info_in->this_size | 8) != sizeof(OrbisVdecswOutputInfo)) {
        LOG_ERROR(Lib_Vdecsw, "Invalid struct size");
        return ORBIS_VDECSW_ERROR_STRUCT_SIZE;
    }
    if (output_info_in->picture_count == 0) {
        LOG_ERROR(Lib_Vdecsw, "No picture info available");
        return ORBIS_OK;
    }

    if (p_1st_picture_info_out) {
        auto size = *reinterpret_cast<u64*>(p_1st_picture_info_out);
        auto* pictureInfo = (u8*)output_info_in->frame_buffer + output_info_in->frame_buffer_size;
        // Copy the requested picture data to the output omitting picture size field.
        memcpy((u8*)p_1st_picture_info_out + sizeof(u64), pictureInfo + sizeof(u64),
               size - sizeof(u64));
    }

    if (output_info_in->picture_count > 1) {
        UNREACHABLE();
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVdecswGetAvcPictureInfo(const OrbisVdecswOutputInfo* output_info_in,
                                            OrbisVdecswAvcPictureInfo* p_1st_picture_info_out,
                                            OrbisVdecswAvcPictureInfo* p_2nd_picture_info_out) {
    LOG_TRACE(Lib_Vdecsw, "called");
    return sceVdecswGetPictureInfo(output_info_in, p_1st_picture_info_out,
                                   p_2nd_picture_info_out);
}

s32 PS4_SYSV_ABI sceVdecswGetHevcPictureInfo(const OrbisVdecswOutputInfo* output_info_in,
                                             OrbisVdecswHevcPictureInfo* picture_info_out) {
    LOG_TRACE(Lib_Vdecsw, "called");
    return sceVdecswGetPictureInfo(output_info_in, picture_info_out, nullptr);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hIgrg5h4V6s", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswAllocateComputeQueue);
    LIB_FUNCTION("+L5ArV1tPGA", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswCreateDecoder);
    LIB_FUNCTION("ecUtPX+dBYk", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswDeleteDecoder);
    LIB_FUNCTION("5Y6nZqIZvBg", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswFinalizeDecodeSequence);
    LIB_FUNCTION("ihNT-uuEAr4", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswGetAvcPictureInfo);
    LIB_FUNCTION("PzF+L5zXoyg", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswGetHevcPictureInfo);
    LIB_FUNCTION("FzECy3Wxxas", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswGetPictureInfo);
    LIB_FUNCTION("0moTubWCsTM", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswQueryComputeMemoryInfo);
    LIB_FUNCTION("A+2M7EivuOU", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswQueryDecoderMemoryInfo);
    LIB_FUNCTION("fX-zOOefbbs", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswReleaseComputeQueue);
    LIB_FUNCTION("veb-YBrOqo0", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswResetDecoder);
    LIB_FUNCTION("aqMiF0AgUYI", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswSetDecodeInput);
    LIB_FUNCTION("rgtMCOpyBSc", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswSetDecodeOutput);
    LIB_FUNCTION("AAMM-Q1X0g0", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswSyncDecodeInput);
    LIB_FUNCTION("tWiSgXov8GM", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswSyncDecodeOutput);
    LIB_FUNCTION("l4sQYy5wPkc", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswTrySyncDecodeInput);
    LIB_FUNCTION("kMBw37oH8nI", "libSceVdecsw", 1, "libSceVdecsw", sceVdecswTrySyncDecodeOutput);
}

} // namespace Libraries::Vdecsw
