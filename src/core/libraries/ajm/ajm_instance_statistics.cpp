// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ajm/ajm.h"
#include "core/libraries/ajm/ajm_instance_statistics.h"

namespace Libraries::Ajm {

void AjmInstanceStatistics::ExecuteJob(AjmJob& job) {
    if (job.output.p_engine) {
        job.output.p_engine->usage_batch = 0.05;
        const auto ic = job.input.statistics_engine_parameters->interval_count;
        for (u32 idx = 0; idx < ic; ++idx) {
            job.output.p_engine->usage_interval[idx] = 0.01;
        }
    }
    if (job.output.p_engine_per_codec) {
        job.output.p_engine_per_codec->codec_count = 1;
        job.output.p_engine_per_codec->codec_id[0] = static_cast<u8>(AjmCodecType::At9Dec);
        job.output.p_engine_per_codec->codec_percentage[0] = 0.01;
    }
    if (job.output.p_memory) {
        job.output.p_memory->instance_free = 0x400000;
        job.output.p_memory->buffer_free = 0x400000;
        job.output.p_memory->batch_size = 0x4200;
        job.output.p_memory->input_size = 0x2000;
        job.output.p_memory->output_size = 0x2000;
        job.output.p_memory->small_size = 0x400;
    }
}

void AjmInstanceStatistics::Reset() {}

AjmInstanceStatistics& AjmInstanceStatistics::Getinstance() {
    static AjmInstanceStatistics instance;
    return instance;
}

} // namespace Libraries::Ajm
