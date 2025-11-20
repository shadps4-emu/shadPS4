// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/ngs2/ngs2_impl.h"

#include <atomic>
#include <mutex>
#include <vector>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ngs2 {

using OrbisNgs2ParseReadHandler = s32 PS4_SYSV_ABI (*)(uintptr_t user_data, u32 offset, void* data,
                                                       size_t size);

enum class OrbisNgs2HandleType : u32 {
    Invalid = 0,
    System = 1,
    Rack = 2,
    Voice = 3,
    VoiceControl = 6
};

static const int ORBIS_NGS2_MAX_VOICE_CHANNELS = 8;
static const int ORBIS_NGS2_WAVEFORM_INFO_MAX_BLOCKS = 4;
static const int ORBIS_NGS2_MAX_MATRIX_LEVELS =
    (ORBIS_NGS2_MAX_VOICE_CHANNELS * ORBIS_NGS2_MAX_VOICE_CHANNELS);

struct OrbisNgs2WaveformFormat {
    u32 waveformType;
    u32 numChannels;
    u32 sampleRate;
    u32 configData;
    u32 frameOffset;
    u32 frameMargin;
};

struct OrbisNgs2WaveformBlock {
    u32 dataOffset;
    u32 dataSize;
    u32 numRepeats;
    u32 numSkipSamples;
    u32 numSamples;
    u32 reserved;
    uintptr_t userData;
};

struct OrbisNgs2WaveformInfo {
    OrbisNgs2WaveformFormat format;

    u32 dataOffset;
    u32 dataSize;

    u32 loopBeginPosition;
    u32 loopEndPosition;
    u32 numSamples;

    u32 audioUnitSize;
    u32 numAudioUnitSamples;
    u32 numAudioUnitPerFrame;

    u32 audioFrameSize;
    u32 numAudioFrameSamples;

    u32 numDelaySamples;

    u32 numBlocks;
    OrbisNgs2WaveformBlock aBlock[ORBIS_NGS2_WAVEFORM_INFO_MAX_BLOCKS];
};

struct OrbisNgs2EnvelopePoint {
    u32 curve;
    u32 duration;
    float height;
};

struct OrbisNgs2UserFxProcessContext {
    float** aChannelData;
    uintptr_t userData0;
    uintptr_t userData1;
    uintptr_t userData2;
    u32 flags;
    u32 numChannels;
    u32 numGrainSamples;
    u32 sampleRate;
};

using OrbisNgs2UserFxProcessHandler = s32 PS4_SYSV_ABI (*)(OrbisNgs2UserFxProcessContext* context);

struct OrbisNgs2UserFx2SetupContext {
    void* common;
    void* param;
    void* work;
    uintptr_t userData;
    u32 maxVoices;
    u32 voiceIndex;
    u64 reserved[4];
};

using OrbisNgs2UserFx2SetupHandler = s32 PS4_SYSV_ABI (*)(OrbisNgs2UserFx2SetupContext* context);

struct OrbisNgs2UserFx2CleanupContext {
    void* common;
    void* param;
    void* work;
    uintptr_t userData;
    u32 maxVoices;
    u32 voiceIndex;
    u64 reserved[4];
};

using OrbisNgs2UserFx2CleanupHandler =
    s32 PS4_SYSV_ABI (*)(OrbisNgs2UserFx2CleanupContext* context);

struct OrbisNgs2UserFx2ControlContext {
    const void* data;
    size_t dataSize;
    void* common;
    void* param;
    uintptr_t userData;
    u64 reserved[4];
};

using OrbisNgs2UserFx2ControlHandler =
    s32 PS4_SYSV_ABI (*)(OrbisNgs2UserFx2ControlContext* context);

struct OrbisNgs2UserFx2ProcessContext {
    float** aChannelData;
    void* common;
    const void* param;
    void* work;
    void* state;
    uintptr_t userData;
    u32 flags;
    u32 numInputChannels;
    u32 numOutputChannels;
    u32 numGrainSamples;
    u32 sampleRate;
    u32 reserved;
    u64 reserved2[4];
};

using OrbisNgs2UserFx2ProcessHandler =
    s32 PS4_SYSV_ABI (*)(OrbisNgs2UserFx2ProcessContext* context);

struct OrbisNgs2BufferAllocator {
    OrbisNgs2BufferAllocHandler allocHandler;
    OrbisNgs2BufferFreeHandler freeHandler;
    uintptr_t userData;
};

struct OrbisNgs2RenderBufferInfo {
    void* buffer;
    size_t bufferSize;
    u32 waveformType;
    u32 numChannels;
};

struct OrbisNgs2RackOption {
    size_t size;
    char name[ORBIS_NGS2_RACK_NAME_LENGTH];

    u32 flags;
    u32 maxGrainSamples;
    u32 maxVoices;
    u32 maxInputDelayBlocks;
    u32 maxMatrices;
    u32 maxPorts;
    u32 aReserved[20];
};

struct OrbisNgs2VoiceParamHeader {
    u16 size;
    s16 next;
    u32 id;
};

struct OrbisNgs2VoiceMatrixLevelsParam {
    OrbisNgs2VoiceParamHeader header;

    u32 matrixId;
    u32 numLevels;
    const float* aLevel;
};

struct OrbisNgs2VoicePortMatrixParam {
    OrbisNgs2VoiceParamHeader header;

    u32 port;
    s32 matrixId;
};

struct OrbisNgs2VoicePortVolumeParam {
    OrbisNgs2VoiceParamHeader header;

    u32 port;
    float level;
};

struct OrbisNgs2VoicePortDelayParam {
    OrbisNgs2VoiceParamHeader header;

    u32 port;
    u32 numSamples;
};

struct OrbisNgs2VoicePatchParam {
    OrbisNgs2VoiceParamHeader header;

    u32 port;
    u32 destInputId;
    OrbisNgs2Handle destHandle;
};

struct OrbisNgs2VoiceEventParam {
    OrbisNgs2VoiceParamHeader header;

    u32 eventId;
};

struct OrbisNgs2VoiceCallbackInfo {
    uintptr_t callbackData;
    OrbisNgs2Handle voiceHandle;
    u32 flag;
    u32 reserved;
    union {
        struct {
            uintptr_t userData;
            const void* data;
            u32 dataSize;
            u32 repeatedCount;
            u32 attributeFlags;
            u32 reserved2;
        } waveformBlock;
    } param;
};

using OrbisNgs2VoiceCallbackHandler = void PS4_SYSV_ABI (*)(const OrbisNgs2VoiceCallbackInfo* info);

struct OrbisNgs2VoiceCallbackParam {
    OrbisNgs2VoiceParamHeader header;
    OrbisNgs2VoiceCallbackHandler callbackHandler;

    uintptr_t callbackData;
    u32 flags;
    u32 reserved;
};

struct OrbisNgs2VoicePortInfo {
    s32 matrixId;
    float volume;
    u32 numDelaySamples;
    u32 destInputId;
    OrbisNgs2Handle destHandle;
};

struct OrbisNgs2VoiceMatrixInfo {
    u32 numLevels;
    float aLevel[ORBIS_NGS2_MAX_MATRIX_LEVELS];
};

struct OrbisNgs2VoiceState {
    u32 stateFlags;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Ngs2
