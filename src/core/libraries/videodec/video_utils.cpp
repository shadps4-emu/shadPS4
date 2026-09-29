// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_utils.h"

#include "common/alignment.h"
#include "common/assert.h"

#include <libavutil/frame.h>

#include <cstring>

namespace Libraries::Videodec {

void CopyNV12Data(u8* dst, u64 max_size, const AVFrame& src) {
    ASSERT(src.format == AV_PIX_FMT_NV12);

    const u32 width = src.width;
    const u32 height = src.height;
    const u32 dst_pitch = Common::AlignUp<u32>(width, 64);
    const u32 dst_height = Common::AlignUp<u32>(height, 16);
    const u32 chroma_h = (height + 1) / 2;

    ASSERT(u64(dst_pitch) * dst_height * 3 / 2 <= max_size);

    u8* const luma_dst = dst;
    u8* const chroma_dst = dst + u64(dst_pitch) * dst_height;

    const bool packed = src.linesize[0] == int(dst_pitch) && src.linesize[1] == int(dst_pitch);
    if (packed) {
        std::memcpy(luma_dst, src.data[0], u64(dst_pitch) * height);
        std::memcpy(chroma_dst, src.data[1], u64(dst_pitch) * chroma_h);
    } else {
        for (u32 y = 0; y < height; ++y)
            std::memcpy(luma_dst + u64(y) * dst_pitch, src.data[0] + u64(y) * src.linesize[0],
                        width);
        for (u32 y = 0; y < chroma_h; ++y)
            std::memcpy(chroma_dst + u64(y) * dst_pitch, src.data[1] + u64(y) * src.linesize[1],
                        width);
    }

    // Replicate the last row into the alignment padding
    for (u32 y = height; y < dst_height; ++y)
        std::memcpy(luma_dst + u64(y) * dst_pitch, luma_dst + u64(height - 1) * dst_pitch, width);
    for (u32 y = chroma_h; y < dst_height / 2; ++y)
        std::memcpy(chroma_dst + u64(y) * dst_pitch, chroma_dst + u64(chroma_h - 1) * dst_pitch,
                    width);
}

} // namespace Libraries::Videodec
