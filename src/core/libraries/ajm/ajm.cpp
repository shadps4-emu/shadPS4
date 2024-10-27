// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <condition_variable>
#include <mutex>
#include <numeric>
#include <boost/container/small_vector.hpp>
#include <magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/ajm/ajm_at9.h"
#include "core/libraries/ajm/ajm_error.h"
#include "core/libraries/ajm/ajm_instance.h"
#include "core/libraries/ajm/ajm_mp3.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

extern "C" {
#include <libatrac9.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <structures.h>
}

namespace Libraries::Ajm {

static constexpr u32 AJM_INSTANCE_STATISTICS = 0x80000;

static constexpr u32 SCE_AJM_WAIT_INFINITE = -1;

static constexpr u32 MaxInstances = 0x2fff;

static constexpr u32 MaxBatches = 1000;

struct BatchInfo {
    u16 instance{};
    u16 offset_in_qwords{}; // Needed for AjmBatchError?
    bool waiting{};
    bool finished{};
    std::mutex mtx;
    std::condition_variable cv;
    int result{};
};

struct AjmDevice {
    u32 max_prio{};
    u32 min_prio{};
    u32 curr_cursor{};
    u32 release_cursor{MaxInstances - 1};
    std::array<bool, NumAjmCodecs> is_registered{};
    std::array<u32, MaxInstances> free_instances{};
    std::array<std::unique_ptr<AjmInstance>, MaxInstances> instances;
    std::vector<std::shared_ptr<BatchInfo>> batches{};
    std::mutex batches_mutex;

    [[nodiscard]] bool IsRegistered(AjmCodecType type) const {
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

void* PS4_SYSV_ABI sceAjmBatchJobControlBufferRa(AjmControlJob* batch_pos, u32 instance,
                                                 AjmFlags flags, u8* in_buffer, u32 in_size,
                                                 u8* out_buffer, u32 out_size, void* ret_addr) {
    LOG_INFO(Lib_Ajm,
             "called instance = {:#x}, flags = {:#x}, cmd = {}, in_size = {:#x}, out_size = {:#x}, "
             "ret_addr = {}",
             instance, flags.raw, magic_enum::enum_name(flags.control_flags), in_size, out_size,
             fmt::ptr(ret_addr));

    const u64 mask = instance == AJM_INSTANCE_STATISTICS ? 0xc0018007ULL : 0x60000000e7ffULL;
    flags.raw &= mask;

    batch_pos->header.instance = instance;

    AjmControlJobInner* job;
    if (ret_addr == nullptr) {
        batch_pos->header.job_size = sizeof(AjmControlJobInner);
        job = &batch_pos->job;
    } else {
        batch_pos->header.job_size = sizeof(AjmControlJobInner) + sizeof(AjmJobBuffer);
        batch_pos->ret.ret_buf.ident = Identifier::ReturnAddrBuf;
        batch_pos->ret.ret_buf.buf_size = 0;
        batch_pos->ret.ret_buf.buffer = (u8*)ret_addr;
        job = &batch_pos->ret.job;
    }

    job->input.ident = Identifier::InputControlBuf;
    job->input.buf_size = in_size;
    job->input.buffer = in_buffer;
    job->flags.raw1 = (job->flags.raw1 & 0xfc000000) + ((flags.raw >> 0x1a) & 0x180000) + 3;
    job->flags.raw2 = u32(flags.raw);
    job->output.ident = Identifier::OutputRunControlBuf;
    job->output.buf_size = out_size;
    job->output.buffer = out_buffer;
    return ++job;
}

void* PS4_SYSV_ABI sceAjmBatchJobInlineBuffer(u8* batch_pos, const void* in_buffer, size_t in_size,
                                              const void** batch_address) {
    // TODO
    return nullptr;
}

void* PS4_SYSV_ABI sceAjmBatchJobRunBufferRa(AjmRunJob* batch_pos, u32 instance, AjmFlags flags,
                                             u8* in_buffer, u32 in_size, u8* out_buffer,
                                             const u32 out_size, u8* sideband_output,
                                             const u32 sideband_output_size, void* ret_addr) {
    LOG_INFO(Lib_Ajm,
             "called instance = {:#x}, flags = {:#x}, cmd = {}, in_size = {:#x}, out_size = {:#x}, "
             "ret_addr = {}",
             instance, flags.raw, magic_enum::enum_name(flags.run_flags), in_size, out_size,
             fmt::ptr(ret_addr));

    const u64 mask = 0xE00000001FFFLL;
    flags.raw &= mask;

    batch_pos->header.instance = instance;

    AjmRunJobInner* job;
    if (ret_addr == nullptr) {
        batch_pos->header.job_size = sizeof(AjmRunJobInner);
        job = &batch_pos->job;
    } else {
        batch_pos->header.job_size = sizeof(AjmRunJobInner) + sizeof(AjmJobBuffer);
        batch_pos->ret.ret_buf.ident = Identifier::ReturnAddrBuf;
        batch_pos->ret.ret_buf.buf_size = 0;
        batch_pos->ret.ret_buf.buffer = (u8*)ret_addr;
        job = &batch_pos->ret.job;
    }

    job->input.ident = Identifier::InputRunBuf;
    job->input.buf_size = in_size;
    job->input.buffer = in_buffer;
    job->flags.raw1 = (job->flags.raw1 & 0xfc000000) + (flags.raw >> 0x1a) + 4;
    job->flags.raw2 = u32(flags.raw);
    job->output.ident = Identifier::OutputRunControlBuf;
    job->output.buf_size = out_size;
    job->output.buffer = out_buffer;
    job->sideband.ident = Identifier::OutputRunControlBuf;
    job->sideband.buf_size = sideband_output_size;
    job->sideband.buffer = sideband_output;
    return ++job;
}

void* PS4_SYSV_ABI sceAjmBatchJobRunSplitBufferRa(AjmMultiJob* batch_pos, u32 instance,
                                                  AjmFlags flags, const AjmBuffer* in_buffers,
                                                  u64 num_in_buffers, const AjmBuffer* out_buffers,
                                                  u64 num_out_buffers, void* sideband_output,
                                                  u64 sideband_output_size, void* ret_addr) {
    LOG_INFO(Lib_Ajm,
             "called instance = {}, flags = {:#x}, cmd = {}, sideband_cmd = {} num_input_buffers "
             "= {}, num_output_buffers = {}, "
             "ret_addr = {}",
             instance, flags.raw, magic_enum::enum_name(flags.run_flags),
             magic_enum::enum_name(flags.sideband_flags), num_in_buffers, num_out_buffers,
             fmt::ptr(ret_addr));

    const u32 job_size = (num_in_buffers * 2 + 1 + num_out_buffers * 2) * 8;
    batch_pos->header.instance = instance;

    u32* job;
    if (ret_addr == nullptr) {
        batch_pos->header.job_size = job_size + 16;
        job = batch_pos->job;
    } else {
        batch_pos->header.job_size = job_size + 32;
        batch_pos->ret.ret_buf.ident = Identifier::ReturnAddrBuf;
        batch_pos->ret.ret_buf.buf_size = 0;
        batch_pos->ret.ret_buf.buffer = (u8*)ret_addr;
        job = batch_pos->ret.job;
    }

    for (s32 i = 0; i < num_in_buffers; i++) {
        auto* in_buf = reinterpret_cast<AjmJobBuffer*>(job);
        in_buf->ident = Identifier::InputRunBuf;
        in_buf->buf_size = in_buffers[i].size;
        in_buf->buffer = in_buffers[i].addr;
        job += 4;
    }
    job[1] = u32(flags.raw & 0xe00000001fffULL);
    job[0] &= 0xfc000030;
    job[0] = s32((flags.raw & 0xe00000001fffULL) >> 0x1a) + 4;
    job += 2;

    for (s32 i = 0; i < num_out_buffers; i++) {
        auto* out_buf = reinterpret_cast<AjmJobBuffer*>(job);
        out_buf->ident = Identifier::OutputMultijobBuf;
        out_buf->buf_size = out_buffers[i].size;
        out_buf->buffer = out_buffers[i].addr;
        job += 4;
    }
    job[0] = job[0] & 0xffffffe0 | 0x12;              // output.ident
    job[1] = sideband_output_size;                    // output.buf_size
    memcpy(&job[2], &sideband_output, sizeof(void*)); // output.buffer
    return job + 4;
}

int PS4_SYSV_ABI sceAjmBatchStartBuffer(u32 context, const u8* batch, u32 batch_size,
                                        const int priority, AjmBatchError* batch_error,
                                        u32* out_batch_id) {
    LOG_INFO(Lib_Ajm, "called context = {}, batch_size = {:#x}, priority = {}", context, batch_size,
             priority);

    if ((batch_size & 7) != 0) {
        return ORBIS_AJM_ERROR_MALFORMED_BATCH;
    }

    const auto batch_info = std::make_shared<BatchInfo>();
    if (dev->batches.size() >= MaxBatches) {
        LOG_ERROR(Lib_Ajm, "Too many batches in job!");
        return ORBIS_AJM_ERROR_OUT_OF_MEMORY;
    }

    *out_batch_id = static_cast<u32>(dev->batches.size());
    dev->batches.push_back(batch_info);

    const u8* batch_ptr = batch;
    const u8* batch_end = batch + batch_size;
    AjmJobHeader header{};

    while (batch_ptr < batch_end) {
        std::memcpy(&header, batch_ptr, sizeof(u64));
        const u32 instance = header.instance;

        const u8* curr_ptr = batch_ptr + sizeof(AjmJobHeader);
        const u8* job_end = curr_ptr + header.job_size;

        boost::container::small_vector<AjmJobBuffer, 4> input_buffers;
        while (true) {
            Identifier ident{};
            std::memcpy(&ident, curr_ptr, sizeof(u8));

            // Ignore return address buffers.
            if (ident == Identifier::ReturnAddrBuf) {
                curr_ptr += sizeof(AjmJobBuffer);
                continue;
            }

            // Add input buffer to the list of inputs.
            if (ident == Identifier::InputRunBuf || ident == Identifier::InputControlBuf) {
                auto& buffer = input_buffers.emplace_back();
                std::memcpy(&buffer, curr_ptr, sizeof(buffer));
                curr_ptr += sizeof(AjmJobBuffer);
                continue;
            }

            // A control or run flags identifier stops input buffer collection
            // and provides necessary flags for the operation requested.
            if (ident == Identifier::ControlFlags) {
                AjmFlagsIdentifier flags;
                std::memcpy(&flags, curr_ptr, sizeof(flags));

                ASSERT_MSG(input_buffers.size() == 1,
                           "Only 1 input buffer is allowed for control commands");
                const auto& in_buffer = input_buffers.back();

                const auto command = AjmJobControlFlags(flags.control_flags.Value());
                if (True(command & AjmJobControlFlags::Reset)) {
                    LOG_INFO(Lib_Ajm, "Resetting instance {}", instance);
                    dev->instances[instance]->Reset();
                }
                if (True(command & AjmJobControlFlags::Initialize)) {
                    LOG_INFO(Lib_Ajm, "Initializing instance {}", instance);
                    dev->instances[instance]->Initialize(in_buffer.buffer, in_buffer.buf_size);
                }
                if (True(command & AjmJobControlFlags::Resample)) {
                    LOG_WARNING(Lib_Ajm, "Set resample params of instance {}", instance);
                }

                curr_ptr += sizeof(flags);
                AjmJobBuffer out_buffer;
                std::memcpy(&out_buffer, curr_ptr, sizeof(out_buffer));

                // Write sideband structures.
                auto* result = reinterpret_cast<AjmSidebandResult*>(out_buffer.buffer);
                result->result = 0;
                result->internal_result = 0;
                break;
            }
            if (ident == Identifier::RunFlags) {
                AjmFlagsIdentifier flags;
                std::memcpy(&flags, curr_ptr, sizeof(flags));

                const auto command = AjmJobRunFlags(flags.run_flags.Value());
                const auto sideband = AjmJobSidebandFlags(flags.sideband_flags.Value());
                curr_ptr += sizeof(flags);

                // Collect output buffers.
                boost::container::small_vector<AjmJobBuffer, 4> output_buffers;
                while (curr_ptr < job_end) {
                    auto& buffer = output_buffers.emplace_back();
                    std::memcpy(&buffer, curr_ptr, sizeof(buffer));
                    curr_ptr += sizeof(buffer);
                }

                ASSERT_MSG(input_buffers.size() == 1 && output_buffers.size() == 2,
                           "Run operation with multiple buffers untested in = {}, out = {}",
                           input_buffers.size(), output_buffers.size());
                AjmJobBuffer in_buffer = input_buffers.back();
                AjmJobBuffer out_buffer = output_buffers.front();
                AjmJobBuffer sideband_buffer = output_buffers.back();

                // Write sideband structures.
                auto* sideband_ptr = sideband_buffer.buffer;
                auto* result = reinterpret_cast<AjmSidebandResult*>(sideband_ptr);
                result->result = 0;
                result->internal_result = 0;
                sideband_ptr += sizeof(AjmSidebandResult);

                // Perform operation requested by run flags.
                AjmInstance* decoder_instance = dev->instances[instance].get();
                if (True(command & AjmJobRunFlags::GetCodecInfo)) {
                    decoder_instance->GetCodecInfo(sideband_ptr);
                } else {
                    LOG_INFO(Lib_Ajm,
                             "Decode job cmd = {}, sideband = {}, in_addr = {}, in_size = {}",
                             magic_enum::enum_name(command), magic_enum::enum_name(sideband),
                             fmt::ptr(in_buffer.buffer), in_buffer.buf_size);

                    // Decode as much of the input bitstream as possible.
                    const auto [in_remain, out_remain, num_frames] =
                        decoder_instance->Decode(in_buffer.buffer, in_buffer.buf_size,
                                                 out_buffer.buffer, out_buffer.buf_size);

                    // Check sideband flags for decoding
                    if (True(sideband & AjmJobSidebandFlags::Stream)) {
                        auto* stream = reinterpret_cast<AjmSidebandStream*>(sideband_ptr);
                        stream->input_consumed = in_buffer.buf_size - in_remain;
                        stream->output_written = out_buffer.buf_size - out_remain;
                        stream->total_decoded_samples = decoder_instance->decoded_samples;
                        sideband_ptr += sizeof(AjmSidebandStream);
                    }
                    if (True(command & AjmJobRunFlags::MultipleFrames)) {
                        auto* mframe = reinterpret_cast<AjmSidebandMFrame*>(sideband_ptr);
                        mframe->num_frames = num_frames;
                        sideband_ptr += sizeof(AjmSidebandMFrame);
                    }
                }
                break;
            }
            UNREACHABLE_MSG("Unknown ident = {}", u32(ident));
        }
        batch_ptr += sizeof(AjmJobHeader) + header.job_size;
    }

    batch_info->finished = true;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchWait(const u32 context, const u32 batch_id, const u32 timeout,
                                 AjmBatchError* const batch_error) {
    LOG_INFO(Lib_Ajm, "called context = {}, batch_id = {}, timeout = {}", context, batch_id,
             timeout);

    if (batch_id > 0xFF || batch_id >= dev->batches.size()) {
        return ORBIS_AJM_ERROR_INVALID_BATCH;
    }

    const auto& batch = dev->batches[batch_id];

    if (batch->waiting) {
        return ORBIS_AJM_ERROR_BUSY;
    }
    batch->waiting = true;

    {
        std::unique_lock lk{batch->mtx};
        if (!batch->cv.wait_for(lk, std::chrono::milliseconds(timeout),
                                [&] { return batch->finished; })) {
            return ORBIS_AJM_ERROR_IN_PROGRESS;
        }
    }

    dev->batches.erase(dev->batches.begin() + batch_id);
    return 0;
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
    ASSERT_MSG(flags.format == 0, "Only signed 16-bit PCM output is supported currently!");
    const u32 index = dev->free_instances[dev->curr_cursor++];
    dev->curr_cursor %= MaxInstances;
    std::unique_ptr<AjmInstance> instance;
    switch (codec_type) {
    case AjmCodecType::Mp3Dec:
        instance = std::make_unique<AjmMp3Decoder>();
        break;
    case AjmCodecType::At9Dec:
        instance = std::make_unique<AjmAt9Decoder>();
        break;
    default:
        UNREACHABLE_MSG("Codec #{} not implemented", u32(codec_type));
    }
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
