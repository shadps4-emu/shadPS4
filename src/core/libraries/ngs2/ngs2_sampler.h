// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ngs2.h"

namespace Libraries::Ngs2 {

class Ngs2Sampler;

struct OrbisNgs2SamplerRackOption {
    OrbisNgs2RackOption rackOption;
    u32 maxChannelWorks;
    u32 maxCodecCaches;
    u32 maxWaveformBlocks;
    u32 maxEnvelopePoints;
    u32 maxFilters;
    u32 maxAtrac9Decoders;
    u32 maxAtrac9ChannelWorks;
    u32 maxAjmAtrac9Decoders;
    u32 numPeakMeterBlocks;
};

struct OrbisNgs2SamplerVoiceSetupParam {
    OrbisNgs2VoiceParamHeader header;

    OrbisNgs2WaveformFormat format;
    u32 flags;
    u32 reserved;
};

struct OrbisNgs2SamplerVoiceWaveformBlocksParam {
    OrbisNgs2VoiceParamHeader header;

    const void* data;
    u32 flags;
    u32 numBlocks;
    const OrbisNgs2WaveformBlock* aBlock;
    // Blocks
};

struct OrbisNgs2SamplerVoiceWaveformAddressParam {
    OrbisNgs2VoiceParamHeader header;

    const void* from;
    const void* to;
};

struct OrbisNgs2SamplerVoiceWaveformFrameOffsetParam {
    OrbisNgs2VoiceParamHeader header;

    u32 frameOffset;
    u32 reserved;
};

struct OrbisNgs2SamplerVoiceExitLoopParam {
    OrbisNgs2VoiceParamHeader header;
};

struct OrbisNgs2SamplerVoicePitchParam {
    OrbisNgs2VoiceParamHeader header;

    float ratio;
    u32 reserved;
};

struct OrbisNgs2SamplerVoiceEnvelopeParam {
    OrbisNgs2VoiceParamHeader header;

    u32 numForwardPoints;
    u32 numReleasePoints;
    const OrbisNgs2EnvelopePoint* aPoint;
};

struct OrbisNgs2SamplerVoiceDistortionParam {
    OrbisNgs2VoiceParamHeader header;

    u32 flags;
    float a;
    float b;
    float clip;
    float gate;
    float wetLevel;
    float dryLevel;
    u32 reserved;
};

struct OrbisNgs2SamplerVoiceUserFxParam {
    OrbisNgs2VoiceParamHeader header;

    OrbisNgs2UserFxProcessHandler handler;

    uintptr_t userData0;
    uintptr_t userData1;
    uintptr_t userData2;
};

struct OrbisNgs2SamplerVoicePeakMeterParam {
    OrbisNgs2VoiceParamHeader header;

    u32 enableFlag;
    u32 reserved;
};

struct OrbisNgs2SamplerVoiceFilterParam {
    OrbisNgs2VoiceParamHeader header;

    u32 index;
    u32 location;
    u32 type;
    u32 channelMask;
    union {
        struct {
            float i0;
            float i1;
            float i2;
            float o1;
            float o2;
        } direct;
        struct {
            float fc;
            float q;
            float level;
            u32 reserved;
            u32 reserved2;
        } fcq;
    } param;
    u32 reserved3;
};

struct OrbisNgs2SamplerVoiceNumFilters {
    OrbisNgs2VoiceParamHeader header;

    u32 numFilters;
    u32 reserved;
};

struct OrbisNgs2SamplerVoiceState {
    OrbisNgs2VoiceState voiceState;
    float envelopeHeight;
    float peakHeight;
    u32 reserved;
    u64 numDecodedSamples;
    u64 decodedDataSize;
    u64 userData;
    const void* waveformData;
};

struct OrbisNgs2SamplerRackInfo {
    OrbisNgs2RackInfo rackInfo;
    u32 maxChannelWorks;
    u32 maxCodecCaches;
    u32 maxWaveformBlocks;
    u32 maxEnvelopePoints;
    u32 maxFilters;
    u32 maxAtrac9Decoders;
    u32 maxAtrac9ChannelWorks;
    u32 maxAjmAtrac9Decoders;
};

} // namespace Libraries::Ngs2
