// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include "common/logging/log.h"
#include "core/libraries/audiodec_cpu/audiodec_cpu.h"
#include "core/libraries/audiodec_cpu/audiodec_cpu_backend.h"
#include "core/libraries/audiodec_cpu/audiodec_cpu_error.h"
#include "core/libraries/libs.h"

namespace Libraries::AudiodecCpu {
namespace {

struct CodecInfo {
    OrbisAudiodecCpuCodec codec;
    const char* name;
    const char* module;
    const char* ops_nid;
    const CodecOps* hle_ops{};
};

constexpr std::array CodecInfos{
    CodecInfo{OrbisAudiodecCpuCodec::M4Aac, "M4AAC", "libSceAudiodecCpuM4aac", "ijtKzWkV5h0"},
    CodecInfo{OrbisAudiodecCpuCodec::Dts, "DTS", "libSceAudiodecCpuDts", "k0Gp2cXDeqU"},
    CodecInfo{OrbisAudiodecCpuCodec::LpcmBd2, "LPCM BD2", "libSceAudiodecCpuLpcm", "31BlqiSZ3fw"},
    CodecInfo{OrbisAudiodecCpuCodec::LpcmDvd2, "LPCM DVD2", "libSceAudiodecCpuLpcm", "Psa+mnQ+wRg"},
    CodecInfo{OrbisAudiodecCpuCodec::DtsHdMa, "DTS HD MA", "libSceAudiodecCpuDtsHdMa",
              "Hnrc4jUGrL4"},
    CodecInfo{OrbisAudiodecCpuCodec::DtsHdLbr, "DTS HD LBR", "libSceAudiodecCpuDtsHdLbr",
              "nR0nX7p2jFo"},
    CodecInfo{OrbisAudiodecCpuCodec::Ddp, "DDP", "libSceAudiodecCpuDdp", "xqlFMfkztKg"},
    CodecInfo{OrbisAudiodecCpuCodec::Hevag, "HEVAG", "libSceAudiodecCpuHevag", "lYA31T9O1KU"},
    CodecInfo{OrbisAudiodecCpuCodec::Alac2, "ALAC2", "libSceAudiodecCpuAlac", "ZEH+UsL9PFQ"},
    CodecInfo{OrbisAudiodecCpuCodec::Flac2, "FLAC2", "libSceAudiodecCpuFlac", "WkjdgojiCdQ"},
};

const char* GetCodecName(const s32 codec_id) {
    for (const auto& codec : CodecInfos) {
        if (static_cast<s32>(codec.codec) == codec_id) {
            return codec.name;
        }
    }
    return "Unknown";
}

enum class BackendLogState {
    Unreported,
    Unavailable,
    Incomplete,
    Hle,
    Lle,
};
std::array<std::atomic<BackendLogState>, CodecInfos.size()> g_backend_log_states{};
std::array<std::atomic<const CodecOps*>, CodecInfos.size()> g_codec_ops{};

void ReportBackend(const size_t index, const BackendLogState state) {
    // Report changes in availability once, so we don't need to log every decode attempt.
    if (g_backend_log_states[index].exchange(state, std::memory_order_relaxed) == state) {
        return;
    }
    const auto& codec = CodecInfos[index];
    const auto id = static_cast<u32>(codec.codec);
    switch (state) {
    case BackendLogState::Unavailable:
        LOG_WARNING(Lib_AudiodecCpu, "{} ({:#x}): decoder export {} from {} is unavailable",
                    codec.name, id, codec.ops_nid, codec.module);
        break;
    case BackendLogState::Incomplete:
        LOG_ERROR(Lib_AudiodecCpu, "{} ({:#x}): decoder operation table is incomplete", codec.name,
                  id);
        break;
    case BackendLogState::Hle:
        LOG_INFO(Lib_AudiodecCpu, "{} ({:#x}): selected HLE decoder", codec.name, id);
        break;
    case BackendLogState::Lle:
        LOG_INFO(Lib_AudiodecCpu, "{} ({:#x}): selected LLE decoder from {}", codec.name, id,
                 codec.module);
        break;
    case BackendLogState::Unreported:
        break;
    }
}

const CodecOps* GetCodecOps(const s32 codec_id) {
    for (size_t i = 0; i < CodecInfos.size(); ++i) {
        const auto& codec = CodecInfos[i];
        if (static_cast<s32>(codec.codec) != codec_id) {
            continue;
        }
        // Keep one backend for the lifetime of the process, so a later module load cannot
        // change the work memory layout after QueryMemSize or InitDecoder has used it.
        const auto* ops = g_codec_ops[i].load(std::memory_order_acquire);
        if (ops) {
            return ops;
        }
        ops = ResolveLleCodecOps(codec.module, codec.ops_nid);
        if (!ops) {
            ops = codec.hle_ops;
        }
        if (!ops) {
            ReportBackend(i, BackendLogState::Unavailable);
            return nullptr;
        }
        if (!ops->query_mem_size || !ops->init_decoder || !ops->decode || !ops->clear_context) {
            ReportBackend(i, BackendLogState::Incomplete);
            return nullptr;
        }
        // Loaded modules remain mapped for the process lifetime.
        const CodecOps* expected = nullptr;
        if (!g_codec_ops[i].compare_exchange_strong(expected, ops, std::memory_order_acq_rel,
                                                    std::memory_order_acquire)) {
            ops = expected;
        }
        ReportBackend(i, ops == codec.hle_ops ? BackendLogState::Hle : BackendLogState::Lle);
        return ops;
    }
    LOG_DEBUG(Lib_AudiodecCpu, "Unknown codec ID {:#x}", static_cast<u32>(codec_id));
    return nullptr;
}

int ValidationError(const s32 codec_id, const char* reason, const s32 error) {
    LOG_DEBUG(Lib_AudiodecCpu, "Codec {} ({:#x}): {}, returning {:#x}", GetCodecName(codec_id),
              static_cast<u32>(codec_id), reason, static_cast<u32>(error));
    return error;
}

int DispatchQueryMemSize(const OrbisAudiodecCpuQueryCtrl* ctrl,
                         OrbisAudiodecCpuMemoryDescriptor* resource, const s32 codec_id) {
    if (!ctrl) {
        return ValidationError(codec_id, "null control pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_CTRL_POINTER);
    }

    if (!ctrl->param) {
        return ValidationError(codec_id, "null parameter pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_PARAM_POINTER);
    }

    if (!resource) {
        return ValidationError(codec_id, "null resource pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_RESOURCE_POINTER);
    }

    if (resource->this_size != ORBIS_AUDIODECCPU_DESCRIPTOR_SIZE) {
        return ValidationError(codec_id, "invalid resource size",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_RESOURCE_SIZE);
    }

    const CodecOps* ops = GetCodecOps(codec_id);
    if (!ops) {
        return ORBIS_AUDIODECCPU_ERROR_INVALID_TYPE;
    }

    // Query reports required storage without requiring an allocated work memory buffer.
    const int result = ops->query_mem_size(ctrl, resource);
    LOG_DEBUG(Lib_AudiodecCpu, "codec={}, work_mem_size={}, result={:#x}", GetCodecName(codec_id),
              resource->data_size, static_cast<u32>(result));
    return result;
}

int DispatchInitDecoder(const OrbisAudiodecCpuInitCtrl* ctrl,
                        OrbisAudiodecCpuMemoryDescriptor* resource, const s32 codec_id) {
    if (!ctrl) {
        return ValidationError(codec_id, "null control pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_CTRL_POINTER);
    }

    if (!ctrl->param) {
        return ValidationError(codec_id, "null parameter pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_PARAM_POINTER);
    }

    if (!ctrl->bsi_info) {
        return ValidationError(codec_id, "null BSI info pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_BSI_INFO_POINTER);
    }

    if (!resource) {
        return ValidationError(codec_id, "null resource pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_RESOURCE_POINTER);
    }

    if (!resource->data) {
        return ValidationError(codec_id, "null work memory pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_WORK_MEM_POINTER);
    }

    if (resource->this_size != ORBIS_AUDIODECCPU_DESCRIPTOR_SIZE) {
        return ValidationError(codec_id, "invalid resource size",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_RESOURCE_SIZE);
    }

    const CodecOps* ops = GetCodecOps(codec_id);
    if (!ops) {
        return ORBIS_AUDIODECCPU_ERROR_INVALID_TYPE;
    }

    const int result = ops->init_decoder(ctrl, resource);
    LOG_DEBUG(Lib_AudiodecCpu, "codec={}, work_mem_size={}, result={:#x}", GetCodecName(codec_id),
              resource->data_size, static_cast<u32>(result));
    return result;
}

int DispatchDecode(const OrbisAudiodecCpuDecodeCtrl* ctrl,
                   OrbisAudiodecCpuMemoryDescriptor* resource, const s32 codec_id) {
    if (!ctrl) {
        return ValidationError(codec_id, "null control pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_CTRL_POINTER);
    }

    if (!ctrl->param) {
        return ValidationError(codec_id, "null parameter pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_PARAM_POINTER);
    }

    if (!ctrl->bsi_info) {
        return ValidationError(codec_id, "null BSI info pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_BSI_INFO_POINTER);
    }

    if (!ctrl->au_info) {
        return ValidationError(codec_id, "null AU info pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_AU_INFO_POINTER);
    }

    if (!ctrl->au_info->data) {
        return ValidationError(codec_id, "null AU pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_AU_POINTER);
    }

    if (!ctrl->pcm_item) {
        return ValidationError(codec_id, "null PCM item pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_PCM_ITEM_POINTER);
    }

    if (!ctrl->pcm_item->data) {
        return ValidationError(codec_id, "null PCM pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_PCM_POINTER);
    }

    if (ctrl->au_info->data_size == 0) {
        return ValidationError(codec_id, "invalid AU size",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_AU_SIZE);
    }

    if (ctrl->pcm_item->data_size == 0) {
        return ValidationError(codec_id, "invalid PCM size",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_PCM_SIZE);
    }

    if (ctrl->au_info->this_size != ORBIS_AUDIODECCPU_DESCRIPTOR_SIZE) {
        return ValidationError(codec_id, "invalid AU info size",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_AU_INFO_SIZE);
    }

    if (ctrl->pcm_item->this_size != ORBIS_AUDIODECCPU_DESCRIPTOR_SIZE) {
        return ValidationError(codec_id, "invalid PCM item size",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_PCM_ITEM_SIZE);
    }

    if (!resource) {
        return ValidationError(codec_id, "null resource pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_RESOURCE_POINTER);
    }

    if (!resource->data) {
        return ValidationError(codec_id, "null work memory pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_WORK_MEM_POINTER);
    }

    if (resource->this_size != ORBIS_AUDIODECCPU_DESCRIPTOR_SIZE) {
        return ValidationError(codec_id, "invalid resource size",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_RESOURCE_SIZE);
    }

    const CodecOps* ops = GetCodecOps(codec_id);
    if (!ops) {
        return ORBIS_AUDIODECCPU_ERROR_INVALID_TYPE;
    }

    LOG_TRACE(Lib_AudiodecCpu, "codec={}, input_capacity={}, output_capacity={}",
              GetCodecName(codec_id), ctrl->au_info->data_size, ctrl->pcm_item->data_size);
    const int result = ops->decode(ctrl, resource);
    LOG_TRACE(Lib_AudiodecCpu, "codec={}, consumed={}, produced={}, result={:#x}",
              GetCodecName(codec_id), ctrl->au_info->data_size, ctrl->pcm_item->data_size,
              static_cast<u32>(result));
    return result;
}

int DispatchClearContext(OrbisAudiodecCpuMemoryDescriptor* resource, const s32 codec_id) {
    // ClearContext uses ARG for a null resource, unlike the other entry points.
    if (!resource) {
        return ValidationError(codec_id, "null resource pointer", ORBIS_AUDIODECCPU_ERROR_ARG);
    }
    if (!resource->data) {
        return ValidationError(codec_id, "null work memory pointer",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_WORK_MEM_POINTER);
    }
    if (resource->this_size != ORBIS_AUDIODECCPU_DESCRIPTOR_SIZE) {
        return ValidationError(codec_id, "invalid resource size",
                               ORBIS_AUDIODECCPU_ERROR_INVALID_RESOURCE_SIZE);
    }
    const CodecOps* ops = GetCodecOps(codec_id);
    if (!ops) {
        return ORBIS_AUDIODECCPU_ERROR_INVALID_TYPE;
    }
    const int result = ops->clear_context(resource);
    LOG_DEBUG(Lib_AudiodecCpu, "codec={}, result={:#x}", GetCodecName(codec_id),
              static_cast<u32>(result));
    return result;
}

} // namespace

int PS4_SYSV_ABI sceAudiodecCpuQueryMemSize(const OrbisAudiodecCpuQueryCtrl* ctrl,
                                            OrbisAudiodecCpuMemoryDescriptor* resource,
                                            const s32 codec_id) {
    return DispatchQueryMemSize(ctrl, resource, codec_id);
}

int PS4_SYSV_ABI sceAudiodecCpuInitDecoder(const OrbisAudiodecCpuInitCtrl* ctrl,
                                           OrbisAudiodecCpuMemoryDescriptor* resource,
                                           const s32 codec_id) {
    return DispatchInitDecoder(ctrl, resource, codec_id);
}

int PS4_SYSV_ABI sceAudiodecCpuDecode(const OrbisAudiodecCpuDecodeCtrl* ctrl,
                                      OrbisAudiodecCpuMemoryDescriptor* resource,
                                      const s32 codec_id) {
    return DispatchDecode(ctrl, resource, codec_id);
}

int PS4_SYSV_ABI sceAudiodecCpuClearContext(OrbisAudiodecCpuMemoryDescriptor* resource,
                                            const s32 codec_id) {
    return DispatchClearContext(resource, codec_id);
}

int PS4_SYSV_ABI sceAudiodecCpuInternalQueryMemSize(const OrbisAudiodecCpuQueryCtrl* ctrl,
                                                    OrbisAudiodecCpuMemoryDescriptor* resource,
                                                    const s32 codec_id) {
    return DispatchQueryMemSize(ctrl, resource, codec_id);
}

int PS4_SYSV_ABI sceAudiodecCpuInternalInitDecoder(const OrbisAudiodecCpuInitCtrl* ctrl,
                                                   OrbisAudiodecCpuMemoryDescriptor* resource,
                                                   const s32 codec_id) {
    return DispatchInitDecoder(ctrl, resource, codec_id);
}

int PS4_SYSV_ABI sceAudiodecCpuInternalDecode(const OrbisAudiodecCpuDecodeCtrl* ctrl,
                                              OrbisAudiodecCpuMemoryDescriptor* resource,
                                              const s32 codec_id) {
    return DispatchDecode(ctrl, resource, codec_id);
}

int PS4_SYSV_ABI sceAudiodecCpuInternalClearContext(OrbisAudiodecCpuMemoryDescriptor* resource,
                                                    const s32 codec_id) {
    return DispatchClearContext(resource, codec_id);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ktD2w3D4G2U", "libSceAudiodecCpu", 1, "libSceAudiodecCpu",
                 sceAudiodecCpuQueryMemSize);
    LIB_FUNCTION("hdFsxo3MFu8", "libSceAudiodecCpu", 1, "libSceAudiodecCpu",
                 sceAudiodecCpuInitDecoder);
    LIB_FUNCTION("lSVTiWV5wLc", "libSceAudiodecCpu", 1, "libSceAudiodecCpu", sceAudiodecCpuDecode);
    LIB_FUNCTION("hAS5WH6hxrE", "libSceAudiodecCpu", 1, "libSceAudiodecCpu",
                 sceAudiodecCpuClearContext);
    LIB_FUNCTION("R8v5kdZ55mY", "libSceAudiodecCpu", 1, "libSceAudiodecCpu",
                 sceAudiodecCpuInternalQueryMemSize);
    LIB_FUNCTION("KkhdeVCyo6Y", "libSceAudiodecCpu", 1, "libSceAudiodecCpu",
                 sceAudiodecCpuInternalInitDecoder);
    LIB_FUNCTION("-0jDlM2hG5k", "libSceAudiodecCpu", 1, "libSceAudiodecCpu",
                 sceAudiodecCpuInternalDecode);
    LIB_FUNCTION("CnY1NGmdi7I", "libSceAudiodecCpu", 1, "libSceAudiodecCpu",
                 sceAudiodecCpuInternalClearContext);
}
} // namespace Libraries::AudiodecCpu
