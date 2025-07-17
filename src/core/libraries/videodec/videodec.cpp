// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/videodec/videodec.h"
#include "core/libraries/videodec/videodec_error.h"
#include "core/libraries/videodec/videodec_impl.h"

namespace Libraries::Videodec {

static constexpr u64 kMinimumMemorySize = 16_MB; ///> Fake minimum memory size for querying

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
    pCtrlOut->version = 1; //???
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

    pRsrcInfoOut->thisSize = sizeof(OrbisVideodecResourceInfo);
    pRsrcInfoOut->pCpuMemory = nullptr;
    pRsrcInfoOut->pCpuGpuMemory = nullptr;

    pRsrcInfoOut->cpuGpuMemorySize = kMinimumMemorySize;
    pRsrcInfoOut->cpuMemorySize = kMinimumMemorySize;

    pRsrcInfoOut->maxFrameBufferSize = kMinimumMemorySize;
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
    LIB_FUNCTION("qkgRiwHyheU", "libSceVideodec", 1, "libSceVideodec", 1, 1,
                 sceVideodecCreateDecoder);
    LIB_FUNCTION("q0W5GJMovMs", "libSceVideodec", 1, "libSceVideodec", 1, 1, sceVideodecDecode);
    LIB_FUNCTION("U0kpGF1cl90", "libSceVideodec", 1, "libSceVideodec", 1, 1,
                 sceVideodecDeleteDecoder);
    LIB_FUNCTION("jeigLlKdp5I", "libSceVideodec", 1, "libSceVideodec", 1, 1, sceVideodecFlush);
    LIB_FUNCTION("kg+lH0V61hM", "libSceVideodec", 1, "libSceVideodec", 1, 1, sceVideodecMapMemory);
    LIB_FUNCTION("leCAscipfFY", "libSceVideodec", 1, "libSceVideodec", 1, 1,
                 sceVideodecQueryResourceInfo);
    LIB_FUNCTION("f8AgDv-1X8A", "libSceVideodec", 1, "libSceVideodec", 1, 1, sceVideodecReset);
};

} // namespace Libraries::Videodec
