// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/ajm/ajm_context.h"
#include "core/libraries/ajm/ajm_error.h"
#include "core/libraries/ajm/ajm_mp3.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

#include <magic_enum/magic_enum.hpp>

namespace Libraries::Ajm {

constexpr int ORBIS_AJM_CHANNELMASK_MONO = 0x0004;
constexpr int ORBIS_AJM_CHANNELMASK_STEREO = 0x0003;
constexpr int ORBIS_AJM_CHANNELMASK_QUAD = 0x0033;
constexpr int ORBIS_AJM_CHANNELMASK_5POINT1 = 0x060F;
constexpr int ORBIS_AJM_CHANNELMASK_7POINT1 = 0x063F;

static std::unordered_map<u32, std::unique_ptr<AjmContext>> contexts{};

u32 GetChannelMask(u32 num_channels) {
    switch (num_channels) {
    case 1:
        return ORBIS_AJM_CHANNELMASK_MONO;
    case 2:
        return ORBIS_AJM_CHANNELMASK_STEREO;
    case 4:
        return ORBIS_AJM_CHANNELMASK_QUAD;
    case 6:
        return ORBIS_AJM_CHANNELMASK_5POINT1;
    case 8:
        return ORBIS_AJM_CHANNELMASK_7POINT1;
    default:
        UNREACHABLE();
    }
}

int PS4_SYSV_ABI sceAjmBatchCancel(const u32 context_id, const u32 batch_id) {
    LOG_INFO(Lib_Ajm, "called context_id = {} batch_id = {}", context_id, batch_id);

    auto it = contexts.find(context_id);
    if (it == contexts.end()) {
        return ORBIS_AJM_ERROR_INVALID_CONTEXT;
    }

    return it->second->BatchCancel(batch_id);
}

int PS4_SYSV_ABI sceAjmBatchErrorDump() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

void* PS4_SYSV_ABI sceAjmBatchJobControlBufferRa(void* p_buffer, u32 instance_id, u64 flags,
                                                 void* p_sideband_input, size_t sideband_input_size,
                                                 void* p_sideband_output,
                                                 size_t sideband_output_size,
                                                 void* p_return_address) {
    return BatchJobControlBufferRa(p_buffer, instance_id, flags, p_sideband_input,
                                   sideband_input_size, p_sideband_output, sideband_output_size,
                                   p_return_address);
}

void* PS4_SYSV_ABI sceAjmBatchJobInlineBuffer(void* p_buffer, const void* p_data_input,
                                              size_t data_input_size,
                                              const void** pp_batch_address) {
    return BatchJobInlineBuffer(p_buffer, p_data_input, data_input_size, pp_batch_address);
}

void* PS4_SYSV_ABI sceAjmBatchJobRunBufferRa(void* p_buffer, u32 instance_id, u64 flags,
                                             void* p_data_input, size_t data_input_size,
                                             void* p_data_output, size_t data_output_size,
                                             void* p_sideband_output, size_t sideband_output_size,
                                             void* p_return_address) {
    return BatchJobRunBufferRa(p_buffer, instance_id, flags, p_data_input, data_input_size,
                               p_data_output, data_output_size, p_sideband_output,
                               sideband_output_size, p_return_address);
}

void* PS4_SYSV_ABI sceAjmBatchJobRunSplitBufferRa(
    void* p_buffer, u32 instance_id, u64 flags, const AjmBuffer* p_data_input_buffers,
    size_t num_data_input_buffers, const AjmBuffer* p_data_output_buffers,
    size_t num_data_output_buffers, void* p_sideband_output, size_t sideband_output_size,
    void* p_return_address) {
    return BatchJobRunSplitBufferRa(p_buffer, instance_id, flags, p_data_input_buffers,
                                    num_data_input_buffers, p_data_output_buffers,
                                    num_data_output_buffers, p_sideband_output,
                                    sideband_output_size, p_return_address);
}

int PS4_SYSV_ABI sceAjmBatchStartBuffer(u32 context_id, u8* p_batch, u32 batch_size,
                                        const int priority, AjmBatchError* batch_error,
                                        u32* out_batch_id) {
    LOG_TRACE(Lib_Ajm, "called context = {}, batch_size = {:#x}, priority = {}", context_id,
              batch_size, priority);

    auto it = contexts.find(context_id);
    if (it == contexts.end()) {
        return ORBIS_AJM_ERROR_INVALID_CONTEXT;
    }

    return it->second->BatchStartBuffer(p_batch, batch_size, priority, batch_error, out_batch_id);
}

int PS4_SYSV_ABI sceAjmBatchWait(const u32 context_id, const u32 batch_id, const u32 timeout,
                                 AjmBatchError* const batch_error) {
    LOG_TRACE(Lib_Ajm, "called context = {}, batch_id = {}, timeout = {}", context_id, batch_id,
              timeout);

    auto it = contexts.find(context_id);
    if (it == contexts.end()) {
        return ORBIS_AJM_ERROR_INVALID_CONTEXT;
    }

    return it->second->BatchWait(batch_id, timeout, batch_error);
}

int PS4_SYSV_ABI sceAjmDecAt9ParseConfigData() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmDecMp3ParseFrame(const u8* buf, u32 stream_size, int parse_ofl,
                                        AjmDecMp3ParseFrame* frame) {
    return AjmMp3Decoder::ParseMp3Header(buf, stream_size, parse_ofl, frame);
}

int PS4_SYSV_ABI sceAjmFinalize() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInitialize(s64 reserved, u32* p_context_id) {
    LOG_INFO(Lib_Ajm, "called reserved = {}", reserved);
    if (p_context_id == nullptr || reserved != 0) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    u32 id = contexts.size() + 1;
    *p_context_id = id;
    contexts.emplace(id, std::make_unique<AjmContext>());
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceCodecType() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceCreate(u32 context_id, AjmCodecType codec_type,
                                      AjmInstanceFlags flags, u32* out_instance) {
    LOG_INFO(Lib_Ajm, "called context = {}, codec_type = {}, flags = {:#x}", context_id,
             magic_enum::enum_name(codec_type), flags.raw);

    auto it = contexts.find(context_id);
    if (it == contexts.end()) {
        return ORBIS_AJM_ERROR_INVALID_CONTEXT;
    }

    return it->second->InstanceCreate(codec_type, flags, out_instance);
}

int PS4_SYSV_ABI sceAjmInstanceDestroy(u32 context_id, u32 instance_id) {
    LOG_INFO(Lib_Ajm, "called context = {}, instance = {}", context_id, instance_id);

    auto it = contexts.find(context_id);
    if (it == contexts.end()) {
        return ORBIS_AJM_ERROR_INVALID_CONTEXT;
    }

    return it->second->InstanceDestroy(instance_id);
}

int PS4_SYSV_ABI sceAjmInstanceExtend() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceSwitch() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmMemoryRegister(u32 context_id, void* ptr, size_t num_pages) {
    // All memory is already shared with our implementation since we do not use any hardware.
    LOG_TRACE(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmMemoryUnregister(u32 context_id, void* ptr) {
    // All memory is already shared with our implementation since we do not use any hardware.
    LOG_TRACE(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmModuleRegister(u32 context_id, AjmCodecType codec_type, s64 reserved) {
    LOG_INFO(Lib_Ajm, "called context = {}, codec_type = {}", context_id, u32(codec_type));
    if (reserved != 0) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }

    auto it = contexts.find(context_id);
    if (it == contexts.end()) {
        return ORBIS_AJM_ERROR_INVALID_CONTEXT;
    }

    return it->second->ModuleRegister(codec_type);
}

int PS4_SYSV_ABI sceAjmModuleUnregister() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmStrError() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("NVDXiUesSbA", "libSceAjm", 1, "libSceAjm", sceAjmBatchCancel);
    LIB_FUNCTION("WfAiBW8Wcek", "libSceAjm", 1, "libSceAjm", sceAjmBatchErrorDump);
    LIB_FUNCTION("dmDybN--Fn8", "libSceAjm", 1, "libSceAjm", sceAjmBatchJobControlBufferRa);
    LIB_FUNCTION("stlghnic3Jc", "libSceAjm", 1, "libSceAjm", sceAjmBatchJobInlineBuffer);
    LIB_FUNCTION("ElslOCpOIns", "libSceAjm", 1, "libSceAjm", sceAjmBatchJobRunBufferRa);
    LIB_FUNCTION("7jdAXK+2fMo", "libSceAjm", 1, "libSceAjm", sceAjmBatchJobRunSplitBufferRa);
    LIB_FUNCTION("fFFkk0xfGWs", "libSceAjm", 1, "libSceAjm", sceAjmBatchStartBuffer);
    LIB_FUNCTION("-qLsfDAywIY", "libSceAjm", 1, "libSceAjm", sceAjmBatchWait);
    LIB_FUNCTION("1t3ixYNXyuc", "libSceAjm", 1, "libSceAjm", sceAjmDecAt9ParseConfigData);
    LIB_FUNCTION("eDFeTyi+G3Y", "libSceAjm", 1, "libSceAjm", sceAjmDecMp3ParseFrame);
    LIB_FUNCTION("MHur6qCsUus", "libSceAjm", 1, "libSceAjm", sceAjmFinalize);
    LIB_FUNCTION("dl+4eHSzUu4", "libSceAjm", 1, "libSceAjm", sceAjmInitialize);
    LIB_FUNCTION("diXjQNiMu-s", "libSceAjm", 1, "libSceAjm", sceAjmInstanceCodecType);
    LIB_FUNCTION("AxoDrINp4J8", "libSceAjm", 1, "libSceAjm", sceAjmInstanceCreate);
    LIB_FUNCTION("RbLbuKv8zho", "libSceAjm", 1, "libSceAjm", sceAjmInstanceDestroy);
    LIB_FUNCTION("YDFR0dDVGAg", "libSceAjm", 1, "libSceAjm", sceAjmInstanceExtend);
    LIB_FUNCTION("rgLjmfdXocI", "libSceAjm", 1, "libSceAjm", sceAjmInstanceSwitch);
    LIB_FUNCTION("bkRHEYG6lEM", "libSceAjm", 1, "libSceAjm", sceAjmMemoryRegister);
    LIB_FUNCTION("pIpGiaYkHkM", "libSceAjm", 1, "libSceAjm", sceAjmMemoryUnregister);
    LIB_FUNCTION("Q3dyFuwGn64", "libSceAjm", 1, "libSceAjm", sceAjmModuleRegister);
    LIB_FUNCTION("Wi7DtlLV+KI", "libSceAjm", 1, "libSceAjm", sceAjmModuleUnregister);
    LIB_FUNCTION("AxhcqVv5AYU", "libSceAjm", 1, "libSceAjm", sceAjmStrError);
};

} // namespace Libraries::Ajm
