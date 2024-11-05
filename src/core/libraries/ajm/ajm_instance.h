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

constexpr int ORBIS_AJM_RESULT_NOT_INITIALIZED = 0x00000001;
constexpr int ORBIS_AJM_RESULT_INVALID_DATA = 0x00000002;
constexpr int ORBIS_AJM_RESULT_INVALID_PARAMETER = 0x00000004;
constexpr int ORBIS_AJM_RESULT_PARTIAL_INPUT = 0x00000008;
constexpr int ORBIS_AJM_RESULT_NOT_ENOUGH_ROOM = 0x00000010;
constexpr int ORBIS_AJM_RESULT_STREAM_CHANGE = 0x00000020;
constexpr int ORBIS_AJM_RESULT_TOO_MANY_CHANNELS = 0x00000040;
constexpr int ORBIS_AJM_RESULT_UNSUPPORTED_FLAG = 0x00000080;
constexpr int ORBIS_AJM_RESULT_SIDEBAND_TRUNCATED = 0x00000100;
constexpr int ORBIS_AJM_RESULT_PRIORITY_PASSED = 0x00000200;
constexpr int ORBIS_AJM_RESULT_CODEC_ERROR = 0x40000000;
constexpr int ORBIS_AJM_RESULT_FATAL = 0x80000000;

class SparseOutputBuffer {
public:
    SparseOutputBuffer(std::span<std::span<u8>> chunks)
        : m_chunks(chunks), m_current(m_chunks.begin()) {}

    template <class T>
    size_t Write(std::span<T> pcm) {
        size_t bytes_written = 0;
        while (!pcm.empty() && !IsEmpty()) {
            auto size = std::min(pcm.size() * sizeof(T), m_current->size());
            std::memcpy(m_current->data(), pcm.data(), size);
            bytes_written += size;
            pcm = pcm.subspan(size / sizeof(T));
            *m_current = m_current->subspan(size);
            if (m_current->empty()) {
                ++m_current;
            }
        }
        return bytes_written;
    }

    bool IsEmpty() {
        return m_current == m_chunks.end();
    }

    size_t Size() {
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

struct DecodeResult {
    u32 bytes_consumed{};
    u32 bytes_written{};
};

class AjmCodec {
public:
    virtual ~AjmCodec() = default;

    virtual void Initialize(const void* buffer, u32 buffer_size) = 0;
    virtual void Reset() = 0;
    virtual void GetInfo(void* out_info) = 0;
    virtual std::tuple<u32, u32> ProcessData(std::span<u8>& input, SparseOutputBuffer& output,
                                             AjmSidebandGaplessDecode& gapless,
                                             u32 max_samples) = 0;
};

class AjmInstance {
public:
    AjmInstance(AjmCodecType codec_type, AjmInstanceFlags flags);

    void ExecuteJob(AjmJob& job);

private:
    bool IsGaplessEnd();

    AjmInstanceFlags m_flags{};
    AjmSidebandFormat m_format{};
    AjmSidebandGaplessDecode m_gapless{};
    AjmSidebandResampleParameters m_resample_parameters{};

    u32 m_gapless_samples{};
    u32 m_total_samples{};

    std::unique_ptr<AjmCodec> m_codec;

    // AjmCodecType codec_type;
    // u32 index{};
};

} // namespace Libraries::Ajm
