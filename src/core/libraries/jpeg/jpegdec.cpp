// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <magic_enum/magic_enum.hpp>
#include <stb_image.h>

#include "common/alignment.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/jpeg/jpeg_error.h"
#include "core/libraries/jpeg/jpegdec.h"
#include "core/libraries/libs.h"

namespace Libraries::JpegDec {

s32 PS4_SYSV_ABI sceJpegDecParseHeader(const OrbisJpegDecParseParam* param,
                                       OrbisJpegDecImageInfo* image_info) {
    LOG_TRACE(Lib_Jpeg, "called param = {}, image_info = {}", static_cast<const void*>(param),
              static_cast<const void*>(image_info));
    if (!param || !param->jpeg_addr) {
        LOG_ERROR(Lib_Jpeg, "Invalid address");
        return ORBIS_JPEG_DEC_ERROR_INVALID_ADDR;
    }
    if (param->jpeg_size == 0) {
        LOG_ERROR(Lib_Jpeg, "Invalid size");
        return ORBIS_JPEG_DEC_ERROR_INVALID_SIZE;
    }
    if (std::to_underlying(param->decode_mode) > 1) {
        LOG_ERROR(Lib_Jpeg, "Invalid decode mode: {}", std::to_underlying(param->decode_mode));
        return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
    }
    const u32 ratio = std::to_underlying(param->downscale_ratio);
    if (ratio != 1 && ratio != 2 && ratio != 4 && ratio != 8) {
        LOG_ERROR(Lib_Jpeg, "Invalid downscale ratio: {}", ratio);
        return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
    }
    if (!image_info) {
        LOG_ERROR(Lib_Jpeg, "Invalid image_info pointer");
        return ORBIS_JPEG_DEC_ERROR_INVALID_ADDR;
    }

    const auto* ptr = reinterpret_cast<const u8*>(param->jpeg_addr);
    const u32 size = param->jpeg_size;
    if (size < 4 || ptr[0] != 0xFF || ptr[1] != 0xD8) {
        LOG_ERROR(Lib_Jpeg, "Invalid JPEG signature or stream too short");
        return ORBIS_JPEG_DEC_ERROR_INVALID_DATA;
    }

    u32 width = 0;
    u32 height = 0;
    u16 num_components = 3;
    u32 pos = 2;
    while (pos + 4 < size) {
        if (ptr[pos] != 0xFF) {
            pos++;
            continue;
        }
        const u8 marker = ptr[pos + 1];
        if (marker == 0xD9 || marker == 0xDA) {
            break;
        }
        const u16 len = (static_cast<u16>(ptr[pos + 2]) << 8) | ptr[pos + 3];
        if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2) {
            if (pos + 9 < size) {
                height = (static_cast<u32>(ptr[pos + 5]) << 8) | ptr[pos + 6];
                width = (static_cast<u32>(ptr[pos + 7]) << 8) | ptr[pos + 8];
                num_components = ptr[pos + 9];
            }
            break;
        }
        pos += 2 + len;
    }

    if (width == 0 || height == 0) {
        LOG_ERROR(Lib_Jpeg, "Failed to parse SOF marker in JPEG stream");
        return ORBIS_JPEG_DEC_ERROR_DECODE_ERROR;
    }

    std::memset(image_info, 0, sizeof(OrbisJpegDecImageInfo));
    image_info->image_width = width;
    image_info->image_height = height;
    image_info->color_space =
        (num_components == 1) ? OrbisJpegDecColorSpace::Grayscale : OrbisJpegDecColorSpace::Ycc;
    image_info->num_components = num_components;
    image_info->component_id[0] = 1;
    image_info->component_id[1] = 2;
    image_info->component_id[2] = 3;
    image_info->sampling_factor[0] = 0x22;
    image_info->sampling_factor[1] = 0x11;
    image_info->sampling_factor[2] = 0x11;
    image_info->decode_mode = std::to_underlying(param->decode_mode);
    image_info->output_width = (width + ratio - 1) / ratio;
    image_info->output_height = (height + ratio - 1) / ratio;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegDecQueryMemorySize(const OrbisJpegDecCreateParam* param) {
    LOG_TRACE(Lib_Jpeg, "called param = {}", static_cast<const void*>(param));
    if (!param) {
        LOG_ERROR(Lib_Jpeg, "Invalid param pointer");
        return ORBIS_JPEG_DEC_ERROR_INVALID_ADDR;
    }
    if (param->size != sizeof(OrbisJpegDecCreateParam)) {
        LOG_ERROR(Lib_Jpeg, "Invalid param size: {}", param->size);
        return ORBIS_JPEG_DEC_ERROR_INVALID_SIZE;
    }
    const u32 mode = param->decode_mode;
    if (mode < 1 || mode > 2) {
        LOG_ERROR(Lib_Jpeg, "Invalid decode mode: {}", mode);
        return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
    }
    const u32 width = param->max_width;
    if (width == 0 || width > 0xFFFF) {
        LOG_ERROR(Lib_Jpeg, "Invalid max width: {}", width);
        return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
    }
    u32 aligned_w = (width + 0x1f) & ~0x1f;
    aligned_w |= 0x10;
    u32 sz = 0;
    if (mode == 2) {
        sz = (aligned_w << 5) * 5;
    } else {
        sz = (aligned_w << 4) * 3;
    }
    return static_cast<s32>(sz + aligned_w * 6 + 0x7360);
}

s32 PS4_SYSV_ABI sceJpegDecCreate(const OrbisJpegDecCreateParam* param, void* memory,
                                  u32 memory_size, OrbisJpegDecHandle* handle) {
    LOG_TRACE(Lib_Jpeg, "called param = {}, memory = {}, memory_size = {}",
              static_cast<const void*>(param), static_cast<const void*>(memory), memory_size);
    if (!param || !memory) {
        LOG_ERROR(Lib_Jpeg, "Invalid address param = {}, memory = {}",
                  static_cast<const void*>(param), static_cast<const void*>(memory));
        return ORBIS_JPEG_DEC_ERROR_INVALID_ADDR;
    }
    const s32 req_size = sceJpegDecQueryMemorySize(param);
    if (req_size < 0) {
        return req_size;
    }
    if (static_cast<u32>(req_size) > memory_size) {
        LOG_ERROR(Lib_Jpeg, "Memory size too small: {} < {}", memory_size, req_size);
        return ORBIS_JPEG_DEC_ERROR_INVALID_SIZE;
    }
    if (!handle) {
        LOG_ERROR(Lib_Jpeg, "Invalid handle pointer");
        return ORBIS_JPEG_DEC_ERROR_INVALID_ADDR;
    }
    const auto aligned_mem = Common::AlignUp(reinterpret_cast<uintptr_t>(memory), 0x20);
    auto* dec = reinterpret_cast<OrbisJpegDecHandleInternal*>(aligned_mem);
    dec->handle = dec;
    dec->handle_size = sizeof(OrbisJpegDecCreateParam);
    dec->decode_mode = param->decode_mode;
    dec->max_width = (param->max_width + 0x1f) & ~0x1f;
    dec->stride_or_aux = 0;
    *handle = dec;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegDecDelete(OrbisJpegDecHandle handle) {
    LOG_TRACE(Lib_Jpeg, "called handle = {}", static_cast<const void*>(handle));
    if (!handle || !Common::IsAligned(reinterpret_cast<uintptr_t>(handle), 0x20) ||
        handle->handle != handle) {
        LOG_ERROR(Lib_Jpeg, "Invalid handle: {}", static_cast<const void*>(handle));
        return ORBIS_JPEG_DEC_ERROR_INVALID_HANDLE;
    }
    handle->handle = nullptr;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegDecDecode(OrbisJpegDecHandle handle, const OrbisJpegDecDecodeParam* param,
                                  OrbisJpegDecImageInfo* image_info) {
    LOG_TRACE(Lib_Jpeg, "called handle = {}, param = {}, image_info = {}",
              static_cast<const void*>(handle), static_cast<const void*>(param),
              static_cast<const void*>(image_info));
    if (!handle || !Common::IsAligned(reinterpret_cast<uintptr_t>(handle), 0x20) ||
        handle->handle != handle) {
        LOG_ERROR(Lib_Jpeg, "Invalid handle: {}", static_cast<const void*>(handle));
        return ORBIS_JPEG_DEC_ERROR_INVALID_HANDLE;
    }
    if (!param || !param->jpeg_addr || !param->image_addr) {
        LOG_ERROR(Lib_Jpeg, "Invalid address");
        return ORBIS_JPEG_DEC_ERROR_INVALID_ADDR;
    }
    if (param->jpeg_size == 0 || param->image_size == 0) {
        LOG_ERROR(Lib_Jpeg, "Invalid buffer size (jpeg_size={}, image_size={})", param->jpeg_size,
                  param->image_size);
        return ORBIS_JPEG_DEC_ERROR_INVALID_SIZE;
    }
    if (std::to_underlying(param->decode_mode) > 1) {
        LOG_ERROR(Lib_Jpeg, "Invalid decode mode: {}", std::to_underlying(param->decode_mode));
        return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
    }
    const u32 ratio = std::to_underlying(param->downscale_ratio);
    if (ratio != 1 && ratio != 2 && ratio != 4 && ratio != 8) {
        LOG_ERROR(Lib_Jpeg, "Invalid downscale ratio: {}", ratio);
        return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
    }
    const auto format = param->pixel_format;
    const u16 fmt_val = std::to_underlying(format);
    if ((0xe03 & (1 << fmt_val)) == 0) {
        LOG_ERROR(Lib_Jpeg, "Invalid pixel format: {}", fmt_val);
        return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
    }
    if (param->alpha_value > 0xff) {
        LOG_ERROR(Lib_Jpeg, "Invalid alpha value: {}", param->alpha_value);
        return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
    }

    int w = 0;
    int h = 0;
    int ch = 0;
    stbi_uc* decoded = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(param->jpeg_addr),
                                             static_cast<int>(param->jpeg_size), &w, &h, &ch, 4);
    if (!decoded) {
        LOG_ERROR(Lib_Jpeg, "stbi_load_from_memory failed: {}", stbi_failure_reason());
        return ORBIS_JPEG_DEC_ERROR_DECODE_ERROR;
    }

    if (static_cast<u32>(w) > handle->max_width) {
        LOG_ERROR(Lib_Jpeg, "Image width exceeds handle max_width ({} > {})", w, handle->max_width);
        stbi_image_free(decoded);
        return ORBIS_JPEG_DEC_ERROR_EXCEED_MAX_WIDTH;
    }

    const u32 out_w = (static_cast<u32>(w) + ratio - 1) / ratio;
    const u32 out_h = (static_cast<u32>(h) + ratio - 1) / ratio;
    const u32 bpp = (format == OrbisJpegDecPixelFormat::Y8) ? 1 : 4;
    const u32 min_pitch = out_w * bpp;

    if (param->image_pitch != 0) {
        if (bpp == 4 && (param->image_pitch & 3) != 0) {
            LOG_ERROR(Lib_Jpeg, "Misaligned image_pitch: {}", param->image_pitch);
            stbi_image_free(decoded);
            return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
        }
        if (param->image_pitch < min_pitch) {
            LOG_ERROR(Lib_Jpeg, "image_pitch too small: {} < {}", param->image_pitch, min_pitch);
            stbi_image_free(decoded);
            return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
        }
        if (static_cast<u64>(param->image_pitch) * out_h > param->image_size) {
            LOG_ERROR(Lib_Jpeg, "Buffer size too small for pitch (size={}, required={})",
                      param->image_size, static_cast<u64>(param->image_pitch) * out_h);
            stbi_image_free(decoded);
            return ORBIS_JPEG_DEC_ERROR_INVALID_PARAM;
        }
    } else {
        if (static_cast<u64>(min_pitch) * out_h > param->image_size) {
            LOG_ERROR(Lib_Jpeg, "Buffer size too small (size={}, required={})", param->image_size,
                      static_cast<u64>(min_pitch) * out_h);
            stbi_image_free(decoded);
            return ORBIS_JPEG_DEC_ERROR_INVALID_SIZE;
        }
    }

    const u32 pitch = (param->image_pitch != 0) ? param->image_pitch : min_pitch;
    auto* dst_base = reinterpret_cast<u8*>(param->image_addr);

    for (u32 y = 0; y < out_h; ++y) {
        u8* dst_row = dst_base + y * pitch;
        const u32 src_y = y * ratio;
        for (u32 x = 0; x < out_w; ++x) {
            const u32 src_x = x * ratio;
            const stbi_uc* src = &decoded[(src_y * static_cast<u32>(w) + src_x) * 4];

            if (format == OrbisJpegDecPixelFormat::B8G8R8A8) {
                dst_row[x * 4 + 0] = src[2];
                dst_row[x * 4 + 1] = src[1];
                dst_row[x * 4 + 2] = src[0];
                dst_row[x * 4 + 3] =
                    (param->alpha_value != 0) ? static_cast<u8>(param->alpha_value) : src[3];
            } else if (format == OrbisJpegDecPixelFormat::R8G8B8A8) {
                dst_row[x * 4 + 0] = src[0];
                dst_row[x * 4 + 1] = src[1];
                dst_row[x * 4 + 2] = src[2];
                dst_row[x * 4 + 3] =
                    (param->alpha_value != 0) ? static_cast<u8>(param->alpha_value) : src[3];
            } else if (format == OrbisJpegDecPixelFormat::Y8) {
                dst_row[x] = static_cast<u8>((src[0] * 77 + src[1] * 150 + src[2] * 29) >> 8);
            }
        }
    }

    stbi_image_free(decoded);

    if (image_info) {
        std::memset(image_info, 0, sizeof(OrbisJpegDecImageInfo));
        image_info->image_width = static_cast<u32>(w);
        image_info->image_height = static_cast<u32>(h);
        image_info->color_space =
            (ch == 1) ? OrbisJpegDecColorSpace::Grayscale : OrbisJpegDecColorSpace::Ycc;
        image_info->num_components = static_cast<u16>(ch);
        image_info->component_id[0] = 1;
        image_info->component_id[1] = 2;
        image_info->component_id[2] = 3;
        image_info->sampling_factor[0] = 0x22;
        image_info->sampling_factor[1] = 0x11;
        image_info->sampling_factor[2] = 0x11;
        image_info->decode_mode = std::to_underlying(param->decode_mode);
        image_info->output_width = out_w;
        image_info->output_height = out_h;
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceJpegDecDecodeWithInputControl(OrbisJpegDecHandle handle,
                                                  const OrbisJpegDecDecodeParam* param,
                                                  OrbisJpegDecImageInfo* image_info) {
    return sceJpegDecDecode(handle, param, image_info);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("uNAUmANZMEw", "libSceJpegDec", 1, "libSceJpegDec", sceJpegDecQueryMemorySize);
    LIB_FUNCTION("JPh3Zgg0Zwc", "libSceJpegDec", 1, "libSceJpegDec", sceJpegDecCreate);
    LIB_FUNCTION("Hwh11+m5KoI", "libSceJpegDec", 1, "libSceJpegDec", sceJpegDecDelete);
    LIB_FUNCTION("1kzQRoWEgSA", "libSceJpegDec", 1, "libSceJpegDec", sceJpegDecDecode);
    LIB_FUNCTION("LSinoSQH790", "libSceJpegDec", 1, "libSceJpegDec", sceJpegDecParseHeader);
    LIB_FUNCTION("919MhccOiII", "libSceJpegDec_jvm", 1, "libSceJpegDec",
                 sceJpegDecDecodeWithInputControl);
}

} // namespace Libraries::JpegDec
