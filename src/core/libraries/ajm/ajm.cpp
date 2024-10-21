// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma clang optimize off
#include <numeric>
#include <magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/ajm/ajm_error.h"
#include "core/libraries/ajm/ajm_instance.h"
#include "core/libraries/ajm/ajm_mp3.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace Libraries::Ajm {

static constexpr u32 AJM_INSTANCE_STATISTICS = 0x80000;

static constexpr u32 MaxInstances = 0x2fff;

struct AjmDevice {
    u32 max_prio;
    u32 min_prio;
    u32 curr_cursor{};
    u32 release_cursor{MaxInstances - 1};
    std::array<bool, NumAjmCodecs> is_registered{};
    std::array<u32, MaxInstances> free_instances{};
    std::array<std::unique_ptr<AjmInstance>, MaxInstances> instances;

    bool IsRegistered(AjmCodecType type) const {
        return is_registered[static_cast<u32>(type)];
    }

    void Register(AjmCodecType type) {
        is_registered[static_cast<u32>(type)] = true;
    }

    AjmDevice() {
        std::iota(free_instances.begin(), free_instances.end(), 1);
    }
};

static std::unique_ptr<AjmDevice> dev{};

int PS4_SYSV_ABI sceAjmBatchCancel() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchErrorDump() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

void* PS4_SYSV_ABI sceAjmBatchJobControlBufferRa(AjmSingleJob* batch_pos, u32 instance,
                                                 AjmFlags flags, u8* in_buffer, u32 in_size,
                                                 u8* out_buffer, u32 out_size,
                                                 const void* ret_addr) {
    LOG_INFO(Lib_Ajm,
             "called instance = {:#x}, flags = {:#x}, cmd = {}, in_size = {:#x}, out_size = {:#x}, "
             "ret_addr = {}",
             instance, flags.raw, magic_enum::enum_name(AjmJobControlFlags(flags.command)), in_size,
             out_size, fmt::ptr(ret_addr));

    const u64 mask = instance == AJM_INSTANCE_STATISTICS ? 0xc0018007ULL : 0x60000000e7ffULL;
    flags.raw &= mask;

    const bool is_debug = ret_addr != nullptr;
    batch_pos->opcode.instance = instance;
    batch_pos->opcode.codec_flags = flags.codec;
    batch_pos->opcode.command_flags = flags.command;
    batch_pos->opcode.sideband_flags = AjmJobSidebandFlags(flags.sideband);
    batch_pos->opcode.is_debug = is_debug;
    batch_pos->opcode.is_statistic = instance == AJM_INSTANCE_STATISTICS;
    batch_pos->opcode.is_control = true;

    AjmInOutJob* job = nullptr;
    if (ret_addr == nullptr) {
        batch_pos->job_size = sizeof(AjmInOutJob);
        job = &batch_pos->job;
    } else {
        batch_pos->job_size = sizeof(AjmInOutJob) + 16;
        batch_pos->ret.unk1 = batch_pos->ret.unk1 & 0xfffffff0 | 6;
        batch_pos->ret.unk2 = 0;
        batch_pos->ret.ret_addr = ret_addr;
        job = &batch_pos->ret.job;
    }

    job->input.props &= 0xffffffe0;
    job->input.props |= 2;
    job->input.buf_size = in_size;
    job->input.buffer = in_buffer;
    job->unk1 = (job->unk1 & 0xfc000030) + ((flags.raw >> 0x1a) & 0x180000) + 3;
    job->flags = u32(flags.raw);
    job->output.props &= 0xffffffe0;
    job->output.props |= 0x12;
    job->output.buf_size = out_size;
    job->output.buffer = out_buffer;
    return ++job;
}

int PS4_SYSV_ABI sceAjmBatchJobInlineBuffer() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchJobRunBufferRa() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

void* PS4_SYSV_ABI sceAjmBatchJobRunSplitBufferRa(AjmMultiJob* batch_pos, u32 instance,
                                                  AjmFlags flags, const AjmBuffer* in_buffers,
                                                  u64 num_in_buffers, const AjmBuffer* out_buffers,
                                                  u64 num_out_buffers, void* sideband_output,
                                                  u64 sideband_output_size, const void* ret_addr) {
    LOG_DEBUG(Lib_Ajm,
              "called instance = {}, flags = {:#x}, cmd = {}, sideband_cmd = {} num_input_buffers "
              "= {}, num_output_buffers = {}, "
              "ret_addr = {}",
              instance, flags.raw, magic_enum::enum_name(AjmJobRunFlags(flags.command)),
              magic_enum::enum_name(AjmJobSidebandFlags(flags.sideband)), num_in_buffers,
              num_out_buffers, fmt::ptr(ret_addr));
    const u32 job_size = (num_in_buffers * 2 + 1 + num_out_buffers * 2) * 8;
    const bool is_debug = ret_addr != nullptr;
    batch_pos->opcode.instance = instance;
    batch_pos->opcode.codec_flags = flags.codec;
    batch_pos->opcode.command_flags = flags.command;
    batch_pos->opcode.sideband_flags = AjmJobSidebandFlags(flags.sideband);
    batch_pos->opcode.is_debug = is_debug;
    batch_pos->opcode.is_statistic = false;
    batch_pos->opcode.is_control = false;

    u32* job = nullptr;
    if (!is_debug) {
        batch_pos->job_size = job_size + 16;
        job = batch_pos->job;
    } else {
        batch_pos->job_size = job_size + 32;
        batch_pos->ret.unk1 &= 0xfffffff0;
        batch_pos->ret.unk1 |= 6;
        batch_pos->ret.unk2 = 0;
        batch_pos->ret.ret_addr = ret_addr;
        job = batch_pos->ret.job;
    }

    for (s32 i = 0; i < num_in_buffers; i++) {
        AjmJobBuffer* in_buf = reinterpret_cast<AjmJobBuffer*>(job);
        in_buf->props &= 0xffffffe0;
        in_buf->props |= 1;
        in_buf->buf_size = in_buffers[i].size;
        in_buf->buffer = in_buffers[i].addr;
        job += 4;
    }
    job[1] = u32(flags.raw & 0xe00000001fffULL);
    job[0] &= 0xfc000030;
    job[0] = s32((flags.raw & 0xe00000001fffULL) >> 0x1a) + 4;
    job += 2;

    for (s32 i = 0; i < num_out_buffers; i++) {
        AjmJobBuffer* out_buf = reinterpret_cast<AjmJobBuffer*>(job);
        out_buf->props &= 0xffffffe0;
        out_buf->props |= 0x11;
        out_buf->buf_size = out_buffers[i].size;
        out_buf->buffer = out_buffers[i].addr;
        job += 4;
    }
    job[0] = job[0] & 0xffffffe0 | 0x12;
    job[1] = sideband_output_size;
    memcpy(&job[2], &sideband_output, sizeof(void*));
    return job + 4;
}

int PS4_SYSV_ABI sceAjmBatchStartBuffer(u32 context, const u8* batch, u32 batch_size,
                                        const int priority, AjmBatchError* patch_error,
                                        u32* out_batch_id) {
    LOG_DEBUG(Lib_Ajm, "called context = {}, batch_size = {:#x}, priority = {}", context,
              batch_size, priority);

    if ((batch_size & 7) != 0) {
        return ORBIS_AJM_ERROR_MALFORMED_BATCH;
    }

    static constexpr u32 MaxBatches = 1000;

    struct BatchInfo {
        u16 instance;
        u16 offset_in_qwords;
    };
    std::array<BatchInfo, MaxBatches> batches{};
    u32 num_batches = 0;

    const u8* batch_ptr = batch;
    const u8* batch_end = batch + batch_size;
    while (batch_ptr < batch_end) {
        if (num_batches >= MaxBatches) {
            LOG_ERROR(Lib_Ajm, "Too many batches in job!");
            return ORBIS_AJM_ERROR_OUT_OF_MEMORY;
        }
        AjmJobHeader header;
        std::memcpy(&header, batch_ptr, sizeof(u64));

        const auto& opcode = header.opcode;
        const u32 instance = opcode.instance;
        const u8* job_ptr = batch_ptr + sizeof(AjmJobHeader) + opcode.is_debug * 16;

        if (opcode.is_control) {
            ASSERT_MSG(!opcode.is_statistic, "Statistic instance is not handled");
            const auto command = AjmJobControlFlags(opcode.command_flags);
            switch (command) {
            case AjmJobControlFlags::Reset: {
                LOG_INFO(Lib_Ajm, "Resetting instance {}", opcode.instance);
                dev->instances[opcode.instance]->Reset();
                break;
            }
            case (AjmJobControlFlags::Initialize | AjmJobControlFlags::Reset):
                LOG_INFO(Lib_Ajm, "Initializing instance {}", opcode.instance);
                break;
            case AjmJobControlFlags::Resample:
                LOG_INFO(Lib_Ajm, "Set resample params of instance {}", opcode.instance);
                break;
            default:
                break;
            }

            // Write sideband structures.
            const AjmJobBuffer* out_buffer = reinterpret_cast<const AjmJobBuffer*>(job_ptr + 24);
            auto* result = reinterpret_cast<AjmSidebandResult*>(out_buffer->buffer);
            result->result = 0;
            result->internal_result = 0;
        } else {
            const auto command = AjmJobRunFlags(opcode.command_flags);
            const auto sideband = AjmJobSidebandFlags(opcode.sideband_flags);
            const AjmJobBuffer* in_buffer = reinterpret_cast<const AjmJobBuffer*>(job_ptr);
            const AjmJobBuffer* out_buffer = reinterpret_cast<const AjmJobBuffer*>(job_ptr + 24);
            job_ptr += 24;

            LOG_INFO(Lib_Ajm, "Decode job cmd = {}, sideband = {}, in_addr = {}, in_size = {}",
                     magic_enum::enum_name(command), magic_enum::enum_name(sideband),
                     fmt::ptr(in_buffer->buffer), in_buffer->buf_size);

            // Decode as much of the input bitstream as possible.
            auto* instance = dev->instances[opcode.instance].get();
            const auto [in_remain, out_remain, num_frames] = instance->Decode(
                in_buffer->buffer, in_buffer->buf_size, out_buffer->buffer, out_buffer->buf_size);

            // Write sideband structures.
            auto* sideband_ptr = *reinterpret_cast<u8* const*>(job_ptr + 8);
            auto* result = reinterpret_cast<AjmSidebandResult*>(sideband_ptr);
            result->result = 0;
            result->internal_result = 0;
            sideband_ptr += sizeof(AjmSidebandResult);

            // Check sideband flags
            if (True(sideband & AjmJobSidebandFlags::Stream)) {
                auto* stream = reinterpret_cast<AjmSidebandStream*>(sideband_ptr);
                stream->input_consumed = in_buffer->buf_size - in_remain;
                stream->output_written = out_buffer->buf_size - out_remain;
                stream->total_decoded_samples = instance->decoded_samples;
                sideband_ptr += sizeof(AjmSidebandStream);
            }
            if (True(command & AjmJobRunFlags::MultipleFrames)) {
                auto* mframe = reinterpret_cast<AjmSidebandMFrame*>(sideband_ptr);
                mframe->num_frames = num_frames;
                sideband_ptr += sizeof(AjmSidebandMFrame);
            }
        }

        batch_ptr += sizeof(AjmJobHeader) + header.job_size;
    }
    static int batch_id = 0;
    *out_batch_id = ++batch_id;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchWait() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmDecAt9ParseConfigData() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmDecMp3ParseFrame(const u8* buf, u32 stream_size, int parse_ofl,
                                        AjmDecMp3ParseFrame* frame) {
    LOG_INFO(Lib_Ajm, "called parse_ofl = {}", parse_ofl);
    if (buf == nullptr || stream_size < 4 || frame == nullptr) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    if ((buf[0] & SYNCWORDH) != SYNCWORDH || (buf[1] & SYNCWORDL) != SYNCWORDL) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    return AjmMp3Decoder::ParseMp3Header(buf, stream_size, parse_ofl, frame);
}

int PS4_SYSV_ABI sceAjmFinalize() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInitialize(s64 reserved, u32* out_context) {
    LOG_INFO(Lib_Ajm, "called reserved = {}", reserved);
    if (out_context == nullptr || reserved != 0) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    *out_context = 1;
    dev = std::make_unique<AjmDevice>();
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceCodecType() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceCreate(u32 context, AjmCodecType codec_type, AjmInstanceFlags flags,
                                      u32* out_instance) {
    if (codec_type >= AjmCodecType::Max) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    if (flags.version == 0) {
        return ORBIS_AJM_ERROR_WRONG_REVISION_FLAG;
    }
    if (!dev->IsRegistered(codec_type)) {
        return ORBIS_AJM_ERROR_CODEC_NOT_REGISTERED;
    }
    if (dev->curr_cursor == dev->release_cursor) {
        return ORBIS_AJM_ERROR_OUT_OF_RESOURCES;
    }
    const u32 index = dev->free_instances[dev->curr_cursor++];
    dev->curr_cursor %= MaxInstances;
    auto instance = std::make_unique<AjmMp3Decoder>();
    instance->index = index;
    instance->codec_type = codec_type;
    instance->num_channels = flags.channels;
    dev->instances[index] = std::move(instance);
    *out_instance = index;
    LOG_INFO(Lib_Ajm, "called codec_type = {}, flags = {:#x}, instance = {}",
             magic_enum::enum_name(codec_type), flags.raw, index);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmInstanceDestroy(u32 context, u32 instance) {
    LOG_INFO(Lib_Ajm, "called context = {}, instance = {}", context, instance);
    if ((instance & 0x3fff) > MaxInstances) {
        return ORBIS_AJM_ERROR_INVALID_INSTANCE;
    }
    const u32 next_slot = (dev->release_cursor + 1) % MaxInstances;
    if (next_slot != dev->curr_cursor) {
        dev->free_instances[dev->release_cursor] = instance;
        dev->release_cursor = next_slot;
    }
    dev->instances[instance].reset();
    return ORBIS_OK;
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

int PS4_SYSV_ABI sceAjmModuleRegister(u32 context, AjmCodecType codec_type, s64 reserved) {
    LOG_INFO(Lib_Ajm, "called context = {}, codec_type = {}, reserved = {}", context,
             u32(codec_type), reserved);
    if (codec_type >= AjmCodecType::Max || reserved != 0) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    if (dev->IsRegistered(codec_type)) {
        return ORBIS_AJM_ERROR_CODEC_ALREADY_REGISTERED;
    }
    dev->Register(codec_type);
    return ORBIS_OK;
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