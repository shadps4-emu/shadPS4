// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ngs2_error.h"
#include "ngs2_impl.h"
#include "ngs2_internal.h"

#include <algorithm>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"

using namespace Libraries::Kernel;

namespace Libraries::Ngs2 {

s32 HandleReportInvalid(OrbisNgs2Handle handle, u32 handle_type) {
    switch (handle_type) {
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

s32 StackBufferClose(StackBuffer* stack_buffer, size_t* out_total_size) {
    if (out_total_size) {
        *out_total_size = stack_buffer->usedSize + stack_buffer->alignment;
    }
    return ORBIS_OK;
}

s32 StackBufferOpen(StackBuffer* stack_buffer, void* buffer_start, size_t buffer_size,
                    void** out_buffer, u8 flags) {
    stack_buffer->top = out_buffer;
    stack_buffer->base = buffer_start;
    stack_buffer->size = (size_t)buffer_start;
    stack_buffer->currentOffset = (size_t)buffer_start;
    stack_buffer->usedSize = 0;
    stack_buffer->totalSize = buffer_size;
    stack_buffer->alignment = 8; // this is a fixed value
    stack_buffer->flags = flags;

    if (out_buffer != NULL) {
        *out_buffer = NULL;
    }

    return ORBIS_OK;
}

s32 SystemCleanup(OrbisNgs2Handle system_handle, OrbisNgs2ContextBufferInfo* out_info) {
    if (!system_handle) {
        return ORBIS_NGS2_ERROR_INVALID_HANDLE;
    }

    // TODO

    return ORBIS_OK;
}

s32 SystemSetupCore(StackBuffer* stack_buffer, const OrbisNgs2SystemOption* option,
                    SystemInternal* out_system) {
    u32 max_grain_samples = 512;
    u32 num_grain_samples = 256;
    u32 sample_rate = 48000;

    if (option) {
        sample_rate = option->sampleRate;
        max_grain_samples = option->maxGrainSamples;
        num_grain_samples = option->numGrainSamples;
    }

    if (max_grain_samples < 64 || max_grain_samples > 1024 || (max_grain_samples & 63) != 0) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option (maxGrainSamples={},x64)", max_grain_samples);
        return ORBIS_NGS2_ERROR_INVALID_MAX_GRAIN_SAMPLES;
    }

    if (num_grain_samples < 64 || num_grain_samples > 1024 || (num_grain_samples & 63) != 0) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option (numGrainSamples={},x64)", num_grain_samples);
        return ORBIS_NGS2_ERROR_INVALID_NUM_GRAIN_SAMPLES;
    }

    if (sample_rate != 11025 && sample_rate != 12000 && sample_rate != 22050 &&
        sample_rate != 24000 && sample_rate != 44100 && sample_rate != 48000 &&
        sample_rate != 88200 && sample_rate != 96000 && sample_rate != 176400 &&
        sample_rate != 192000) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option(sampleRate={}:44.1/48kHz series)", sample_rate);
        return ORBIS_NGS2_ERROR_INVALID_SAMPLE_RATE;
    }

    if (out_system) {
        // Initialize system
        std::memset(out_system, 0, sizeof(SystemInternal));
        out_system->systemHandle = reinterpret_cast<OrbisNgs2Handle>(out_system);
        out_system->sampleRate = sample_rate;
        out_system->currentSampleRate = sample_rate;
        out_system->maxGrainSamples = static_cast<u16>(max_grain_samples);
        out_system->minGrainSamples = 64;
        out_system->numGrainSamples = static_cast<u16>(num_grain_samples);
        out_system->currentNumGrainSamples = num_grain_samples;
        out_system->renderCount = 0;
        out_system->rackCount = 0;
        out_system->isActive = 1;

        if (option && option->name[0] != '\0') {
            std::strncpy(out_system->name, option->name, ORBIS_NGS2_SYSTEM_NAME_LENGTH - 1);
            out_system->name[ORBIS_NGS2_SYSTEM_NAME_LENGTH - 1] = '\0';
        }
    }

    return ORBIS_OK;
}

s32 SystemSetup(const OrbisNgs2SystemOption* option, OrbisNgs2ContextBufferInfo* host_buffer_info,
                OrbisNgs2BufferFreeHandler host_free, OrbisNgs2Handle* out_handle) {
    u8 option_flags = 0;
    StackBuffer stack_buffer;
    SystemInternal setup_result;
    void* system_list = NULL;
    size_t required_buffer_size = 0;
    u32 result = ORBIS_NGS2_ERROR_INVALID_BUFFER_SIZE;

    if (option) {
        if (option->size != 64) {
            LOG_ERROR(Lib_Ngs2, "Invalid system option size ({})", option->size);
            return ORBIS_NGS2_ERROR_INVALID_OPTION_SIZE;
        }
        option_flags = option->flags >> 31;
    }

    // Init
    StackBufferOpen(&stack_buffer, NULL, 0, NULL, option_flags);
    result = SystemSetupCore(&stack_buffer, option, 0);

    if (result < 0) {
        return result;
    }

    StackBufferClose(&stack_buffer, &required_buffer_size);

    // out_handle unprovided
    if (!out_handle) {
        host_buffer_info->hostBuffer = NULL;
        host_buffer_info->hostBufferSize = required_buffer_size;
        MemoryClear(&host_buffer_info->reserved, sizeof(host_buffer_info->reserved));
        return ORBIS_OK;
    }

    if (!host_buffer_info->hostBuffer) {
        LOG_ERROR(Lib_Ngs2, "Invalid system buffer address ({})", host_buffer_info->hostBuffer);
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_ADDRESS;
    }

    if (host_buffer_info->hostBufferSize < required_buffer_size) {
        LOG_ERROR(Lib_Ngs2, "Invalid system buffer size ({}<{}[byte])",
                  host_buffer_info->hostBufferSize, required_buffer_size);
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_SIZE;
    }

    // Setup
    StackBufferOpen(&stack_buffer, host_buffer_info->hostBuffer, host_buffer_info->hostBufferSize,
                    &system_list, option_flags);

    // Allocate SystemInternal from the buffer
    auto* system = new SystemInternal();
    result = SystemSetupCore(&stack_buffer, option, system);

    if (result < 0) {
        delete system;
        return result;
    }

    StackBufferClose(&stack_buffer, &required_buffer_size);

    system->bufferInfo = *host_buffer_info;
    system->hostFree = host_free;
    system->systemHandle = reinterpret_cast<OrbisNgs2Handle>(system);

    if (host_buffer_info->hostBufferSize >= required_buffer_size) {
        *out_handle = system->systemHandle;
        return ORBIS_OK;
    }

    delete system;

    LOG_ERROR(Lib_Ngs2, "Invalid system buffer size ({}<{}[byte])",
              host_buffer_info->hostBufferSize, required_buffer_size);
    return ORBIS_NGS2_ERROR_INVALID_BUFFER_SIZE;
}

u32 RackIdToIndex(u32 rack_id) {
    switch (rack_id) {
    case 0x1000:
        return 0; // Sampler
    case 0x3000:
        return 1; // Mastering
    case 0x2000:
        return 2; // Submixer
    case 0x2001:
        return 3; // Submixer alt
    case 0x4001:
        return 4; // Reverb
    case 0x4002:
        return 5; // Equalizer
    case 0x4003:
        return 6; // Custom
    default:
        return 0xFF;
    }
}

s32 RackCreate(SystemInternal* system, u32 rack_id, const OrbisNgs2RackOption* option,
               const OrbisNgs2ContextBufferInfo* buffer_info, OrbisNgs2Handle* out_handle) {
    if (!system) {
        LOG_ERROR(Lib_Ngs2, "RackCreate: Invalid system handle");
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    u32 rack_index = RackIdToIndex(rack_id);
    if (rack_index == 0xFF) {
        LOG_ERROR(Lib_Ngs2, "Invalid rack ID: {:#x}", rack_id);
        return ORBIS_NGS2_ERROR_INVALID_RACK_ID;
    }

    auto* rack = new RackInternal();
    rack->ownerSystem = system;
    rack->rackType = rack_index;
    rack->rackId = rack_id;
    rack->handle.systemData = system;

    // Setup rack info with defaults or from option
    rack->info.rackHandle = reinterpret_cast<OrbisNgs2Handle>(rack);
    rack->info.ownerSystemHandle = system->systemHandle;
    rack->info.type = rack_index;
    rack->info.rackId = rack_id;
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
        if (rack_id == 0x1000) {
            rack->info.maxVoices = 256; // Sampler default
        } else {
            rack->info.maxVoices = 1;
        }
        rack->info.maxGrainSamples = 512;
    }

    // Allocate voices
    u32 num_voices = rack->info.maxVoices;
    rack->voices.reserve(num_voices);
    for (u32 i = 0; i < num_voices; i++) {
        auto voice = std::make_unique<VoiceInternal>();
        voice->ownerRack = rack;
        voice->voiceIndex = i;
        voice->handle.systemData = system;
        voice->stateFlags = 0; // Not playing
        rack->voices.push_back(std::move(voice));
    }

    system->racks.push_back(rack);
    system->rackCount++;

    if (out_handle) {
        *out_handle = reinterpret_cast<OrbisNgs2Handle>(rack);
    }

    return ORBIS_OK;
}

s32 RackDestroy(RackInternal* rack, OrbisNgs2ContextBufferInfo* out_buffer_info) {
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
