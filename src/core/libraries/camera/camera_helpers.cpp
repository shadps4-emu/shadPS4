// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "camera_helpers.h"

namespace Libraries::Camera {

static inline u8 ClampU8(int v) {
    return static_cast<u8>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

static inline u16 ClampU16(int v) {
    return static_cast<u16>(v < 0 ? 0 : (v > 65535 ? 65535 : v));
}

static inline u8 RgbaToLuma(const u8* px) {
    const u8 b = px[0];
    const u8 g = px[1];
    const u8 r = px[2];
    return ClampU8((77 * r + 150 * g + 29 * b) >> 8);
}

void ConvertRGBA8888ToRAW16(const u8* src, u16* dst, int width, int height) {
    for (int y = 0; y < height; ++y) {
        const u8* row = src + static_cast<u64>(y) * width * 4;
        u16* outRow = dst + static_cast<u64>(y) * width;
        const bool evenRow = (y & 1) == 0;
        for (int x = 0; x < width; ++x) {
            const u8* px = row + x * 4;
            const u16 b = u16(px[1]) << 4;
            const u16 g = u16(px[2]) << 4;
            const u16 r = u16(px[3]) << 4;
            const bool evenCol = (x & 1) == 0;
            if (evenRow && evenCol)
                outRow[x] = b;
            else if (evenRow && !evenCol)
                outRow[x] = g;
            else if (!evenRow && evenCol)
                outRow[x] = g;
            else
                outRow[x] = r;
        }
    }
}

void ConvertRGBA8888ToRAW8(const u8* src, u8* dst, int width, int height) {
    for (int y = 0; y < height; ++y) {
        const u8* row = src + static_cast<u64>(y) * width * 4;
        u8* outRow = dst + static_cast<u64>(y) * width;
        const bool evenRow = (y & 1) == 0;
        for (int x = 0; x < width; ++x) {
            const u8* px = row + x * 4;
            const u8 b = px[1];
            const u8 g = px[2];
            const u8 r = px[3];
            const bool evenCol = (x & 1) == 0;
            if (evenRow && evenCol)
                outRow[x] = b;
            else if (evenRow && !evenCol)
                outRow[x] = g;
            else if (!evenRow && evenCol)
                outRow[x] = g;
            else
                outRow[x] = r;
        }
    }
}

void ConvertRGBA8888ToY8(const u8* src, u8* dst, int width, int height) {
    for (int y = 0; y < height; ++y) {
        const u8* row = src + static_cast<u64>(y) * width * 4;
        u8* outRow = dst + static_cast<u64>(y) * width;
        for (int x = 0; x < width; ++x) {
            outRow[x] = RgbaToLuma(row + x * 4);
        }
    }
}

void ConvertRGBA8888ToY16(const u8* src, u16* dst, int width, int height) {
    for (int y = 0; y < height; ++y) {
        const u8* row = src + static_cast<u64>(y) * width * 4;
        u16* outRow = dst + static_cast<u64>(y) * width;
        for (int x = 0; x < width; ++x) {
            outRow[x] = static_cast<u16>(RgbaToLuma(row + x * 4)) << 2;
        }
    }
}

void ConvertYUY2ToY8(const u8* src, u8* dst, int width, int height) {
    for (int y = 0; y < height; ++y) {
        const u8* row = src + static_cast<u64>(y) * width * 2;
        u8* outRow = dst + static_cast<u64>(y) * width;
        for (int x = 0; x < width; x += 2) {
            outRow[x] = row[x * 2 + 0];     // Y0
            outRow[x + 1] = row[x * 2 + 2]; // Y1
        }
    }
}

void ConvertYUY2ToY16(const u8* src, u16* dst, int width, int height) {
    for (int y = 0; y < height; ++y) {
        const u8* row = src + static_cast<u64>(y) * width * 2;
        u16* outRow = dst + static_cast<u64>(y) * width;
        for (int x = 0; x < width; x += 2) {
            outRow[x] = static_cast<u16>(row[x * 2 + 0]) << 2;
            outRow[x + 1] = static_cast<u16>(row[x * 2 + 2]) << 2;
        }
    }
}

s32 SizeOfBaseFormat(OrbisCameraBaseFormat f) {
    switch (f) {
    case ORBIS_CAMERA_FORMAT_YUV422:
        return 2;
    case ORBIS_CAMERA_FORMAT_RAW16:
        return 2;
    case ORBIS_CAMERA_FORMAT_RAW8:
        return 1;
    case ORBIS_CAMERA_FORMAT_NO_USE:
    default:
        return 0;
    }
}

s32 SizeOfScaleFormat(OrbisCameraScaleFormat f) {
    switch (f) {
    case ORBIS_CAMERA_SCALE_FORMAT_YUV422:
        return 2;
    case ORBIS_CAMERA_SCALE_FORMAT_Y16:
        return 2;
    case ORBIS_CAMERA_SCALE_FORMAT_Y8:
        return 1;
    case ORBIS_CAMERA_SCALE_FORMAT_NO_USE:
    default:
        return 0;
    }
}
} // namespace Libraries::Camera