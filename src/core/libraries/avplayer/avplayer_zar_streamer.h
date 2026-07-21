// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>

#include "common/io_file.h"
#include "core/libraries/avplayer/avplayer_data_streamer.h"

namespace Libraries::AvPlayer {

class AvPlayerZarStreamer : public IDataStreamer {
public:
    ~AvPlayerZarStreamer();

    bool Init(std::string_view path) override;
    void Reset() override;

    AVIOContext* GetContext() override {
        return m_avio_context;
    }

private:
    static s32 ReadPacket(void* opaque, u8* buffer, s32 size);
    static s64 Seek(void* opaque, s64 offset, int whence);

    Common::FS::IOFile m_file;
    u64 m_position{};
    u64 m_file_size{};
    AVIOContext* m_avio_context{};
};

} // namespace Libraries::AvPlayer
