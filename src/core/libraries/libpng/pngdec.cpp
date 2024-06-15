// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "externals/stb_image.h"
#include "pngdec.h"

namespace Libraries::PngDec {

void setImageInfoParams(OrbisPngDecImageInfo* imageInfo, int width, int height, int channels,
                        bool isInterlaced, bool isTransparent) {
    if (imageInfo != nullptr) {
        imageInfo->imageWidth = width;
        imageInfo->imageHeight = height;
        imageInfo->bitDepth = 8; // always 8?
        switch (channels) {      // clut missing
        case 1:
            imageInfo->colorSpace = OrbisPngDecColorSpace::ORBIS_PNG_DEC_COLOR_SPACE_GRAYSCALE;
            break;
        case 2:
            imageInfo->colorSpace =
                OrbisPngDecColorSpace::ORBIS_PNG_DEC_COLOR_SPACE_GRAYSCALE_ALPHA;
            break;
        case 3:
            imageInfo->colorSpace = OrbisPngDecColorSpace::ORBIS_PNG_DEC_COLOR_SPACE_RGB;
            break;
        case 4:
            imageInfo->colorSpace = OrbisPngDecColorSpace::ORBIS_PNG_DEC_COLOR_SPACE_RGBA;
            break;
        default:
            imageInfo->colorSpace = OrbisPngDecColorSpace::ORBIS_PNG_DEC_COLOR_SPACE_RGB;
            break;
        }
        imageInfo->imageFlag = 0;
        if (isInterlaced) {
            imageInfo->imageFlag |= ORBIS_PNG_DEC_IMAGE_FLAG_ADAM7_INTERLACE;
        }
        if (isTransparent) {
            imageInfo->imageFlag |= ORBIS_PNG_DEC_IMAGE_FLAG_TRNS_CHUNK_EXIST;
        }
    }
}

bool checktRNS(const u8* png_raw, int size) {
    for (int i = 30; i < size - 4; i += 4) {
        if (std::memcmp(png_raw + i, "tRNS", 4) == 0) {
            return true;
        }
    }
    return false;
}

s32 PS4_SYSV_ABI scePngDecCreate(const OrbisPngDecCreateParam* param, void* memoryAddress,
                                 u32 memorySize, OrbisPngDecHandle* handle) {
    if (param == nullptr || param->attribute > 1) {
        LOG_ERROR(Lib_Png, "Invalid param!");
        return ORBIS_PNG_DEC_ERROR_INVALID_PARAM;
    }
    if (memoryAddress == nullptr) {
        LOG_ERROR(Lib_Png, "Invalid memory address!");
        return ORBIS_PNG_DEC_ERROR_INVALID_ADDR;
    }
    if (param->maxImageWidth - 1 > 1000000) {
        LOG_ERROR(Lib_Png, "Invalid size! width = {}", param->maxImageWidth);
        return ORBIS_PNG_DEC_ERROR_INVALID_SIZE;
    }
    const OrbisPngDecCreateParam* nextParam = param + 1;
    int ret = (8 << (reinterpret_cast<uintptr_t>(nextParam) & 0x1f)) *
                  (param->maxImageWidth + 0x47U & 0xfffffff8) +
              0xd000;
    *handle = reinterpret_cast<OrbisPngDecHandle>(ret);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePngDecDecode(OrbisPngDecHandle handle, const OrbisPngDecDecodeParam* param,
                                 OrbisPngDecImageInfo* imageInfo) {
    if (handle == nullptr) {
        LOG_ERROR(Lib_Png, "invalid handle!");
        return ORBIS_PNG_DEC_ERROR_INVALID_HANDLE;
    }
    if (param == nullptr) {
        LOG_ERROR(Lib_Png, "Invalid param!");
        return ORBIS_PNG_DEC_ERROR_INVALID_PARAM;
    }
    if (param->pngMemAddr == nullptr || param->pngMemAddr == nullptr) {
        LOG_ERROR(Lib_Png, "invalid image address!");
        return ORBIS_PNG_DEC_ERROR_INVALID_ADDR;
    }

    int width, height, channels;
    const u8* png_raw = (const u8*)param->pngMemAddr;
    u8* img = stbi_load_from_memory(png_raw, param->pngMemSize, &width, &height, &channels,
                                    STBI_rgb_alpha); // STBI_rgb_alpha?
    if (!img) {
        LOG_ERROR(Lib_Png, "Decoding failed!");
        return ORBIS_PNG_DEC_ERROR_DECODE_ERROR;
    }
    bool isInterlaced = (png_raw[28] == 1);
    bool isTransparent = checktRNS(png_raw, param->pngMemSize);
    setImageInfoParams(imageInfo, width, height, channels, isInterlaced, isTransparent);
    u8* imageBuffer = (u8*)(param->imageMemAddr);
    memcpy(imageBuffer, img, width * height * 4); // copy/pass decoded data
    stbi_image_free(img);
    return 0;
}

s32 PS4_SYSV_ABI scePngDecDecodeWithInputControl() {
    LOG_ERROR(Lib_Png, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePngDecDelete(OrbisPngDecHandle handle) {
    handle = nullptr; // ?
    LOG_ERROR(Lib_Png, "(STUBBED)called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePngDecParseHeader(const OrbisPngDecParseParam* param,
                                      OrbisPngDecImageInfo* imageInfo) {
    if (param == nullptr) {
        LOG_ERROR(Lib_Png, "Invalid param!");
        return ORBIS_PNG_DEC_ERROR_INVALID_PARAM;
    }
    int width, height, channels;
    const u8* png_raw = (const u8*)(param->pngMemAddr);
    int img = stbi_info_from_memory(png_raw, param->pngMemSize, &width, &height, &channels);
    if (img == 0) {
        LOG_ERROR(Lib_Png, "Decoding failed!");
        return ORBIS_PNG_DEC_ERROR_DECODE_ERROR;
    }
    bool isInterlaced = (png_raw[28] == 1);
    bool isTransparent = checktRNS(png_raw, param->pngMemSize);
    setImageInfoParams(imageInfo, width, height, channels, isInterlaced, isTransparent);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scePngDecQueryMemorySize(const OrbisPngDecCreateParam* param) {
    if (param == nullptr) {
        LOG_ERROR(Lib_Png, "Invalid param!");
        return ORBIS_PNG_DEC_ERROR_INVALID_PARAM;
    }
    if (param->attribute > 1) {
        LOG_ERROR(Lib_Png, "Invalid attribute! attribute = {}", param->attribute);
        return ORBIS_PNG_DEC_ERROR_INVALID_ADDR;
    }
    if (param->maxImageWidth - 1 > 1000000) {
        LOG_ERROR(Lib_Png, "Invalid size! width = {}", param->maxImageWidth);
        return ORBIS_PNG_DEC_ERROR_INVALID_SIZE;
    }
    int ret =
        (8 << ((u8)param->attribute & 0x1f)) * (param->maxImageWidth + 0x47U & 0xfffffff8) + 0xd090;
    return ret;
}

void RegisterlibScePngDec(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("m0uW+8pFyaw", "libScePngDec", 1, "libScePngDec", 1, 1, scePngDecCreate);
    LIB_FUNCTION("WC216DD3El4", "libScePngDec", 1, "libScePngDec", 1, 1, scePngDecDecode);
    LIB_FUNCTION("cJ--1xAbj-I", "libScePngDec", 1, "libScePngDec", 1, 1,
                 scePngDecDecodeWithInputControl);
    LIB_FUNCTION("QbD+eENEwo8", "libScePngDec", 1, "libScePngDec", 1, 1, scePngDecDelete);
    LIB_FUNCTION("U6h4e5JRPaQ", "libScePngDec", 1, "libScePngDec", 1, 1, scePngDecParseHeader);
    LIB_FUNCTION("-6srIGbLTIU", "libScePngDec", 1, "libScePngDec", 1, 1, scePngDecQueryMemorySize);
    LIB_FUNCTION("cJ--1xAbj-I", "libScePngDec_jvm", 1, "libScePngDec", 1, 1,
                 scePngDecDecodeWithInputControl);
};

} // namespace Libraries::PngDec