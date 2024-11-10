// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Videodec {

struct OrbisVideodecConfigInfo {
    u64 thisSize;
    u32 codecType;
    u32 profile;
    u32 maxLevel;
    s32 maxFrameWidth;
    s32 maxFrameHeight;
    s32 maxDpbFrameCount;
    u64 videodecFlags;
};

struct OrbisVideodecResourceInfo {
    u64 thisSize;
    u64 cpuMemorySize;
    void* pCpuMemory;
    u64 cpuGpuMemorySize;
    void* pCpuGpuMemory;
    u64 maxFrameBufferSize;
    u32 frameBufferAlignment;
};

struct OrbisVideodecCtrl {
    u64 thisSize;
    void* handle;
    u64 version;
};

struct OrbisVideodecFrameBuffer {
    u64 thisSize;
    void* pFrameBuffer;
    u64 frameBufferSize;
};

struct OrbisVideodecAvcInfo {
    u32 numUnitsInTick;
    u32 timeScale;
    u8 fixedFrameRateFlag;
    u8 aspectRatioIdc;
    u16 sarWidth;
    u16 sarHeight;
    u8 colourPrimaries;
    u8 transferCharacteristics;
    u8 matrixCoefficients;
    u8 videoFullRangeFlag;
    u32 frameCropLeftOffset;
    u32 frameCropRightOffset;
    u32 frameCropTopOffset;
    u32 frameCropBottomOffset;
};

union OrbisVideodecCodecInfo {
    u8 reserved[64];
    OrbisVideodecAvcInfo avc;
};

struct OrbisVideodecPictureInfo {
    u64 thisSize;
    u32 isValid;
    u32 codecType;
    u32 frameWidth;
    u32 framePitch;
    u32 frameHeight;
    u32 isErrorPic;
    u64 ptsData;
    u64 attachedData;
    OrbisVideodecCodecInfo codec;
};

struct OrbisVideodecInputData {
    u64 thisSize;
    void* pAuData;
    u64 auSize;
    u64 ptsData;
    u64 dtsData;
    u64 attachedData;
};

int PS4_SYSV_ABI sceVideodecCreateDecoder(const OrbisVideodecConfigInfo* pCfgInfoIn,
                                          const OrbisVideodecResourceInfo* pRsrcInfoIn,
                                          OrbisVideodecCtrl* pCtrlOut);
int PS4_SYSV_ABI sceVideodecDecode(OrbisVideodecCtrl* pCtrlIn,
                                   const OrbisVideodecInputData* pInputDataIn,
                                   OrbisVideodecFrameBuffer* pFrameBufferInOut,
                                   OrbisVideodecPictureInfo* pPictureInfoOut);
int PS4_SYSV_ABI sceVideodecDeleteDecoder(OrbisVideodecCtrl* pCtrlIn);
int PS4_SYSV_ABI sceVideodecFlush(OrbisVideodecCtrl* pCtrlIn,
                                  OrbisVideodecFrameBuffer* pFrameBufferInOut,
                                  OrbisVideodecPictureInfo* pPictureInfoOut);
int PS4_SYSV_ABI sceVideodecMapMemory();
int PS4_SYSV_ABI sceVideodecQueryResourceInfo(const OrbisVideodecConfigInfo* pCfgInfoIn,
                                              OrbisVideodecResourceInfo* pRsrcInfoOut);
int PS4_SYSV_ABI sceVideodecReset(OrbisVideodecCtrl* pCtrlIn);

void RegisterlibSceVideodec(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Videodec