// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ngs2_error.h"
#include "ngs2_impl.h"
#include "ngs2_internal.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"
#include <algorithm>

using namespace Libraries::Kernel;

namespace Libraries::Ngs2 {

s32 HandleReportInvalid(OrbisNgs2Handle handle, u32 handleType) {
    switch (handleType) {
    case 1:
        LOG_ERROR(Lib_Ngs2, "Invalid system handle {}", handle);
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    case 2:
        LOG_ERROR(Lib_Ngs2, "Invalid rack handle {}", handle);
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    case 4:
        LOG_ERROR(Lib_Ngs2, "Invalid voice handle {}", handle);
        return ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
    case 8:
        LOG_ERROR(Lib_Ngs2, "Invalid report handle {}", handle);
        return ORBIS_NGS2_ERROR_INVALID_REPORT_HANDLE;
    default:
        LOG_ERROR(Lib_Ngs2, "Invalid handle {}", handle);
        return ORBIS_NGS2_ERROR_INVALID_HANDLE;
    }
}

void* MemoryClear(void* buffer, size_t size) {
    return memset(buffer, 0, size);
}

s32 StackBufferClose(StackBuffer* stackBuffer, size_t* outTotalSize) {
    if (outTotalSize) {
        *outTotalSize = stackBuffer->usedSize + stackBuffer->alignment;
    }
    return ORBIS_OK;
}

s32 StackBufferOpen(StackBuffer* stackBuffer, void* bufferStart, size_t bufferSize,
                    void** outBuffer, u8 flags) {
    stackBuffer->top = outBuffer;
    stackBuffer->base = bufferStart;
    stackBuffer->size = (size_t)bufferStart;
    stackBuffer->currentOffset = (size_t)bufferStart;
    stackBuffer->usedSize = 0;
    stackBuffer->totalSize = bufferSize;
    stackBuffer->alignment = 8; // this is a fixed value
    stackBuffer->flags = flags;

    if (outBuffer != NULL) {
        *outBuffer = NULL;
    }

    return ORBIS_OK;
}

s32 SystemCleanup(OrbisNgs2Handle systemHandle, OrbisNgs2ContextBufferInfo* outInfo) {
    if (!systemHandle) {
        return ORBIS_NGS2_ERROR_INVALID_HANDLE;
    }

    // TODO

    return ORBIS_OK;
}

s32 SystemSetupCore(StackBuffer* stackBuffer, const OrbisNgs2SystemOption* option,
                    SystemInternal* outSystem) {
    u32 maxGrainSamples = 512;
    u32 numGrainSamples = 256;
    u32 sampleRate = 48000;

    if (option) {
        sampleRate = option->sampleRate;
        maxGrainSamples = option->maxGrainSamples;
        numGrainSamples = option->numGrainSamples;
    }

    if (maxGrainSamples < 64 || maxGrainSamples > 1024 || (maxGrainSamples & 63) != 0) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option (maxGrainSamples={},x64)", maxGrainSamples);
        return ORBIS_NGS2_ERROR_INVALID_MAX_GRAIN_SAMPLES;
    }

    if (numGrainSamples < 64 || numGrainSamples > 1024 || (numGrainSamples & 63) != 0) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option (numGrainSamples={},x64)", numGrainSamples);
        return ORBIS_NGS2_ERROR_INVALID_NUM_GRAIN_SAMPLES;
    }

    if (sampleRate != 11025 && sampleRate != 12000 && sampleRate != 22050 && sampleRate != 24000 &&
        sampleRate != 44100 && sampleRate != 48000 && sampleRate != 88200 && sampleRate != 96000 &&
        sampleRate != 176400 && sampleRate != 192000) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option(sampleRate={}:44.1/48kHz series)", sampleRate);
        return ORBIS_NGS2_ERROR_INVALID_SAMPLE_RATE;
    }

    if (outSystem) {
        // Initialize system
        std::memset(outSystem, 0, sizeof(SystemInternal));
        outSystem->systemHandle = reinterpret_cast<OrbisNgs2Handle>(outSystem);
        outSystem->sampleRate = sampleRate;
        outSystem->currentSampleRate = sampleRate;
        outSystem->maxGrainSamples = static_cast<u16>(maxGrainSamples);
        outSystem->minGrainSamples = 64;
        outSystem->numGrainSamples = static_cast<u16>(numGrainSamples);
        outSystem->currentNumGrainSamples = numGrainSamples;
        outSystem->renderCount = 0;
        outSystem->rackCount = 0;
        outSystem->isActive = 1;
        
        if (option && option->name[0] != '\0') {
            std::strncpy(outSystem->name, option->name, ORBIS_NGS2_SYSTEM_NAME_LENGTH - 1);
            outSystem->name[ORBIS_NGS2_SYSTEM_NAME_LENGTH - 1] = '\0';
        }
    }

    return ORBIS_OK;
}

s32 SystemSetup(const OrbisNgs2SystemOption* option, OrbisNgs2ContextBufferInfo* hostBufferInfo,
                OrbisNgs2BufferFreeHandler hostFree, OrbisNgs2Handle* outHandle) {
    u8 optionFlags = 0;
    StackBuffer stackBuffer;
    SystemInternal setupResult;
    void* systemList = NULL;
    size_t requiredBufferSize = 0;
    u32 result = ORBIS_NGS2_ERROR_INVALID_BUFFER_SIZE;

    if (option) {
        if (option->size != 64) {
            LOG_ERROR(Lib_Ngs2, "Invalid system option size ({})", option->size);
            return ORBIS_NGS2_ERROR_INVALID_OPTION_SIZE;
        }
        optionFlags = option->flags >> 31;
    }

    // Init
    StackBufferOpen(&stackBuffer, NULL, 0, NULL, optionFlags);
    result = SystemSetupCore(&stackBuffer, option, 0);

    if (result < 0) {
        return result;
    }

    StackBufferClose(&stackBuffer, &requiredBufferSize);

    // outHandle unprovided
    if (!outHandle) {
        hostBufferInfo->hostBuffer = NULL;
        hostBufferInfo->hostBufferSize = requiredBufferSize;
        MemoryClear(&hostBufferInfo->reserved, sizeof(hostBufferInfo->reserved));
        return ORBIS_OK;
    }

    if (!hostBufferInfo->hostBuffer) {
        LOG_ERROR(Lib_Ngs2, "Invalid system buffer address ({})", hostBufferInfo->hostBuffer);
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_ADDRESS;
    }

    if (hostBufferInfo->hostBufferSize < requiredBufferSize) {
        LOG_ERROR(Lib_Ngs2, "Invalid system buffer size ({}<{}[byte])",
                  hostBufferInfo->hostBufferSize, requiredBufferSize);
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_SIZE;
    }

    // Setup
    StackBufferOpen(&stackBuffer, hostBufferInfo->hostBuffer, hostBufferInfo->hostBufferSize,
                    &systemList, optionFlags);
    
    // Allocate SystemInternal from the buffer
    auto* system = new SystemInternal();
    result = SystemSetupCore(&stackBuffer, option, system);

    if (result < 0) {
        delete system;
        return result;
    }

    StackBufferClose(&stackBuffer, &requiredBufferSize);

    system->bufferInfo = *hostBufferInfo;
    system->hostFree = hostFree;
    system->systemHandle = reinterpret_cast<OrbisNgs2Handle>(system);

    if (hostBufferInfo->hostBufferSize >= requiredBufferSize) {
        *outHandle = system->systemHandle;
        return ORBIS_OK;
    }

    delete system;

    LOG_ERROR(Lib_Ngs2, "Invalid system buffer size ({}<{}[byte])", hostBufferInfo->hostBufferSize,
              requiredBufferSize);
    return ORBIS_NGS2_ERROR_INVALID_BUFFER_SIZE;
}

u32 RackIdToIndex(u32 rackId) {
    switch (rackId) {
    case 0x1000: return 0; // Sampler
    case 0x3000: return 1; // Mastering
    case 0x2000: return 2; // Submixer
    case 0x2001: return 3; // Submixer alt
    case 0x4001: return 4; // Reverb
    case 0x4002: return 5; // Equalizer
    case 0x4003: return 6; // Custom
    default: return 0xFF;
    }
}

s32 RackCreate(SystemInternal* system, u32 rackId, const OrbisNgs2RackOption* option,
               const OrbisNgs2ContextBufferInfo* bufferInfo, OrbisNgs2Handle* outHandle) {
    if (!system) {
        LOG_ERROR(Lib_Ngs2, "RackCreate: Invalid system handle");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }
    
    u32 rackIndex = RackIdToIndex(rackId);
    if (rackIndex == 0xFF) {
        LOG_ERROR(Lib_Ngs2, "Invalid rack ID: {:#x}", rackId);
        return ORBIS_NGS2_ERROR_INVALID_RACK_ID;
    }
    
    auto* rack = new RackInternal();
    rack->ownerSystem = system;
    rack->rackType = rackIndex;
    rack->rackId = rackId;
    rack->handle.systemData = system;
    
    // Setup rack info with defaults or from option
    rack->info.rackHandle = reinterpret_cast<OrbisNgs2Handle>(rack);
    rack->info.ownerSystemHandle = system->systemHandle;
    rack->info.type = rackIndex;
    rack->info.rackId = rackId;
    rack->info.minGrainSamples = 64;
    rack->info.stateFlags = 0;
    
    // Use option values if provided, otherwise use defaults
    if (option && option->size >= sizeof(OrbisNgs2RackOption)) {
        std::strncpy(rack->info.name, option->name, ORBIS_NGS2_RACK_NAME_LENGTH - 1);
        rack->info.name[ORBIS_NGS2_RACK_NAME_LENGTH - 1] = '\0';
        rack->info.maxVoices = option->maxVoices > 0 ? option->maxVoices : 1;
        rack->info.maxGrainSamples = option->maxGrainSamples > 0 ? option->maxGrainSamples : 512;
    } else {
        // Default values when option is NULL - based on libSceNgs2.c analysis
        rack->info.name[0] = '\0';
        // Sampler rack (0x1000) defaults to 0x100 (256) voices, others default to 1
        if (rackId == 0x1000) {
            rack->info.maxVoices = 256;  // Sampler default
        } else {
            rack->info.maxVoices = 1;
        }
        rack->info.maxGrainSamples = 512;
    }
    
    // Allocate voices
    u32 numVoices = rack->info.maxVoices;
    rack->voices.reserve(numVoices);
    for (u32 i = 0; i < numVoices; i++) {
        auto voice = std::make_unique<VoiceInternal>();
        voice->ownerRack = rack;
        voice->voiceIndex = i;
        voice->handle.systemData = system;
        voice->stateFlags = 0; // Not playing
        rack->voices.push_back(std::move(voice));
    }
    
    system->racks.push_back(rack);
    system->rackCount++;
    
    if (outHandle) {
        *outHandle = reinterpret_cast<OrbisNgs2Handle>(rack);
    }
    
    return ORBIS_OK;
}

s32 RackDestroy(RackInternal* rack, OrbisNgs2ContextBufferInfo* outBufferInfo) {
    if (!rack) {
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    }
    
    SystemInternal* system = rack->ownerSystem;
    if (system) {
        auto it = std::find(system->racks.begin(), system->racks.end(), rack);
        if (it != system->racks.end()) {
            system->racks.erase(it);
            system->rackCount--;
        }
    }
    
    delete rack;
    return ORBIS_OK;
}

} // namespace Libraries::Ngs2
