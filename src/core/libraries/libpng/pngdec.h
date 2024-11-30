// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/enum.h"
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::PngDec {

enum class OrbisPngDecColorSpace : u16 { Grayscale = 2, Rgb, Clut, GrayscaleAlpha = 18, Rgba };

enum class OrbisPngDecImageFlag : u32 { None = 0, Adam7Interlace = 1, TrnsChunkExist = 2 };
DECLARE_ENUM_FLAG_OPERATORS(OrbisPngDecImageFlag)

enum class OrbisPngDecPixelFormat : u16 { R8G8B8A8 = 0, B8G8R8A8 };

enum class OrbisPngDecAttribute { None = 0, BitDepth16 };

struct OrbisPngDecParseParam {
    const void* pngMemAddr;
    u32 pngMemSize;
    u32 reserved;
};

struct OrbisPngDecImageInfo {
    u32 imageWidth;
    u32 imageHeight;
    OrbisPngDecColorSpace colorSpace;
    u16 bitDepth;
    OrbisPngDecImageFlag imageFlag;
};

struct OrbisPngDecCreateParam {
    u32 thisSize;
    u32 attribute;
    u32 maxImageWidth;
};

using OrbisPngDecHandle = void*;

struct OrbisPngDecDecodeParam {
    const void* pngMemAddr;
    void* imageMemAddr;
    u32 pngMemSize;
    u32 imageMemSize;
    OrbisPngDecPixelFormat pixelFormat;
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
