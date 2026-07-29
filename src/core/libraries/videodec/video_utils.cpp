// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_utils.h"

#include "common/alignment.h"

#include <libavutil/frame.h>

#include <cstring>

namespace Libraries::Videodec {

void CopyNV12Data(u8* dst, const AVFrame& src) {
    const auto dst_pitch = Common::AlignUp<u32>(src.width, 64);
    const auto dst_height = Common::AlignUp<u32>(src.height, 16);

    const auto luma_dst = dst;
    const auto chroma_dst = dst + dst_pitch * dst_height;

    if (src.width != dst_pitch) {
        for (u32 y = 0; y < src.height; ++y) {
            std::memcpy(luma_dst + y * dst_pitch, src.data[0] + y * src.linesize[0], src.width);
        }
        for (u32 y = 0; y < src.height / 2; ++y) {
            std::memcpy(chroma_dst + y * dst_pitch, src.data[1] + y * src.linesize[1], src.width);
        }
    } else {
        std::memcpy(luma_dst, src.data[0], src.width * src.height);
        std::memcpy(chroma_dst, src.data[1], (src.width * src.height) / 2);
    }

    if (src.height != dst_height) {
        // Extend the data vertically to the crop space
        const auto ly = src.height - 1;
        for (u32 y = src.height; y < dst_height; ++y) {
            std::memcpy(luma_dst + y * dst_pitch, src.data[0] + ly * src.linesize[0], src.width);
        }
        const auto cy = (src.height / 2) - 1;
        for (u32 y = src.height / 2; y < dst_height / 2; ++y) {
            std::memcpy(chroma_dst + y * dst_pitch, src.data[1] + cy * src.linesize[1], src.width);
        }
    }
}

} // namespace Libraries::Videodec
