// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdio>
#include <utility>

#include "core/libraries/avplayer/avplayer_zar_streamer.h"

namespace Libraries::AvPlayer {

AvPlayerZarStreamer::AvPlayerZarStreamer(std::filesystem::path path) : m_path(std::move(path)) {}

bool AvPlayerZarStreamer::Init() {
    if (m_file.Open(m_path, Common::FS::FileAccessMode::Read) != 0) {
        return false;
    }
    m_file_size = m_file.GetSize();
    return true;
}

void AvPlayerZarStreamer::Reset() {
    m_position = 0;
    m_file.Seek(0);
}

s32 AvPlayerZarStreamer::Read(u8* buffer, s32 size) {
    if (m_position >= m_file_size) {
        return 0;
    }
    if (m_position + size > m_file_size) {
        size = m_file_size - m_position;
    }
    const auto bytes_read = static_cast<s32>(m_file.ReadRaw<u8>(buffer, size));
    m_position += bytes_read;
    return bytes_read;
}

s64 AvPlayerZarStreamer::Seek(s64 offset, int whence) {
    u64 position;
    if (whence == SEEK_CUR) {
        position = std::min(u64(std::max(s64(0), s64(m_position) + offset)), m_file_size);
    } else if (whence == SEEK_SET) {
        position = std::min(u64(std::max(s64(0), offset)), m_file_size);
    } else if (whence == SEEK_END) {
        position = std::min(u64(std::max(s64(0), s64(m_file_size) + offset)), m_file_size);
    } else {
        return -1;
    }

    if (!m_file.Seek(static_cast<s64>(position))) {
        return -1;
    }
    m_position = position;
    return position;
}

} // namespace Libraries::AvPlayer
