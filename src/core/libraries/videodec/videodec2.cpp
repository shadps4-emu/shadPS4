// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "videodec2.h"
#include "videodec2_impl.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Vdec2 {

int PS4_SYSV_ABI sceVideodec2QueryComputeMemoryInfo(SceVideodec2ComputeMemoryInfo* pMemInfoOut) {
    LOG_ERROR(Lib_Vdec2, "called");

    if (pMemInfoOut->thisSize != sizeof(SceVideodec2ComputeMemoryInfo)) {
        return 0x811d0101;
    }

    pMemInfoOut->pCpuGpuMemory = nullptr;
    pMemInfoOut->cpuGpuMemorySize = 33554432; // Bogus value

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVideodec2AllocateComputeQueue(const SceVideodec2ComputeConfigInfo* pComputeCfgInfoIn,
                                 const SceVideodec2ComputeMemoryInfo* pComputeMemInfoIn,
                                 SceVideodec2ComputeQueue* pComputeQueueOut) {
    LOG_ERROR(Lib_Vdec2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2ReleaseComputeQueue(SceVideodec2ComputeQueue computeQueueIn) {
    LOG_ERROR(Lib_Vdec2, "called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceVideodec2QueryDecoderMemoryInfo(const SceVideodec2DecoderConfigInfo* pCfgInfoIn,
                                                    SceVideodec2DecoderMemoryInfo* pMemInfoOut) {
    LOG_ERROR(Lib_Vdec2, "called");

    if ((pCfgInfoIn->thisSize == 0x48) && (pMemInfoOut->thisSize == 0x48)) {
        pMemInfoOut->pCpuMemory = nullptr;
        pMemInfoOut->pGpuMemory = nullptr;
        pMemInfoOut->pCpuGpuMemory = nullptr;

        pMemInfoOut->cpuGpuMemorySize = 33554432; // Bogus value
        pMemInfoOut->cpuMemorySize = 33554432;    // Bogus value
        pMemInfoOut->gpuMemorySize = 33554432;    // Bogus value

        pMemInfoOut->maxFrameBufferSize = 33554432;   // Bogus value
        pMemInfoOut->frameBufferAlignment = 33554432; // Bogus value
    } else {
        return 0x811d0101;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceVideodec2CreateDecoder(const SceVideodec2DecoderConfigInfo* pDecoderConfigInfoIn,
                          const SceVideodec2DecoderMemoryInfo* pDecoderMemoryInfoIn,
                          SceVideodec2Decoder* pDecoderInstanceOut) {
    LOG_ERROR(Lib_Vdec2, "called");

    *pDecoderInstanceOut = new Videodec2(*pDecoderConfigInfoIn, *pDecoderMemoryInfoIn);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2DeleteDecoder(SceVideodec2Decoder decoder) {
    LOG_ERROR(Lib_Vdec2, "called");
    delete decoder;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2Decode(SceVideodec2Decoder decoder,
                                    const SceVideodec2InputData* pInputDataInOut,
                                    SceVideodec2FrameBuffer* pFrameBufferInOut,
                                    SceVideodec2OutputInfo* pOutputInfoOut) {
    LOG_TRACE(Lib_Vdec2, "called");

    if (decoder == nullptr) {
        return 0x811D0103; // SCE_VIDEODEC2_ERROR_DECODER_INSTANCE;
    }

    return decoder->Decode(*pInputDataInOut, *pFrameBufferInOut, *pOutputInfoOut);
}

s32 PS4_SYSV_ABI sceVideodec2Flush(SceVideodec2Decoder decoder,
                                   SceVideodec2FrameBuffer* pFrameBufferInOut,
                                   SceVideodec2OutputInfo* pOutputInfoOut) {
    LOG_ERROR(Lib_Vdec2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2Reset(SceVideodec2Decoder decoder) {
    LOG_ERROR(Lib_Vdec2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVideodec2GetPictureInfo(const SceVideodec2OutputInfo* pOutputInfoIn,
                                            void* p1stPictureInfoOut, void* p2ndPictureInfoOut) {
    LOG_ERROR(Lib_Vdec2, "called");

    // AVFrame* frame = (AVFrame*)pOutputInfoIn->pFrameBuffer;

    if (p1stPictureInfoOut) {
        SceVideodec2AvcPictureInfo* picInfo = (SceVideodec2AvcPictureInfo*)p1stPictureInfoOut;

        memset(p1stPictureInfoOut, 0, sizeof(SceVideodec2AvcPictureInfo));
        picInfo->isValid = 1;
        picInfo->frameCropLeftOffset = 0;   // frame->crop_left;
        picInfo->frameCropRightOffset = 0;  // frame->crop_right;
        picInfo->frameCropTopOffset = 0;    // frame->crop_top;
        picInfo->frameCropBottomOffset = 0; // frame->crop_bottom;
    }

    if (pOutputInfoIn->pictureCount == 2 && p2ndPictureInfoOut) {
        SceVideodec2AvcPictureInfo* picInfo = (SceVideodec2AvcPictureInfo*)p2ndPictureInfoOut;

        memset(p2ndPictureInfoOut, 0, sizeof(SceVideodec2AvcPictureInfo));
        picInfo->isValid = 1;
        picInfo->frameCropLeftOffset = 0;
        picInfo->frameCropRightOffset = 0;
        picInfo->frameCropTopOffset = 0;
        picInfo->frameCropBottomOffset = 0;
    }

    return ORBIS_OK;
}

void RegisterlibSceVdec2(Core::Loader::SymbolsResolver* sym) {
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