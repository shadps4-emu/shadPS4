// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "core/libraries/avplayer/avplayer_handle_streamer.h"

extern "C" {
#include <libavformat/avio.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

constexpr u32 AVPLAYER_AVIO_BUFFER_SIZE = 4096;

namespace Libraries::AvPlayer {

AvPlayerHandleStreamer::AvPlayerHandleStreamer(std::unique_ptr<Core::FileSys::IFile> handle)
    : m_handle(std::move(handle)) {}

AvPlayerHandleStreamer::~AvPlayerHandleStreamer() {
    if (m_avio_context != nullptr) {
        avio_context_free(&m_avio_context);
    }
}

bool AvPlayerHandleStreamer::Init(std::string_view /*path*/) {
    if (!m_handle || !m_handle->IsOpen()) {
        return false;
    }
    m_file_size = m_handle->Size();

    const auto avio_buffer = reinterpret_cast<u8*>(av_malloc(AVPLAYER_AVIO_BUFFER_SIZE));
    m_avio_context = avio_alloc_context(avio_buffer, AVPLAYER_AVIO_BUFFER_SIZE,
                                        /*write_flag=*/0, this, &AvPlayerHandleStreamer::ReadPacket,
                                        /*write_packet=*/nullptr, &AvPlayerHandleStreamer::Seek);
    return m_avio_context != nullptr;
}

void AvPlayerHandleStreamer::Reset() {
    m_position = 0;
    if (m_handle) {
        m_handle->Seek(0, Common::FS::SeekOrigin::SetOrigin);
    }
}

s32 AvPlayerHandleStreamer::ReadPacket(void* opaque, u8* buffer, s32 size) {
    auto* const self = reinterpret_cast<AvPlayerHandleStreamer*>(opaque);
    if (self->m_position >= self->m_file_size) {
        return AVERROR_EOF;
    }
    if (self->m_position + static_cast<u64>(size) > self->m_file_size) {
        size = static_cast<s32>(self->m_file_size - self->m_position);
    }
    self->m_handle->Seek(static_cast<s64>(self->m_position), Common::FS::SeekOrigin::SetOrigin);
    const s64 got = self->m_handle->Read(buffer, static_cast<u64>(size));
    if (got <= 0) {
        return AVERROR_EOF;
    }
    self->m_position += static_cast<u64>(got);
    return static_cast<s32>(got);
}

s64 AvPlayerHandleStreamer::Seek(void* opaque, s64 offset, int whence) {
    auto* const self = reinterpret_cast<AvPlayerHandleStreamer*>(opaque);
    if ((whence & AVSEEK_SIZE) != 0) {
        return static_cast<s64>(self->m_file_size);
    }
    const int base = whence & ~AVSEEK_FORCE;
    u64 new_pos = self->m_position;
    if (base == SEEK_SET) {
        new_pos = static_cast<u64>(std::max<s64>(0, offset));
    } else if (base == SEEK_CUR) {
        new_pos = static_cast<u64>(std::max<s64>(0, static_cast<s64>(self->m_position) + offset));
    } else if (base == SEEK_END) {
        new_pos = static_cast<u64>(std::max<s64>(0, static_cast<s64>(self->m_file_size) + offset));
    } else {
        return -1;
    }
    new_pos = std::min(new_pos, self->m_file_size);
    self->m_position = new_pos;
    return static_cast<s64>(new_pos);
}

} // namespace Libraries::AvPlayer
