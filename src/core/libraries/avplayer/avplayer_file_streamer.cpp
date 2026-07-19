// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm> // std::max, std::min
#include <utility>

#include "core/libraries/avplayer/avplayer_file_streamer.h"

extern "C" {
#include <libavformat/avio.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

constexpr u32 AVPLAYER_AVIO_BUFFER_SIZE = 4096;

namespace Libraries::AvPlayer {

AvPlayerFileStreamer::AvPlayerFileStreamer(const AvPlayerFileReplacement& file_replacement)
    : m_file_replacement(file_replacement) {}

AvPlayerFileStreamer::AvPlayerFileStreamer(std::filesystem::path path) : m_path(std::move(path)) {}

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

bool AvPlayerFileStreamer::Init() {
    if (m_file.Open(m_path, Common::FS::FileAccessMode::Read) != 0) {
        return false;
    }
    m_file_size = m_file.GetSize();
    return InitAvioContext();
}

bool AvPlayerFileStreamer::Init(std::string_view path) {
    const auto ptr = m_file_replacement.object_ptr;
    m_fd = m_file_replacement.open(ptr, path.data());
    if (m_fd < 0) {
        return false;
    }
    m_file_size = m_file_replacement.size(ptr);
    return InitAvioContext();
}

bool AvPlayerFileStreamer::InitAvioContext() {
    // avio_buffer is deallocated in `avio_context_free`
    const auto avio_buffer = reinterpret_cast<u8*>(av_malloc(AVPLAYER_AVIO_BUFFER_SIZE));
    if (avio_buffer == nullptr) {
        return false;
    }
    m_avio_context =
        avio_alloc_context(avio_buffer, AVPLAYER_AVIO_BUFFER_SIZE, 0, this,
                           &AvPlayerFileStreamer::ReadPacket, nullptr, &AvPlayerFileStreamer::Seek);
    if (m_avio_context == nullptr) {
        av_free(avio_buffer);
        return false;
    }
    return true;
}

void AvPlayerFileStreamer::Reset() {
    m_position = 0;
    if (m_file.IsOpen()) {
        m_file.Seek(0);
    }
}

s32 AvPlayerFileStreamer::ReadPacket(void* opaque, u8* buffer, s32 size) {
    const auto self = reinterpret_cast<AvPlayerFileStreamer*>(opaque);
    if (self->m_position >= self->m_file_size) {
        return AVERROR_EOF;
    }
    if (self->m_position + size > self->m_file_size) {
        size = self->m_file_size - self->m_position;
    }
    s32 bytes_read;
    if (self->m_file.IsOpen()) {
        bytes_read = static_cast<s32>(self->m_file.ReadRaw<u8>(buffer, size));
    } else {
        const auto read_offset = self->m_file_replacement.read_offset;
        const auto ptr = self->m_file_replacement.object_ptr;
        bytes_read = read_offset(ptr, buffer, self->m_position, size);
    }
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

    if (self->m_file.IsOpen() && !self->m_file.Seek(static_cast<s64>(position))) {
        return -1;
    }

    self->m_position = position;
    return position;
}

} // namespace Libraries::AvPlayer
