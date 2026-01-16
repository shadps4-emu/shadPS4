// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"
#include "core/libraries/ajm/ajm_instance.h"

#include <span>
#include <vector>

struct AAC_DECODER_INSTANCE;

namespace Libraries::Ajm {

enum ConfigType : u32 {
    ADTS = 1,
    RAW = 2,
};

enum AjmAacCodecFlags : u32 {
    EnableSbrDecode = 1 << 0,
    EnableNondelayOutput = 1 << 1,
    SurroundChannelInterleaveOrderExtlExtrLsRs = 1 << 2,
    SurroundChannelInterleaveOrderLsRsExtlExtr = 1 << 3,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmAacCodecFlags)

struct AjmSidebandDecM4aacCodecInfo {
    u32 heaac;
    u32 reserved;
};

struct AjmAacDecoder final : AjmCodec {
    explicit AjmAacDecoder(AjmFormatEncoding format, AjmAacCodecFlags flags, u32 channels);
    ~AjmAacDecoder() override;

    void Reset() override;
    void Initialize(const void* buffer, u32 buffer_size) override;
    void GetInfo(void* out_info) const override;
    AjmSidebandFormat GetFormat() const override;
    u32 GetMinimumInputSize() const override;
    u32 GetNextFrameSize(const AjmInstanceGapless& gapless) const override;
    DecoderResult ProcessData(std::span<u8>& input, SparseOutputBuffer& output,
                              AjmInstanceGapless& gapless) override;

private:
    struct InitializeParameters {
        ConfigType config_type;
        u32 sampling_freq_type;
    };

    template <class T>
    size_t WriteOutputSamples(SparseOutputBuffer& output, std::span<const s16> pcm);
    std::span<const s16> GetOuputPcm(u32 skipped_pcm, u32 max_pcm) const;

    const AjmFormatEncoding m_format;
    const AjmAacCodecFlags m_flags;
    const u32 m_channels;
    std::vector<s16> m_pcm_buffer;
    std::vector<float> m_resample_buffer;

    u32 m_skip_frames = 0;
    InitializeParameters m_init_params = {};
    AAC_DECODER_INSTANCE* m_decoder = nullptr;
};

} // namespace Libraries::Ajm