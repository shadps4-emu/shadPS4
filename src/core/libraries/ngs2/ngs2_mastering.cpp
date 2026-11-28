// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

#include "core/libraries/error_codes.h"
#include "core/libraries/ngs2/ngs2_error.h"
#include "core/libraries/ngs2/ngs2_runtime.h"

namespace Libraries::Ngs2 {

namespace {

constexpr u32 WaveformPcmI16LittleEndian = 0x12;
constexpr u32 WaveformPcmI16BigEndian = 0x13;
constexpr u32 WaveformPcmF32LittleEndian = 0x18;
constexpr u32 WaveformPcmF32BigEndian = 0x19;

} // namespace

s32 MasteringVoice::Setup(u32 inputChannels) {
    if (inputChannels < 1 || inputChannels > MaxVoiceChannels) {
        return ORBIS_NGS2_ERROR_INVALID_NUM_CHANNELS;
    }

    Reset();
    numInputChannels = inputChannels;
    setup = true;
    return ORBIS_OK;
}

void MasteringVoice::Reset() {
    setup = false;
    playing = false;
    numInputChannels = 0;
    input.clear();
}

void MasteringVoice::BeginGrain(u32 numFrames) {
    input.assign(static_cast<size_t>(numFrames) * numInputChannels, 0.0f);
}

void MasteringVoice::Mix(std::span<const float> samples, u32 numChannels, u32 numFrames,
                         float volume, std::span<const float> matrix) {
    if (matrix.size() != static_cast<size_t>(numChannels) * numInputChannels) {
        MixChannels(samples, numChannels, input, numInputChannels, numFrames, volume);
        return;
    }

    for (u32 frame = 0; frame < numFrames; ++frame) {
        for (u32 in = 0; in < numChannels; ++in) {
            const float sample = samples[static_cast<size_t>(frame) * numChannels + in] * volume;
            for (u32 out = 0; out < numInputChannels; ++out) {
                input[static_cast<size_t>(frame) * numInputChannels + out] +=
                    sample * matrix[static_cast<size_t>(in) * numInputChannels + out];
            }
        }
    }
}

void MasteringVoice::Output(std::span<float> out, u32 outChannels, u32 numFrames) const {
    MixChannels(input, numInputChannels, out, outChannels, numFrames, 1.0f);
}

void MixChannels(std::span<const float> src, u32 srcChannels, std::span<float> dst, u32 dstChannels,
                 u32 numFrames, float volume) {
    if (srcChannels == 0 || dstChannels == 0 ||
        src.size() < static_cast<size_t>(numFrames) * srcChannels ||
        dst.size() < static_cast<size_t>(numFrames) * dstChannels) {
        return;
    }

    for (u32 frame = 0; frame < numFrames; ++frame) {
        const float* in = src.data() + static_cast<size_t>(frame) * srcChannels;
        float* out = dst.data() + static_cast<size_t>(frame) * dstChannels;
        if (srcChannels == dstChannels) {
            for (u32 channel = 0; channel < dstChannels; ++channel) {
                out[channel] += in[channel] * volume;
            }
        } else if (srcChannels == 1) {
            out[0] += in[0] * volume;
            if (dstChannels > 1) {
                out[1] += in[0] * volume;
            }
        } else if (dstChannels == 1) {
            out[0] += (in[0] + in[1]) * 0.5f * volume;
        } else {
            for (u32 channel = 0; channel < std::min(srcChannels, dstChannels); ++channel) {
                out[channel] += in[channel] * volume;
            }
        }
    }
}

void ConvertRenderOutput(std::span<const float> src, void* dst, u32 waveformType, u32 numSamples) {
    for (u32 i = 0; i < numSamples; ++i) {
        const float value = src[i];
        switch (waveformType) {
        case WaveformPcmI16LittleEndian:
        case WaveformPcmI16BigEndian: {
            const float scaled = std::clamp(std::nearbyint(value * 32768.0f), -32768.0f, 32767.0f);
            u16 sample = static_cast<u16>(static_cast<s16>(scaled));
            if (waveformType == WaveformPcmI16BigEndian) {
                sample = std::byteswap(sample);
            }
            std::memcpy(static_cast<u8*>(dst) + i * sizeof(sample), &sample, sizeof(sample));
            break;
        }
        case WaveformPcmF32LittleEndian:
        case WaveformPcmF32BigEndian: {
            u32 bits = std::bit_cast<u32>(value);
            if (waveformType == WaveformPcmF32BigEndian) {
                bits = std::byteswap(bits);
            }
            std::memcpy(static_cast<u8*>(dst) + i * sizeof(bits), &bits, sizeof(bits));
            break;
        }
        default:
            break;
        }
    }
}

} // namespace Libraries::Ngs2
