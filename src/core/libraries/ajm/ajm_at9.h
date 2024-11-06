// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/ajm/ajm_instance.h"

#include "libatrac9.h"

#include <span>

namespace Libraries::Ajm {

constexpr s32 ORBIS_AJM_DEC_AT9_MAX_CHANNELS = 8;

enum AjmAt9CodecFlags : u32 {
    ParseRiffHeader = 1 << 0,
    NonInterleavedOutput = 1 << 8,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmAt9CodecFlags)

struct AjmSidebandDecAt9CodecInfo {
    u32 super_frame_size;
    u32 frames_in_super_frame;
    u32 next_frame_size;
    u32 frame_samples;
};

struct AjmAt9Decoder final : AjmCodec {
    explicit AjmAt9Decoder(AjmFormatEncoding format, AjmAt9CodecFlags flags);
    ~AjmAt9Decoder() override;

    void Reset() override;
    void Initialize(const void* buffer, u32 buffer_size) override;
    void GetInfo(void* out_info) override;
    AjmSidebandFormat GetFormat() override;
    std::tuple<u32, u32> ProcessData(std::span<u8>& input, SparseOutputBuffer& output,
                                     AjmSidebandGaplessDecode& gapless,
                                     std::optional<u32> max_samples) override;

private:
    u8 GetPointCodeSize();

    template <class T>
    size_t WriteOutputSamples(SparseOutputBuffer& output, u32 skipped_samples, u32 max_samples) {
        std::span<T> pcm_data{reinterpret_cast<T*>(m_pcm_buffer.data()),
                              m_pcm_buffer.size() / sizeof(T)};
        pcm_data = pcm_data.subspan(skipped_samples * m_codec_info.channels);
        const auto pcm_size = std::min(u32(pcm_data.size()), max_samples);
        return output.Write(pcm_data.subspan(0, pcm_size));
    }

    const AjmFormatEncoding m_format;
    const AjmAt9CodecFlags m_flags;
    void* m_handle{};
    u8 m_config_data[ORBIS_AT9_CONFIG_DATA_SIZE]{};
    u32 m_superframe_bytes_remain{};
    u32 m_num_frames{};
    Atrac9CodecInfo m_codec_info{};
    std::vector<u8> m_pcm_buffer;
};

} // namespace Libraries::Ajm
