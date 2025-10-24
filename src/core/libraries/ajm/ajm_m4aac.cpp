// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/ajm/ajm_error.h"
#include "core/libraries/ajm/ajm_m4aac.h"
#include "core/libraries/error_codes.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include "common/support/avdec.h"

namespace Libraries::Ajm {
AjmM4aacDecoder::AjmM4aacDecoder(AjmFormatEncoding format, AjmM4aacCodecFlags flags)
    : m_format(format), m_flags(flags), m_codec(avcodec_find_decoder(AV_CODEC_ID_AAC)),
      m_codec_context(avcodec_alloc_context3(m_codec)), m_parser(av_parser_init(m_codec->id)) {
    int ret = avcodec_open2(m_codec_context, m_codec, nullptr);
    ASSERT_MSG(ret >= 0, "Could not open m_codec");
}
AjmM4aacDecoder::~AjmM4aacDecoder() {
    swr_free(&m_swr_context);
    av_parser_close(m_parser);
    avcodec_free_context(&m_codec_context);
}
void AjmM4aacDecoder::Reset() {}
void AjmM4aacDecoder::GetInfo(void* out_info) const {}
AjmSidebandFormat AjmM4aacDecoder::GetFormat() const {
    return AjmSidebandFormat();
}
u32 AjmM4aacDecoder::GetNextFrameSize(const AjmInstanceGapless& gapless) const {
    return u32();
}

std::tuple<u32, u32, bool> AjmM4aacDecoder::ProcessData(std::span<u8>& input,
                                                        SparseOutputBuffer& output,
                                                        AjmInstanceGapless& gapless) {
    return std::tuple<u32, u32, bool>();
}

AVFrame* AjmM4aacDecoder::ConvertAudioFrame(AVFrame* frame) {
    return nullptr;
}
} // namespace Libraries::Ajm