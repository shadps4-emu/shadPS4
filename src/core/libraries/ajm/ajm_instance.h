// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "core/libraries/ajm/ajm.h"
#include "core/libraries/ajm/ajm_batch.h"

#include <memory>
#include <optional>
#include <tuple>

namespace Libraries::Ajm {

u8 GetPCMSize(AjmFormatEncoding format);

class SparseOutputBuffer {
public:
    SparseOutputBuffer(std::span<std::span<u8>> chunks)
        : m_chunks(chunks), m_current(m_chunks.begin()) {}

    template <class T>
    size_t Write(std::span<T> pcm) {
        size_t samples_written = 0;
        while (!pcm.empty() && !IsEmpty()) {
            auto size = std::min(pcm.size() * sizeof(T), m_current->size());
            std::memcpy(m_current->data(), pcm.data(), size);
            const auto nsamples = size / sizeof(T);
            samples_written += nsamples;
            pcm = pcm.subspan(nsamples);
            *m_current = m_current->subspan(size);
            if (m_current->empty()) {
                ++m_current;
            }
        }
        if (!pcm.empty()) {
            LOG_ERROR(Lib_Ajm, "Could not write {} samples", pcm.size());
        }
        return samples_written;
    }

    bool IsEmpty() const {
        return m_current == m_chunks.end();
    }

    size_t Size() const {
        size_t result = 0;
        for (auto it = m_current; it != m_chunks.end(); ++it) {
            result += it->size();
        }
        return result;
    }

private:
    std::span<std::span<u8>> m_chunks;
    std::span<std::span<u8>>::iterator m_current;
};

struct AjmInstanceGapless {
    AjmSidebandGaplessDecode init{};
    AjmSidebandGaplessDecode current{};

    bool IsEnd() const {
        return init.total_samples != 0 && current.total_samples == 0;
    }

    void Reset() {
        current.total_samples = init.total_samples;
        current.skip_samples = init.skip_samples;
        current.skipped_samples = 0;
    }
};

struct DecoderResult {
    s32 result = 0;
    s32 internal_result = 0;
    u32 frames_decoded = 0;
    u32 samples_written = 0;
    bool is_reset = false;
};

class AjmCodec {
public:
    virtual ~AjmCodec() = default;

    virtual void Initialize(const void* buffer, u32 buffer_size) = 0;
    virtual void Reset() = 0;
    virtual void GetInfo(void* out_info) const = 0;
    virtual AjmSidebandFormat GetFormat() const = 0;
    virtual u32 GetMinimumInputSize() const = 0;
    virtual u32 GetNextFrameSize(const AjmInstanceGapless& gapless) const = 0;
    virtual DecoderResult ProcessData(std::span<u8>& input, SparseOutputBuffer& output,
                                      AjmInstanceGapless& gapless) = 0;
};

class AjmInstance {
public:
    AjmInstance(AjmCodecType codec_type, AjmInstanceFlags flags);

    void ExecuteJob(AjmJob& job);

private:
    bool HasEnoughSpace(const SparseOutputBuffer& output) const;
    void Reset();

    AjmInstanceFlags m_flags{};
    AjmSidebandFormat m_format{};
    AjmInstanceGapless m_gapless{};
    AjmSidebandResampleParameters m_resample_parameters{};
    u32 m_total_samples{};
    std::unique_ptr<AjmCodec> m_codec;
};

} // namespace Libraries::Ajm
