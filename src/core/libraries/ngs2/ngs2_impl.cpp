// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ngs2_error.h"
#include "ngs2_impl.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/kernel.h"

using namespace Libraries::Kernel;

namespace Libraries::Ngs2 {

s32 Ngs2::ReportInvalid(Ngs2Handle* handle, u32 handle_type) const {
    uintptr_t hAddress = reinterpret_cast<uintptr_t>(handle);
    switch (handle_type) {
    case 1:
        LOG_ERROR(Lib_Ngs2, "Invalid system handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    case 2:
        LOG_ERROR(Lib_Ngs2, "Invalid rack handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    case 4:
        LOG_ERROR(Lib_Ngs2, "Invalid voice handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
    case 8:
        LOG_ERROR(Lib_Ngs2, "Invalid report handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_REPORT_HANDLE;
    default:
        LOG_ERROR(Lib_Ngs2, "Invalid handle {}", hAddress);
        return ORBIS_NGS2_ERROR_INVALID_HANDLE;
    }
}

s32 Ngs2::HandleSetup(Ngs2Handle* handle, void* data, std::atomic<u32>* atomic, u32 type,
                      u32 flags) {
    handle->dataPointer = data;
    handle->atomicPtr = atomic;
    handle->handleType = type;
    handle->flags_unk = flags;
    return ORBIS_OK;
}

s32 Ngs2::HandleCleanup(Ngs2Handle* handle, u32 hType, void* dataOut) {
    if (handle && handle->selfPointer == handle) {
        std::atomic<u32>* tmp_atomic = handle->atomicPtr;
        if (tmp_atomic && handle->handleType == hType) {
            while (tmp_atomic->load() != 0) {
                u32 expected = 1;
                if (tmp_atomic->compare_exchange_strong(expected, 0)) {
                    if (dataOut) {
                        dataOut = handle->dataPointer;
                    }
                    // sceNgs2MemoryClear(handle, 32);
                    return ORBIS_OK;
                }
                tmp_atomic = handle->atomicPtr;
            }
        }
    }
    return this->ReportInvalid(handle, hType);
}

s32 Ngs2::HandleEnter(Ngs2Handle* handle, u32 hType, Ngs2Handle* handleOut) {
    if (!handle) {
        return this->ReportInvalid(handle, 0);
    }

    if (handle->selfPointer != handle || !handle->atomicPtr || !handle->dataPointer ||
        (~hType & handle->handleType)) {
        return this->ReportInvalid(handle, handle->handleType);
    }

    std::atomic<u32>* atomic = handle->atomicPtr;
    while (true) {
        u32 i = atomic->load();
        if (i == 0) {
            return this->ReportInvalid(handle, handle->handleType);
        }
        if (atomic->compare_exchange_strong(i, i + 1)) {
            break;
        }
    }

    if (handleOut) {
        handleOut = handle;
    }
    return ORBIS_OK;
}

s32 Ngs2::HandleLeave(Ngs2Handle* handle) {
    std::atomic<u32>* tmp_atomic;
    u32 i;
    do {
        tmp_atomic = handle->atomicPtr;
        i = tmp_atomic->load();
    } while (!tmp_atomic->compare_exchange_strong(i, i - 1));
    return ORBIS_OK;
}

s32 Ngs2::StackBufferOpen(StackBuffer* buf, void* base_addr, size_t size, void** stackTop,
                          bool verify) {
    buf->top = stackTop;
    buf->base = base_addr;
    buf->curr = base_addr;
    buf->usedSize = 0;
    buf->totalSize = size;
    buf->alignment = 8;
    buf->isVerifyEnabled = verify;

    if (stackTop) {
        *stackTop = nullptr;
    }

    return ORBIS_OK;
}

s32 Ngs2::StackBufferClose(StackBuffer* buf, size_t* usedSize) {
    if (usedSize) {
        *usedSize = buf->usedSize + buf->alignment;
    }

    return ORBIS_OK;
}

s32 Ngs2::SystemSetupCore(StackBuffer* buf, SystemOptions* options, Ngs2Handle** sysOut) {
    u32 maxGrainSamples = 512;
    u32 numGrainSamples = 256;
    u32 sampleRate = 48000;

    if (options) {
        maxGrainSamples = options->maxGrainSamples;
        numGrainSamples = options->numGrainSamples;
        sampleRate = options->sampleRate;
    }

    // Validate maxGrainSamples
    if (maxGrainSamples < 64 || maxGrainSamples > 1024 || (maxGrainSamples & 0x3F) != 0) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option (maxGrainSamples={},x64)", maxGrainSamples);
        return ORBIS_NGS2_ERROR_INVALID_MAX_GRAIN_SAMPLES;
    }

    // Validate numGrainSamples
    if (numGrainSamples < 64 || numGrainSamples > 1024 || (numGrainSamples & 0x3F) != 0) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option (numGrainSamples={},x64)", numGrainSamples);
        return ORBIS_NGS2_ERROR_INVALID_NUM_GRAIN_SAMPLES;
    }

    // Validate sampleRate
    if (sampleRate != 11025 && sampleRate != 12000 && sampleRate != 22050 && sampleRate != 24000 &&
        sampleRate != 44100 && sampleRate != 48000 && sampleRate != 88200 && sampleRate != 96000) {
        LOG_ERROR(Lib_Ngs2, "Invalid system option(sampleRate={}:44.1/48kHz series)", sampleRate);
        return ORBIS_NGS2_ERROR_INVALID_SAMPLE_RATE;
    }

    int result = ORBIS_OK;

    // TODO

    return result; // Success
}

} // namespace Libraries::Ngs2
