// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

#include "videodec2_avc.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::Videodec2 {

class VdecDecoder;

using OrbisVideodec2Decoder = VdecDecoder*;
using OrbisVideodec2ComputeQueue = void*;

struct OrbisVideodec2DecoderConfigInfo {
    u64 thisSize;
    u32 resourceType;
    u32 codecType;
    u32 profile;
    u32 maxLevel;
    s32 maxFrameWidth;
    s32 maxFrameHeight;
    s32 maxDpbFrameCount;
    u32 decodePipelineDepth;
    OrbisVideodec2ComputeQueue computeQueue;
    u64 cpuAffinityMask;
    s32 cpuThreadPriority;
    bool optimizeProgressiveVideo;
    bool checkMemoryType;
    u8 reserved0;
    u8 reserved1;
    void* extraConfigInfo;
};
static_assert(sizeof(OrbisVideodec2DecoderConfigInfo) == 0x48);

struct OrbisVideodec2DecoderMemoryInfo {
    u64 thisSize;
    u64 cpuMemorySize;
    void* cpuMemory;
    u64 gpuMemorySize;
    void* gpuMemory;
    u64 cpuGpuMemorySize;
    void* cpuGpuMemory;
    u64 maxFrameBufferSize;
    u32 frameBufferAlignment;
    u32 reserved0;
};
static_assert(sizeof(OrbisVideodec2DecoderMemoryInfo) == 0x48);

struct OrbisVideodec2InputData {
    u64 thisSize;
    void* auData;
    u64 auSize;
    u64 ptsData;
    u64 dtsData;
    u64 attachedData;
};
static_assert(sizeof(OrbisVideodec2InputData) == 0x30);

struct OrbisVideodec2OutputInfo {
    u64 thisSize;
    bool isValid;
    bool isErrorFrame;
    u8 pictureCount;
    u32 codecType;
    u32 frameWidth;
    u32 framePitch;
    u32 frameHeight;
    void* frameBuffer;
    u64 frameBufferSize;
    u32 frameFormat;
    u32 framePitchInBytes;
};
static_assert(sizeof(OrbisVideodec2OutputInfo) == 0x38);

struct OrbisVideodec2FrameBuffer {
    u64 thisSize;
    void* frameBuffer;
    u64 frameBufferSize;
    bool isAccepted;
};
static_assert(sizeof(OrbisVideodec2FrameBuffer) == 0x20);

struct OrbisVideodec2ComputeMemoryInfo {
    u64 thisSize;
    u64 cpuGpuMemorySize;
    void* cpuGpuMemory;
};
static_assert(sizeof(OrbisVideodec2ComputeMemoryInfo) == 0x18);

struct OrbisVideodec2ComputeConfigInfo {
    u64 thisSize;
    u16 computePipeId;
    u16 computeQueueId;
    bool checkMemoryType;
    u8 reserved0;
    u16 reserved1;
};
static_assert(sizeof(OrbisVideodec2ComputeConfigInfo) == 0x10);

s32 PS4_SYSV_ABI
sceVideodec2QueryComputeMemoryInfo(OrbisVideodec2ComputeMemoryInfo* computeMemInfo);

s32 PS4_SYSV_ABI
sceVideodec2AllocateComputeQueue(const OrbisVideodec2ComputeConfigInfo* computeCfgInfo,
                                 const OrbisVideodec2ComputeMemoryInfo* computeMemInfo,
                                 OrbisVideodec2ComputeQueue* computeQueue);

s32 PS4_SYSV_ABI sceVideodec2ReleaseComputeQueue(OrbisVideodec2ComputeQueue computeQueue);

s32 PS4_SYSV_ABI
sceVideodec2QueryDecoderMemoryInfo(const OrbisVideodec2DecoderConfigInfo* decoderCfgInfo,
                                   OrbisVideodec2DecoderMemoryInfo* decoderMemInfo);

s32 PS4_SYSV_ABI sceVideodec2CreateDecoder(const OrbisVideodec2DecoderConfigInfo* decoderCfgInfo,
                                           const OrbisVideodec2DecoderMemoryInfo* decoderMemInfo,
                                           OrbisVideodec2Decoder* decoder);

s32 PS4_SYSV_ABI sceVideodec2DeleteDecoder(OrbisVideodec2Decoder decoder);

s32 PS4_SYSV_ABI sceVideodec2Decode(OrbisVideodec2Decoder decoder,
                                    const OrbisVideodec2InputData* inputData,
                                    OrbisVideodec2FrameBuffer* frameBuffer,
                                    OrbisVideodec2OutputInfo* outputInfo);

s32 PS4_SYSV_ABI sceVideodec2Flush(OrbisVideodec2Decoder decoder,
                                   OrbisVideodec2FrameBuffer* frameBuffer,
                                   OrbisVideodec2OutputInfo* outputInfo);

s32 PS4_SYSV_ABI sceVideodec2Reset(OrbisVideodec2Decoder decoder);

s32 PS4_SYSV_ABI sceVideodec2GetPictureInfo(const OrbisVideodec2OutputInfo* outputInfo,
                                            void* p1stPictureInfo, void* p2ndPictureInfo);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Videodec2