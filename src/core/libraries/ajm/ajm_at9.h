// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/ajm/ajm_instance.h"

#include "libatrac9.h"

namespace Libraries::Ajm {

constexpr s32 ORBIS_AJM_DEC_AT9_MAX_CHANNELS = 8;

struct AjmSidebandDecAt9CodecInfo {
    u32 super_frame_size;
    u32 frames_in_super_frame;
    u32 next_frame_size;
    u32 frame_samples;
};

struct AjmAt9Decoder final : AjmCodec {
    explicit AjmAt9Decoder();
    ~AjmAt9Decoder() override;

    void Reset() override;
    void Initialize(const void* buffer, u32 buffer_size) override;
    void GetInfo(void* out_info) override;
    std::tuple<u32, u32> ProcessData(std::span<u8>& input, SparseOutputBuffer& output,
                                     AjmSidebandGaplessDecode& gapless, u32 max_samples) override;

private:
    void* m_handle{};
    u8 m_config_data[ORBIS_AT9_CONFIG_DATA_SIZE]{};
    u32 m_superframe_bytes_remain{};
    u32 m_num_frames{};
    Atrac9CodecInfo m_codec_info{};
    std::vector<s16> m_pcm_buffer;
};

} // namespace Libraries::Ajm
