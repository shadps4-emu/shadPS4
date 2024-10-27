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
    u8* addr;
    u64 size;
};

enum class Identifier : u32 {
    InputRunBuf = 1,
    InputControlBuf = 2,
    ControlFlags = 3,
    RunFlags = 4,
    ReturnAddrBuf = 6,
    OutputMultijobBuf = 17,
    OutputRunControlBuf = 18,
    IdentMask = 0xff,
};

struct AjmJobBuffer {
    Identifier ident;
    u32 buf_size;
    u8* buffer;
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
    GetInfo = 1 << 1,
    Stream = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmJobSidebandFlags)

union AjmFlags {
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

struct AjmFlagsIdentifier {
    union {
        u32 raw1;
        BitField<0, 4, Identifier> identifier;
        BitField<19, 3, u32> sideband_flags;
    };
    union {
        u32 raw2;
        BitField<0, 3, u32> version;
        BitField<3, 8, u32> codec;
        BitField<11, 2, u32> run_flags;
        BitField<13, 3, u32> control_flags;
    };
};

struct AjmControlJobInner {
    AjmJobBuffer input;
    AjmFlagsIdentifier flags;
    AjmJobBuffer output;
};

struct AjmRunJobInner : public AjmControlJobInner {
    AjmJobBuffer sideband;
};

struct AjmJobHeader {
    struct {
        u32 : 6;
        u32 instance : 16;
    };
    u32 job_size;
};

struct AjmControlJob {
    AjmJobHeader header;
    union {
        AjmControlJobInner job;
        struct {
            AjmJobBuffer ret_buf;
            AjmControlJobInner job;
        } ret;
    };
};
static_assert(sizeof(AjmControlJob) == 64);

struct AjmRunJob {
    AjmJobHeader header;
    union {
        AjmRunJobInner job;
        struct {
            AjmJobBuffer ret_buf;
            AjmRunJobInner job;
        } ret;
    };
};
static_assert(sizeof(AjmRunJob) == 80);

struct AjmMultiJob {
    AjmJobHeader header;
    union {
        u32 job[];
        struct {
            AjmJobBuffer ret_buf;
            u32 job[];
        } ret;
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
void* PS4_SYSV_ABI sceAjmBatchJobControlBufferRa(AjmControlJob* batch_pos, u32 instance,
                                                 AjmFlags flags, u8* in_buffer, u32 in_size,
                                                 u8* out_buffer, u32 out_size, void* ret_addr);
void* PS4_SYSV_ABI sceAjmBatchJobInlineBuffer(u8* batch_pos, const void* in_buffer, u64 in_size,
                                              const void** batch_address);
void* PS4_SYSV_ABI sceAjmBatchJobRunBufferRa(AjmRunJob* batch_pos, u32 instance, AjmFlags flags,
                                             u8* in_buffer, u32 in_size, u8* out_buffer,
                                             const u32 out_size, u8* sideband_output,
                                             const u32 sideband_output_size, void* ret_addr);
void* PS4_SYSV_ABI sceAjmBatchJobRunSplitBufferRa(AjmMultiJob* batch_pos, u32 instance,
                                                  AjmFlags flags, const AjmBuffer* input_buffers,
                                                  u64 num_input_buffers,
                                                  const AjmBuffer* output_buffers,
                                                  u64 num_output_buffers, void* sideband_output,
                                                  u64 sideband_output_size, void* ret_addr);
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
