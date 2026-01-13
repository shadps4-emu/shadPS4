// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "core/libraries/ajm/ajm_batch.h"

namespace Libraries::Ajm {

enum Identifier : u8 {
    AjmIdentJob = 0,
    AjmIdentInputRunBuf = 1,
    AjmIdentInputControlBuf = 2,
    AjmIdentControlFlags = 3,
    AjmIdentRunFlags = 4,
    AjmIdentReturnAddressBuf = 6,
    AjmIdentInlineBuf = 7,
    AjmIdentOutputRunBuf = 17,
    AjmIdentOutputControlBuf = 18,
};

struct AjmChunkHeader {
    u32 ident : 6;
    u32 payload : 20;
    u32 reserved : 6;
};
static_assert(sizeof(AjmChunkHeader) == 4);

struct AjmChunkJob {
    AjmChunkHeader header;
    u32 size;
};
static_assert(sizeof(AjmChunkJob) == 8);

struct AjmChunkFlags {
    AjmChunkHeader header;
    u32 flags_low;
};
static_assert(sizeof(AjmChunkFlags) == 8);

struct AjmChunkBuffer {
    AjmChunkHeader header;
    u32 size;
    void* p_address;
};
static_assert(sizeof(AjmChunkBuffer) == 16);

class AjmBatchBuffer {
public:
    static constexpr size_t s_dynamic_extent = std::numeric_limits<size_t>::max();

    AjmBatchBuffer(u8* begin, u8* end)
        : m_p_begin(begin), m_p_current(begin), m_size(end - begin) {}
    AjmBatchBuffer(u8* begin, size_t size = s_dynamic_extent)
        : m_p_begin(begin), m_p_current(m_p_begin), m_size(size) {}
    AjmBatchBuffer(std::span<u8> data)
        : m_p_begin(data.data()), m_p_current(m_p_begin), m_size(data.size()) {}
    AjmBatchBuffer(AjmChunkBuffer& buffer)
        : AjmBatchBuffer(reinterpret_cast<u8*>(buffer.p_address), buffer.size) {}

    AjmBatchBuffer SubBuffer(size_t size = s_dynamic_extent) {
        auto current = m_p_current;
        if (size != s_dynamic_extent) {
            m_p_current += size;
        }
        return AjmBatchBuffer(current, size);
    }

    template <class T>
    T& Peek() const {
        DEBUG_ASSERT(m_size == s_dynamic_extent ||
                     (m_p_current + sizeof(T)) <= (m_p_begin + m_size));
        return *reinterpret_cast<T*>(m_p_current);
    }

    template <class T>
    T& Consume() {
        auto* const result = reinterpret_cast<T*>(m_p_current);
        m_p_current += sizeof(T);
        DEBUG_ASSERT(m_size == s_dynamic_extent || m_p_current <= (m_p_begin + m_size));
        return *result;
    }

    template <class T>
    void Skip() {
        Advance(sizeof(T));
    }

    void Advance(size_t size) {
        m_p_current += size;
        DEBUG_ASSERT(m_size == s_dynamic_extent || m_p_current <= (m_p_begin + m_size));
    }

    bool IsEmpty() {
        return m_size != s_dynamic_extent && m_p_current >= (m_p_begin + m_size);
    }

    size_t BytesConsumed() const {
        return m_p_current - m_p_begin;
    }

    size_t BytesRemaining() const {
        if (m_size == s_dynamic_extent) {
            return s_dynamic_extent;
        }
        return m_size - (m_p_current - m_p_begin);
    }

    u8* GetCurrent() const {
        return m_p_current;
    }

private:
    u8* m_p_begin{};
    u8* m_p_current{};
    size_t m_size{};
};

AjmJob AjmStatisticsJobFromBatchBuffer(u32 instance_id, AjmBatchBuffer batch_buffer) {
    std::optional<AjmJobFlags> job_flags = {};
    std::optional<AjmChunkBuffer> input_control_buffer = {};
    std::optional<AjmChunkBuffer> output_control_buffer = {};

    AjmJob job;
    job.instance_id = instance_id;

    while (!batch_buffer.IsEmpty()) {
        auto& header = batch_buffer.Peek<AjmChunkHeader>();
        switch (header.ident) {
        case Identifier::AjmIdentInputControlBuf: {
            ASSERT_MSG(!input_control_buffer.has_value(),
                       "Only one instance of input control buffer is allowed per job");
            const auto& buffer = batch_buffer.Consume<AjmChunkBuffer>();
            if (buffer.p_address != nullptr && buffer.size != 0) {
                input_control_buffer = buffer;
            }
            break;
        }
        case Identifier::AjmIdentControlFlags: {
            ASSERT_MSG(!job_flags.has_value(), "Only one instance of job flags is allowed per job");
            auto& chunk = batch_buffer.Consume<AjmChunkFlags>();
            job_flags = AjmJobFlags{
                .raw = (u64(chunk.header.payload) << 32) + chunk.flags_low,
            };
            break;
        }
        case Identifier::AjmIdentReturnAddressBuf: {
            // Ignore return address buffers.
            batch_buffer.Skip<AjmChunkBuffer>();
            break;
        }
        case Identifier::AjmIdentOutputControlBuf: {
            ASSERT_MSG(!output_control_buffer.has_value(),
                       "Only one instance of output control buffer is allowed per job");
            const auto& buffer = batch_buffer.Consume<AjmChunkBuffer>();
            if (buffer.p_address != nullptr && buffer.size != 0) {
                output_control_buffer = buffer;
            }
            break;
        }
        default:
            UNREACHABLE_MSG("Unknown chunk: {}", header.ident);
        }
    }

    ASSERT(job_flags.has_value());
    job.flags = job_flags.value();

    AjmStatisticsJobFlags flags{.raw = job.flags.raw};
    if (input_control_buffer.has_value()) {
        AjmBatchBuffer input_batch(input_control_buffer.value());
        if (True(flags.statistics_flags & AjmStatisticsFlags::Engine)) {
            job.input.statistics_engine_parameters =
                input_batch.Consume<AjmSidebandStatisticsEngineParameters>();
        }
    }

    if (output_control_buffer.has_value()) {
        AjmBatchBuffer output_batch(output_control_buffer.value());
        job.output.p_result = &output_batch.Consume<AjmSidebandResult>();
        *job.output.p_result = AjmSidebandResult{};

        if (True(flags.statistics_flags & AjmStatisticsFlags::Engine)) {
            job.output.p_engine = &output_batch.Consume<AjmSidebandStatisticsEngine>();
            *job.output.p_engine = AjmSidebandStatisticsEngine{};
        }
        if (True(flags.statistics_flags & AjmStatisticsFlags::EnginePerCodec)) {
            job.output.p_engine_per_codec =
                &output_batch.Consume<AjmSidebandStatisticsEnginePerCodec>();
            *job.output.p_engine_per_codec = AjmSidebandStatisticsEnginePerCodec{};
        }
        if (True(flags.statistics_flags & AjmStatisticsFlags::Memory)) {
            job.output.p_memory = &output_batch.Consume<AjmSidebandStatisticsMemory>();
            *job.output.p_memory = AjmSidebandStatisticsMemory{};
        }
    }

    return job;
}

AjmJob AjmJobFromBatchBuffer(u32 instance_id, AjmBatchBuffer batch_buffer) {
    std::optional<AjmJobFlags> job_flags = {};
    std::optional<AjmChunkBuffer> input_control_buffer = {};
    std::optional<AjmChunkBuffer> output_control_buffer = {};
    std::optional<AjmChunkBuffer> inline_buffer = {};

    AjmJob job;
    job.instance_id = instance_id;

    // Read parameters of a job
    while (!batch_buffer.IsEmpty()) {
        auto& header = batch_buffer.Peek<AjmChunkHeader>();
        switch (header.ident) {
        case Identifier::AjmIdentInputRunBuf: {
            auto& buffer = batch_buffer.Consume<AjmChunkBuffer>();
            u8* p_begin = reinterpret_cast<u8*>(buffer.p_address);
            job.input.buffer.insert(job.input.buffer.end(), p_begin, p_begin + buffer.size);
            break;
        }
        case Identifier::AjmIdentInputControlBuf: {
            ASSERT_MSG(!input_control_buffer.has_value(),
                       "Only one instance of input control buffer is allowed per job");
            const auto& buffer = batch_buffer.Consume<AjmChunkBuffer>();
            if (buffer.p_address != nullptr && buffer.size != 0) {
                input_control_buffer = buffer;
            }
            break;
        }
        case Identifier::AjmIdentControlFlags:
        case Identifier::AjmIdentRunFlags: {
            ASSERT_MSG(!job_flags.has_value(), "Only one instance of job flags is allowed per job");
            auto& chunk = batch_buffer.Consume<AjmChunkFlags>();
            job_flags = AjmJobFlags{
                .raw = (u64(chunk.header.payload) << 32) + chunk.flags_low,
            };
            break;
        }
        case Identifier::AjmIdentReturnAddressBuf: {
            // Ignore return address buffers.
            batch_buffer.Skip<AjmChunkBuffer>();
            break;
        }
        case Identifier::AjmIdentOutputRunBuf: {
            auto& buffer = batch_buffer.Consume<AjmChunkBuffer>();
            u8* p_begin = reinterpret_cast<u8*>(buffer.p_address);
            if (p_begin != nullptr && buffer.size != 0) {
                job.output.buffers.emplace_back(std::span<u8>(p_begin, p_begin + buffer.size));
            }
            break;
        }
        case Identifier::AjmIdentOutputControlBuf: {
            ASSERT_MSG(!output_control_buffer.has_value(),
                       "Only one instance of output control buffer is allowed per job");
            const auto& buffer = batch_buffer.Consume<AjmChunkBuffer>();
            if (buffer.p_address != nullptr && buffer.size != 0) {
                output_control_buffer = buffer;
            }
            break;
        }
        default:
            UNREACHABLE_MSG("Unknown chunk: {}", header.ident);
        }
    }

    ASSERT(job_flags.has_value());
    job.flags = job_flags.value();

    // Initialize sideband input parameters
    if (input_control_buffer.has_value()) {
        AjmBatchBuffer input_batch(input_control_buffer.value());
        const auto sideband_flags = job_flags->sideband_flags;
        if (True(sideband_flags & AjmJobSidebandFlags::Format) && !input_batch.IsEmpty()) {
            job.input.format = input_batch.Consume<AjmSidebandFormat>();
        }
        if (True(sideband_flags & AjmJobSidebandFlags::GaplessDecode) && !input_batch.IsEmpty()) {
            job.input.gapless_decode = input_batch.Consume<AjmSidebandGaplessDecode>();
        }

        const auto control_flags = job_flags.value().control_flags;
        if (True(control_flags & AjmJobControlFlags::Resample)) {
            job.input.resample_parameters = input_batch.Consume<AjmSidebandResampleParameters>();
        }
        if (True(control_flags & AjmJobControlFlags::Initialize)) {
            job.input.init_params = input_batch.Consume<AjmSidebandInitParameters>();
        }
    }

    // Initialize sideband output parameters
    if (output_control_buffer.has_value()) {
        AjmBatchBuffer output_batch(output_control_buffer.value());
        job.output.p_result = &output_batch.Consume<AjmSidebandResult>();
        *job.output.p_result = AjmSidebandResult{};

        const auto sideband_flags = job_flags->sideband_flags;
        if (True(sideband_flags & AjmJobSidebandFlags::Stream) && !output_batch.IsEmpty()) {
            job.output.p_stream = &output_batch.Consume<AjmSidebandStream>();
            *job.output.p_stream = AjmSidebandStream{};
        }
        if (True(sideband_flags & AjmJobSidebandFlags::Format) && !output_batch.IsEmpty()) {
            job.output.p_format = &output_batch.Consume<AjmSidebandFormat>();
            *job.output.p_format = AjmSidebandFormat{};
        }
        if (True(sideband_flags & AjmJobSidebandFlags::GaplessDecode) && !output_batch.IsEmpty()) {
            job.output.p_gapless_decode = &output_batch.Consume<AjmSidebandGaplessDecode>();
            *job.output.p_gapless_decode = AjmSidebandGaplessDecode{};
        }

        const auto run_flags = job_flags->run_flags;
        if (True(run_flags & AjmJobRunFlags::MultipleFrames) && !output_batch.IsEmpty()) {
            job.output.p_mframe = &output_batch.Consume<AjmSidebandMFrame>();
            *job.output.p_mframe = AjmSidebandMFrame{};
        }
        if (True(run_flags & AjmJobRunFlags::GetCodecInfo) && !output_batch.IsEmpty()) {
            job.output.p_codec_info = output_batch.GetCurrent();
        }
    }

    return job;
}

std::shared_ptr<AjmBatch> AjmBatch::FromBatchBuffer(std::span<u8> data) {
    auto batch = std::make_shared<AjmBatch>();

    AjmBatchBuffer buffer(data);
    while (!buffer.IsEmpty()) {
        auto& job_chunk = buffer.Consume<AjmChunkJob>();
        if (job_chunk.header.ident == AjmIdentInlineBuf) {
            // Inline buffers are used to store sideband input data.
            // We should just skip them as they do not require any special handling.
            buffer.Advance(job_chunk.size);
            continue;
        }
        ASSERT(job_chunk.header.ident == AjmIdentJob);
        auto instance_id = job_chunk.header.payload;
        if (instance_id == AJM_INSTANCE_STATISTICS) {
            batch->jobs.push_back(
                AjmStatisticsJobFromBatchBuffer(instance_id, buffer.SubBuffer(job_chunk.size)));
        } else {
            batch->jobs.push_back(
                AjmJobFromBatchBuffer(instance_id, buffer.SubBuffer(job_chunk.size)));
        }
    }

    return batch;
}

void* BatchJobControlBufferRa(void* p_buffer, u32 instance_id, u64 flags, void* p_sideband_input,
                              size_t sideband_input_size, void* p_sideband_output,
                              size_t sideband_output_size, void* p_return_address) {
    LOG_TRACE(Lib_Ajm, "called");

    AjmBatchBuffer buffer(reinterpret_cast<u8*>(p_buffer));

    auto& job_chunk = buffer.Consume<AjmChunkJob>();
    job_chunk.header.ident = AjmIdentJob;
    job_chunk.header.payload = instance_id;

    auto job_buffer = buffer.SubBuffer();

    if (p_return_address != nullptr) {
        auto& chunk_ra = job_buffer.Consume<AjmChunkBuffer>();
        chunk_ra.header.ident = AjmIdentReturnAddressBuf;
        chunk_ra.header.payload = 0;
        chunk_ra.size = 0;
        chunk_ra.p_address = p_return_address;
    }

    {
        auto& chunk_input = job_buffer.Consume<AjmChunkBuffer>();
        chunk_input.header.ident = AjmIdentInputControlBuf;
        chunk_input.header.payload = 0;
        chunk_input.size = sideband_input_size;
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

        auto& chunk_flags = job_buffer.Consume<AjmChunkFlags>();
        chunk_flags.header.ident = AjmIdentControlFlags;
        chunk_flags.header.payload = u32(flags >> 32);
        chunk_flags.flags_low = u32(flags);
    }

    {
        auto& chunk_output = job_buffer.Consume<AjmChunkBuffer>();
        chunk_output.header.ident = AjmIdentOutputControlBuf;
        chunk_output.header.payload = 0;
        chunk_output.size = sideband_output_size;
        chunk_output.p_address = p_sideband_output;
    }

    job_chunk.size = job_buffer.BytesConsumed();
    return job_buffer.GetCurrent();
}

void* BatchJobInlineBuffer(void* p_buffer, const void* p_data_input, size_t data_input_size,
                           const void** pp_batch_address) {
    LOG_TRACE(Lib_Ajm, "called");

    AjmBatchBuffer buffer(reinterpret_cast<u8*>(p_buffer));

    auto& job_chunk = buffer.Consume<AjmChunkJob>();
    job_chunk.header.ident = AjmIdentInlineBuf;
    job_chunk.header.payload = 0;
    job_chunk.size = Common::AlignUp(data_input_size, 8);
    *pp_batch_address = buffer.GetCurrent();

    memcpy(buffer.GetCurrent(), p_data_input, data_input_size);
    return buffer.GetCurrent() + job_chunk.size;
}

void* BatchJobRunBufferRa(void* p_buffer, u32 instance_id, u64 flags, void* p_data_input,
                          size_t data_input_size, void* p_data_output, size_t data_output_size,
                          void* p_sideband_output, size_t sideband_output_size,
                          void* p_return_address) {
    LOG_TRACE(Lib_Ajm, "called");

    AjmBatchBuffer buffer(reinterpret_cast<u8*>(p_buffer));

    auto& job_chunk = buffer.Consume<AjmChunkJob>();
    job_chunk.header.ident = AjmIdentJob;
    job_chunk.header.payload = instance_id;

    auto job_buffer = buffer.SubBuffer();

    if (p_return_address != nullptr) {
        auto& chunk_ra = job_buffer.Consume<AjmChunkBuffer>();
        chunk_ra.header.ident = AjmIdentReturnAddressBuf;
        chunk_ra.header.payload = 0;
        chunk_ra.size = 0;
        chunk_ra.p_address = p_return_address;
    }

    {
        auto& chunk_input = job_buffer.Consume<AjmChunkBuffer>();
        chunk_input.header.ident = AjmIdentInputRunBuf;
        chunk_input.header.payload = 0;
        chunk_input.size = data_input_size;
        chunk_input.p_address = p_data_input;
    }

    {
        // 0x0000'E000'0000'1FFF:
        // | sideband | reserved                      | control | run | codec    | revision |
        // | 111      | 00000000000000000000000000000 | 000     | 11  | 11111111 | 111      |
        flags &= 0x0000'E000'0000'1FFF;

        auto& chunk_flags = job_buffer.Consume<AjmChunkFlags>();
        chunk_flags.header.ident = AjmIdentRunFlags;
        chunk_flags.header.payload = u32(flags >> 32);
        chunk_flags.flags_low = u32(flags);
    }

    {
        auto& chunk_output = job_buffer.Consume<AjmChunkBuffer>();
        chunk_output.header.ident = AjmIdentOutputRunBuf;
        chunk_output.header.payload = 0;
        chunk_output.size = data_output_size;
        chunk_output.p_address = p_data_output;
    }

    {
        auto& chunk_output = job_buffer.Consume<AjmChunkBuffer>();
        chunk_output.header.ident = AjmIdentOutputControlBuf;
        chunk_output.header.payload = 0;
        chunk_output.size = sideband_output_size;
        chunk_output.p_address = p_sideband_output;
    }

    job_chunk.size = job_buffer.BytesConsumed();
    return job_buffer.GetCurrent();
}

void* BatchJobRunSplitBufferRa(void* p_buffer, u32 instance_id, u64 flags,
                               const AjmBuffer* p_data_input_buffers, size_t num_data_input_buffers,
                               const AjmBuffer* p_data_output_buffers,
                               size_t num_data_output_buffers, void* p_sideband_output,
                               size_t sideband_output_size, void* p_return_address) {
    LOG_TRACE(Lib_Ajm, "called");

    AjmBatchBuffer buffer(reinterpret_cast<u8*>(p_buffer));

    auto& job_chunk = buffer.Consume<AjmChunkJob>();
    job_chunk.header.ident = AjmIdentJob;
    job_chunk.header.payload = instance_id;

    auto job_buffer = buffer.SubBuffer();

    if (p_return_address != nullptr) {
        auto& chunk_ra = job_buffer.Consume<AjmChunkBuffer>();
        chunk_ra.header.ident = AjmIdentReturnAddressBuf;
        chunk_ra.header.payload = 0;
        chunk_ra.size = 0;
        chunk_ra.p_address = p_return_address;
    }

    for (s32 i = 0; i < num_data_input_buffers; i++) {
        auto& chunk_input = job_buffer.Consume<AjmChunkBuffer>();
        chunk_input.header.ident = AjmIdentInputRunBuf;
        chunk_input.header.payload = 0;
        chunk_input.size = p_data_input_buffers[i].size;
        chunk_input.p_address = p_data_input_buffers[i].p_address;
    }

    {
        // 0x0000'E000'0000'1FFF:
        // | sideband | reserved                      | control | run | codec    | revision |
        // | 111      | 00000000000000000000000000000 | 000     | 11  | 11111111 | 111      |
        flags &= 0x0000'E000'0000'1FFF;

        auto& chunk_flags = job_buffer.Consume<AjmChunkFlags>();
        chunk_flags.header.ident = AjmIdentRunFlags;
        chunk_flags.header.payload = u32(flags >> 32);
        chunk_flags.flags_low = u32(flags);
    }

    for (s32 i = 0; i < num_data_output_buffers; i++) {
        auto& chunk_output = job_buffer.Consume<AjmChunkBuffer>();
        chunk_output.header.ident = AjmIdentOutputRunBuf;
        chunk_output.header.payload = 0;
        chunk_output.size = p_data_output_buffers[i].size;
        chunk_output.p_address = p_data_output_buffers[i].p_address;
    }

    {
        auto& chunk_output = job_buffer.Consume<AjmChunkBuffer>();
        chunk_output.header.ident = AjmIdentOutputControlBuf;
        chunk_output.header.payload = 0;
        chunk_output.size = sideband_output_size;
        chunk_output.p_address = p_sideband_output;
    }

    job_chunk.size = job_buffer.BytesConsumed();
    return job_buffer.GetCurrent();
}

} // namespace Libraries::Ajm
