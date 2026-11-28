// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <cstring>

#include "core/libraries/error_codes.h"
#include "core/libraries/ngs2/ngs2_error.h"
#include "core/libraries/ngs2/ngs2_runtime.h"

namespace Libraries::Ngs2 {

namespace {

constexpr u32 MaxWaveformBlocks = 16;

bool IsSupportedWaveformType(u32 type) {
    switch (static_cast<WaveformType>(type)) {
    case WaveformType::PcmI16LittleEndian:
    case WaveformType::PcmI16BigEndian:
    case WaveformType::PcmI24LittleEndian:
    case WaveformType::PcmI24BigEndian:
    case WaveformType::PcmI32LittleEndian:
    case WaveformType::PcmI32BigEndian:
    case WaveformType::PcmF32LittleEndian:
    case WaveformType::PcmF32BigEndian:
        return true;
    default:
        return false;
    }
}

} // namespace

u32 WaveformSampleBytes(WaveformType type) {
    switch (type) {
    case WaveformType::PcmI16LittleEndian:
    case WaveformType::PcmI16BigEndian:
        return 2;
    case WaveformType::PcmI24LittleEndian:
    case WaveformType::PcmI24BigEndian:
        return 3;
    default:
        return 4;
    }
}

float ReadWaveformSample(const u8* sample, WaveformType type) {
    switch (type) {
    case WaveformType::PcmI16LittleEndian: {
        s16 value;
        std::memcpy(&value, sample, sizeof(value));
        return static_cast<float>(value) / 32768.0f;
    }
    case WaveformType::PcmI16BigEndian: {
        u16 value;
        std::memcpy(&value, sample, sizeof(value));
        return static_cast<float>(static_cast<s16>(std::byteswap(value))) / 32768.0f;
    }
    case WaveformType::PcmI24LittleEndian: {
        const s32 value = sample[0] | (sample[1] << 8) | (sample[2] << 16);
        return static_cast<float>((value ^ 0x800000) - 0x800000) / 8388608.0f;
    }
    case WaveformType::PcmI24BigEndian: {
        const s32 value = sample[2] | (sample[1] << 8) | (sample[0] << 16);
        return static_cast<float>((value ^ 0x800000) - 0x800000) / 8388608.0f;
    }
    case WaveformType::PcmI32LittleEndian: {
        s32 value;
        std::memcpy(&value, sample, sizeof(value));
        return static_cast<float>(static_cast<double>(value) / 2147483648.0);
    }
    case WaveformType::PcmI32BigEndian: {
        u32 value;
        std::memcpy(&value, sample, sizeof(value));
        return static_cast<float>(static_cast<double>(static_cast<s32>(std::byteswap(value))) /
                                  2147483648.0);
    }
    case WaveformType::PcmF32LittleEndian: {
        float value;
        std::memcpy(&value, sample, sizeof(value));
        return value;
    }
    case WaveformType::PcmF32BigEndian: {
        u32 bits;
        std::memcpy(&bits, sample, sizeof(bits));
        return std::bit_cast<float>(std::byteswap(bits));
    }
    default:
        return 0.0f;
    }
}

s32 SamplerVoice::Setup(const OrbisNgs2WaveformFormat& waveformFormat) {
    if (!IsSupportedWaveformType(waveformFormat.waveformType)) {
        return ORBIS_NGS2_ERROR_INVALID_WAVEFORM_TYPE;
    }
    if (waveformFormat.numChannels < 1 || waveformFormat.numChannels > MaxVoiceChannels) {
        return ORBIS_NGS2_ERROR_INVALID_NUM_CHANNELS;
    }
    if (waveformFormat.sampleRate == 0) {
        return ORBIS_NGS2_ERROR_INVALID_SAMPLE_RATE;
    }

    Reset();
    format = waveformFormat;
    sampleBytes = WaveformSampleBytes(static_cast<WaveformType>(format.waveformType));
    frameBytes = sampleBytes * format.numChannels;
    setup = true;
    return ORBIS_OK;
}

s32 SamplerVoice::AddBlocks(const void* data, u32 numBlocks, const OrbisNgs2WaveformBlock* aBlock) {
    if (!setup) {
        return ORBIS_NGS2_ERROR_UNINIT_VOICE;
    }
    if (numBlocks < 1 || numBlocks > MaxWaveformBlocks) {
        return ORBIS_NGS2_ERROR_INVALID_NUM_WAVEFORM_BLOCKS;
    }
    if (!aBlock || !data) {
        return ORBIS_NGS2_ERROR_INVALID_WAVEFORM_BLOCK_ADDRESS;
    }

    waveformData = data;
    for (const auto& block : std::span{aBlock, numBlocks}) {
        const u32 availableFrames = block.dataSize / frameBytes;
        const u32 numFrames = block.numSamples == 0 || block.numSamples == InfiniteRepeats
                                  ? availableFrames
                                  : std::min(block.numSamples, availableFrames);
        const u32 skipFrames = std::min(block.numSkipSamples, numFrames);
        blocks.push_back({.data = static_cast<const u8*>(data) + block.dataOffset,
                          .numFrames = numFrames,
                          .numRepeats = block.numRepeats,
                          .position = static_cast<double>(skipFrames),
                          .countedFrames = skipFrames});
    }
    return ORBIS_OK;
}

void SamplerVoice::Reset() {
    setup = false;
    playing = false;
    paused = false;
    format = {};
    sampleBytes = 0;
    frameBytes = 0;
    numDecodedSamples = 0;
    waveformData = nullptr;
    blocks.clear();
}

void SamplerVoice::Stop() {
    playing = false;
    paused = false;
    blocks.clear();
}

void SamplerVoice::Render(std::span<float> out, u32 numFrames, u32 systemSampleRate) {
    std::ranges::fill(out, 0.0f);
    if (!setup || !playing || paused || systemSampleRate == 0) {
        return;
    }

    const u32 channels = format.numChannels;
    const auto type = static_cast<WaveformType>(format.waveformType);
    const double step = static_cast<double>(format.sampleRate) / systemSampleRate;

    for (u32 frame = 0; frame < numFrames; ++frame) {
        while (!blocks.empty() && blocks.front().position >= blocks.front().numFrames) {
            auto& finished = blocks.front();
            const double carry = finished.position - finished.numFrames;
            if (finished.numFrames > 0 &&
                (finished.numRepeats == InfiniteRepeats || finished.numRepeats > 0)) {
                if (finished.numRepeats != InfiniteRepeats) {
                    --finished.numRepeats;
                }
                finished.position = carry;
                finished.countedFrames = 0;
            } else {
                blocks.pop_front();
                if (!blocks.empty()) {
                    blocks.front().position += carry;
                }
            }
        }
        if (blocks.empty()) {
            break;
        }

        auto& block = blocks.front();
        const u32 index = static_cast<u32>(block.position);
        const float fraction = static_cast<float>(block.position - index);
        const u8* current = block.data + static_cast<size_t>(index) * frameBytes;
        const bool hasNext = index + 1 < block.numFrames;
        for (u32 channel = 0; channel < channels; ++channel) {
            const float first = ReadWaveformSample(current + channel * sampleBytes, type);
            const float second =
                hasNext ? ReadWaveformSample(current + frameBytes + channel * sampleBytes, type)
                        : first;
            out[static_cast<size_t>(frame) * channels + channel] =
                first + (second - first) * fraction;
        }

        block.position += step;
        const u32 reached = std::min(static_cast<u32>(block.position), block.numFrames);
        numDecodedSamples += reached - block.countedFrames;
        block.countedFrames = reached;
    }
}

} // namespace Libraries::Ngs2
