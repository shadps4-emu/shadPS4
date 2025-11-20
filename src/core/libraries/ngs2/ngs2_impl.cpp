// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ngs2_error.h"
#include "ngs2_impl.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"

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
        // dummy handle
        outSystem->systemHandle = 1;
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
    result = SystemSetupCore(&stackBuffer, option, &setupResult);

    if (result < 0) {
        return result;
    }

    StackBufferClose(&stackBuffer, &requiredBufferSize);

    // Copy buffer results
    setupResult.bufferInfo = *hostBufferInfo;
    setupResult.hostFree = hostFree;
    // TODO
    // setupResult.systemList = systemList;

    OrbisNgs2Handle systemHandle = setupResult.systemHandle;
    if (hostBufferInfo->hostBufferSize >= requiredBufferSize) {
        *outHandle = systemHandle;
        return ORBIS_OK;
    }

    SystemCleanup(systemHandle, 0);

    LOG_ERROR(Lib_Ngs2, "Invalid system buffer size ({}<{}[byte])", hostBufferInfo->hostBufferSize,
              requiredBufferSize);
    return ORBIS_NGS2_ERROR_INVALID_BUFFER_SIZE;
}

} // namespace Libraries::Ngs2
