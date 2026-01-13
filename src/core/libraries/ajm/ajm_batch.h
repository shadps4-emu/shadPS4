// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/types.h"
#include "core/libraries/ajm/ajm.h"

#include <boost/container/small_vector.hpp>

#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <semaphore>
#include <span>
#include <vector>

namespace Libraries::Ajm {

struct AjmJob {
    struct Input {
        std::optional<AjmSidebandInitParameters> init_params;
        std::optional<AjmSidebandResampleParameters> resample_parameters;
        std::optional<AjmSidebandStatisticsEngineParameters> statistics_engine_parameters;
        std::optional<AjmSidebandFormat> format;
        std::optional<AjmSidebandGaplessDecode> gapless_decode;
        std::vector<u8> buffer;
    };

    struct Output {
        boost::container::small_vector<std::span<u8>, 8> buffers;
        AjmSidebandResult* p_result = nullptr;
        AjmSidebandStream* p_stream = nullptr;
        AjmSidebandFormat* p_format = nullptr;
        AjmSidebandStatisticsMemory* p_memory = nullptr;
        AjmSidebandStatisticsEnginePerCodec* p_engine_per_codec = nullptr;
        AjmSidebandStatisticsEngine* p_engine = nullptr;
        AjmSidebandGaplessDecode* p_gapless_decode = nullptr;
        AjmSidebandMFrame* p_mframe = nullptr;
        u8* p_codec_info = nullptr;
    };

    u32 instance_id{};
    AjmJobFlags flags{};
    Input input;
    Output output;
};

struct AjmBatch {
    u32 id{};
    std::atomic_bool waiting{};
    std::atomic_bool canceled{};
    std::atomic_bool processed{};
    std::binary_semaphore finished{0};
    boost::container::small_vector<AjmJob, 16> jobs;

    static std::shared_ptr<AjmBatch> FromBatchBuffer(std::span<u8> buffer);
};

void* BatchJobControlBufferRa(void* p_buffer, u32 instance_id, u64 flags, void* p_sideband_input,
                              size_t sideband_input_size, void* p_sideband_output,
                              size_t sideband_output_size, void* p_return_address);

void* BatchJobInlineBuffer(void* p_buffer, const void* p_data_input, size_t data_input_size,
                           const void** pp_batch_address);

void* BatchJobRunBufferRa(void* p_buffer, u32 instance_id, u64 flags, void* p_data_input,
                          size_t data_input_size, void* p_data_output, size_t data_output_size,
                          void* p_sideband_output, size_t sideband_output_size,
                          void* p_return_address);

void* BatchJobRunSplitBufferRa(void* p_buffer, u32 instance_id, u64 flags,
                               const AjmBuffer* p_data_input_buffers, size_t num_data_input_buffers,
                               const AjmBuffer* p_data_output_buffers,
                               size_t num_data_output_buffers, void* p_sideband_output,
                               size_t sideband_output_size, void* p_return_address);

} // namespace Libraries::Ajm
