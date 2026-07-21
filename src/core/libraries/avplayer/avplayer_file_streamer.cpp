// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm> // std::max, std::min
#include <cstdio>

#include "core/libraries/avplayer/avplayer_file_streamer.h"

namespace Libraries::AvPlayer {

AvPlayerFileStreamer::AvPlayerFileStreamer(const AvPlayerFileReplacement& file_replacement)
    : m_file_replacement(file_replacement) {}

AvPlayerFileStreamer::~AvPlayerFileStreamer() {
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
    return true;
}

void AvPlayerFileStreamer::Reset() {
    m_position = 0;
}

s32 AvPlayerFileStreamer::Read(u8* buffer, s32 size) {
    if (m_position >= m_file_size) {
        return 0;
    }
    if (m_position + size > m_file_size) {
        size = m_file_size - m_position;
    }
    const auto read_offset = m_file_replacement.read_offset;
    const auto ptr = m_file_replacement.object_ptr;
    const auto bytes_read = read_offset(ptr, buffer, m_position, size);
    if (bytes_read > 0) {
        m_position += bytes_read;
    }
    return bytes_read;
}

s64 AvPlayerFileStreamer::Seek(s64 offset, int whence) {
    if (whence == SEEK_CUR) {
        m_position = std::min(u64(std::max(s64(0), s64(m_position) + offset)), m_file_size);
    } else if (whence == SEEK_SET) {
        m_position = std::min(u64(std::max(s64(0), offset)), m_file_size);
    } else if (whence == SEEK_END) {
        m_position = std::min(u64(std::max(s64(0), s64(m_file_size) + offset)), m_file_size);
    } else {
        return -1;
    }
    return m_position;
}

} // namespace Libraries::AvPlayer
