// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>
#include <cstring>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/ngs2/ngs2.h"
#include "core/libraries/ngs2/ngs2_custom.h"
#include "core/libraries/ngs2/ngs2_error.h"
#include "core/libraries/ngs2/ngs2_geom.h"
#include "core/libraries/ngs2/ngs2_impl.h"
#include "core/libraries/ngs2/ngs2_internal.h"
#include "core/libraries/ngs2/ngs2_pan.h"
#include "core/libraries/ngs2/ngs2_report.h"
#include "core/libraries/ngs2/ngs2_sampler.h"

namespace Libraries::Ngs2 {

// =============================================================================
// Audio Decoder Interface
// =============================================================================

// Waveform type constants
// Add new formats here as they are discovered/implemented
enum WaveformType : u32 {
    PCM16 = 0x12, // 16-bit PCM little-endian (confirmed working)
    // TODO: Add more formats
};

// Check if waveform type is supported
static bool IsWaveformTypeSupported(u32 waveformType) {
    switch (waveformType) {
    case WaveformType::PCM16:
        return true;
    // TODO: Add cases for new formats here
    default:
        return false;
    }
}

// Get bytes per sample for a waveform type
static u32 GetBytesPerSample(u32 waveformType, u32 numChannels) {
    switch (waveformType) {
    case WaveformType::PCM16:
        return 2 * numChannels;
    // TODO: Add cases for new formats here
    // case WaveformType::FLOAT:
    //     return 4 * numChannels;
    default:
        return 2 * numChannels;
    }
}

// =============================================================================
// PCM16 Decoder
// =============================================================================

static float DecodePCM16Sample(const void* data, u32 sampleIdx, u32 channel, u32 numChannels,
                               u32 totalSamples) {
    if (sampleIdx >= totalSamples) {
        return 0.0f;
    }

    const s16* srcData = reinterpret_cast<const s16*>(data);
    s16 sample = srcData[sampleIdx * numChannels + channel];

    return static_cast<float>(sample) / 32768.0f;
}

// =============================================================================
// Add new decoder functions here
// =============================================================================
// Example:
// static float DecodeFloatSample(const void* data, u32 sampleIdx, u32 channel,
//                                u32 numChannels, u32 totalSamples) {
//     const float* srcData = reinterpret_cast<const float*>(data);
//     return srcData[sampleIdx * numChannels + channel];
// }

// =============================================================================
// Unified Sample Decoder
// =============================================================================

static float DecodeSample(const void* data, u32 sampleIdx, u32 channel, u32 numChannels,
                          u32 totalSamples, u32 waveformType) {
    switch (waveformType) {
    case WaveformType::PCM16:
        return DecodePCM16Sample(data, sampleIdx, channel, numChannels, totalSamples);
    // TODO: Add cases for new formats here
    default:
        return 0.0f;
    }
}

// =============================================================================
// Voice Renderer
// =============================================================================

struct RenderContext {
    const OrbisNgs2RenderBufferInfo* bufferInfo;
    u32 numBufferInfo;
    u32 grainSamples;
    u32 outputSampleRate;
};

static bool RenderVoice(VoiceInternal* voice, const RenderContext& ctx) {
    if (!voice || !voice->isSetup) {
        LOG_DEBUG(Lib_Ngs2, "(STUBBED) Voice not setup or invalid");
        return false;
    }

    // Check if voice is playing
    if (!(voice->stateFlags & 0x01)) {
        LOG_DEBUG(Lib_Ngs2, "(STUBBED) Voice not playing");
        return false;
    }

    // Check if voice is paused, stopped, or killed
    if (voice->stateFlags & (0x200 | 0x400 | 0x800)) {
        LOG_DEBUG(Lib_Ngs2, "(STUBBED) Voice paused, stopped, or killed");
        return false;
    }

    u32 waveformType = voice->format.waveformType;
    if (!IsWaveformTypeSupported(waveformType)) {
        LOG_DEBUG(Lib_Ngs2, "(STUBBED) Unsupported waveform type: {:#x}", waveformType);
        return false;
    }

    // Get current slot from ring buffer
    RingBufferSlot* currentSlot = voice->getCurrentSlot();
    if (!currentSlot || !currentSlot->valid || currentSlot->consumed) {
        LOG_DEBUG(Lib_Ngs2, "(STUBBED) No valid buffer in ring");
        return false;
    }

    voice->currentBufferPtr = currentSlot->basePtr;

    u32 numChannels = voice->format.numChannels;
    if (numChannels == 0 || numChannels > 8) {
        LOG_DEBUG(Lib_Ngs2, "(STUBBED) Invalid number of channels: {}", numChannels);
        return false;
    }

    // Calculate sample rate ratio for resampling
    u32 sourceSampleRate = voice->format.sampleRate;
    if (sourceSampleRate == 0) {
        sourceSampleRate = 48000;
    }

    float pitchRatio = voice->pitchRatio;
    if (pitchRatio <= 0.0f) {
        pitchRatio = 1.0f;
    }

    float sampleRateRatio = (static_cast<float>(sourceSampleRate) * pitchRatio) /
                            static_cast<float>(ctx.outputSampleRate);

    // Find matching output buffer and render
    for (u32 bufIdx = 0; bufIdx < ctx.numBufferInfo; bufIdx++) {
        const auto& bufInfo = ctx.bufferInfo[bufIdx];
        if (!bufInfo.buffer || bufInfo.bufferSize == 0) {
            continue;
        }

        const void* srcData = currentSlot->data;
        u32 totalSamples = currentSlot->numSamples;
        if (totalSamples == 0) {
            continue;
        }

        // Determine output format
        // Output buffer format detection - use raw values since we only know PCM16 for sure
        float* dstFloat = nullptr;
        s16* dstS16 = nullptr;
        u32 dstChannels = 2;
        bool outputIsFloat = false;

        if (bufInfo.waveformType == WaveformType::PCM16) {
            dstS16 = reinterpret_cast<s16*>(bufInfo.buffer);
            dstChannels = bufInfo.numChannels > 0 ? bufInfo.numChannels : numChannels;
            outputIsFloat = false;
        } else {
            // Default to float output for unknown types
            dstFloat = reinterpret_cast<float*>(bufInfo.buffer);
            dstChannels = bufInfo.numChannels > 0 ? bufInfo.numChannels : 2;
            outputIsFloat = true;
        }

        // Render samples
        float currentPos = voice->samplePosFloat;
        bool voiceStopped = false;

        for (u32 outSample = 0; outSample < ctx.grainSamples && !voiceStopped; outSample++) {
            u32 sampleInt = static_cast<u32>(currentPos);
            float frac = currentPos - static_cast<float>(sampleInt);

            // Check if we've reached the end of current buffer
            if (sampleInt >= totalSamples) {
                voice->lastConsumedBuffer = currentSlot->basePtr;
                voice->totalDecodedSamples += totalSamples;
                voice->advanceReadIndex();

                if (voice->getReadyBufferCount() <= VoiceInternal::STARVATION_THRESHOLD) {
                    voice->stateFlags |= 0x80;
                }

                currentSlot = voice->getCurrentSlot();
                if (currentSlot && currentSlot->valid && !currentSlot->consumed) {
                    srcData = currentSlot->data;
                    totalSamples = currentSlot->numSamples;
                    currentPos = 0.0f;
                    sampleInt = 0;
                    frac = 0.0f;
                    voice->isStreaming = true;
                    voice->currentBufferPtr = currentSlot->basePtr;
                } else {
                    if (voice->isStreaming) {
                        currentPos = 0.0f;
                        break;
                    } else {
                        voice->stateFlags &= ~0x01;
                        voice->stateFlags |= 0x400;
                        voiceStopped = true;
                        break;
                    }
                }
            }

            // Decode and interpolate samples for each output channel
            for (u32 ch = 0; ch < dstChannels; ch++) {
                u32 srcCh = ch < numChannels ? ch : 0;

                float sample0 = DecodeSample(srcData, sampleInt, srcCh, numChannels, totalSamples,
                                             waveformType);
                float sample1 = DecodeSample(srcData, sampleInt + 1, srcCh, numChannels,
                                             totalSamples, waveformType);
                float sample = sample0 + frac * (sample1 - sample0);

                // Apply port volume
                sample *= voice->portVolume;

                // Write to output buffer
                u32 dstIdx = outSample * dstChannels + ch;
                if (outputIsFloat) {
                    if (dstIdx * sizeof(float) < bufInfo.bufferSize) {
                        dstFloat[dstIdx] += sample;
                        dstFloat[dstIdx] = std::clamp(dstFloat[dstIdx], -1.0f, 1.0f);
                    }
                } else {
                    if (dstIdx * 2 < bufInfo.bufferSize) {
                        s32 mixed = dstS16[dstIdx] + static_cast<s32>(sample * 32768.0f);
                        dstS16[dstIdx] = static_cast<s16>(std::clamp(mixed, -32768, 32767));
                    }
                }
            }

            currentPos += sampleRateRatio;
        }

        voice->samplePosFloat = currentPos;
        voice->currentSamplePos = static_cast<u32>(currentPos);
        return true;
    }

    return false;
}

// =============================================================================
// API Functions
// =============================================================================

// Ngs2

s32 PS4_SYSV_ABI sceNgs2CalcWaveformBlock(const OrbisNgs2WaveformFormat* format, u32 samplePos,
                                          u32 numSamples, OrbisNgs2WaveformBlock* outBlock) {
    LOG_ERROR(Lib_Ngs2, "samplePos = {}, numSamples = {}", samplePos, numSamples);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2GetWaveformFrameInfo(const OrbisNgs2WaveformFormat* format,
                                             u32* outFrameSize, u32* outNumFrameSamples,
                                             u32* outUnitsPerFrame, u32* outNumDelaySamples) {
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2ParseWaveformData(const void* data, size_t dataSize,
                                          OrbisNgs2WaveformInfo* outInfo) {
    LOG_ERROR(Lib_Ngs2, "dataSize = {}", dataSize);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2ParseWaveformFile(const char* path, u64 offset,
                                          OrbisNgs2WaveformInfo* outInfo) {
    LOG_ERROR(Lib_Ngs2, "path = {}, offset = {}", path, offset);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2ParseWaveformUser(OrbisNgs2ParseReadHandler handler, uintptr_t userData,
                                          OrbisNgs2WaveformInfo* outInfo) {
    LOG_ERROR(Lib_Ngs2, "userData = {}", userData);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2RackCreate(OrbisNgs2Handle systemHandle, u32 rackId,
                                   const OrbisNgs2RackOption* option,
                                   const OrbisNgs2ContextBufferInfo* bufferInfo,
                                   OrbisNgs2Handle* outHandle) {
    LOG_DEBUG(Lib_Ngs2, "rackId = {:#x}, maxVoices = {}", rackId, option ? option->maxVoices : 0);
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    auto* system = reinterpret_cast<SystemInternal*>(systemHandle);
    return RackCreate(system, rackId, option, bufferInfo, outHandle);
}

s32 PS4_SYSV_ABI sceNgs2RackCreateWithAllocator(OrbisNgs2Handle systemHandle, u32 rackId,
                                                const OrbisNgs2RackOption* option,
                                                const OrbisNgs2BufferAllocator* allocator,
                                                OrbisNgs2Handle* outHandle) {
    LOG_DEBUG(Lib_Ngs2, "rackId = {:#x}, maxVoices = {}", rackId, option ? option->maxVoices : 0);
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    if (!outHandle) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    auto* system = reinterpret_cast<SystemInternal*>(systemHandle);

    // Create rack with null buffer info (allocator version doesn't use pre-allocated buffer)
    return RackCreate(system, rackId, option, nullptr, outHandle);
}

s32 PS4_SYSV_ABI sceNgs2RackDestroy(OrbisNgs2Handle rackHandle,
                                    OrbisNgs2ContextBufferInfo* outBufferInfo) {
    LOG_DEBUG(Lib_Ngs2, "called");
    if (!rackHandle) {
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    }

    auto* rack = reinterpret_cast<RackInternal*>(rackHandle);
    return RackDestroy(rack, outBufferInfo);
}

s32 PS4_SYSV_ABI sceNgs2RackGetInfo(OrbisNgs2Handle rackHandle, OrbisNgs2RackInfo* outInfo,
                                    size_t infoSize) {
    LOG_ERROR(Lib_Ngs2, "infoSize = {}", infoSize);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2RackGetUserData(OrbisNgs2Handle rackHandle, uintptr_t* outUserData) {
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2RackGetVoiceHandle(OrbisNgs2Handle rackHandle, u32 voiceIndex,
                                           OrbisNgs2Handle* outHandle) {
    LOG_DEBUG(Lib_Ngs2, "voiceIndex = {}", voiceIndex);
    if (!rackHandle) {
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    }

    auto* rack = reinterpret_cast<RackInternal*>(rackHandle);
    if (voiceIndex >= rack->voices.size()) {
        LOG_ERROR(Lib_Ngs2, "Invalid voice index {} (max {})", voiceIndex, rack->voices.size());
        return ORBIS_NGS2_ERROR_INVALID_VOICE_INDEX;
    }

    if (outHandle) {
        *outHandle = reinterpret_cast<OrbisNgs2Handle>(rack->voices[voiceIndex].get());
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2RackLock(OrbisNgs2Handle rackHandle) {
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2RackQueryBufferSize(u32 rackId, const OrbisNgs2RackOption* option,
                                            OrbisNgs2ContextBufferInfo* outBufferInfo) {
    LOG_DEBUG(Lib_Ngs2, "rackId = {:#x}, option = {}", rackId, static_cast<const void*>(option));

    if (!outBufferInfo) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    u32 rackIndex = RackIdToIndex(rackId);
    if (rackIndex == 0xFF) {
        LOG_ERROR(Lib_Ngs2, "Invalid rack ID: {:#x}", rackId);
        return ORBIS_NGS2_ERROR_INVALID_RACK_ID;
    }

    // Use defaults if option is NULL - based on libSceNgs2.c analysis
    u32 maxVoices = 1;
    u32 maxPorts = 1;
    u32 maxMatrices = 1;

    if (option && option->size >= sizeof(OrbisNgs2RackOption)) {
        maxVoices = option->maxVoices > 0 ? option->maxVoices : 1;
        maxPorts = option->maxPorts > 0 ? option->maxPorts : 1;
        maxMatrices = option->maxMatrices > 0 ? option->maxMatrices : 1;
    } else {
        // Sampler rack (0x1000) defaults to 256 voices
        if (rackId == 0x1000) {
            maxVoices = 256;
        }
    }

    // Calculate required buffer size
    size_t baseSize = sizeof(RackInternal);
    size_t voiceSize = sizeof(VoiceInternal) * maxVoices;
    size_t portSize = sizeof(OrbisNgs2VoicePortInfo) * maxPorts * maxVoices;
    size_t matrixSize = sizeof(OrbisNgs2VoiceMatrixInfo) * maxMatrices * maxVoices;

    size_t totalSize = baseSize + voiceSize + portSize + matrixSize;
    totalSize = (totalSize + 0xFF) & ~0xFF; // Align to 256 bytes

    outBufferInfo->hostBuffer = nullptr;
    outBufferInfo->hostBufferSize = totalSize;
    std::memset(outBufferInfo->reserved, 0, sizeof(outBufferInfo->reserved));

    LOG_DEBUG(Lib_Ngs2, "Required buffer size: {} bytes", totalSize);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2RackSetUserData(OrbisNgs2Handle rackHandle, uintptr_t userData) {
    LOG_ERROR(Lib_Ngs2, "userData = {}", userData);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2RackUnlock(OrbisNgs2Handle rackHandle) {
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemCreate(const OrbisNgs2SystemOption* option,
                                     const OrbisNgs2ContextBufferInfo* bufferInfo,
                                     OrbisNgs2Handle* outHandle) {
    s32 result;
    OrbisNgs2ContextBufferInfo localInfo;
    if (!bufferInfo || !outHandle) {
        if (!bufferInfo) {
            result = ORBIS_NGS2_ERROR_INVALID_BUFFER_INFO;
            LOG_ERROR(Lib_Ngs2, "Invalid system buffer info {}", (void*)bufferInfo);
        } else {
            result = ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
            LOG_ERROR(Lib_Ngs2, "Invalid system handle address {}", (void*)outHandle);
        }

        // TODO: Report errors?
    } else {
        // Make bufferInfo copy
        localInfo.hostBuffer = bufferInfo->hostBuffer;
        localInfo.hostBufferSize = bufferInfo->hostBufferSize;
        for (int i = 0; i < 5; i++) {
            localInfo.reserved[i] = bufferInfo->reserved[i];
        }
        localInfo.userData = bufferInfo->userData;

        result = SystemSetup(option, &localInfo, 0, outHandle);
    }

    // TODO: API reporting?

    LOG_DEBUG(Lib_Ngs2, "called");
    return result;
}

s32 PS4_SYSV_ABI sceNgs2SystemCreateWithAllocator(const OrbisNgs2SystemOption* option,
                                                  const OrbisNgs2BufferAllocator* allocator,
                                                  OrbisNgs2Handle* outHandle) {
    s32 result;
    if (allocator && allocator->allocHandler != 0) {
        OrbisNgs2BufferAllocHandler hostAlloc = allocator->allocHandler;
        if (outHandle) {
            OrbisNgs2BufferFreeHandler hostFree = allocator->freeHandler;
            OrbisNgs2ContextBufferInfo bufferInfo;
            result = SystemSetup(option, &bufferInfo, 0, 0);
            if (result >= 0) {
                uintptr_t sysUserData = allocator->userData;
                result = Core::ExecuteGuest(hostAlloc, &bufferInfo);
                if (result >= 0) {
                    OrbisNgs2Handle* handleCopy = outHandle;
                    result = SystemSetup(option, &bufferInfo, hostFree, handleCopy);
                    if (result < 0) {
                        if (hostFree) {
                            Core::ExecuteGuest(hostFree, &bufferInfo);
                        }
                    }
                }
            }
        } else {
            result = ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
            LOG_ERROR(Lib_Ngs2, "Invalid system handle address {}", (void*)outHandle);
        }
    } else {
        result = ORBIS_NGS2_ERROR_INVALID_BUFFER_ALLOCATOR;
        LOG_ERROR(Lib_Ngs2, "Invalid system buffer allocator {}", (void*)allocator);
    }
    LOG_DEBUG(Lib_Ngs2, "called");
    return result;
}

s32 PS4_SYSV_ABI sceNgs2SystemDestroy(OrbisNgs2Handle systemHandle,
                                      OrbisNgs2ContextBufferInfo* outBufferInfo) {
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }
    LOG_DEBUG(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemEnumHandles(OrbisNgs2Handle* aOutHandle, u32 maxHandles) {
    LOG_ERROR(Lib_Ngs2, "maxHandles = {}", maxHandles);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemEnumRackHandles(OrbisNgs2Handle systemHandle,
                                              OrbisNgs2Handle* aOutHandle, u32 maxHandles) {
    LOG_ERROR(Lib_Ngs2, "maxHandles = {}", maxHandles);
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemGetInfo(OrbisNgs2Handle rackHandle, OrbisNgs2SystemInfo* outInfo,
                                      size_t infoSize) {
    LOG_ERROR(Lib_Ngs2, "infoSize = {}", infoSize);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemGetUserData(OrbisNgs2Handle systemHandle, uintptr_t* outUserData) {
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemLock(OrbisNgs2Handle systemHandle) {
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemQueryBufferSize(const OrbisNgs2SystemOption* option,
                                              OrbisNgs2ContextBufferInfo* outBufferInfo) {
    s32 result;
    if (outBufferInfo) {
        result = SystemSetup(option, outBufferInfo, 0, 0);
        LOG_DEBUG(Lib_Ngs2, "called");
    } else {
        result = ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
        LOG_ERROR(Lib_Ngs2, "Invalid system buffer info {}", (void*)outBufferInfo);
    }

    return result;
}

s32 PS4_SYSV_ABI sceNgs2SystemRender(OrbisNgs2Handle systemHandle,
                                     const OrbisNgs2RenderBufferInfo* aBufferInfo,
                                     u32 numBufferInfo) {
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    if (!aBufferInfo || numBufferInfo == 0 || numBufferInfo > 16) {
        LOG_ERROR(Lib_Ngs2, "Invalid buffer info: ptr={}, count={}",
                  static_cast<const void*>(aBufferInfo), numBufferInfo);
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_ADDRESS;
    }

    auto* system = reinterpret_cast<SystemInternal*>(systemHandle);

    // Clear all output buffers first
    for (u32 i = 0; i < numBufferInfo; i++) {
        if (aBufferInfo[i].buffer && aBufferInfo[i].bufferSize > 0) {
            std::memset(aBufferInfo[i].buffer, 0, aBufferInfo[i].bufferSize);
        }
    }

    // Setup render context
    RenderContext ctx{};
    ctx.bufferInfo = aBufferInfo;
    ctx.numBufferInfo = numBufferInfo;
    ctx.grainSamples = system->numGrainSamples > 0 ? system->numGrainSamples : 256;
    ctx.outputSampleRate = system->sampleRate > 0 ? system->sampleRate : 48000;

    u32 voicesRendered = 0;

    // Process each rack
    for (auto* rack : system->racks) {
        if (!rack) {
            continue;
        }

        // Process sampler racks (0x1000)
        if (rack->rackId == 0x1000) {
            for (auto& voice : rack->voices) {
                if (RenderVoice(voice.get(), ctx)) {
                    voicesRendered++;
                }
            }
        }
        // TODO: Process submixer racks (0x2000, 0x2001)
        // TODO: Process mastering racks (0x3000)
        // TODO: Process effect racks (0x4001, 0x4002, 0x4003)
    }

    system->renderCount++;

    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceNgs2SystemResetOption(OrbisNgs2SystemOption* outOption) {
    static const OrbisNgs2SystemOption option = {
        sizeof(OrbisNgs2SystemOption), "", 0, 512, 256, 48000, {0}};

    if (!outOption) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option address {}", (void*)outOption);
        return ORBIS_NGS2_ERROR_INVALID_OPTION_ADDRESS;
    }
    *outOption = option;

    LOG_DEBUG(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemSetGrainSamples(OrbisNgs2Handle systemHandle, u32 numSamples) {
    LOG_ERROR(Lib_Ngs2, "numSamples = {}", numSamples);
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemSetSampleRate(OrbisNgs2Handle systemHandle, u32 sampleRate) {
    LOG_ERROR(Lib_Ngs2, "sampleRate = {}", sampleRate);
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemSetUserData(OrbisNgs2Handle systemHandle, uintptr_t userData) {
    LOG_ERROR(Lib_Ngs2, "userData = {}", userData);
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2SystemUnlock(OrbisNgs2Handle systemHandle) {
    if (!systemHandle) {
        LOG_ERROR(Lib_Ngs2, "systemHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2VoiceControl(OrbisNgs2Handle voiceHandle,
                                     const OrbisNgs2VoiceParamHeader* paramList) {
    if (!voiceHandle) {
        LOG_ERROR(Lib_Ngs2, "voiceHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
    }

    auto* voice = reinterpret_cast<VoiceInternal*>(voiceHandle);

    const OrbisNgs2VoiceParamHeader* current = paramList;
    while (current != nullptr) {

        switch (current->id) {
        // Sampler rack-specific params (0x1000000X)
        case 0x10000000: { // Sampler Voice Setup
            auto* setup = reinterpret_cast<const OrbisNgs2SamplerVoiceSetupParam*>(current);
            voice->format = setup->format;
            voice->flags = setup->flags;
            voice->isSetup = true;
            break;
        }
        case 0x10000001: { // Sampler Waveform Blocks
            auto* blocks =
                reinterpret_cast<const OrbisNgs2SamplerVoiceWaveformBlocksParam*>(current);

            u32 totalDataSize = 0;
            u32 totalSamples = 0;
            u32 numChannels = voice->format.numChannels > 0 ? voice->format.numChannels : 2;

            if (blocks->numBlocks > 0 && blocks->aBlock) {
                for (u32 i = 0; i < blocks->numBlocks; i++) {
                    totalDataSize += blocks->aBlock[i].dataSize;
                    totalSamples += blocks->aBlock[i].numSamples;
                }

                voice->waveformBlocks.resize(blocks->numBlocks);
                for (u32 i = 0; i < blocks->numBlocks; i++) {
                    voice->waveformBlocks[i].dataOffset = blocks->aBlock[i].dataOffset;
                    voice->waveformBlocks[i].dataSize = blocks->aBlock[i].dataSize;
                    voice->waveformBlocks[i].numRepeats = blocks->aBlock[i].numRepeats;
                    voice->waveformBlocks[i].numSkipSamples = blocks->aBlock[i].numSkipSamples;
                    voice->waveformBlocks[i].numSamples = blocks->aBlock[i].numSamples;
                    voice->waveformBlocks[i].currentRepeat = 0;
                }
            }

            const u8* basePtr = reinterpret_cast<const u8*>(blocks->data);
            u32 dataOffset =
                (blocks->numBlocks > 0 && blocks->aBlock) ? blocks->aBlock[0].dataOffset : 0;
            const void* actualDataPtr = basePtr + dataOffset;

            bool isFirstSetup = (voice->ringBufferCount == 0) && !voice->isStreaming;

            if (isFirstSetup) {
                voice->resetRing();
                voice->addToRing(blocks->data, actualDataPtr, totalDataSize, totalSamples);
                voice->lastConsumedBuffer = nullptr;
                voice->currentBufferPtr = actualDataPtr;
                voice->stateFlags &= ~0x80;
                voice->currentSamplePos = 0;
                voice->samplePosFloat = 0.0f;
                voice->currentBlockIndex = 0;
                voice->isStreaming = false;
            } else {
                bool added =
                    voice->addToRing(blocks->data, actualDataPtr, totalDataSize, totalSamples);

                if (added) {
                    voice->isStreaming = true;
                    voice->stateFlags &= ~0x80;
                }
            }
            break;
        }
        case 0x10000005: { // Sampler Pitch
            auto* pitch = reinterpret_cast<const OrbisNgs2SamplerVoicePitchParam*>(current);
            voice->pitchRatio = pitch->ratio;
            break;
        }

        // Mastering rack-specific params (0x3000000X) - not yet implemented
        case 0x30000000:
        case 0x30000001:
        case 0x30000002:
        case 0x30000003:
        case 0x30000004:
        case 0x30000005:
        case 0x30000006:
            LOG_DEBUG(Lib_Ngs2, "(STUBBED) Mastering param ID: {:#x}", current->id);
            break;

        // Generic voice params
        case 0x06: { // Voice Event
            auto* event = reinterpret_cast<const OrbisNgs2VoiceEventParam*>(current);
            switch (event->eventId) {
            case 0: // Reset
                voice->stateFlags = (voice->stateFlags & 0xDF000000) | 0x20000101;
                voice->totalDecodedSamples = 0;
                voice->currentBufferPtr = nullptr;
                voice->lastConsumedBuffer = nullptr;
                voice->currentSamplePos = 0;
                voice->samplePosFloat = 0.0f;
                voice->currentBlockIndex = 0;
                voice->isStreaming = false;
                voice->waveformBlocks.clear();
                voice->resetRing();
                break;
            case 1: // Pause
                voice->stateFlags |= 0x200;
                break;
            case 2: // Stop
                voice->stateFlags &= ~0x01;
                voice->stateFlags |= 0x400;
                break;
            case 3: // Kill
                voice->stateFlags &= ~0x01;
                voice->stateFlags |= 0x800;
                voice->resetRing();
                break;
            case 4: // Resume A
                voice->stateFlags = (voice->stateFlags & ~0x200) | 0x1000 | 0x01;
                break;
            case 5: // Resume B
                voice->stateFlags = (voice->stateFlags & ~0x200) | 0x2000 | 0x01;
                break;
            default:
                LOG_WARNING(Lib_Ngs2, "Unknown voice event ID: {}", event->eventId);
                break;
            }
            break;
        }
        case 0x01: { // Matrix Levels - not yet implemented
            LOG_DEBUG(Lib_Ngs2, "(STUBBED) Matrix levels param");
            break;
        }
        case 0x02: { // Port Volume
            auto* portVol = reinterpret_cast<const OrbisNgs2VoicePortVolumeParam*>(current);
            voice->portVolume = portVol->level;
            break;
        }
        case 0x03: { // Port Matrix - not yet implemented
            LOG_DEBUG(Lib_Ngs2, "(STUBBED) Port matrix param");
            break;
        }
        case 0x04: { // Port Delay - not yet implemented
            LOG_DEBUG(Lib_Ngs2, "(STUBBED) Port delay param");
            break;
        }
        case 0x05: { // Patch - not yet implemented
            LOG_DEBUG(Lib_Ngs2, "(STUBBED) Patch param");
            break;
        }
        case 0x07: { // Callback - not yet implemented
            LOG_DEBUG(Lib_Ngs2, "(STUBBED) Callback param");
            break;
        }
        default:
            LOG_DEBUG(Lib_Ngs2, "(STUBBED) Unhandled voice param ID: {:#x}", current->id);
            break;
        }

        // Move to next parameter
        if (current->next == 0 || current->next == -1) {
            break;
        }
        current = reinterpret_cast<const OrbisNgs2VoiceParamHeader*>(
            reinterpret_cast<const u8*>(current) + current->next);
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2VoiceGetMatrixInfo(OrbisNgs2Handle voiceHandle, u32 matrixId,
                                           OrbisNgs2VoiceMatrixInfo* outInfo, size_t outInfoSize) {
    if (!voiceHandle) {
        return ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
    }

    if (!outInfo || outInfoSize < sizeof(OrbisNgs2VoiceMatrixInfo)) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    auto* voice = reinterpret_cast<VoiceInternal*>(voiceHandle);

    // Return default matrix info - identity matrix for stereo (1.0 on diagonal)
    outInfo->numLevels = voice->format.numChannels * voice->format.numChannels;
    if (outInfo->numLevels == 0) {
        outInfo->numLevels = 4; // Default stereo 2x2
    }

    // Initialize to identity-like matrix
    std::memset(outInfo->aLevel, 0, sizeof(outInfo->aLevel));
    u32 channels = voice->format.numChannels > 0 ? voice->format.numChannels : 2;
    for (u32 i = 0; i < channels && i < 8; i++) {
        outInfo->aLevel[i * channels + i] = 1.0f;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2VoiceGetOwner(OrbisNgs2Handle voiceHandle, OrbisNgs2Handle* outRackHandle,
                                      u32* outVoiceId) {
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2VoiceGetPortInfo(OrbisNgs2Handle voiceHandle, u32 port,
                                         OrbisNgs2VoicePortInfo* outInfo, size_t outInfoSize) {
    LOG_ERROR(Lib_Ngs2, "port = {}, outInfoSize = {}", port, outInfoSize);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2VoiceGetState(OrbisNgs2Handle voiceHandle, OrbisNgs2VoiceState* outState,
                                      size_t stateSize) {
    if (!outState) {
        LOG_ERROR(Lib_Ngs2, "Invalid voice state address");
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    // Only accept valid state sizes: 0x4 (basic) or 0x30 (sampler)
    if (stateSize != sizeof(OrbisNgs2VoiceState) &&
        stateSize != sizeof(OrbisNgs2SamplerVoiceState)) {
        LOG_ERROR(Lib_Ngs2, "Invalid voice state size: {:#x}", stateSize);
        return ORBIS_NGS2_ERROR_INVALID_OUT_SIZE;
    }

    if (!voiceHandle) {
        LOG_ERROR(Lib_Ngs2, "Invalid voice handle");
        // On invalid handle, zero out the state (LLE behavior)
        if (stateSize == sizeof(OrbisNgs2VoiceState)) {
            outState->stateFlags = 0;
        } else {
            std::memset(outState, 0, sizeof(OrbisNgs2SamplerVoiceState));
        }
        return ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
    }

    auto* voice = reinterpret_cast<VoiceInternal*>(voiceHandle);

    if (stateSize == sizeof(OrbisNgs2VoiceState)) {
        outState->stateFlags = voice->stateFlags;
        return ORBIS_OK;
    }

    auto* samplerState = reinterpret_cast<OrbisNgs2SamplerVoiceState*>(outState);
    samplerState->voiceState.stateFlags = voice->stateFlags;

    if (voice->isStreaming && voice->getReadyBufferCount() <= VoiceInternal::STARVATION_THRESHOLD) {
        samplerState->userData = 1;
    } else {
        samplerState->userData = 0;
    }
    samplerState->envelopeHeight = 1.0f;
    samplerState->peakHeight = 1.0f;
    samplerState->reserved = 0;

    samplerState->numDecodedSamples = voice->totalDecodedSamples;

    u32 bytesPerSample = 2 * (voice->format.numChannels > 0 ? voice->format.numChannels : 2);
    samplerState->decodedDataSize = samplerState->numDecodedSamples * bytesPerSample;

    samplerState->waveformData = voice->currentBufferPtr;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2VoiceGetStateFlags(OrbisNgs2Handle voiceHandle, u32* outStateFlags) {
    if (!outStateFlags) {
        LOG_ERROR(Lib_Ngs2, "Invalid voice state address");
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    if (!voiceHandle) {
        LOG_ERROR(Lib_Ngs2, "Invalid voice handle");
        *outStateFlags = 0; // LLE behavior: zero out on invalid handle
        return ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
    }

    auto* voice = reinterpret_cast<VoiceInternal*>(voiceHandle);

    u32 flags = voice->stateFlags & 0xFF;

    if (voice->isStreaming && voice->getReadyBufferCount() <= VoiceInternal::STARVATION_THRESHOLD) {
        flags |= 0x80;
    }

    *outStateFlags = flags;

    return ORBIS_OK;
}

// Ngs2Custom

s32 PS4_SYSV_ABI sceNgs2CustomRackGetModuleInfo(OrbisNgs2Handle rackHandle, u32 moduleIndex,
                                                OrbisNgs2CustomModuleInfo* outInfo,
                                                size_t infoSize) {
    LOG_ERROR(Lib_Ngs2, "moduleIndex = {}, infoSize = {}", moduleIndex, infoSize);
    return ORBIS_OK;
}

// Ngs2Geom

s32 PS4_SYSV_ABI sceNgs2GeomResetListenerParam(OrbisNgs2GeomListenerParam* outListenerParam) {
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2GeomResetSourceParam(OrbisNgs2GeomSourceParam* outSourceParam) {
    LOG_ERROR(Lib_Ngs2, "called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2GeomCalcListener(const OrbisNgs2GeomListenerParam* param,
                                         OrbisNgs2GeomListenerWork* outWork, u32 flags) {
    LOG_ERROR(Lib_Ngs2, "flags = {}", flags);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2GeomApply(const OrbisNgs2GeomListenerWork* listener,
                                  const OrbisNgs2GeomSourceParam* source,
                                  OrbisNgs2GeomAttribute* outAttrib, u32 flags) {
    LOG_ERROR(Lib_Ngs2, "flags = {}", flags);
    return ORBIS_OK;
}

// Ngs2Pan

s32 PS4_SYSV_ABI sceNgs2PanInit(OrbisNgs2PanWork* work, const float* aSpeakerAngle, float unitAngle,
                                u32 numSpeakers) {
    LOG_DEBUG(Lib_Ngs2, "unitAngle = {}, numSpeakers = {}", unitAngle, numSpeakers);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2PanGetVolumeMatrix(OrbisNgs2PanWork* work, const OrbisNgs2PanParam* aParam,
                                           u32 numParams, u32 matrixFormat,
                                           float* outVolumeMatrix) {
    if (!outVolumeMatrix) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    // matrixFormat: 1 = mono, 2 = stereo, etc.
    u32 numOutputChannels = matrixFormat;
    if (numOutputChannels == 0)
        numOutputChannels = 2;
    if (numOutputChannels > 8)
        numOutputChannels = 8;

    // Initialize volume matrix to identity/center pan
    // For stereo output (format 2), we create a simple center pan
    for (u32 p = 0; p < numParams; p++) {
        float* matrix = outVolumeMatrix + p * numOutputChannels;

        if (aParam && numParams > 0) {
            // Use the pan angle to compute left/right levels
            float angle = aParam[p].angle;
            // Simple stereo panning: angle 0 = center, -1 = left, +1 = right
            float leftLevel = 0.5f * (1.0f - angle);
            float rightLevel = 0.5f * (1.0f + angle);

            if (numOutputChannels >= 2) {
                matrix[0] = leftLevel;
                matrix[1] = rightLevel;
                for (u32 ch = 2; ch < numOutputChannels; ch++) {
                    matrix[ch] = 0.0f;
                }
            } else {
                matrix[0] = 1.0f;
            }
        } else {
            for (u32 ch = 0; ch < numOutputChannels; ch++) {
                matrix[ch] = 1.0f / numOutputChannels;
            }
        }
    }

    return ORBIS_OK;
}

// Ngs2Report

s32 PS4_SYSV_ABI sceNgs2ReportRegisterHandler(u32 reportType, OrbisNgs2ReportHandler handler,
                                              uintptr_t userData, OrbisNgs2Handle* outHandle) {
    LOG_DEBUG(Lib_Ngs2, "reportType = {}, userData = {}", reportType, userData);
    if (!handler) {
        LOG_ERROR(Lib_Ngs2, "handler is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_REPORT_HANDLE;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNgs2ReportUnregisterHandler(OrbisNgs2Handle reportHandle) {
    if (!reportHandle) {
        LOG_ERROR(Lib_Ngs2, "reportHandle is nullptr");
        return ORBIS_NGS2_ERROR_INVALID_REPORT_HANDLE;
    }
    LOG_DEBUG(Lib_Ngs2, "called");
    return ORBIS_OK;
}

// Unknown

int PS4_SYSV_ABI sceNgs2FftInit() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2FftProcess() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2FftQuerySize() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2JobSchedulerResetOption() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2ModuleArrayEnumItems() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2ModuleEnumConfigs() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2ModuleQueueEnumItems() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2RackQueryInfo() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2RackRunCommands() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2SystemQueryInfo() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2SystemRunCommands() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2SystemSetLoudThreshold() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2StreamCreate() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2StreamCreateWithAllocator() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2StreamDestroy() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2StreamQueryBufferSize() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2StreamQueryInfo() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2StreamResetOption() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2StreamRunCommands() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2VoiceQueryInfo() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNgs2VoiceRunCommands() {
    LOG_ERROR(Lib_Ngs2, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("3pCNbVM11UA", "libSceNgs2", 1, "libSceNgs2", sceNgs2CalcWaveformBlock);
    LIB_FUNCTION("6qN1zaEZuN0", "libSceNgs2", 1, "libSceNgs2", sceNgs2CustomRackGetModuleInfo);
    LIB_FUNCTION("Kg1MA5j7KFk", "libSceNgs2", 1, "libSceNgs2", sceNgs2FftInit);
    LIB_FUNCTION("D8eCqBxSojA", "libSceNgs2", 1, "libSceNgs2", sceNgs2FftProcess);
    LIB_FUNCTION("-YNfTO6KOMY", "libSceNgs2", 1, "libSceNgs2", sceNgs2FftQuerySize);
    LIB_FUNCTION("eF8yRCC6W64", "libSceNgs2", 1, "libSceNgs2", sceNgs2GeomApply);
    LIB_FUNCTION("1WsleK-MTkE", "libSceNgs2", 1, "libSceNgs2", sceNgs2GeomCalcListener);
    LIB_FUNCTION("7Lcfo8SmpsU", "libSceNgs2", 1, "libSceNgs2", sceNgs2GeomResetListenerParam);
    LIB_FUNCTION("0lbbayqDNoE", "libSceNgs2", 1, "libSceNgs2", sceNgs2GeomResetSourceParam);
    LIB_FUNCTION("ekGJmmoc8j4", "libSceNgs2", 1, "libSceNgs2", sceNgs2GetWaveformFrameInfo);
    LIB_FUNCTION("BcoPfWfpvVI", "libSceNgs2", 1, "libSceNgs2", sceNgs2JobSchedulerResetOption);
    LIB_FUNCTION("EEemGEQCjO8", "libSceNgs2", 1, "libSceNgs2", sceNgs2ModuleArrayEnumItems);
    LIB_FUNCTION("TaoNtmMKkXQ", "libSceNgs2", 1, "libSceNgs2", sceNgs2ModuleEnumConfigs);
    LIB_FUNCTION("ve6bZi+1sYQ", "libSceNgs2", 1, "libSceNgs2", sceNgs2ModuleQueueEnumItems);
    LIB_FUNCTION("gbMKV+8Enuo", "libSceNgs2", 1, "libSceNgs2", sceNgs2PanGetVolumeMatrix);
    LIB_FUNCTION("xa8oL9dmXkM", "libSceNgs2", 1, "libSceNgs2", sceNgs2PanInit);
    LIB_FUNCTION("hyVLT2VlOYk", "libSceNgs2", 1, "libSceNgs2", sceNgs2ParseWaveformData);
    LIB_FUNCTION("iprCTXPVWMI", "libSceNgs2", 1, "libSceNgs2", sceNgs2ParseWaveformFile);
    LIB_FUNCTION("t9T0QM17Kvo", "libSceNgs2", 1, "libSceNgs2", sceNgs2ParseWaveformUser);
    LIB_FUNCTION("cLV4aiT9JpA", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackCreate);
    LIB_FUNCTION("U546k6orxQo", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackCreateWithAllocator);
    LIB_FUNCTION("lCqD7oycmIM", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackDestroy);
    LIB_FUNCTION("M4LYATRhRUE", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackGetInfo);
    LIB_FUNCTION("Mn4XNDg03XY", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackGetUserData);
    LIB_FUNCTION("MwmHz8pAdAo", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackGetVoiceHandle);
    LIB_FUNCTION("MzTa7VLjogY", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackLock);
    LIB_FUNCTION("0eFLVCfWVds", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackQueryBufferSize);
    LIB_FUNCTION("TZqb8E-j3dY", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackQueryInfo);
    LIB_FUNCTION("MI2VmBx2RbM", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackRunCommands);
    LIB_FUNCTION("JNTMIaBIbV4", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackSetUserData);
    LIB_FUNCTION("++YZ7P9e87U", "libSceNgs2", 1, "libSceNgs2", sceNgs2RackUnlock);
    LIB_FUNCTION("uBIN24Tv2MI", "libSceNgs2", 1, "libSceNgs2", sceNgs2ReportRegisterHandler);
    LIB_FUNCTION("nPzb7Ly-VjE", "libSceNgs2", 1, "libSceNgs2", sceNgs2ReportUnregisterHandler);
    LIB_FUNCTION("koBbCMvOKWw", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemCreate);
    LIB_FUNCTION("mPYgU4oYpuY", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemCreateWithAllocator);
    LIB_FUNCTION("u-WrYDaJA3k", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemDestroy);
    LIB_FUNCTION("vubFP0T6MP0", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemEnumHandles);
    LIB_FUNCTION("U-+7HsswcIs", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemEnumRackHandles);
    LIB_FUNCTION("vU7TQ62pItw", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemGetInfo);
    LIB_FUNCTION("4lFaRxd-aLs", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemGetUserData);
    LIB_FUNCTION("gThZqM5PYlQ", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemLock);
    LIB_FUNCTION("pgFAiLR5qT4", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemQueryBufferSize);
    LIB_FUNCTION("3oIK7y7O4k0", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemQueryInfo)
    LIB_FUNCTION("i0VnXM-C9fc", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemRender);
    LIB_FUNCTION("AQkj7C0f3PY", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemResetOption);
    LIB_FUNCTION("gXiormHoZZ4", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemRunCommands);
    LIB_FUNCTION("l4Q2dWEH6UM", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemSetGrainSamples);
    LIB_FUNCTION("Wdlx0ZFTV9s", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemSetLoudThreshold);
    LIB_FUNCTION("-tbc2SxQD60", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemSetSampleRate);
    LIB_FUNCTION("GZB2v0XnG0k", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemSetUserData);
    LIB_FUNCTION("JXRC5n0RQls", "libSceNgs2", 1, "libSceNgs2", sceNgs2SystemUnlock);
    LIB_FUNCTION("sU2St3agdjg", "libSceNgs2", 1, "libSceNgs2", sceNgs2StreamCreate);
    LIB_FUNCTION("I+RLwaauggA", "libSceNgs2", 1, "libSceNgs2", sceNgs2StreamCreateWithAllocator);
    LIB_FUNCTION("bfoMXnTRtwE", "libSceNgs2", 1, "libSceNgs2", sceNgs2StreamDestroy);
    LIB_FUNCTION("dxulc33msHM", "libSceNgs2", 1, "libSceNgs2", sceNgs2StreamQueryBufferSize);
    LIB_FUNCTION("rfw6ufRsmow", "libSceNgs2", 1, "libSceNgs2", sceNgs2StreamQueryInfo);
    LIB_FUNCTION("q+2W8YdK0F8", "libSceNgs2", 1, "libSceNgs2", sceNgs2StreamResetOption);
    LIB_FUNCTION("qQHCi9pjDps", "libSceNgs2", 1, "libSceNgs2", sceNgs2StreamRunCommands);
    LIB_FUNCTION("uu94irFOGpA", "libSceNgs2", 1, "libSceNgs2", sceNgs2VoiceControl);
    LIB_FUNCTION("jjBVvPN9964", "libSceNgs2", 1, "libSceNgs2", sceNgs2VoiceGetMatrixInfo);
    LIB_FUNCTION("W-Z8wWMBnhk", "libSceNgs2", 1, "libSceNgs2", sceNgs2VoiceGetOwner);
    LIB_FUNCTION("WCayTgob7-o", "libSceNgs2", 1, "libSceNgs2", sceNgs2VoiceGetPortInfo);
    LIB_FUNCTION("-TOuuAQ-buE", "libSceNgs2", 1, "libSceNgs2", sceNgs2VoiceGetState);
    LIB_FUNCTION("rEh728kXk3w", "libSceNgs2", 1, "libSceNgs2", sceNgs2VoiceGetStateFlags);
    LIB_FUNCTION("9eic4AmjGVI", "libSceNgs2", 1, "libSceNgs2", sceNgs2VoiceQueryInfo);
    LIB_FUNCTION("AbYvTOZ8Pts", "libSceNgs2", 1, "libSceNgs2", sceNgs2VoiceRunCommands);
};

} // namespace Libraries::Ngs2