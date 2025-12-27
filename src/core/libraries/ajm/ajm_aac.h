// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
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
    enableSbrDecode = 1 << 0,
    enableNondelayOutput = 1 << 1,
    surroundChannelInterleaveOrderExtlExtrLsRs = 1 << 2,
    surroundChannelInterleaveOrderLsRsExtlExtr = 1 << 3,
};
DECLARE_ENUM_FLAG_OPERATORS(AjmAacCodecFlags)

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

    u32 GetNumChannels() const;

    template <class T>
    size_t WriteOutputSamples(SparseOutputBuffer& output, u32 skipped_samples, u32 max_samples) {
        std::span<T> pcm_data{reinterpret_cast<T*>(m_pcm_buffer.data()),
                              m_pcm_buffer.size() / sizeof(T)};
        pcm_data = pcm_data.subspan(skipped_samples * GetNumChannels());
        const auto pcm_size = std::min(u32(pcm_data.size()), max_samples);
        return output.Write(pcm_data.subspan(0, pcm_size));
    }

    const AjmFormatEncoding m_format;
    const AjmAacCodecFlags m_flags;
    const u32 m_channels;
    std::vector<u8> m_pcm_buffer;

    InitializeParameters m_init_params = {};
    AAC_DECODER_INSTANCE* m_decoder = nullptr;
};

} // namespace Libraries::Ajm
