// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <condition_variable>
#include <mutex>
#include <numeric>
#include <boost/container/small_vector.hpp>
#include <magic_enum.hpp>

#include "common/alignment.h"
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

template <class ChunkType, class CursorType>
ChunkType& AjmGetChunk(CursorType& p_cursor) {
    auto* const result = reinterpret_cast<ChunkType*>(p_cursor);
    p_cursor += sizeof(ChunkType);
    return *result;
}

template <class ChunkType, class CursorType>
void AjmSkipChunk(CursorType& p_cursor) {
    p_cursor += sizeof(ChunkType);
}

template <class ChunkType, class CursorType>
ChunkType& AjmPeekChunk(CursorType p_cursor) {
    return *reinterpret_cast<ChunkType*>(p_cursor);
}

void* PS4_SYSV_ABI sceAjmBatchJobControlBufferRa(void* p_buffer, u32 instance_id, u64 flags,
                                                 void* p_sideband_input, size_t sideband_input_size,
                                                 void* p_sideband_output,
                                                 size_t sideband_output_size,
                                                 void* p_return_address) {
    LOG_TRACE(Lib_Ajm, "called");

    u8* p_current = (u8*)p_buffer;

    auto& header = AjmGetChunk<AjmChunkHeader>(p_current);
    header.ident = AjmIdentJob;
    header.payload = instance_id;

    const u8* const p_begin = p_current;

    if (p_return_address != nullptr) {
        auto& chunk_ra = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_ra.header.ident = AjmIdentReturnAddressBuf;
        chunk_ra.header.size = 0;
        chunk_ra.p_address = p_return_address;
    }

    {
        auto& chunk_input = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_input.header.ident = AjmIdentInputControlBuf;
        chunk_input.header.size = sideband_input_size;
        chunk_input.p_address = p_sideband_input;
    }

    {
        // 0x0000'0000'C001'8007 (AJM_INSTANCE_STATISTICS):
        // | sideband | reserved      | statistics        | command | codec    | revision |
        // | 000      | 0000000000000 | 11000000000000011 | 0000    | 00000000 | 111      |
        // statistics flags:
        // STATISTICS_ENGINE | STATISTICS_ENGINE_PER_CODEC | ??STATISTICS_UNK?? | STATISTICS_MEMORY

        // 0x0000'6000'0000'E7FF:
        // | sideband | reserved                      | control | run | codec    | revision |
        // | 011      | 00000000000000000000000000000 | 111     | 00  | 11111111 | 111      |
        const bool is_statistics = instance_id == AJM_INSTANCE_STATISTICS;
        flags &= is_statistics ? 0x0000'0000'C001'8007 : 0x0000'6000'0000'E7FF;

        auto& chunk_flags = AjmGetChunk<AjmChunkHeader>(p_current);
        chunk_flags.ident = AjmIdentControlFlags;
        chunk_flags.payload = u32(flags >> 32);
        chunk_flags.size = u32(flags);
    }

    {
        auto& chunk_output = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputControlBuf;
        chunk_output.header.size = sideband_output_size;
        chunk_output.p_address = p_sideband_output;
    }

    header.size = u32(p_current - p_begin);
    return p_current;
}

void* PS4_SYSV_ABI sceAjmBatchJobInlineBuffer(void* p_buffer, const void* p_data_input,
                                              size_t data_input_size,
                                              const void** pp_batch_address) {
    LOG_TRACE(Lib_Ajm, "called");

    u8* p_current = (u8*)p_buffer;

    auto& header = AjmGetChunk<AjmChunkHeader>(p_current);
    header.ident = AjmIdentInlineBuf;
    header.size = Common::AlignUp(data_input_size, 8);
    *pp_batch_address = p_current;

    memcpy(p_current, p_data_input, data_input_size);
    return p_current + header.size;
}

void* PS4_SYSV_ABI sceAjmBatchJobRunBufferRa(void* p_buffer, u32 instance_id, u64 flags,
                                             void* p_data_input, size_t data_input_size,
                                             void* p_data_output, size_t data_output_size,
                                             void* p_sideband_output, size_t sideband_output_size,
                                             void* p_return_address) {
    LOG_TRACE(Lib_Ajm, "called");

    u8* p_current = (u8*)p_buffer;

    auto& header = AjmGetChunk<AjmChunkHeader>(p_current);
    header.ident = AjmIdentJob;
    header.payload = instance_id;

    const u8* const p_begin = p_current;

    if (p_return_address != nullptr) {
        auto& chunk_ra = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_ra.header.ident = AjmIdentReturnAddressBuf;
        chunk_ra.header.size = 0;
        chunk_ra.p_address = p_return_address;
    }

    {
        auto& chunk_input = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_input.header.ident = AjmIdentInputRunBuf;
        chunk_input.header.size = data_input_size;
        chunk_input.p_address = p_data_input;
    }

    {
        // 0x0000'E000'0000'1FFF:
        // | sideband | reserved                      | control | run | codec    | revision |
        // | 111      | 00000000000000000000000000000 | 000     | 11  | 11111111 | 111      |
        flags &= 0x0000'E000'0000'1FFF;

        auto& chunk_flags = AjmGetChunk<AjmChunkHeader>(p_current);
        chunk_flags.ident = AjmIdentRunFlags;
        chunk_flags.payload = u32(flags >> 32);
        chunk_flags.size = u32(flags);
    }

    {
        auto& chunk_output = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputRunBuf;
        chunk_output.header.size = data_output_size;
        chunk_output.p_address = p_data_output;
    }

    {
        auto& chunk_output = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputControlBuf;
        chunk_output.header.size = sideband_output_size;
        chunk_output.p_address = p_sideband_output;
    }

    header.size = u32(p_current - p_begin);
    return p_current;
}

void* PS4_SYSV_ABI sceAjmBatchJobRunSplitBufferRa(
    void* p_buffer, u32 instance_id, u64 flags, const AjmBuffer* p_data_input_buffers,
    size_t num_data_input_buffers, const AjmBuffer* p_data_output_buffers,
    size_t num_data_output_buffers, void* p_sideband_output, size_t sideband_output_size,
    void* p_return_address) {
    LOG_TRACE(Lib_Ajm, "called");

    u8* p_current = (u8*)p_buffer;

    auto& header = AjmGetChunk<AjmChunkHeader>(p_current);
    header.ident = AjmIdentJob;
    header.payload = instance_id;

    const u8* const p_begin = p_current;

    if (p_return_address != nullptr) {
        auto& chunk_ra = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_ra.header.ident = AjmIdentReturnAddressBuf;
        chunk_ra.header.size = 0;
        chunk_ra.p_address = p_return_address;
    }

    for (s32 i = 0; i < num_data_input_buffers; i++) {
        auto& chunk_input = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_input.header.ident = AjmIdentInputRunBuf;
        chunk_input.header.size = p_data_input_buffers[i].size;
        chunk_input.p_address = p_data_input_buffers[i].p_address;
    }

    {
        // 0x0000'E000'0000'1FFF:
        // | sideband | reserved                      | control | run | codec    | revision |
        // | 111      | 00000000000000000000000000000 | 000     | 11  | 11111111 | 111      |
        flags &= 0x0000'E000'0000'1FFF;

        auto& chunk_flags = AjmGetChunk<AjmChunkHeader>(p_current);
        chunk_flags.ident = AjmIdentRunFlags;
        chunk_flags.payload = u32(flags >> 32);
        chunk_flags.size = u32(flags);
    }

    for (s32 i = 0; i < num_data_output_buffers; i++) {
        auto& chunk_output = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputRunBuf;
        chunk_output.header.size = p_data_output_buffers[i].size;
        chunk_output.p_address = p_data_output_buffers[i].p_address;
    }

    {
        auto& chunk_output = AjmGetChunk<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputControlBuf;
        chunk_output.header.size = sideband_output_size;
        chunk_output.p_address = p_sideband_output;
    }

    header.size = u32(p_current - p_begin);
    return p_current;
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

    const u8* p_current = batch;
    const u8* p_batch_end = batch + batch_size;

    while (p_current < p_batch_end) {
        auto& header = AjmGetChunk<const AjmChunkHeader>(p_current);
        ASSERT(header.ident == AjmIdentJob);

        std::optional<AjmJobFlags> job_flags = {};
        std::optional<AjmChunkBuffer> input_control_buffer = {};
        std::optional<AjmChunkBuffer> output_control_buffer = {};
        boost::container::small_vector<AjmChunkBuffer, 16> input_run_buffers;
        boost::container::small_vector<AjmChunkBuffer, 16> output_run_buffers;

        // Read parameters of a job
        auto* const p_job_end = p_current + header.size;
        while (p_current < p_job_end) {
            auto& header = AjmPeekChunk<const AjmChunkHeader>(p_current);
            switch (header.ident) {
            case Identifier::AjmIdentInputRunBuf: {
                input_run_buffers.emplace_back(AjmGetChunk<const AjmChunkBuffer>(p_current));
                break;
            }
            case Identifier::AjmIdentInputControlBuf: {
                ASSERT_MSG(!input_control_buffer.has_value(),
                           "Only one instance of input control buffer is allowed per job");
                input_control_buffer = AjmGetChunk<const AjmChunkBuffer>(p_current);
                break;
            }
            case Identifier::AjmIdentControlFlags:
            case Identifier::AjmIdentRunFlags: {
                ASSERT_MSG(!job_flags.has_value(),
                           "Only one instance of job flags is allowed per job");
                auto& flags_chunk = AjmGetChunk<const AjmChunkHeader>(p_current);
                job_flags = AjmJobFlags{
                    .raw = (u64(flags_chunk.payload) << 32) + flags_chunk.size,
                };
                break;
            }
            case Identifier::AjmIdentReturnAddressBuf: {
                // Ignore return address buffers.
                AjmSkipChunk<const AjmChunkBuffer>(p_current);
                break;
            }
            case Identifier::AjmIdentOutputRunBuf: {
                output_run_buffers.emplace_back(AjmGetChunk<const AjmChunkBuffer>(p_current));
                break;
            }
            case Identifier::AjmIdentOutputControlBuf: {
                ASSERT_MSG(!output_control_buffer.has_value(),
                           "Only one instance of output control buffer is allowed per job");
                output_control_buffer = AjmGetChunk<const AjmChunkBuffer>(p_current);
                break;
            }
            default:
                LOG_ERROR(Lib_Ajm, "Unknown chunk: {}", header.ident);
                p_current += header.size;
                break;
            }
        }

        const u32 instance = header.payload;
        AjmInstance* p_instance = dev->instances[instance].get();

        // Perform operation requested by control flags.
        const auto control_flags = job_flags.value().control_flags;
        if (True(control_flags & AjmJobControlFlags::Reset)) {
            LOG_TRACE(Lib_Ajm, "Resetting instance {}", instance);
            p_instance->Reset();
        }
        if (True(control_flags & AjmJobControlFlags::Initialize)) {
            LOG_TRACE(Lib_Ajm, "Initializing instance {}", instance);
            ASSERT_MSG(input_control_buffer.has_value(),
                       "Initialize called without control buffer");
            const auto& in_buffer = input_control_buffer.value();
            p_instance->Initialize(in_buffer.p_address, in_buffer.header.size);
        }
        if (True(control_flags & AjmJobControlFlags::Resample)) {
            LOG_ERROR(Lib_Ajm, "Unimplemented: Set resample params of instance {}", instance);
        }

        // Write sideband structures.
        auto* p_sideband = reinterpret_cast<u8*>(output_control_buffer.value().p_address);
        auto* result = reinterpret_cast<AjmSidebandResult*>(p_sideband);
        result->result = 0;
        result->internal_result = 0;
        p_sideband += sizeof(AjmSidebandResult);

        // Perform operation requested by run flags.
        ASSERT_MSG(input_run_buffers.size() == output_run_buffers.size(),
                   "Run operation with uneven input/output of buffers.");

        const auto run_flags = job_flags.value().run_flags;
        const auto sideband_flags = job_flags.value().sideband_flags;

        for (size_t i = 0; i < input_run_buffers.size(); ++i) {
            // Decode as much of the input bitstream as possible.
            const auto& in_buffer = input_run_buffers[i];
            const auto& out_buffer = output_run_buffers[i];

            const auto [in_remain, out_remain, num_frames] = p_instance->Decode(
                reinterpret_cast<u8*>(in_buffer.p_address), in_buffer.header.size,
                reinterpret_cast<u8*>(out_buffer.p_address), out_buffer.header.size);

            // Check sideband flags for decoding
            if (True(sideband_flags & AjmJobSidebandFlags::Stream)) {
                auto* stream = reinterpret_cast<AjmSidebandStream*>(p_sideband);
                stream->input_consumed = in_buffer.header.size - in_remain;
                stream->output_written = out_buffer.header.size - out_remain;
                stream->total_decoded_samples = p_instance->decoded_samples;
                p_sideband += sizeof(AjmSidebandStream);
            }
            if (True(run_flags & AjmJobRunFlags::MultipleFrames)) {
                auto* mframe = reinterpret_cast<AjmSidebandMFrame*>(p_sideband);
                mframe->num_frames = num_frames;
                p_sideband += sizeof(AjmSidebandMFrame);
            }
        }

        if (True(run_flags & AjmJobRunFlags::GetCodecInfo)) {
            p_instance->GetCodecInfo(p_sideband);
        }
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
