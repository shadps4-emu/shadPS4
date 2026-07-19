// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <string_view>

#include "common/io_file.h"
#include "core/libraries/avplayer/avplayer.h"
#include "core/libraries/avplayer/avplayer_data_streamer.h"

struct AVIOContext;

namespace Libraries::AvPlayer {

class AvPlayerFileStreamer : public IDataStreamer {
public:
    explicit AvPlayerFileStreamer(const AvPlayerFileReplacement& file_replacement);
    explicit AvPlayerFileStreamer(std::filesystem::path path);
    ~AvPlayerFileStreamer();

    bool Init();
    bool Init(std::string_view path) override;
    void Reset() override;

    AVIOContext* GetContext() override {
        return m_avio_context;
    }

private:
    bool InitAvioContext();

    static s32 ReadPacket(void* opaque, u8* buffer, s32 size);
    static s64 Seek(void* opaque, s64 buffer, int whence);

    AvPlayerFileReplacement m_file_replacement{};
    std::filesystem::path m_path;
    Common::FS::IOFile m_file;

    int m_fd = -1;
    u64 m_position{};
    u64 m_file_size{};
    AVIOContext* m_avio_context{};
};

} // namespace Libraries::AvPlayer
