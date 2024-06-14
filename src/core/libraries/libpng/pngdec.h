// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::PngDec {

constexpr int ORBIS_PNG_DEC_ERROR_INVALID_ADDR = 0x80690001;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_SIZE = 0x80690002;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_PARAM = 0x80690003;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_HANDLE = 0x80690004;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_WORK_MEMORY = 0x80690005;
constexpr int ORBIS_PNG_DEC_ERROR_INVALID_DATA = 0x80690010;
constexpr int ORBIS_PNG_DEC_ERROR_UNSUPPORT_DATA = 0x80690011;
constexpr int ORBIS_PNG_DEC_ERROR_DECODE_ERROR = 0x80690012;
constexpr int ORBIS_PNG_DEC_ERROR_FATAL = 0x80690020;

typedef struct OrbisPngDecParseParam {
    const void* pngMemAddr;
    u32 pngMemSize;
    u32 reserved;
} OrbisPngDecParseParam;

typedef struct OrbisPngDecImageInfo {
    u32 imageWidth;
    u32 imageHeight;
    u16 colorSpace;
    u16 bitDepth;
    u32 imageFlag;
} OrbisPngDecImageInfo;

typedef enum OrbisPngDecColorSpace {
    ORBIS_PNG_DEC_COLOR_SPACE_GRAYSCALE = 2,
    ORBIS_PNG_DEC_COLOR_SPACE_RGB,
    ORBIS_PNG_DEC_COLOR_SPACE_CLUT,
    ORBIS_PNG_DEC_COLOR_SPACE_GRAYSCALE_ALPHA = 18,
    ORBIS_PNG_DEC_COLOR_SPACE_RGBA
} ScePngDecColorSpace;

typedef enum OrbisPngDecImageFlag {
    ORBIS_PNG_DEC_IMAGE_FLAG_ADAM7_INTERLACE = 1,
    ORBIS_PNG_DEC_IMAGE_FLAG_TRNS_CHUNK_EXIST = 2
} OrbisPngDecImageFlag;

typedef struct OrbisPngDecCreateParam {
    u32 thisSize;
    u32 attribute;
    u32 maxImageWidth;
} OrbisPngDecCreateParam;

typedef void* OrbisPngDecHandle;

typedef struct OrbisPngDecDecodeParam {
    const void* pngMemAddr;
    void* imageMemAddr;
    u32 pngMemSize;
    u32 imageMemSize;
    u16 pixelFormat;
    u16 alphaValue;
    u32 imagePitch;
} OrbisPngDecDecodeParam;

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