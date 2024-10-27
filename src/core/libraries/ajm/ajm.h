// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/enum.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ajm {

struct AjmBatchInfo {
    void* pBuffer;
    u64 offset;
    u64 size;
};

struct AjmBatchError {
    int error_code;
    const void* job_addr;
    u32 cmd_offset;
    const void* job_ra;
};

struct AjmBuffer {
    u8* p_address;
    u64 size;
};

enum Identifier : u8 {
    AjmIdentJob = 0,
    AjmIdentInputRunBuf = 1,
    AjmIdentInputControlBuf = 2,
    AjmIdentControlFlags = 3,
    AjmIdentRunFlags = 4,
    AjmIdentReturnAddressBuf = 6,
    AjmIdentInlineBuf = 7,
    AjmIdentOutputRunBuf = 17,
    AjmIdentOutputControlBuf = 18,
};

struct AjmChunkHeader {
    u32 ident : 6;
    u32 payload : 20;
    u32 reserved : 6;
    u32 size;
};

struct AjmChunkBuffer {
    AjmChunkHeader header;
    void* p_address;
};

enum class AjmJobControlFlags : u64 {
    Reset = 1 << 0,
    Initialize = 1 << 1,
    Resample = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmJobControlFlags)

enum class AjmJobRunFlags : u64 {
    GetCodecInfo = 1 << 0,
    MultipleFrames = 1 << 1,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmJobRunFlags)

enum class AjmJobSidebandFlags : u64 {
    GaplessDecode = 1 << 0,
    Format = 1 << 1,
    Stream = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmJobSidebandFlags)

union AjmJobFlags {
    u64 raw;
    struct {
        u64 version : 3;
        u64 codec : 8;
        AjmJobRunFlags run_flags : 2;
        AjmJobControlFlags control_flags : 3;
        u64 reserved : 29;
        AjmJobSidebandFlags sideband_flags : 3;
    };
};

union AjmInstanceFlags {
    u64 raw;
    struct {
        u64 version : 3;
        u64 channels : 4;
        u64 format : 3;
        u64 pad : 22;
        u64 codec : 28;
    };
};

struct AjmDecMp3ParseFrame;
enum class AjmCodecType : u32;

int PS4_SYSV_ABI sceAjmBatchCancel();
int PS4_SYSV_ABI sceAjmBatchErrorDump();
void* PS4_SYSV_ABI sceAjmBatchJobControlBufferRa(void* p_buffer, u32 instance_id, u64 flags,
                                                 void* p_sideband_input, size_t sideband_input_size,
                                                 void* p_sideband_output,
                                                 size_t sideband_output_size,
                                                 void* p_return_address);
void* PS4_SYSV_ABI sceAjmBatchJobInlineBuffer(void* p_buffer, const void* p_data_input,
                                              size_t data_input_size,
                                              const void** pp_batch_address);
void* PS4_SYSV_ABI sceAjmBatchJobRunBufferRa(void* p_buffer, u32 instance_id, u64 flags,
                                             void* p_data_input, size_t data_input_size,
                                             void* p_data_output, size_t data_output_size,
                                             void* p_sideband_output, size_t sideband_output_size,
                                             void* p_return_address);
void* PS4_SYSV_ABI sceAjmBatchJobRunSplitBufferRa(
    void* p_buffer, u32 instance_id, u64 flags, const AjmBuffer* p_data_input_buffers,
    size_t num_data_input_buffers, const AjmBuffer* p_data_output_buffers,
    size_t num_data_output_buffers, void* p_sideband_output, size_t sideband_output_size,
    void* p_return_address);
int PS4_SYSV_ABI sceAjmBatchStartBuffer(u32 context, const u8* batch, u32 batch_size,
                                        const int priority, AjmBatchError* batch_error,
                                        u32* out_batch_id);
int PS4_SYSV_ABI sceAjmBatchWait(const u32 context, const u32 batch_id, const u32 timeout,
                                 AjmBatchError* const batch_error);
int PS4_SYSV_ABI sceAjmDecAt9ParseConfigData();
int PS4_SYSV_ABI sceAjmDecMp3ParseFrame(const u8* stream, u32 stream_size, int parse_ofl,
                                        AjmDecMp3ParseFrame* frame);
int PS4_SYSV_ABI sceAjmFinalize();
int PS4_SYSV_ABI sceAjmInitialize(s64 reserved, u32* out_context);
int PS4_SYSV_ABI sceAjmInstanceCodecType();
int PS4_SYSV_ABI sceAjmInstanceCreate(u32 context, AjmCodecType codec_type, AjmInstanceFlags flags,
                                      u32* instance);
int PS4_SYSV_ABI sceAjmInstanceDestroy(u32 context, u32 instance);
int PS4_SYSV_ABI sceAjmInstanceExtend();
int PS4_SYSV_ABI sceAjmInstanceSwitch();
int PS4_SYSV_ABI sceAjmMemoryRegister();
int PS4_SYSV_ABI sceAjmMemoryUnregister();
int PS4_SYSV_ABI sceAjmModuleRegister(u32 context, AjmCodecType codec_type, s64 reserved);
int PS4_SYSV_ABI sceAjmModuleUnregister();
int PS4_SYSV_ABI sceAjmStrError();

void RegisterlibSceAjm(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ajm
