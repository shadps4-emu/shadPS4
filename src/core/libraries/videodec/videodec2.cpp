// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/videodec/videodec2.h"
#include "core/libraries/videodec/videodec2_impl.h"
#include "core/libraries/videodec/videodec_error.h"

namespace Libraries::Videodec2 {

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
    if (cfg.maxDpbFrameCount > 0) {
        return cfg.maxDpbFrameCount;
    }

    return 8;
}

static void ComputeWorstCaseDimensions(const OrbisVideodec2DecoderConfigInfo& cfg, s32& out_width,
                                       s32& out_height) {
    if (cfg.maxFrameWidth > 0 && cfg.maxFrameHeight > 0) {
        out_width = cfg.maxFrameWidth;
        out_height = cfg.maxFrameHeight;
        return;
    }

    out_width = 1920;
    out_height = 1080;

    if (cfg.maxLevel >= 150) {
        out_width = 3840;
        out_height = 2160;
    }
}

s32 PS4_SYSV_ABI
sceVideodec2QueryComputeMemoryInfo(OrbisVideodec2ComputeMemoryInfo* computeMemInfo) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!computeMemInfo) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (computeMemInfo->thisSize != sizeof(OrbisVideodec2ComputeMemoryInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    computeMemInfo->cpuGpuMemory = nullptr;
    computeMemInfo->cpuGpuMemorySize = kMinimumMemorySize;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVideodec2AllocateComputeQueue(const OrbisVideodec2ComputeConfigInfo* computeCfgInfo,
                                 const OrbisVideodec2ComputeMemoryInfo* computeMemInfo,
                                 OrbisVideodec2ComputeQueue* computeQueue) {
    LOG_WARNING(Lib_Vdec2, "called");
    if (!computeCfgInfo || !computeMemInfo || !computeQueue) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (computeCfgInfo->thisSize != sizeof(OrbisVideodec2ComputeConfigInfo) ||
        computeMemInfo->thisSize != sizeof(OrbisVideodec2ComputeMemoryInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    if (computeCfgInfo->reserved0 != 0 || computeCfgInfo->reserved1 != 0) {
        LOG_ERROR(Lib_Vdec2, "Invalid compute config");
        return ORBIS_VIDEODEC2_ERROR_CONFIG_INFO;
    }
    if (computeCfgInfo->computePipeId > 4) {
        LOG_ERROR(Lib_Vdec2, "Invalid compute pipe id");
        return ORBIS_VIDEODEC2_ERROR_COMPUTE_PIPE_ID;
    }
    if (computeCfgInfo->computeQueueId > 7) {
        LOG_ERROR(Lib_Vdec2, "Invalid compute queue id");
        return ORBIS_VIDEODEC2_ERROR_COMPUTE_QUEUE_ID;
    }
    if (!computeMemInfo->cpuGpuMemory) {
        LOG_ERROR(Lib_Vdec2, "Invalid memory pointer");
        return ORBIS_VIDEODEC2_ERROR_MEMORY_POINTER;
    }

    // The real library returns a pointer to memory inside cpuGpuMemory
    *computeQueue = computeMemInfo->cpuGpuMemory;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2ReleaseComputeQueue(OrbisVideodec2ComputeQueue computeQueue) {
    LOG_INFO(Lib_Vdec2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVideodec2QueryDecoderMemoryInfo(const OrbisVideodec2DecoderConfigInfo* decoderCfgInfo,
                                   OrbisVideodec2DecoderMemoryInfo* decoderMemInfo) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!decoderCfgInfo || !decoderMemInfo) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (decoderCfgInfo->thisSize != sizeof(OrbisVideodec2DecoderConfigInfo) ||
        decoderMemInfo->thisSize != sizeof(OrbisVideodec2DecoderMemoryInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    s32 width = 0;
    s32 height = 0;
    ComputeWorstCaseDimensions(*decoderCfgInfo, width, height);

    const u64 frame_size = ComputeFrameSizeBytes(width, height);
    u64 max_frame_buffer = 0;
    if (frame_size == 0) {
        max_frame_buffer = kMinimumMemorySize;
    } else {
        max_frame_buffer = Common::AlignUp<u64>(frame_size, 256) + 0x4000;
    }

    decoderMemInfo->cpuMemory = nullptr;
    decoderMemInfo->gpuMemory = nullptr;
    decoderMemInfo->cpuGpuMemory = nullptr;

    decoderMemInfo->cpuGpuMemorySize = kMinimumMemorySize;
    decoderMemInfo->cpuMemorySize = kMinimumMemorySize;
    decoderMemInfo->gpuMemorySize = kMinimumMemorySize;

    decoderMemInfo->maxFrameBufferSize = max_frame_buffer;
    decoderMemInfo->frameBufferAlignment = 0x100;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2CreateDecoder(const OrbisVideodec2DecoderConfigInfo* decoderCfgInfo,
                                           const OrbisVideodec2DecoderMemoryInfo* decoderMemInfo,
                                           OrbisVideodec2Decoder* decoder) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!decoderCfgInfo || !decoderMemInfo || !decoder) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (decoderCfgInfo->thisSize != sizeof(OrbisVideodec2DecoderConfigInfo) ||
        decoderMemInfo->thisSize != sizeof(OrbisVideodec2DecoderMemoryInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    *decoder = new VdecDecoder(*decoderCfgInfo, *decoderMemInfo);
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
                                    const OrbisVideodec2InputData* inputData,
                                    OrbisVideodec2FrameBuffer* frameBuffer,
                                    OrbisVideodec2OutputInfo* outputInfo) {
    LOG_TRACE(Lib_Vdec2, "called");

    if (!decoder) {
        LOG_ERROR(Lib_Vdec2, "Invalid decoder instance");
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }
    if (!inputData || !frameBuffer || !outputInfo) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (inputData->thisSize != sizeof(OrbisVideodec2InputData) ||
        frameBuffer->thisSize != sizeof(OrbisVideodec2FrameBuffer)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    return decoder->Decode(*inputData, *frameBuffer, *outputInfo);
}

s32 PS4_SYSV_ABI sceVideodec2Flush(OrbisVideodec2Decoder decoder,
                                   OrbisVideodec2FrameBuffer* frameBuffer,
                                   OrbisVideodec2OutputInfo* outputInfo) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!decoder) {
        LOG_ERROR(Lib_Vdec2, "Invalid decoder instance");
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }
    if (!frameBuffer || !outputInfo) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if (frameBuffer->thisSize != sizeof(OrbisVideodec2FrameBuffer) ||
        (outputInfo->thisSize | 8) != sizeof(OrbisVideodec2OutputInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }

    return decoder->Flush(*frameBuffer, *outputInfo);
}

s32 PS4_SYSV_ABI sceVideodec2Reset(OrbisVideodec2Decoder decoder) {
    LOG_INFO(Lib_Vdec2, "called");

    if (!decoder) {
        LOG_ERROR(Lib_Vdec2, "Invalid decoder instance");
        return ORBIS_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }

    return decoder->Reset();
}

s32 PS4_SYSV_ABI sceVideodec2GetPictureInfo(const OrbisVideodec2OutputInfo* outputInfo,
                                            void* p1stPictureInfoOut, void* p2ndPictureInfoOut) {
    LOG_TRACE(Lib_Vdec2, "called");

    if (!outputInfo) {
        LOG_ERROR(Lib_Vdec2, "Invalid arguments");
        return ORBIS_VIDEODEC2_ERROR_ARGUMENT_POINTER;
    }
    if ((outputInfo->thisSize | 8) != sizeof(OrbisVideodec2OutputInfo)) {
        LOG_ERROR(Lib_Vdec2, "Invalid struct size");
        return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
    }
    if (outputInfo->pictureCount == 0) {
        LOG_ERROR(Lib_Vdec2, "No picture info available");
        return ORBIS_OK;
    }

    if (p1stPictureInfoOut) {
        auto* info = reinterpret_cast<OrbisVideodec2AvcPictureInfo*>(p1stPictureInfoOut);
        // Copy enough data to check thisSize.
        u64 picture_size = info->thisSize;
        if ((picture_size | 0x10) != sizeof(OrbisVideodec2AvcPictureInfo)) {
            LOG_ERROR(Lib_Vdec2, "Invalid struct size");
            return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
        }
        auto& pictureInfo = *(OrbisVideodec2AvcPictureInfo*)((u8*)outputInfo->frameBuffer +
                                                             outputInfo->frameBufferSize);

        // Copy the requested picture data to the output.
        memcpy(p1stPictureInfoOut, &pictureInfo, picture_size);

        // Correct the outputted picture struct size.
        info->thisSize = picture_size;
    }

    if (outputInfo->pictureCount > 1) {
        UNREACHABLE();
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2GetAvcPictureInfo(const OrbisVideodec2OutputInfo* outputInfo,
                                               void* p1stPictureInfoOut, void* p2ndPictureInfoOut) {
    LOG_TRACE(Lib_Vdec2, "called");
    return sceVideodec2GetPictureInfo(outputInfo, p1stPictureInfoOut, p2ndPictureInfoOut);
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
}

} // namespace Libraries::Videodec2
