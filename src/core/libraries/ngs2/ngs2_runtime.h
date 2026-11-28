// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>
#include <span>
#include <vector>

#include "common/types.h"
#include "core/libraries/ngs2/ngs2.h"

namespace Libraries::Ngs2 {

enum class VoiceEvent : u32 {
    Play = 0,
    Stop = 1,
    StopImmediately = 2,
    Kill = 3,
    Pause = 4,
    Resume = 5,
};

enum class VoiceParamId : u32 {
    MatrixLevels = 1,
    PortVolume = 2,
    PortMatrix = 3,
    PortDelay = 4,
    Patch = 5,
    Event = 6,
    Callback = 7,
    SamplerSetup = 0x10000000,
    SamplerWaveformBlocks = 0x10000001,
    MasteringSetup = 0x30000000,
};

enum class WaveformType : u32 {
    PcmI16LittleEndian = 0x12,
    PcmI16BigEndian = 0x13,
    PcmI24LittleEndian = 0x14,
    PcmI24BigEndian = 0x15,
    PcmI32LittleEndian = 0x16,
    PcmI32BigEndian = 0x17,
    PcmF32LittleEndian = 0x18,
    PcmF32BigEndian = 0x19,
};

constexpr u32 MaxVoiceChannels = 8;
constexpr u32 InfiniteRepeats = 0xFFFFFFFF;

struct VoicePort {
    float volume{1.0f};
    s32 matrixId{-1};
    u32 delaySamples{};
    OrbisNgs2Handle destHandle{};
    u32 destInputId{};
};

struct SamplerBlock {
    const u8* data{};
    u32 numFrames{};
    u32 numRepeats{};
    double position{};
    u32 countedFrames{};
};

class SamplerVoice {
public:
    s32 Setup(const OrbisNgs2WaveformFormat& format);
    s32 AddBlocks(const void* data, u32 numBlocks, const OrbisNgs2WaveformBlock* blocks);
    void Reset();
    void Stop();
    void Render(std::span<float> out, u32 numFrames, u32 systemSampleRate);

    bool IsSetup() const {
        return setup;
    }

    u32 NumChannels() const {
        return format.numChannels;
    }

    u64 NumDecodedSamples() const {
        return numDecodedSamples;
    }

    u64 DecodedDataSize() const {
        return numDecodedSamples * frameBytes;
    }

    const void* WaveformData() const {
        return waveformData;
    }

    bool playing{};
    bool paused{};

private:
    bool setup{};
    OrbisNgs2WaveformFormat format{};
    u32 sampleBytes{};
    u32 frameBytes{};
    u64 numDecodedSamples{};
    const void* waveformData{};
    std::deque<SamplerBlock> blocks;
};

class MasteringVoice {
public:
    s32 Setup(u32 numInputChannels);
    void Reset();
    void BeginGrain(u32 numFrames);
    void Mix(std::span<const float> samples, u32 numChannels, u32 numFrames, float volume,
             std::span<const float> matrix);
    void Output(std::span<float> out, u32 outChannels, u32 numFrames) const;

    bool IsSetup() const {
        return setup;
    }

    bool playing{};

private:
    bool setup{};
    u32 numInputChannels{};
    std::vector<float> input;
};

struct VoiceRuntime {
    std::vector<VoicePort> ports;
    std::vector<std::vector<float>> matrices;
    SamplerVoice sampler;
    MasteringVoice mastering;
};

u32 WaveformSampleBytes(WaveformType type);
float ReadWaveformSample(const u8* sample, WaveformType type);
void MixChannels(std::span<const float> src, u32 srcChannels, std::span<float> dst, u32 dstChannels,
                 u32 numFrames, float volume);
void ConvertRenderOutput(std::span<const float> src, void* dst, u32 waveformType, u32 numSamples);

} // namespace Libraries::Ngs2
