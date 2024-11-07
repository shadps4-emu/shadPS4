// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/ajm/ajm_context.h"
#include "core/libraries/ajm/ajm_error.h"
#include "core/libraries/ajm/ajm_mp3.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

#include <magic_enum.hpp>

namespace Libraries::Ajm {

constexpr int ORBIS_AJM_CHANNELMASK_MONO = 0x0004;
constexpr int ORBIS_AJM_CHANNELMASK_STEREO = 0x0003;
constexpr int ORBIS_AJM_CHANNELMASK_QUAD = 0x0033;
constexpr int ORBIS_AJM_CHANNELMASK_5POINT1 = 0x060F;
constexpr int ORBIS_AJM_CHANNELMASK_7POINT1 = 0x063F;

static std::unique_ptr<AjmContext> context{};

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
    return context->BatchCancel(batch_id);
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
    return context->BatchStartBuffer(p_batch, batch_size, priority, batch_error, out_batch_id);
}

int PS4_SYSV_ABI sceAjmBatchWait(const u32 context_id, const u32 batch_id, const u32 timeout,
                                 AjmBatchError* const batch_error) {
    LOG_TRACE(Lib_Ajm, "called context = {}, batch_id = {}, timeout = {}", context_id, batch_id,
              timeout);
    return context->BatchWait(batch_id, timeout, batch_error);
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
    ASSERT_MSG(context == nullptr, "Multiple contexts are currently unsupported.");
    if (p_context_id == nullptr || reserved != 0) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    *p_context_id = 1;
    context = std::make_unique<AjmContext>();
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
    return context->InstanceCreate(codec_type, flags, out_instance);
}

int PS4_SYSV_ABI sceAjmInstanceDestroy(u32 context_id, u32 instance_id) {
    LOG_INFO(Lib_Ajm, "called context = {}, instance = {}", context_id, instance_id);
    return context->InstanceDestroy(instance_id);
}

int PS4_SYSV_ABI sceAjmInstanceExtend() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceSwitch() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmMemoryRegister() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmMemoryUnregister() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmModuleRegister(u32 context_id, AjmCodecType codec_type, s64 reserved) {
    LOG_INFO(Lib_Ajm, "called context = {}, codec_type = {}", context_id, u32(codec_type));
    if (reserved != 0) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    return context->ModuleRegister(codec_type);
}

int PS4_SYSV_ABI sceAjmModuleUnregister() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmStrError() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceAjm(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("NVDXiUesSbA", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchCancel);
    LIB_FUNCTION("WfAiBW8Wcek", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchErrorDump);
    LIB_FUNCTION("dmDybN--Fn8", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchJobControlBufferRa);
    LIB_FUNCTION("stlghnic3Jc", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchJobInlineBuffer);
    LIB_FUNCTION("ElslOCpOIns", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchJobRunBufferRa);
    LIB_FUNCTION("7jdAXK+2fMo", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchJobRunSplitBufferRa);
    LIB_FUNCTION("fFFkk0xfGWs", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchStartBuffer);
    LIB_FUNCTION("-qLsfDAywIY", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmBatchWait);
    LIB_FUNCTION("1t3ixYNXyuc", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmDecAt9ParseConfigData);
    LIB_FUNCTION("eDFeTyi+G3Y", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmDecMp3ParseFrame);
    LIB_FUNCTION("MHur6qCsUus", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmFinalize);
    LIB_FUNCTION("dl+4eHSzUu4", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInitialize);
    LIB_FUNCTION("diXjQNiMu-s", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceCodecType);
    LIB_FUNCTION("AxoDrINp4J8", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceCreate);
    LIB_FUNCTION("RbLbuKv8zho", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceDestroy);
    LIB_FUNCTION("YDFR0dDVGAg", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceExtend);
    LIB_FUNCTION("rgLjmfdXocI", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmInstanceSwitch);
    LIB_FUNCTION("bkRHEYG6lEM", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmMemoryRegister);
    LIB_FUNCTION("pIpGiaYkHkM", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmMemoryUnregister);
    LIB_FUNCTION("Q3dyFuwGn64", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmModuleRegister);
    LIB_FUNCTION("Wi7DtlLV+KI", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmModuleUnregister);
    LIB_FUNCTION("AxhcqVv5AYU", "libSceAjm", 1, "libSceAjm", 1, 1, sceAjmStrError);
};

} // namespace Libraries::Ajm
