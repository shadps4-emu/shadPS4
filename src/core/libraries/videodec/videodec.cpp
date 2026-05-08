// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/videodec/videodec.h"
#include "core/libraries/videodec/videodec_error.h"
#include "core/libraries/videodec/videodec_impl.h"

namespace Libraries::Videodec {

static constexpr u64 kFallbackMemorySize = 16_MB;

static u64 ComputeFrameSizeBytes(s32 width, s32 height) {
    if (width <= 0 || height <= 0) {
        return 0;
    }

    const u32 aligned_width = Common::AlignUp<u32>((u32)width, 256);
    const u32 aligned_height = Common::AlignUp<u32>((u32)height, 16);

    const u64 pixels = (u64)aligned_width * (u64)aligned_height;
    return (pixels * 3) / 2;
}

static s32 ComputeDpbCount(const OrbisVideodecConfigInfo& cfg) {
    if (cfg.maxDpbFrameCount > 0) {
        return cfg.maxDpbFrameCount;
    }

    return 8;
}

static void ComputeWorstCaseDimensions(const OrbisVideodecConfigInfo& cfg, s32& out_width,
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

int PS4_SYSV_ABI sceVideodecCreateDecoder(const OrbisVideodecConfigInfo* pCfgInfoIn,
                                          const OrbisVideodecResourceInfo* pRsrcInfoIn,
                                          OrbisVideodecCtrl* pCtrlOut) {
    LOG_INFO(Lib_Videodec, "called");

    if (!pCfgInfoIn || !pRsrcInfoIn || !pCtrlOut) {
        LOG_ERROR(Lib_Videodec, "Invalid arguments");
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    if (pCfgInfoIn->thisSize != sizeof(OrbisVideodecConfigInfo) ||
        pRsrcInfoIn->thisSize != sizeof(OrbisVideodecResourceInfo)) {
        LOG_ERROR(Lib_Videodec, "Invalid struct size");
        return ORBIS_VIDEODEC_ERROR_STRUCT_SIZE;
    }

    VdecDecoder* decoder = new VdecDecoder(*pCfgInfoIn, *pRsrcInfoIn);
    pCtrlOut->thisSize = sizeof(OrbisVideodecCtrl);
    pCtrlOut->handle = decoder;
    pCtrlOut->version = 1;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceVideodecDecode(OrbisVideodecCtrl* pCtrlIn,
                                   const OrbisVideodecInputData* pInputDataIn,
                                   OrbisVideodecFrameBuffer* pFrameBufferInOut,
                                   OrbisVideodecPictureInfo* pPictureInfoOut) {
    LOG_TRACE(Lib_Videodec, "called");
    if (!pCtrlIn || !pInputDataIn || !pPictureInfoOut) {
        LOG_ERROR(Lib_Videodec, "Invalid arguments");
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    if (pCtrlIn->thisSize != sizeof(OrbisVideodecCtrl) ||
        pFrameBufferInOut->thisSize != sizeof(OrbisVideodecFrameBuffer)) {
        LOG_ERROR(Lib_Videodec, "Invalid struct size");
        return ORBIS_VIDEODEC_ERROR_STRUCT_SIZE;
    }

    VdecDecoder* decoder = (VdecDecoder*)pCtrlIn->handle;
    if (!decoder) {
        LOG_ERROR(Lib_Videodec, "Invalid decoder handle");
        return ORBIS_VIDEODEC_ERROR_HANDLE;
    }
    return decoder->Decode(*pInputDataIn, *pFrameBufferInOut, *pPictureInfoOut);
}

int PS4_SYSV_ABI sceVideodecDeleteDecoder(OrbisVideodecCtrl* pCtrlIn) {
    LOG_INFO(Lib_Videodec, "(STUBBED) called");

    VdecDecoder* decoder = (VdecDecoder*)pCtrlIn->handle;
    if (!decoder) {
        LOG_ERROR(Lib_Videodec, "Invalid decoder handle");
        return ORBIS_VIDEODEC_ERROR_HANDLE;
    }
    delete decoder;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceVideodecFlush(OrbisVideodecCtrl* pCtrlIn,
                                  OrbisVideodecFrameBuffer* pFrameBufferInOut,
                                  OrbisVideodecPictureInfo* pPictureInfoOut) {
    LOG_INFO(Lib_Videodec, "called");

    if (!pFrameBufferInOut || !pPictureInfoOut) {
        LOG_ERROR(Lib_Videodec, "Invalid arguments");
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    if (pFrameBufferInOut->thisSize != sizeof(OrbisVideodecFrameBuffer) ||
        pPictureInfoOut->thisSize != sizeof(OrbisVideodecPictureInfo)) {
        LOG_ERROR(Lib_Videodec, "Invalid struct size");
        return ORBIS_VIDEODEC_ERROR_STRUCT_SIZE;
    }

    VdecDecoder* decoder = (VdecDecoder*)pCtrlIn->handle;
    if (!decoder) {
        LOG_ERROR(Lib_Videodec, "Invalid decoder handle");
        return ORBIS_VIDEODEC_ERROR_HANDLE;
    }
    return decoder->Flush(*pFrameBufferInOut, *pPictureInfoOut);
}

int PS4_SYSV_ABI sceVideodecMapMemory() {
    LOG_ERROR(Lib_Videodec, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceVideodecQueryResourceInfo(const OrbisVideodecConfigInfo* pCfgInfoIn,
                                              OrbisVideodecResourceInfo* pRsrcInfoOut) {
    LOG_INFO(Lib_Videodec, "called");

    if (!pCfgInfoIn || !pRsrcInfoOut) {
        LOG_ERROR(Lib_Videodec, "Invalid arguments");
        return ORBIS_VIDEODEC_ERROR_ARGUMENT_POINTER;
    }
    if (pCfgInfoIn->thisSize != sizeof(OrbisVideodecConfigInfo) ||
        pRsrcInfoOut->thisSize != sizeof(OrbisVideodecResourceInfo)) {
        LOG_ERROR(Lib_Videodec, "Invalid struct size");
        return ORBIS_VIDEODEC_ERROR_STRUCT_SIZE;
    }

    s32 width = 0;
    s32 height = 0;
    ComputeWorstCaseDimensions(*pCfgInfoIn, width, height);

    const u64 frame_size = ComputeFrameSizeBytes(width, height);
    const s32 dpb_count = ComputeDpbCount(*pCfgInfoIn);

    u64 cpu_gpu_size = 0;
    u64 cpu_size = 0;
    u64 max_frame_buffer = 0;

    if (frame_size == 0) {
        cpu_gpu_size = kFallbackMemorySize;
        cpu_size = kFallbackMemorySize;
        max_frame_buffer = kFallbackMemorySize;
    } else {
        const u64 padded_frame = Common::AlignUp<u64>(frame_size, 256) + 0x4000;
        const u64 surfaces = (u64)dpb_count + 2;

        max_frame_buffer = padded_frame;

        cpu_gpu_size = (padded_frame * surfaces) + 8_MB;
        cpu_size = 16_MB;
    }

    pRsrcInfoOut->thisSize = sizeof(OrbisVideodecResourceInfo);
    pRsrcInfoOut->pCpuMemory = nullptr;
    pRsrcInfoOut->pCpuGpuMemory = nullptr;

    pRsrcInfoOut->cpuGpuMemorySize = cpu_gpu_size;
    pRsrcInfoOut->cpuMemorySize = cpu_size;

    pRsrcInfoOut->maxFrameBufferSize = max_frame_buffer;
    pRsrcInfoOut->frameBufferAlignment = 0x100;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceVideodecReset(OrbisVideodecCtrl* pCtrlIn) {
    LOG_INFO(Lib_Videodec, "(STUBBED) called");

    VdecDecoder* decoder = (VdecDecoder*)pCtrlIn->handle;
    decoder->Reset();
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("qkgRiwHyheU", "libSceVideodec", 1, "libSceVideodec", sceVideodecCreateDecoder);
    LIB_FUNCTION("q0W5GJMovMs", "libSceVideodec", 1, "libSceVideodec", sceVideodecDecode);
    LIB_FUNCTION("U0kpGF1cl90", "libSceVideodec", 1, "libSceVideodec", sceVideodecDeleteDecoder);
    LIB_FUNCTION("jeigLlKdp5I", "libSceVideodec", 1, "libSceVideodec", sceVideodecFlush);
    LIB_FUNCTION("kg+lH0V61hM", "libSceVideodec", 1, "libSceVideodec", sceVideodecMapMemory);
    LIB_FUNCTION("leCAscipfFY", "libSceVideodec", 1, "libSceVideodec",
                 sceVideodecQueryResourceInfo);
    LIB_FUNCTION("f8AgDv-1X8A", "libSceVideodec", 1, "libSceVideodec", sceVideodecReset);
};

} // namespace Libraries::Videodec
