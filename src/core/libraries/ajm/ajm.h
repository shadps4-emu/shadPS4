// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "common/enum.h"
#include "core/libraries/ajm/ajm_mp3.h"

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
    u8* addr;
    u64 size;
};

struct AjmJobBuffer {
    u32 props;
    u32 buf_size;
    const u8* buffer;
};

struct AjmInOutJob {
    AjmJobBuffer input;
    u32 unk1;
    u32 flags;
    AjmJobBuffer output;
};

enum class AjmJobControlFlags : u32 {
    Reset = 1 << 2,
    Initialize = 1 << 3,
    Resample = 1 << 4,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmJobControlFlags)

enum class AjmJobRunFlags : u32 {
    GetCodecInfo = 1 << 0,
    MultipleFrames = 1 << 1,
};

enum class AjmJobSidebandFlags : u32 {
    GaplessDecode = 1 << 0,
    GetInfo = 1 << 1,
    Stream = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmJobSidebandFlags)

struct AjmHLEOpcode {
    u32 instance : 14;
    u32 codec_flags : 8;
    u32 command_flags : 4;
    AjmJobSidebandFlags sideband_flags : 3;
    u32 is_statistic : 1;
    u32 is_debug : 1;
    u32 is_control : 1;
};

struct AjmJobHeader {
    AjmHLEOpcode opcode;
    u32 job_size;
};

struct AjmSingleJob {
    AjmHLEOpcode opcode;
    u32 job_size;
    union {
        AjmInOutJob job;
        struct {
            u32 unk1;
            u32 unk2;
            const void* ret_addr;
            AjmInOutJob job;
        } ret;
    };
};

struct AjmMultiJob {
    AjmHLEOpcode opcode;
    u32 job_size;
    union {
        u32 job[];
        struct {
            u32 unk1;
            u32 unk2;
            const void* ret_addr;
            u32 job[];
        } ret;
    };
};

enum class AjmCodecType : u32 {
    Mp3Dec = 0,
    At9Dec = 1,
    M4aacDec = 2,
    Max = 23,
};
static constexpr u32 NumAjmCodecs = u32(AjmCodecType::Max);

union AjmFlags {
    u64 raw;
    struct {
        u64 version : 3;
        u64 codec : 8;
        u64 command : 4;
        u64 reserved : 30;
        u64 sideband : 3;
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

int PS4_SYSV_ABI sceAjmBatchCancel();
int PS4_SYSV_ABI sceAjmBatchErrorDump();
void* PS4_SYSV_ABI sceAjmBatchJobControlBufferRa(AjmSingleJob* batch_pos, u32 instance, AjmFlags flags,
                                                 const u8* in_buffer, u32 in_size, u8* out_buffer,
                                                 u32 out_size, const void* ret_addr);
int PS4_SYSV_ABI sceAjmBatchJobInlineBuffer();
int PS4_SYSV_ABI sceAjmBatchJobRunBufferRa();
void* PS4_SYSV_ABI sceAjmBatchJobRunSplitBufferRa(AjmMultiJob* batch_pos, u32 instance, AjmFlags flags,
                                                  const AjmBuffer* input_buffers,
                                                  u64 num_input_buffers,
                                                  const AjmBuffer* output_buffers,
                                                  u64 num_output_buffers,
                                                  void* sideband_output,
                                                  u64 sideband_output_size,
                                                  const void* ret_addr);
int PS4_SYSV_ABI sceAjmBatchStartBuffer(u32 context, const u8* batch, u32 batch_size, const int priority,
                                        AjmBatchError* patch_error, u32* out_batch_id);
int PS4_SYSV_ABI sceAjmBatchWait();
int PS4_SYSV_ABI sceAjmDecAt9ParseConfigData();
int PS4_SYSV_ABI sceAjmDecMp3ParseFrame(const u8* stream, u32 stream_size, int parse_ofl,
                                        AjmDecMp3ParseFrame* frame);
int PS4_SYSV_ABI sceAjmFinalize();
int PS4_SYSV_ABI sceAjmInitialize(s64 reserved, u32* out_context);
int PS4_SYSV_ABI sceAjmInstanceCodecType();
int PS4_SYSV_ABI sceAjmInstanceCreate(u32 context, AjmCodecType codec_type, AjmInstanceFlags flags, u32* instance);
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
