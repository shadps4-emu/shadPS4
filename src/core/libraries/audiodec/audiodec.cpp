// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/ajm/ajm_aac.h"
#include "core/libraries/ajm/ajm_error.h"
#include "core/libraries/ajm/ajm_instance.h"
#include "core/libraries/ajm/ajm_mp3.h"
#include "core/libraries/ajm/ajm_result.h"
#include "core/libraries/audiodec/audiodec.h"
#include "core/libraries/audiodec/audiodec_error.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Audiodec {

namespace {

constexpr u32 MaxCodecType = 10;
constexpr u32 BsiInfoSize = 0x14;
constexpr u32 BsiInfoSizeCheckSdkVersion = 0x1700000;
constexpr s32 DefaultPriority = 0x28;
constexpr s32 MinPriority = 0x14;
constexpr s32 MaxPriority = 0x64;

constexpr u32 Mp3MaxAuSize = 0x5A1;
constexpr u32 Mp3MaxPcmSize = 0x2400;
constexpr u32 AacMaxAuSize = 0x1867;
constexpr u32 AacMaxPcmSize = 0x8000;

constexpr u32 Mp3ParamVersion = 1;
constexpr u32 AacParamSizeBase = 0x18;
constexpr u32 AacParamSizeNondelay = 0x1C;
constexpr u32 AacParamSizeSurround = 0x20;
constexpr u32 AacMaxSamplingFreqType = 0xB;

constexpr u64 Mp3InstanceFlags = 0x200000011;
constexpr u64 Mp3DecodeJobFlags = 0x800000000801;
constexpr u64 AacDecodeJobFlags = 0xC00000000801;
constexpr u64 ResetJobFlags = 0x2001;
constexpr u64 InitializeJobFlags = 0x4001;

struct Mp3Sideband {
    Ajm::AjmSidebandResult result;
    Ajm::AjmSidebandStream stream;
    Ajm::AjmSidebandDecMp3CodecInfo codec_info;
};
static_assert(sizeof(Mp3Sideband) == 0x28);

struct AacSideband {
    Ajm::AjmSidebandResult result;
    Ajm::AjmSidebandStream stream;
    Ajm::AjmSidebandFormat format;
    Ajm::AjmSidebandDecM4aacCodecInfo codec_info;
};
static_assert(sizeof(AacSideband) == 0x38);

struct AacInitParams {
    u32 config_type;
    u32 sampling_freq_type;
};
static_assert(sizeof(AacInitParams) == sizeof(Ajm::AjmSidebandInitParameters));

struct Decoder {
    Decoder(Ajm::AjmCodecType codec_type, Ajm::AjmInstanceFlags flags)
        : type(codec_type), instance(codec_type, flags) {}

    Ajm::AjmCodecType type;
    Ajm::AjmInstance instance;
    std::mutex mutex;
};

struct State {
    std::mutex mutex;
    std::array<bool, MaxCodecType + 1> registered{};
    std::unordered_map<s32, std::shared_ptr<Decoder>> decoders;
    s32 next_handle = 1;
};

State g_state;

bool IsSupportedCodecType(u32 codec_type) {
    return codec_type == u32(OrbisAudiodecCodecType::Mp3) ||
           codec_type == u32(OrbisAudiodecCodecType::Aac);
}

bool IsBsiInfoSizeChecked() {
    return Common::ElfInfo::Instance().CompiledSdkVer() >= BsiInfoSizeCheckSdkVersion;
}

void SetBsiResult(void* bsi_info, s32 result) {
    *reinterpret_cast<s32*>(static_cast<u8*>(bsi_info) +
                            offsetof(OrbisAudiodecMp3BsiInfo, result)) = result;
}

std::shared_ptr<Decoder> FindDecoder(s32 handle) {
    std::scoped_lock lock{g_state.mutex};
    const auto it = g_state.decoders.find(handle);
    return it == g_state.decoders.end() ? nullptr : it->second;
}

bool IsCodecRegistered(OrbisAudiodecCodecType codec_type) {
    std::scoped_lock lock{g_state.mutex};
    return g_state.registered[u32(codec_type)];
}

s32 AddDecoder(const std::shared_ptr<Decoder>& decoder) {
    std::scoped_lock lock{g_state.mutex};
    const s32 handle = g_state.next_handle++;
    g_state.decoders.emplace(handle, decoder);
    return handle;
}

s32 ValidateCreateCtrl(const OrbisAudiodecCtrl* ctrl) {
    if (!ctrl) {
        return ORBIS_AUDIODEC_ERROR_INVALID_CTRL_ADDRESS;
    }
    if (!ctrl->param) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PARAM_ADDRESS;
    }
    if (!ctrl->bsi_info) {
        return ORBIS_AUDIODEC_ERROR_INVALID_BSI_INFO_ADDRESS;
    }
    return ORBIS_OK;
}

s32 ValidateDecodeCtrl(const OrbisAudiodecCtrl* ctrl, s32 priority) {
    if (!ctrl) {
        return ORBIS_AUDIODEC_ERROR_INVALID_CTRL_ADDRESS;
    }
    if (!ctrl->param) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PARAM_ADDRESS;
    }
    if (!ctrl->bsi_info) {
        return ORBIS_AUDIODEC_ERROR_INVALID_BSI_INFO_ADDRESS;
    }
    const auto* au = ctrl->au_info;
    if (!au) {
        return ORBIS_AUDIODEC_ERROR_INVALID_AU_INFO_ADDRESS;
    }
    if (!au->buf) {
        return ORBIS_AUDIODEC_ERROR_INVALID_AU_BUF_ADDRESS;
    }
    const auto* pcm = ctrl->pcm_item;
    if (!pcm) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PCM_ITEM_ADDRESS;
    }
    if (!pcm->buf) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PCM_BUF_ADDRESS;
    }
    if (au->buf_size == 0 || au->buf_size == 0xFFFFFFFFu) {
        return ORBIS_AUDIODEC_ERROR_INVALID_AU_BUF_SIZE;
    }
    if (pcm->buf_size == 0 || pcm->buf_size == 0xFFFFFFFFu) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PCM_BUF_SIZE;
    }
    if (au->size != sizeof(OrbisAudiodecAuInfo)) {
        return ORBIS_AUDIODEC_ERROR_INVALID_AU_INFO_SIZE;
    }
    if (pcm->size != sizeof(OrbisAudiodecPcmItem)) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PCM_ITEM_SIZE;
    }
    if (priority < MinPriority || priority > MaxPriority) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PRIORITY;
    }
    return ORBIS_OK;
}

s32 CreateMp3Decoder(OrbisAudiodecCtrl* ctrl) {
    const auto* param = static_cast<const OrbisAudiodecMp3Param*>(ctrl->param);
    const auto* bsi = static_cast<const OrbisAudiodecMp3BsiInfo*>(ctrl->bsi_info);
    if (param->size != sizeof(OrbisAudiodecMp3Param)) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PARAM_SIZE;
    }
    if (IsBsiInfoSizeChecked() && bsi->size != BsiInfoSize) {
        return ORBIS_AUDIODEC_ERROR_INVALID_BSI_INFO_SIZE;
    }
    if (param->version != Mp3ParamVersion) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PARAM;
    }
    if (!IsCodecRegistered(OrbisAudiodecCodecType::Mp3)) {
        SetBsiResult(ctrl->bsi_info, ORBIS_AJM_ERROR_CODEC_NOT_REGISTERED);
        return ORBIS_AUDIODEC_ERROR_FAILED;
    }

    Ajm::AjmInstanceFlags flags{};
    flags.raw = Mp3InstanceFlags;
    const auto decoder = std::make_shared<Decoder>(Ajm::AjmCodecType::Mp3Dec, flags);
    return AddDecoder(decoder);
}

s32 CreateAacDecoder(OrbisAudiodecCtrl* ctrl) {
    const auto* param = static_cast<const OrbisAudiodecAacParam*>(ctrl->param);
    const auto* bsi = static_cast<const OrbisAudiodecAacBsiInfo*>(ctrl->bsi_info);
    if (IsBsiInfoSizeChecked() && bsi->size != BsiInfoSize) {
        return ORBIS_AUDIODEC_ERROR_INVALID_BSI_INFO_SIZE;
    }
    if (param->size != AacParamSizeBase && param->size != AacParamSizeNondelay &&
        param->size != AacParamSizeSurround) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PARAM_SIZE;
    }

    u32 codec_flags = 0;
    if (param->size >= AacParamSizeNondelay) {
        if (param->enable_nondelay_output == 1) {
            codec_flags |= Ajm::AjmAacCodecFlags::EnableNondelayOutput;
        } else if (param->enable_nondelay_output != 0) {
            return ORBIS_AUDIODEC_ERROR_AAC_INVALID_NONDELAY_OUTPUT;
        }
    }
    if (param->size >= AacParamSizeSurround) {
        switch (param->surround_channel_order) {
        case 0:
            break;
        case 1:
            codec_flags |= Ajm::AjmAacCodecFlags::SurroundChannelInterleaveOrderExtlExtrLsRs;
            break;
        case 2:
            codec_flags |= Ajm::AjmAacCodecFlags::SurroundChannelInterleaveOrderLsRsExtlExtr;
            break;
        default:
            return ORBIS_AUDIODEC_ERROR_AAC_INVALID_SURROUND_ORDER;
        }
    }

    const u32 num_channels = param->num_channels;
    if (num_channels != 1 && num_channels != 2 && num_channels != 6 && num_channels != 8) {
        return ORBIS_AUDIODEC_ERROR_AAC_INVALID_CHANNELS;
    }

    Ajm::AjmInstanceFlags flags{};
    flags.version = 1;
    flags.channels = num_channels;
    switch (param->sample_encoding) {
    case 1:
        flags.format = u64(Ajm::AjmFormatEncoding::S16);
        break;
    case 2:
        flags.format = u64(Ajm::AjmFormatEncoding::Float);
        break;
    default:
        return ORBIS_AUDIODEC_ERROR_INVALID_PARAM;
    }

    if (param->enable_sbr == 1 && num_channels <= 2) {
        codec_flags |= Ajm::AjmAacCodecFlags::EnableSbrDecode;
    } else if (param->enable_sbr != 0) {
        return ORBIS_AUDIODEC_ERROR_AAC_INVALID_ENABLE_SBR;
    }

    if (param->config_type != Ajm::ConfigType::ADTS && param->config_type != Ajm::ConfigType::RAW) {
        return ORBIS_AUDIODEC_ERROR_AAC_INVALID_CONFIG_TYPE;
    }
    if (param->config_type != Ajm::ConfigType::ADTS &&
        param->sampling_freq_type > AacMaxSamplingFreqType) {
        return ORBIS_AUDIODEC_ERROR_AAC_INVALID_SAMPLING_FREQ_TYPE;
    }
    flags.codec = codec_flags;

    if (!IsCodecRegistered(OrbisAudiodecCodecType::Aac)) {
        SetBsiResult(ctrl->bsi_info, ORBIS_AJM_ERROR_CODEC_NOT_REGISTERED);
        return ORBIS_AUDIODEC_ERROR_FAILED;
    }

    const auto decoder = std::make_shared<Decoder>(Ajm::AjmCodecType::M4aacDec, flags);

    Ajm::AjmSidebandResult init_result{};
    Ajm::AjmJob job{};
    job.flags.raw = InitializeJobFlags;
    job.output.p_result = &init_result;
    const AacInitParams init_params{param->config_type, param->sampling_freq_type};
    Ajm::AjmSidebandInitParameters& job_params = job.input.init_params.emplace();
    std::memcpy(&job_params, &init_params, sizeof(init_params));
    {
        std::scoped_lock lock{decoder->mutex};
        decoder->instance.ExecuteJob(job);
    }
    if (init_result.result != 0) {
        SetBsiResult(ctrl->bsi_info, init_result.internal_result);
        return ORBIS_AUDIODEC_ERROR_FAILED;
    }
    return AddDecoder(decoder);
}

s32 DecodeMp3(s32 handle, Decoder& decoder, OrbisAudiodecCtrl* ctrl) {
    auto* bsi = static_cast<OrbisAudiodecMp3BsiInfo*>(ctrl->bsi_info);
    if (bsi->size != BsiInfoSize) {
        return ORBIS_AUDIODEC_ERROR_INVALID_BSI_INFO_SIZE;
    }
    auto* au = ctrl->au_info;
    auto* pcm = ctrl->pcm_item;

    Mp3Sideband sideband{};
    Ajm::AjmJob job{};
    job.instance_id = handle;
    job.flags.raw = Mp3DecodeJobFlags;
    job.input.buffer.assign(au->buf, au->buf + std::min(au->buf_size, Mp3MaxAuSize));
    job.output.buffers.emplace_back(pcm->buf, std::min(pcm->buf_size, Mp3MaxPcmSize));
    job.output.p_result = &sideband.result;
    job.output.p_stream = &sideband.stream;
    job.output.p_codec_info = reinterpret_cast<u8*>(&sideband.codec_info);
    {
        std::scoped_lock lock{decoder.mutex};
        decoder.instance.ExecuteJob(job);
    }

    au->buf_size = sideband.stream.input_consumed;
    pcm->buf_size = sideband.stream.output_written;
    bsi->header = sideband.codec_info.header;
    bsi->has_crc = sideband.codec_info.has_crc;
    bsi->channel_mode = u8(sideband.codec_info.channel_mode);
    bsi->mode_extension = sideband.codec_info.mode_extension;
    bsi->copyright = sideband.codec_info.copyright;
    bsi->original = sideband.codec_info.original;
    bsi->emphasis = sideband.codec_info.emphasis;
    bsi->reserved = 0;

    const s32 result = sideband.result.result;
    if (result < 0) {
        bsi->result = sideband.result.internal_result;
        return ORBIS_AUDIODEC_ERROR_FAILED;
    }
    bsi->result = result;
    if (result & ORBIS_AJM_RESULT_PARTIAL_INPUT) {
        return ORBIS_AUDIODEC_ERROR_INVALID_AU_BUF_SIZE;
    }
    if (result & ORBIS_AJM_RESULT_NOT_ENOUGH_ROOM) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PCM_BUF_SIZE;
    }
    return ORBIS_OK;
}

s32 DecodeAac(s32 handle, Decoder& decoder, OrbisAudiodecCtrl* ctrl) {
    auto* bsi = static_cast<OrbisAudiodecAacBsiInfo*>(ctrl->bsi_info);
    if (bsi->size != BsiInfoSize) {
        return ORBIS_AUDIODEC_ERROR_INVALID_BSI_INFO_SIZE;
    }
    auto* au = ctrl->au_info;
    auto* pcm = ctrl->pcm_item;

    AacSideband sideband{};
    Ajm::AjmJob job{};
    job.instance_id = handle;
    job.flags.raw = AacDecodeJobFlags;
    job.input.buffer.assign(au->buf, au->buf + std::min(au->buf_size, AacMaxAuSize));
    job.output.buffers.emplace_back(pcm->buf, std::min(pcm->buf_size, AacMaxPcmSize));
    job.output.p_result = &sideband.result;
    job.output.p_stream = &sideband.stream;
    job.output.p_format = &sideband.format;
    job.output.p_codec_info = reinterpret_cast<u8*>(&sideband.codec_info);
    {
        std::scoped_lock lock{decoder.mutex};
        decoder.instance.ExecuteJob(job);
    }

    au->buf_size = sideband.stream.input_consumed;
    pcm->buf_size = sideband.stream.output_written;
    bsi->sample_rate = sideband.format.sampl_freq;
    bsi->num_channels = sideband.format.num_channels;
    bsi->heaac = sideband.codec_info.heaac;

    const s32 result = sideband.result.result;
    if (result == 0) {
        bsi->result = 0;
        return ORBIS_OK;
    }
    if (sideband.result.internal_result != 0) {
        bsi->result = sideband.result.internal_result;
        return ORBIS_AUDIODEC_ERROR_FAILED;
    }
    bsi->result = result;
    if (result & ORBIS_AJM_RESULT_NOT_ENOUGH_ROOM) {
        return ORBIS_AUDIODEC_ERROR_INVALID_PCM_BUF_SIZE;
    }
    return ORBIS_AUDIODEC_ERROR_FAILED;
}

s32 DecodeInternal(s32 handle, OrbisAudiodecCtrl* ctrl, s32 priority) {
    if (const s32 ret = ValidateDecodeCtrl(ctrl, priority); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Audiodec, "invalid arguments, error = {:#x}", u32(ret));
        return ret;
    }
    const auto decoder = FindDecoder(handle);
    if (!decoder) {
        LOG_ERROR(Lib_Audiodec, "invalid handle {}", handle);
        return ORBIS_AUDIODEC_ERROR_INVALID_HANDLE;
    }
    switch (decoder->type) {
    case Ajm::AjmCodecType::Mp3Dec:
        return DecodeMp3(handle, *decoder, ctrl);
    case Ajm::AjmCodecType::M4aacDec:
        return DecodeAac(handle, *decoder, ctrl);
    default:
        return ORBIS_AUDIODEC_ERROR_INVALID_HANDLE;
    }
}

} // namespace

s32 PS4_SYSV_ABI sceAudiodecInitLibrary(u32 codec_type) {
    LOG_INFO(Lib_Audiodec, "codec_type = {}", codec_type);
    if (codec_type == 0 || codec_type > MaxCodecType) {
        return ORBIS_AUDIODEC_ERROR_INVALID_CODEC_TYPE;
    }
    if (!IsSupportedCodecType(codec_type)) {
        LOG_ERROR(Lib_Audiodec, "unsupported codec type {}", codec_type);
        return ORBIS_AUDIODEC_ERROR_FAILED;
    }
    std::scoped_lock lock{g_state.mutex};
    if (g_state.registered[codec_type]) {
        return ORBIS_AJM_ERROR_CODEC_ALREADY_REGISTERED;
    }
    g_state.registered[codec_type] = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudiodecTermLibrary(u32 codec_type) {
    LOG_INFO(Lib_Audiodec, "codec_type = {}", codec_type);
    if (codec_type == 0 || codec_type > MaxCodecType) {
        return ORBIS_AUDIODEC_ERROR_INVALID_CODEC_TYPE;
    }
    if (!IsSupportedCodecType(codec_type)) {
        return ORBIS_AUDIODEC_ERROR_FAILED;
    }
    std::scoped_lock lock{g_state.mutex};
    g_state.registered[codec_type] = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudiodecCreateDecoder(OrbisAudiodecCtrl* ctrl, u32 codec_type) {
    LOG_INFO(Lib_Audiodec, "codec_type = {}", codec_type);
    if (const s32 ret = ValidateCreateCtrl(ctrl); ret != ORBIS_OK) {
        LOG_ERROR(Lib_Audiodec, "invalid arguments, error = {:#x}", u32(ret));
        return ret;
    }
    if (codec_type == 0 || codec_type > MaxCodecType) {
        return ORBIS_AUDIODEC_ERROR_INVALID_CODEC_TYPE;
    }
    s32 ret = ORBIS_AUDIODEC_ERROR_FAILED;
    switch (OrbisAudiodecCodecType(codec_type)) {
    case OrbisAudiodecCodecType::Mp3:
        ret = CreateMp3Decoder(ctrl);
        break;
    case OrbisAudiodecCodecType::Aac:
        ret = CreateAacDecoder(ctrl);
        break;
    default:
        LOG_ERROR(Lib_Audiodec, "unsupported codec type {}", codec_type);
        break;
    }
    if (ret < 0) {
        LOG_ERROR(Lib_Audiodec, "failed to create decoder, error = {:#x}", u32(ret));
    }
    return ret;
}

s32 PS4_SYSV_ABI sceAudiodecDeleteDecoder(s32 handle) {
    LOG_INFO(Lib_Audiodec, "handle = {}", handle);
    std::scoped_lock lock{g_state.mutex};
    if (g_state.decoders.erase(handle) == 0) {
        return ORBIS_AUDIODEC_ERROR_INVALID_HANDLE;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAudiodecClearContext(s32 handle) {
    LOG_DEBUG(Lib_Audiodec, "handle = {}", handle);
    const auto decoder = FindDecoder(handle);
    if (!decoder) {
        return ORBIS_AUDIODEC_ERROR_INVALID_HANDLE;
    }
    Ajm::AjmSidebandResult result{};
    Ajm::AjmJob job{};
    job.instance_id = handle;
    job.flags.raw = ResetJobFlags;
    job.output.p_result = &result;
    {
        std::scoped_lock lock{decoder->mutex};
        decoder->instance.ExecuteJob(job);
    }
    return result.result >= 0 ? result.result : ORBIS_AUDIODEC_ERROR_FAILED;
}

s32 PS4_SYSV_ABI sceAudiodecDecode(s32 handle, OrbisAudiodecCtrl* ctrl) {
    return DecodeInternal(handle, ctrl, DefaultPriority);
}

s32 PS4_SYSV_ABI sceAudiodecDecode2(s32 handle, OrbisAudiodecCtrl* ctrl) {
    return DecodeInternal(handle, ctrl, DefaultPriority);
}

s32 PS4_SYSV_ABI sceAudiodecDecodeWithPriority(s32 handle, OrbisAudiodecCtrl* ctrl, s32 priority) {
    return DecodeInternal(handle, ctrl, priority);
}

s32 PS4_SYSV_ABI sceAudiodecDecode2WithPriority(s32 handle, OrbisAudiodecCtrl* ctrl, s32 priority) {
    return DecodeInternal(handle, ctrl, priority);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("VjhsmxpcezI", "libSceAudiodec", 1, "libSceAudiodec", sceAudiodecInitLibrary);
    LIB_FUNCTION("h5jSB2QIDV0", "libSceAudiodec", 1, "libSceAudiodec", sceAudiodecTermLibrary);
    LIB_FUNCTION("O3f1sLMWRvs", "libSceAudiodec", 1, "libSceAudiodec", sceAudiodecCreateDecoder);
    LIB_FUNCTION("Tp+ZEy69mLk", "libSceAudiodec", 1, "libSceAudiodec", sceAudiodecDeleteDecoder);
    LIB_FUNCTION("6Vf9WTLDoss", "libSceAudiodec", 1, "libSceAudiodec", sceAudiodecClearContext);
    LIB_FUNCTION("KHXHMDLkILw", "libSceAudiodec", 1, "libSceAudiodec", sceAudiodecDecode);
    LIB_FUNCTION("+eFOGH-2A8M", "libSceAudiodec", 1, "libSceAudiodec", sceAudiodecDecode2);
    LIB_FUNCTION("VUMO2kAbo7w", "libSceAudiodec", 1, "libSceAudiodec",
                 sceAudiodecDecodeWithPriority);
    LIB_FUNCTION("S4per83VASY", "libSceAudiodec", 1, "libSceAudiodec",
                 sceAudiodecDecode2WithPriority);
}

} // namespace Libraries::Audiodec
