// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm> // std::max, std::min
#include <magic_enum/magic_enum.hpp>
#include "core/libraries/avplayer/avplayer_file_streamer.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

constexpr u32 AVPLAYER_AVIO_BUFFER_SIZE = 4096;

namespace Libraries::AvPlayer {

AvPlayerFileStreamer::AvPlayerFileStreamer(const AvPlayerFileReplacement& file_replacement)
    : m_file_replacement(file_replacement) {}

AvPlayerFileStreamer::~AvPlayerFileStreamer() {
    if (m_avio_context != nullptr) {
        avio_context_free(&m_avio_context);
    }
    if (m_file_replacement.close != nullptr && m_fd >= 0) {
        const auto close = m_file_replacement.close;
        const auto ptr = m_file_replacement.object_ptr;
        close(ptr);
    }
}

bool AvPlayerFileStreamer::Init(std::string_view path) {
    const auto ptr = m_file_replacement.object_ptr;
    m_fd = m_file_replacement.open(ptr, path.data());
    if (m_fd < 0) {
        return false;
    }
    m_file_size = m_file_replacement.size(ptr);
    // avio_buffer is deallocated in `avio_context_free`
    const auto avio_buffer = reinterpret_cast<u8*>(av_malloc(AVPLAYER_AVIO_BUFFER_SIZE));
    m_avio_context =
        avio_alloc_context(avio_buffer, AVPLAYER_AVIO_BUFFER_SIZE, 0, this,
                           &AvPlayerFileStreamer::ReadPacket, nullptr, &AvPlayerFileStreamer::Seek);
    return true;
}

void AvPlayerFileStreamer::Reset() {
    m_position = 0;
}

s32 AvPlayerFileStreamer::ReadPacket(void* opaque, u8* buffer, s32 size) {
    const auto self = reinterpret_cast<AvPlayerFileStreamer*>(opaque);
    if (self->m_position >= self->m_file_size) {
        return AVERROR_EOF;
    }
    if (self->m_position + size > self->m_file_size) {
        size = self->m_file_size - self->m_position;
    }
    const auto read_offset = self->m_file_replacement.read_offset;
    const auto ptr = self->m_file_replacement.object_ptr;
    const auto bytes_read = read_offset(ptr, buffer, self->m_position, size);
    if (bytes_read == 0 && size != 0) {
        return AVERROR_EOF;
    }
    self->m_position += bytes_read;
    return bytes_read;
}

s64 AvPlayerFileStreamer::Seek(void* opaque, s64 offset, int whence) {
    const auto self = reinterpret_cast<AvPlayerFileStreamer*>(opaque);
    if (whence & AVSEEK_SIZE) {
        return self->m_file_size;
    }

    if (whence == SEEK_CUR) {
        self->m_position =
            std::min(u64(std::max(s64(0), s64(self->m_position) + offset)), self->m_file_size);
        return self->m_position;
    } else if (whence == SEEK_SET) {
        self->m_position = std::min(u64(std::max(s64(0), offset)), self->m_file_size);
        return self->m_position;
    } else if (whence == SEEK_END) {
        self->m_position =
            std::min(u64(std::max(s64(0), s64(self->m_file_size) + offset)), self->m_file_size);
        return self->m_position;
    }

    return -1;
}

} // namespace Libraries::AvPlayer
