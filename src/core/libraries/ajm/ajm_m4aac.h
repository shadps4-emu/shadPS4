// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/ajm/ajm_instance.h"

extern "C" {
#include <libavcodec/avcodec.h>
struct SwrContext;
}

namespace Libraries::Ajm {

enum AjmM4aacCodecFlags : u32 {
    SbrDecode = 1 << 0,
    NodelayOutput = 1 << 8,
    InterleaveOrderExtlExtrLsRs = 1 << 16,
    InterleaveOrderLsRsExtlExtr = 1 << 24
};
DECLARE_ENUM_FLAG_OPERATORS(AjmM4aacCodecFlags)

class AjmM4aacDecoder : public AjmCodec {
public:
    explicit AjmM4aacDecoder(AjmFormatEncoding format, AjmM4aacCodecFlags flags);
    ~AjmM4aacDecoder() override;

    void Reset() override;
    void Initialize(const void* buffer, u32 buffer_size) override {}
    void GetInfo(void* out_info) const override;
    AjmSidebandFormat GetFormat() const override;
    u32 GetNextFrameSize(const AjmInstanceGapless& gapless) const override;
    std::tuple<u32, u32, bool> ProcessData(std::span<u8>& input, SparseOutputBuffer& output,
                                           AjmInstanceGapless& gapless) override;

private:
    template <class T>
    size_t WriteOutputPCM(AVFrame* frame, SparseOutputBuffer& output, u32 skipped_samples,
                          u32 max_pcm) {
        std::span<T> pcm_data(reinterpret_cast<T*>(frame->data[0]),
                              frame->nb_samples * frame->ch_layout.nb_channels);
        pcm_data = pcm_data.subspan(skipped_samples * frame->ch_layout.nb_channels);
        return output.Write(pcm_data.subspan(0, std::min(u32(pcm_data.size()), max_pcm)));
    }

    AVFrame* ConvertAudioFrame(AVFrame* frame);

    const AjmFormatEncoding m_format;
    const AjmM4aacCodecFlags m_flags;
    const AVCodec* m_codec = nullptr;
    AVCodecContext* m_codec_context = nullptr;
    AVCodecParserContext* m_parser = nullptr;
    SwrContext* m_swr_context = nullptr;
    std::optional<u32> m_header;
    u32 m_frame_samples = 0;
};

} // namespace Libraries::Ajm
