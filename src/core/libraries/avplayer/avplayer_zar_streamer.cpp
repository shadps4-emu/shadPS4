// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdio>
#include <filesystem>

#include "core/libraries/avplayer/avplayer_zar_streamer.h"

extern "C" {
#include <libavformat/avio.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

namespace Libraries::AvPlayer {

constexpr s32 AvioBufferSize = 4096;

AvPlayerZarStreamer::~AvPlayerZarStreamer() {
    if (m_avio_context != nullptr) {
        avio_context_free(&m_avio_context);
    }
}

bool AvPlayerZarStreamer::Init(std::string_view path) {
    if (m_file.Open(std::filesystem::path{path}, Common::FS::FileAccessMode::Read) != 0) {
        return false;
    }
    m_file_size = m_file.GetSize();

    // The buffer is owned and deallocated by avio_context_free after a successful allocation.
    auto* buffer = static_cast<u8*>(av_malloc(AvioBufferSize));
    if (buffer == nullptr) {
        return false;
    }
    m_avio_context =
        avio_alloc_context(buffer, AvioBufferSize, 0, this, &AvPlayerZarStreamer::ReadPacket,
                           nullptr, &AvPlayerZarStreamer::Seek);
    if (m_avio_context == nullptr) {
        av_free(buffer);
        return false;
    }
    return true;
}

void AvPlayerZarStreamer::Reset() {
    m_position = 0;
    m_file.Seek(0);
}

s32 AvPlayerZarStreamer::ReadPacket(void* opaque, u8* buffer, s32 size) {
    const auto self = static_cast<AvPlayerZarStreamer*>(opaque);
    if (self->m_position >= self->m_file_size) {
        return AVERROR_EOF;
    }
    if (self->m_position + size > self->m_file_size) {
        size = self->m_file_size - self->m_position;
    }
    const auto bytes_read = static_cast<s32>(self->m_file.ReadRaw<u8>(buffer, size));
    if (bytes_read == 0 && size != 0) {
        return AVERROR_EOF;
    }
    self->m_position += bytes_read;
    return bytes_read;
}

s64 AvPlayerZarStreamer::Seek(void* opaque, s64 offset, int whence) {
    const auto self = static_cast<AvPlayerZarStreamer*>(opaque);
    if (whence & AVSEEK_SIZE) {
        return self->m_file_size;
    }

    u64 position;
    if (whence == SEEK_CUR) {
        position =
            std::min(u64(std::max(s64(0), s64(self->m_position) + offset)), self->m_file_size);
    } else if (whence == SEEK_SET) {
        position = std::min(u64(std::max(s64(0), offset)), self->m_file_size);
    } else if (whence == SEEK_END) {
        position =
            std::min(u64(std::max(s64(0), s64(self->m_file_size) + offset)), self->m_file_size);
    } else {
        return -1;
    }

    if (!self->m_file.Seek(static_cast<s64>(position))) {
        return -1;
    }
    self->m_position = position;
    return position;
}

} // namespace Libraries::AvPlayer
