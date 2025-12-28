// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/thread.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/ajm/ajm_at9.h"
#include "core/libraries/ajm/ajm_context.h"
#include "core/libraries/ajm/ajm_error.h"
#include "core/libraries/ajm/ajm_instance.h"
#include "core/libraries/ajm/ajm_instance_statistics.h"
#include "core/libraries/ajm/ajm_mp3.h"
#include "core/libraries/error_codes.h"

#include <span>
#include <utility>

namespace Libraries::Ajm {

constexpr u32 ORBIS_AJM_WAIT_INFINITE = -1;
constexpr int INSTANCE_ID_MASK = 0x3FFF;

AjmContext::AjmContext() {
    worker_thread = std::jthread([this](std::stop_token stop) { this->WorkerThread(stop); });
}

bool AjmContext::IsRegistered(AjmCodecType type) const {
    return registered_codecs[std::to_underlying(type)];
}

s32 AjmContext::BatchCancel(const u32 batch_id) {
    std::shared_ptr<AjmBatch> batch{};
    {
        std::shared_lock guard(batches_mutex);
        const auto p_batch = batches.Get(batch_id);
        if (p_batch == nullptr) {
            return ORBIS_AJM_ERROR_INVALID_BATCH;
        }
        batch = *p_batch;
    }

    if (batch->processed) {
        return ORBIS_OK;
    }

    bool expected = false;
    batch->canceled.compare_exchange_strong(expected, true);
    return ORBIS_OK;
}

s32 AjmContext::ModuleRegister(AjmCodecType type) {
    if (std::to_underlying(type) >= NumAjmCodecs) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    if (IsRegistered(type)) {
        return ORBIS_AJM_ERROR_CODEC_ALREADY_REGISTERED;
    }
    registered_codecs[std::to_underlying(type)] = true;
    return ORBIS_OK;
}

void AjmContext::WorkerThread(std::stop_token stop) {
    Common::SetCurrentThreadName("shadPS4:AjmWorker");
    while (!stop.stop_requested()) {
        auto batch = batch_queue.PopWait(stop);
        if (batch != nullptr && !batch->canceled) {
            bool expected = false;
            batch->processed.compare_exchange_strong(expected, true);
            ProcessBatch(batch->id, batch->jobs);
            batch->finished.release();
        }
    }
}

void AjmContext::ProcessBatch(u32 id, std::span<AjmJob> jobs) {
    // Perform operation requested by control flags.
    for (auto& job : jobs) {
        LOG_TRACE(Lib_Ajm, "Processing job {} for instance {}. flags = {:#x}", id, job.instance_id,
                  job.flags.raw);

        if (job.instance_id == AJM_INSTANCE_STATISTICS) {
            AjmInstanceStatistics::Getinstance().ExecuteJob(job);
        } else {
            std::shared_ptr<AjmInstance> instance;
            {
                std::shared_lock lock(instances_mutex);
                auto* p_instance = instances.Get(job.instance_id & INSTANCE_ID_MASK);
                ASSERT_MSG(p_instance != nullptr, "Attempting to execute job on null instance");
                instance = *p_instance;
            }

            instance->ExecuteJob(job);
        }
    }
}

s32 AjmContext::BatchWait(const u32 batch_id, const u32 timeout, AjmBatchError* const batch_error) {
    std::shared_ptr<AjmBatch> batch{};
    {
        std::shared_lock guard(batches_mutex);
        const auto p_batch = batches.Get(batch_id);
        if (p_batch == nullptr) {
            return ORBIS_AJM_ERROR_INVALID_BATCH;
        }
        batch = *p_batch;
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
        std::unique_lock guard(batches_mutex);
        batches.Destroy(batch_id);
    }

    if (batch->canceled) {
        return ORBIS_AJM_ERROR_CANCELLED;
    }

    return ORBIS_OK;
}

int AjmContext::BatchStartBuffer(u8* p_batch, u32 batch_size, const int priority,
                                 AjmBatchError* batch_error, u32* out_batch_id) {
    if ((batch_size & 7) != 0) {
        LOG_ERROR(Lib_Ajm, "ORBIS_AJM_ERROR_MALFORMED_BATCH");
        return ORBIS_AJM_ERROR_MALFORMED_BATCH;
    }

    const auto batch_info = AjmBatch::FromBatchBuffer({p_batch, batch_size});
    std::optional<u32> batch_id;
    {
        std::unique_lock guard(batches_mutex);
        batch_id = batches.Create(batch_info);
    }
    if (!batch_id.has_value()) {
        return ORBIS_AJM_ERROR_OUT_OF_MEMORY;
    }
    *out_batch_id = batch_id.value();
    batch_info->id = *out_batch_id;

    if (!batch_info->jobs.empty()) {
        batch_queue.EmplaceWait(batch_info);
    } else {
        // Empty batches are not submitted to the processor and are marked as finished
        batch_info->finished.release();
    }

    return ORBIS_OK;
}

s32 AjmContext::InstanceCreate(AjmCodecType codec_type, AjmInstanceFlags flags, u32* out_instance) {
    if (codec_type >= AjmCodecType::Max) {
        return ORBIS_AJM_ERROR_INVALID_PARAMETER;
    }
    if (flags.version == 0) {
        return ORBIS_AJM_ERROR_WRONG_REVISION_FLAG;
    }
    if (!IsRegistered(codec_type)) {
        return ORBIS_AJM_ERROR_CODEC_NOT_REGISTERED;
    }
    std::optional<u32> opt_index;
    {
        std::unique_lock lock(instances_mutex);
        opt_index = instances.Create(std::move(std::make_unique<AjmInstance>(codec_type, flags)));
    }
    if (!opt_index.has_value()) {
        return ORBIS_AJM_ERROR_OUT_OF_RESOURCES;
    }
    *out_instance = opt_index.value() | (static_cast<u32>(codec_type) << 14);

    LOG_INFO(Lib_Ajm, "instance = {}", *out_instance);
    return ORBIS_OK;
}

s32 AjmContext::InstanceDestroy(u32 instance_id) {
    std::unique_lock lock(instances_mutex);
    if (!instances.Destroy(instance_id & INSTANCE_ID_MASK)) {
        return ORBIS_AJM_ERROR_INVALID_INSTANCE;
    }
    return ORBIS_OK;
}

} // namespace Libraries::Ajm
