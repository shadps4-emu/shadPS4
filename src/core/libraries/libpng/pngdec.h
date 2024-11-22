// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::PngDec {

enum OrbisPngDecColorSpace {
    ORBIS_PNG_DEC_COLOR_SPACE_GRAYSCALE = 2,
    ORBIS_PNG_DEC_COLOR_SPACE_RGB,
    ORBIS_PNG_DEC_COLOR_SPACE_CLUT,
    ORBIS_PNG_DEC_COLOR_SPACE_GRAYSCALE_ALPHA = 18,
    ORBIS_PNG_DEC_COLOR_SPACE_RGBA
};

enum OrbisPngDecImageFlag {
    ORBIS_PNG_DEC_IMAGE_FLAG_ADAM7_INTERLACE = 1,
    ORBIS_PNG_DEC_IMAGE_FLAG_TRNS_CHUNK_EXIST = 2
};

enum OrbisPngDecPixelFormat {
    ORBIS_PNG_DEC_PIXEL_FORMAT_R8G8B8A8 = 0,
    ORBIS_PNG_DEC_PIXEL_FORMAT_B8G8R8A8
};

enum OrbisPngDecAttribute {
    ORBIS_PNG_DEC_ATTRIBUTE_NONE = 0,
    ORBIS_PNG_DEC_ATTRIBUTE_BIT_DEPTH_16
};

struct OrbisPngDecParseParam {
    const void* pngMemAddr;
    u32 pngMemSize;
    u32 reserved;
};

struct OrbisPngDecImageInfo {
    u32 imageWidth;
    u32 imageHeight;
    u16 colorSpace;
    u16 bitDepth;
    u32 imageFlag;
};

struct OrbisPngDecCreateParam {
    u32 thisSize;
    u32 attribute;
    u32 maxImageWidth;
};

typedef void* OrbisPngDecHandle;

struct OrbisPngDecDecodeParam {
    const void* pngMemAddr;
    void* imageMemAddr;
    u32 pngMemSize;
    u32 imageMemSize;
    u16 pixelFormat;
    u16 alphaValue;
    u32 imagePitch;
};

s32 PS4_SYSV_ABI scePngDecCreate(const OrbisPngDecCreateParam* param, void* memoryAddress,
                                 u32 memorySize, OrbisPngDecHandle* handle);
s32 PS4_SYSV_ABI scePngDecDecode(OrbisPngDecHandle handle, const OrbisPngDecDecodeParam* param,
                                 OrbisPngDecImageInfo* imageInfo);
s32 PS4_SYSV_ABI scePngDecDecodeWithInputControl();
s32 PS4_SYSV_ABI scePngDecDelete(OrbisPngDecHandle handle);
s32 PS4_SYSV_ABI scePngDecParseHeader(const OrbisPngDecParseParam* param,
                                      OrbisPngDecImageInfo* imageInfo);
s32 PS4_SYSV_ABI scePngDecQueryMemorySize(const OrbisPngDecCreateParam* param);

void RegisterlibScePngDec(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::PngDec