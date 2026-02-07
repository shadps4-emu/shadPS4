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

constexpr u32 ORBIS_AT9_CONFIG_DATA_SIZE = 4;
constexpr u32 AJM_INSTANCE_STATISTICS = 0x80000;

enum class AjmCodecType : u32 {
    Mp3Dec = 0,
    At9Dec = 1,
    M4aacDec = 2,
    Max = 23,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmCodecType);

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

enum class AjmStatisticsFlags : u64 {
    Memory = 1 << 0,
    EnginePerCodec = 1 << 15,
    Engine = 1 << 16,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmStatisticsFlags)

union AjmStatisticsJobFlags {
    u64 raw;
    struct {
        u64 version : 3;
        u64 : 12;
        AjmStatisticsFlags statistics_flags : 17;
        u64 : 32;
    };
};
static_assert(sizeof(AjmStatisticsJobFlags) == 8);

struct AjmSidebandResult {
    s32 result;
    s32 internal_result;
};

struct AjmSidebandMFrame {
    u32 num_frames;
    u32 reserved;
};

struct AjmSidebandStream {
    s32 input_consumed;
    s32 output_written;
    u64 total_decoded_samples;
};

enum class AjmFormatEncoding : u32 {
    S16 = 0,
    S32 = 1,
    Float = 2,
};

struct AjmSidebandFormat {
    u32 num_channels;
    u32 channel_mask;
    u32 sampl_freq;
    AjmFormatEncoding sample_encoding;
    u32 bitrate;
    u32 reserved;
};

struct AjmSidebandGaplessDecode {
    u32 total_samples;
    u16 skip_samples;
    u16 skipped_samples;
};

struct AjmSidebandResampleParameters {
    float ratio;
    u32 flags;
};

struct AjmDecAt9InitializeParameters {
    u8 config_data[ORBIS_AT9_CONFIG_DATA_SIZE];
    u32 reserved;
};

union AjmSidebandInitParameters {
    AjmDecAt9InitializeParameters at9;
    u8 reserved[8];
};

struct AjmSidebandStatisticsEngine {
    float usage_batch;
    float usage_interval[3];
};

struct AjmSidebandStatisticsEnginePerCodec {
    u8 codec_count;
    u8 codec_id[3];
    float codec_percentage[3];
};

struct AjmSidebandStatisticsMemory {
    u32 instance_free;
    u32 buffer_free;
    u32 batch_size;
    u32 input_size;
    u32 output_size;
    u32 small_size;
};

struct AjmSidebandStatisticsEngineParameters {
    u32 interval_count;
    float interval[3];
};

union AjmInstanceFlags {
    u64 raw;
    struct {
        u64 version : 3;
        u64 channels : 4;
        u64 format : 3;
        u64 gapless_loop : 1;
        u64 : 21;
        u64 codec : 28;
    };
};
static_assert(sizeof(AjmInstanceFlags) == 8);

struct AjmDecMp3ParseFrame;

u32 GetChannelMask(u32 num_channels);

int PS4_SYSV_ABI sceAjmBatchCancel(const u32 context_id, const u32 batch_id);
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
int PS4_SYSV_ABI sceAjmBatchStartBuffer(u32 context, u8* batch, u32 batch_size, const int priority,
                                        AjmBatchError* batch_error, u32* out_batch_id);
int PS4_SYSV_ABI sceAjmBatchWait(const u32 context, const u32 batch_id, const u32 timeout,
                                 AjmBatchError* const batch_error);
int PS4_SYSV_ABI sceAjmDecAt9ParseConfigData();
int PS4_SYSV_ABI sceAjmDecMp3ParseFrame(const u8* stream, u32 stream_size, int parse_ofl,
                                        AjmDecMp3ParseFrame* frame);
int PS4_SYSV_ABI sceAjmFinalize();
int PS4_SYSV_ABI sceAjmInitialize(s64 reserved, u32* out_context);
AjmCodecType PS4_SYSV_ABI sceAjmInstanceCodecType(u32 instance_id);
int PS4_SYSV_ABI sceAjmInstanceCreate(u32 context, AjmCodecType codec_type, AjmInstanceFlags flags,
                                      u32* instance);
int PS4_SYSV_ABI sceAjmInstanceDestroy(u32 context, u32 instance);
int PS4_SYSV_ABI sceAjmInstanceExtend();
int PS4_SYSV_ABI sceAjmInstanceSwitch();
int PS4_SYSV_ABI sceAjmMemoryRegister(u32 context_id, void* ptr, size_t num_pages);
int PS4_SYSV_ABI sceAjmMemoryUnregister(u32 context_id, void* ptr);
int PS4_SYSV_ABI sceAjmModuleRegister(u32 context, AjmCodecType codec_type, s64 reserved);
int PS4_SYSV_ABI sceAjmModuleUnregister();
int PS4_SYSV_ABI sceAjmStrError();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Ajm
