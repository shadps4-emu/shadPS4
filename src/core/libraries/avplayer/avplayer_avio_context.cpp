// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/avplayer/avplayer_avio_context.h"
#include "core/libraries/avplayer/avplayer_data_streamer.h"

extern "C" {
#include <libavformat/avio.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

namespace Libraries::AvPlayer {

constexpr s32 AvioBufferSize = 4096;

AvioContext::~AvioContext() {
    if (m_context != nullptr) {
        avio_context_free(&m_context);
    }
}

bool AvioContext::Init(IDataStreamer& streamer) {
    if (m_context != nullptr) {
        return false;
    }

    // The buffer is owned and deallocated by avio_context_free after a successful allocation.
    auto* buffer = static_cast<u8*>(av_malloc(AvioBufferSize));
    if (buffer == nullptr) {
        return false;
    }

    m_context = avio_alloc_context(buffer, AvioBufferSize, 0, &streamer, &AvioContext::ReadPacket,
                                   nullptr, &AvioContext::Seek);
    if (m_context == nullptr) {
        av_free(buffer);
        return false;
    }
    return true;
}

s32 AvioContext::ReadPacket(void* opaque, u8* buffer, s32 size) {
    const auto bytes_read = static_cast<IDataStreamer*>(opaque)->Read(buffer, size);
    return bytes_read == 0 && size != 0 ? AVERROR_EOF : bytes_read;
}

s64 AvioContext::Seek(void* opaque, s64 offset, int whence) {
    auto* streamer = static_cast<IDataStreamer*>(opaque);
    if (whence & AVSEEK_SIZE) {
        return streamer->GetSize();
    }
    return streamer->Seek(offset, whence);
}

} // namespace Libraries::AvPlayer
