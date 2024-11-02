// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <numeric>
#include <semaphore>
#include <span>
#include <thread>
#include <boost/container/small_vector.hpp>
#include <magic_enum.hpp>
#include <queue>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/indexed_resources.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
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

static constexpr u32 ORBIS_AJM_WAIT_INFINITE = -1;

static constexpr u32 MaxInstances = 0x2fff;

static constexpr u32 MaxBatches = 1000;

struct AjmJob {
    u32 instance_id = 0;
    AjmJobFlags flags = {.raw = 0};
    AjmJobInput input;
    AjmJobOutput output;
};

struct BatchInfo {
    u32 id;
    std::atomic_bool waiting{};
    std::atomic_bool canceled{};
    std::binary_semaphore finished{0};
    boost::container::small_vector<AjmJob, 8> jobs;
};

template <class ChunkType>
ChunkType& AjmBufferExtract(u8*& p_cursor) {
    auto* const result = reinterpret_cast<ChunkType*>(p_cursor);
    p_cursor += sizeof(ChunkType);
    return *result;
}

template <class ChunkType>
void AjmBufferSkip(u8*& p_cursor) {
    p_cursor += sizeof(ChunkType);
}

template <class ChunkType>
ChunkType& AjmBufferPeek(u8* p_cursor) {
    return *reinterpret_cast<ChunkType*>(p_cursor);
}

struct AjmDevice {
    u32 max_prio{};
    u32 min_prio{};
    u32 curr_cursor{};
    u32 release_cursor{MaxInstances - 1};
    std::array<bool, NumAjmCodecs> is_registered{};
    std::array<u32, MaxInstances> free_instances{};
    std::array<std::unique_ptr<AjmInstance>, MaxInstances> instances;
    Common::IndexedResources<u32, std::shared_ptr<BatchInfo>, MaxBatches> batches;
    std::mutex batches_mutex;

    std::jthread worker_thread{};
    std::queue<std::shared_ptr<BatchInfo>> batch_queue{};
    std::mutex batch_queue_mutex{};
    std::mutex batch_queue_cv_mutex{};
    std::condition_variable_any batch_queue_cv{};

    [[nodiscard]] bool IsRegistered(AjmCodecType type) const {
        return is_registered[static_cast<u32>(type)];
    }

    void Register(AjmCodecType type) {
        is_registered[static_cast<u32>(type)] = true;
    }

    AjmDevice() {
        std::iota(free_instances.begin(), free_instances.end(), 1);
        worker_thread = std::jthread([this](std::stop_token stop) { this->WorkerThread(stop); });
    }

    void WorkerThread(std::stop_token stop) {
        while (!stop.stop_requested()) {
            {
                std::unique_lock lock(batch_queue_cv_mutex);
                if (!batch_queue_cv.wait(lock, stop, [this] { return !batch_queue.empty(); })) {
                    continue;
                }
            }

            std::shared_ptr<BatchInfo> batch;
            {
                std::lock_guard lock(batch_queue_mutex);
                batch = batch_queue.front();
                batch_queue.pop();
            }
            ProcessBatch(batch->id, batch->jobs);
            batch->finished.release();
        }
    }

    void ProcessBatch(u32 id, std::span<AjmJob> jobs) {
        // Perform operation requested by control flags.
        for (auto& job : jobs) {
            LOG_DEBUG(Lib_Ajm, "Processing job {} for instance {}. flags = {:#x}", id,
                      job.instance_id, job.flags.raw);

            AjmInstance* p_instance = instances[job.instance_id].get();

            const auto control_flags = job.flags.control_flags;
            if (True(control_flags & AjmJobControlFlags::Reset)) {
                LOG_INFO(Lib_Ajm, "Resetting instance {}", job.instance_id);
                p_instance->Reset();
            }
            if (True(control_flags & AjmJobControlFlags::Initialize)) {
                LOG_INFO(Lib_Ajm, "Initializing instance {}", job.instance_id);
                ASSERT_MSG(job.input.init_params.has_value(),
                           "Initialize called without control buffer");
                auto& params = job.input.init_params.value();
                p_instance->Initialize(&params, sizeof(params));
            }
            if (True(control_flags & AjmJobControlFlags::Resample)) {
                LOG_ERROR(Lib_Ajm, "Unimplemented: resample params");
                ASSERT_MSG(job.input.resample_parameters.has_value(),
                           "Resample paramters are absent");
                p_instance->resample_parameters = job.input.resample_parameters.value();
            }

            const auto sideband_flags = job.flags.sideband_flags;
            if (True(sideband_flags & AjmJobSidebandFlags::Format)) {
                ASSERT_MSG(job.input.format.has_value(), "Format parameters are absent");
                p_instance->format = job.input.format.value();
            }
            if (True(sideband_flags & AjmJobSidebandFlags::GaplessDecode)) {
                ASSERT_MSG(job.input.gapless_decode.has_value(),
                           "Gapless decode parameters are absent");
                auto& params = job.input.gapless_decode.value();
                p_instance->gapless.total_samples = params.total_samples;
                p_instance->gapless.skip_samples = params.skip_samples;
            }

            if (!job.input.buffer.empty()) {
                p_instance->Decode(&job.input, &job.output);
            }

            if (job.output.p_gapless_decode != nullptr) {
                *job.output.p_gapless_decode = p_instance->gapless;
            }

            if (job.output.p_codec_info != nullptr) {
                p_instance->GetCodecInfo(job.output.p_codec_info);
            }
        }
    }
};

static std::unique_ptr<AjmDevice> dev{};

int PS4_SYSV_ABI sceAjmBatchCancel(const u32 context_id, const u32 batch_id) {
    std::shared_ptr<BatchInfo> batch{};
    {
        std::lock_guard guard(dev->batches_mutex);
        const auto opt_batch = dev->batches.Get(batch_id);
        if (!opt_batch.has_value()) {
            return ORBIS_AJM_ERROR_INVALID_BATCH;
        }

        batch = opt_batch.value().get();
    }

    batch->canceled = true;

    return ORBIS_OK;
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
    LOG_TRACE(Lib_Ajm, "called");

    u8* p_current = (u8*)p_buffer;

    auto& header = AjmBufferExtract<AjmChunkHeader>(p_current);
    header.ident = AjmIdentJob;
    header.payload = instance_id;

    const u8* const p_begin = p_current;

    if (p_return_address != nullptr) {
        auto& chunk_ra = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_ra.header.ident = AjmIdentReturnAddressBuf;
        chunk_ra.header.payload = 0;
        chunk_ra.header.size = 0;
        chunk_ra.p_address = p_return_address;
    }

    {
        auto& chunk_input = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_input.header.ident = AjmIdentInputControlBuf;
        chunk_input.header.payload = 0;
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

        auto& chunk_flags = AjmBufferExtract<AjmChunkHeader>(p_current);
        chunk_flags.ident = AjmIdentControlFlags;
        chunk_flags.payload = u32(flags >> 32);
        chunk_flags.size = u32(flags);
    }

    {
        auto& chunk_output = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputControlBuf;
        chunk_output.header.payload = 0;
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

    auto& header = AjmBufferExtract<AjmChunkHeader>(p_current);
    header.ident = AjmIdentInlineBuf;
    header.payload = 0;
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

    auto& header = AjmBufferExtract<AjmChunkHeader>(p_current);
    header.ident = AjmIdentJob;
    header.payload = instance_id;

    const u8* const p_begin = p_current;

    if (p_return_address != nullptr) {
        auto& chunk_ra = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_ra.header.ident = AjmIdentReturnAddressBuf;
        chunk_ra.header.payload = 0;
        chunk_ra.header.size = 0;
        chunk_ra.p_address = p_return_address;
    }

    {
        auto& chunk_input = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_input.header.ident = AjmIdentInputRunBuf;
        chunk_input.header.payload = 0;
        chunk_input.header.size = data_input_size;
        chunk_input.p_address = p_data_input;
    }

    {
        // 0x0000'E000'0000'1FFF:
        // | sideband | reserved                      | control | run | codec    | revision |
        // | 111      | 00000000000000000000000000000 | 000     | 11  | 11111111 | 111      |
        flags &= 0x0000'E000'0000'1FFF;

        auto& chunk_flags = AjmBufferExtract<AjmChunkHeader>(p_current);
        chunk_flags.ident = AjmIdentRunFlags;
        chunk_flags.payload = u32(flags >> 32);
        chunk_flags.size = u32(flags);
    }

    {
        auto& chunk_output = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputRunBuf;
        chunk_output.header.payload = 0;
        chunk_output.header.size = data_output_size;
        chunk_output.p_address = p_data_output;
    }

    {
        auto& chunk_output = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputControlBuf;
        chunk_output.header.payload = 0;
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

    auto& header = AjmBufferExtract<AjmChunkHeader>(p_current);
    header.ident = AjmIdentJob;
    header.payload = instance_id;

    const u8* const p_begin = p_current;

    if (p_return_address != nullptr) {
        auto& chunk_ra = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_ra.header.ident = AjmIdentReturnAddressBuf;
        chunk_ra.header.payload = 0;
        chunk_ra.header.size = 0;
        chunk_ra.p_address = p_return_address;
    }

    for (s32 i = 0; i < num_data_input_buffers; i++) {
        auto& chunk_input = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_input.header.ident = AjmIdentInputRunBuf;
        chunk_input.header.payload = 0;
        chunk_input.header.size = p_data_input_buffers[i].size;
        chunk_input.p_address = p_data_input_buffers[i].p_address;
    }

    {
        // 0x0000'E000'0000'1FFF:
        // | sideband | reserved                      | control | run | codec    | revision |
        // | 111      | 00000000000000000000000000000 | 000     | 11  | 11111111 | 111      |
        flags &= 0x0000'E000'0000'1FFF;

        auto& chunk_flags = AjmBufferExtract<AjmChunkHeader>(p_current);
        chunk_flags.ident = AjmIdentRunFlags;
        chunk_flags.payload = u32(flags >> 32);
        chunk_flags.size = u32(flags);
    }

    for (s32 i = 0; i < num_data_output_buffers; i++) {
        auto& chunk_output = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputRunBuf;
        chunk_output.header.payload = 0;
        chunk_output.header.size = p_data_output_buffers[i].size;
        chunk_output.p_address = p_data_output_buffers[i].p_address;
    }

    {
        auto& chunk_output = AjmBufferExtract<AjmChunkBuffer>(p_current);
        chunk_output.header.ident = AjmIdentOutputControlBuf;
        chunk_output.header.payload = 0;
        chunk_output.header.size = sideband_output_size;
        chunk_output.p_address = p_sideband_output;
    }

    header.size = u32(p_current - p_begin);
    return p_current;
}

int PS4_SYSV_ABI sceAjmBatchStartBuffer(u32 context, u8* p_batch, u32 batch_size,
                                        const int priority, AjmBatchError* batch_error,
                                        u32* out_batch_id) {
    LOG_INFO(Lib_Ajm, "called context = {}, batch_size = {:#x}, priority = {}", context, batch_size,
             priority);

    if ((batch_size & 7) != 0) {
        LOG_ERROR(Lib_Ajm, "ORBIS_AJM_ERROR_MALFORMED_BATCH");
        return ORBIS_AJM_ERROR_MALFORMED_BATCH;
    }

    const auto batch_info = std::make_shared<BatchInfo>();
    auto batch_id = dev->batches.Create(batch_info);
    if (!batch_id.has_value()) {
        return ORBIS_AJM_ERROR_OUT_OF_MEMORY;
    }
    batch_info->id = batch_id.value();
    *out_batch_id = batch_id.value();

    u8* p_current = p_batch;
    u8* const p_batch_end = p_current + batch_size;

    while (p_current < p_batch_end) {
        auto& header = AjmBufferExtract<const AjmChunkHeader>(p_current);
        ASSERT(header.ident == AjmIdentJob);

        batch_info->jobs.push_back(AjmJob{});
        auto& job = batch_info->jobs.back();
        job.instance_id = header.payload;

        std::optional<AjmJobFlags> job_flags = {};
        std::optional<AjmChunkBuffer> input_control_buffer = {};
        std::optional<AjmChunkBuffer> output_control_buffer = {};
        std::optional<AjmChunkBuffer> inline_buffer = {};
        boost::container::small_vector<AjmChunkBuffer, 16> input_run_buffers;
        boost::container::small_vector<AjmChunkBuffer, 16> output_run_buffers;

        // Read parameters of a job
        auto* const p_job_end = p_current + header.size;
        while (p_current < p_job_end) {
            auto& header = AjmBufferPeek<AjmChunkHeader>(p_current);
            switch (header.ident) {
            case Identifier::AjmIdentInputRunBuf: {
                auto& buffer = AjmBufferExtract<AjmChunkBuffer>(p_current);
                u8* p_begin = reinterpret_cast<u8*>(buffer.p_address);
                job.input.buffer.insert(job.input.buffer.end(), p_begin,
                                        p_begin + buffer.header.size);
                break;
            }
            case Identifier::AjmIdentInputControlBuf: {
                ASSERT_MSG(!input_control_buffer.has_value(),
                           "Only one instance of input control buffer is allowed per job");
                input_control_buffer = AjmBufferExtract<AjmChunkBuffer>(p_current);
                break;
            }
            case Identifier::AjmIdentControlFlags:
            case Identifier::AjmIdentRunFlags: {
                ASSERT_MSG(!job_flags.has_value(),
                           "Only one instance of job flags is allowed per job");
                auto& flags_chunk = AjmBufferExtract<AjmChunkHeader>(p_current);
                job_flags = AjmJobFlags{
                    .raw = (u64(flags_chunk.payload) << 32) + flags_chunk.size,
                };
                break;
            }
            case Identifier::AjmIdentReturnAddressBuf: {
                // Ignore return address buffers.
                AjmBufferSkip<AjmChunkBuffer>(p_current);
                break;
            }
            case Identifier::AjmIdentInlineBuf: {
                ASSERT_MSG(!output_control_buffer.has_value(),
                           "Only one instance of inline buffer is allowed per job");
                inline_buffer = AjmBufferExtract<AjmChunkBuffer>(p_current);
                break;
            }
            case Identifier::AjmIdentOutputRunBuf: {
                auto& buffer = AjmBufferExtract<AjmChunkBuffer>(p_current);
                u8* p_begin = reinterpret_cast<u8*>(buffer.p_address);
                job.output.buffers.emplace_back(
                    std::span<u8>(p_begin, p_begin + buffer.header.size));
                break;
            }
            case Identifier::AjmIdentOutputControlBuf: {
                ASSERT_MSG(!output_control_buffer.has_value(),
                           "Only one instance of output control buffer is allowed per job");
                output_control_buffer = AjmBufferExtract<AjmChunkBuffer>(p_current);
                break;
            }
            default:
                UNREACHABLE_MSG("Unknown chunk: {}", header.ident);
            }
        }

        job.flags = job_flags.value();

        // Perform operation requested by control flags.
        const auto control_flags = job_flags.value().control_flags;
        if (True(control_flags & AjmJobControlFlags::Initialize)) {
            ASSERT_MSG(input_control_buffer.has_value(),
                       "Initialize called without control buffer");
            const auto& in_buffer = input_control_buffer.value();
            job.input.init_params = AjmDecAt9InitializeParameters{};
            std::memcpy(&job.input.init_params.value(), in_buffer.p_address, in_buffer.header.size);
        }
        if (True(control_flags & AjmJobControlFlags::Resample)) {
            ASSERT_MSG(inline_buffer.has_value(),
                       "Resample paramters are stored in the inline buffer");
            auto* p_buffer = reinterpret_cast<u8*>(inline_buffer.value().p_address);
            job.input.resample_parameters =
                AjmBufferExtract<AjmSidebandResampleParameters>(p_buffer);
        }

        // Initialize sideband input parameters
        if (input_control_buffer.has_value()) {
            auto* p_sideband = reinterpret_cast<u8*>(input_control_buffer.value().p_address);
            auto* const p_end = p_sideband + input_control_buffer.value().header.size;

            const auto sideband_flags = job_flags.value().sideband_flags;
            if (True(sideband_flags & AjmJobSidebandFlags::Format) && p_sideband < p_end) {
                job.input.format = AjmBufferExtract<AjmSidebandFormat>(p_sideband);
            }
            if (True(sideband_flags & AjmJobSidebandFlags::GaplessDecode) && p_sideband < p_end) {
                job.input.gapless_decode = AjmBufferExtract<AjmSidebandGaplessDecode>(p_sideband);
            }

            ASSERT_MSG(p_sideband <= p_end, "Input sideband out of bounds");
        }

        // Initialize sideband output parameters
        if (output_control_buffer.has_value()) {
            auto* p_sideband = reinterpret_cast<u8*>(output_control_buffer.value().p_address);
            auto* const p_end = p_sideband + output_control_buffer.value().header.size;
            job.output.p_result = &AjmBufferExtract<AjmSidebandResult>(p_sideband);
            *job.output.p_result = AjmSidebandResult{};

            const auto sideband_flags = job_flags.value().sideband_flags;
            if (True(sideband_flags & AjmJobSidebandFlags::Stream) && p_sideband < p_end) {
                job.output.p_stream = &AjmBufferExtract<AjmSidebandStream>(p_sideband);
                *job.output.p_stream = AjmSidebandStream{};
            }
            if (True(sideband_flags & AjmJobSidebandFlags::Format) && p_sideband < p_end) {
                LOG_ERROR(Lib_Ajm, "SIDEBAND_FORMAT is not implemented");
                job.output.p_format = &AjmBufferExtract<AjmSidebandFormat>(p_sideband);
                *job.output.p_format = AjmSidebandFormat{};
            }
            if (True(sideband_flags & AjmJobSidebandFlags::GaplessDecode) && p_sideband < p_end) {
                job.output.p_gapless_decode =
                    &AjmBufferExtract<AjmSidebandGaplessDecode>(p_sideband);
                *job.output.p_gapless_decode = AjmSidebandGaplessDecode{};
            }

            const auto run_flags = job_flags.value().run_flags;
            if (True(run_flags & AjmJobRunFlags::MultipleFrames) && p_sideband < p_end) {
                job.output.p_mframe = &AjmBufferExtract<AjmSidebandMFrame>(p_sideband);
                *job.output.p_mframe = AjmSidebandMFrame{};
            }
            if (True(run_flags & AjmJobRunFlags::GetCodecInfo) && p_sideband < p_end) {
                job.output.p_codec_info = p_sideband;
            }

            ASSERT_MSG(p_sideband <= p_end, "Output sideband out of bounds");
        }
    }

    {
        std::lock_guard lock(dev->batch_queue_mutex);
        dev->batch_queue.push(batch_info);
    }

    {
        std::unique_lock lock(dev->batch_queue_cv_mutex);
        dev->batch_queue_cv.notify_all();
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmBatchWait(const u32 context, const u32 batch_id, const u32 timeout,
                                 AjmBatchError* const batch_error) {
    LOG_INFO(Lib_Ajm, "called context = {}, batch_id = {}, timeout = {}", context, batch_id,
             timeout);

    std::shared_ptr<BatchInfo> batch{};
    {
        std::lock_guard guard(dev->batches_mutex);
        const auto opt_batch = dev->batches.Get(batch_id);
        if (!opt_batch.has_value()) {
            return ORBIS_AJM_ERROR_INVALID_BATCH;
        }

        batch = opt_batch.value().get();
    }

    bool expected = false;
    if (!batch->waiting.compare_exchange_strong(expected, true)) {
        return ORBIS_AJM_ERROR_BUSY;
    }

    if (timeout == ORBIS_AJM_WAIT_INFINITE) {
        batch->finished.acquire();
    } else if (!batch->finished.try_acquire_for(std::chrono::milliseconds(timeout))) {
        batch->waiting = false;
        return ORBIS_AJM_ERROR_IN_PROGRESS;
    }

    {
        std::lock_guard guard(dev->batches_mutex);
        dev->batches.Destroy(batch_id);
    }

    if (batch->canceled) {
        return ORBIS_AJM_ERROR_CANCELLED;
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmDecAt9ParseConfigData() {
    LOG_ERROR(Lib_Ajm, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAjmDecMp3ParseFrame(const u8* buf, u32 stream_size, int parse_ofl,
                                        AjmDecMp3ParseFrame* frame) {
    LOG_INFO(Lib_Ajm, "called stream_size = {} parse_ofl = {}", stream_size, parse_ofl);
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
    LOG_INFO(Lib_Ajm, "called context = {}, codec_type = {}, flags = {:#x}", context,
             magic_enum::enum_name(codec_type), flags.raw);

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
    instance->flags = flags;
    dev->instances[index] = std::move(instance);
    *out_instance = index;

    LOG_INFO(Lib_Ajm, "instance = {}", index);

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
