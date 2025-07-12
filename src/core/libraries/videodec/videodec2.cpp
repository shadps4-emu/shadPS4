// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/videodec/videodec2.h"
#include "core/libraries/videodec/videodec2_impl.h"
#include "core/libraries/videodec/videodec_error.h"

namespace Libraries::Vdec2 {

static constexpr u64 kMinimumMemorySize = 16_MB; ///> Fake minimum memory size for querying

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
    LOG_INFO(Lib_Vdec2, "called");
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

    decoderMemInfo->cpuMemory = nullptr;
    decoderMemInfo->gpuMemory = nullptr;
    decoderMemInfo->cpuGpuMemory = nullptr;

    decoderMemInfo->cpuGpuMemorySize = kMinimumMemorySize;
    decoderMemInfo->cpuMemorySize = kMinimumMemorySize;
    decoderMemInfo->gpuMemorySize = kMinimumMemorySize;

    decoderMemInfo->maxFrameBufferSize = kMinimumMemorySize;
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

    // If the game uses the older Videodec2 structs, we need to accomodate that.
    if (outputInfo->thisSize != sizeof(OrbisVideodec2OutputInfo)) {
        if (gLegacyPictureInfos.empty()) {
            LOG_ERROR(Lib_Vdec2, "No picture info available");
            return ORBIS_OK;
        }
        if (p1stPictureInfoOut) {
            OrbisVideodec2LegacyAvcPictureInfo* picInfo =
                static_cast<OrbisVideodec2LegacyAvcPictureInfo*>(p1stPictureInfoOut);
            if (picInfo->thisSize != sizeof(OrbisVideodec2LegacyAvcPictureInfo)) {
                LOG_ERROR(Lib_Vdec2, "Invalid struct size");
                return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
            }
            *picInfo = gLegacyPictureInfos.back();
        }
    } else {
        if (gPictureInfos.empty()) {
            LOG_ERROR(Lib_Vdec2, "No picture info available");
            return ORBIS_OK;
        }
        if (p1stPictureInfoOut) {
            OrbisVideodec2AvcPictureInfo* picInfo =
                static_cast<OrbisVideodec2AvcPictureInfo*>(p1stPictureInfoOut);
            if (picInfo->thisSize != sizeof(OrbisVideodec2AvcPictureInfo)) {
                LOG_ERROR(Lib_Vdec2, "Invalid struct size");
                return ORBIS_VIDEODEC2_ERROR_STRUCT_SIZE;
            }
            *picInfo = gPictureInfos.back();
        }
    }

    if (outputInfo->pictureCount > 1) {
        UNREACHABLE();
    }

    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("RnDibcGCPKw", "libSceVideodec2", 1, "libSceVideodec2", 1, 1,
                 sceVideodec2QueryComputeMemoryInfo);
    LIB_FUNCTION("eD+X2SmxUt4", "libSceVideodec2", 1, "libSceVideodec2", 1, 1,
                 sceVideodec2AllocateComputeQueue);
    LIB_FUNCTION("UvtA3FAiF4Y", "libSceVideodec2", 1, "libSceVideodec2", 1, 1,
                 sceVideodec2ReleaseComputeQueue);

    LIB_FUNCTION("qqMCwlULR+E", "libSceVideodec2", 1, "libSceVideodec2", 1, 1,
                 sceVideodec2QueryDecoderMemoryInfo);
    LIB_FUNCTION("CNNRoRYd8XI", "libSceVideodec2", 1, "libSceVideodec2", 1, 1,
                 sceVideodec2CreateDecoder);
    LIB_FUNCTION("jwImxXRGSKA", "libSceVideodec2", 1, "libSceVideodec2", 1, 1,
                 sceVideodec2DeleteDecoder);
    LIB_FUNCTION("852F5+q6+iM", "libSceVideodec2", 1, "libSceVideodec2", 1, 1, sceVideodec2Decode);
    LIB_FUNCTION("l1hXwscLuCY", "libSceVideodec2", 1, "libSceVideodec2", 1, 1, sceVideodec2Flush);
    LIB_FUNCTION("wJXikG6QFN8", "libSceVideodec2", 1, "libSceVideodec2", 1, 1, sceVideodec2Reset);
    LIB_FUNCTION("NtXRa3dRzU0", "libSceVideodec2", 1, "libSceVideodec2", 1, 1,
                 sceVideodec2GetPictureInfo);
}

} // namespace Libraries::Vdec2
