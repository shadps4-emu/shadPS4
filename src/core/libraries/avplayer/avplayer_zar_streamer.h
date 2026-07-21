// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>

#include "common/io_file.h"
#include "core/libraries/avplayer/avplayer_data_streamer.h"

namespace Libraries::AvPlayer {

class AvPlayerZarStreamer : public IDataStreamer {
public:
    explicit AvPlayerZarStreamer(std::filesystem::path path);

    bool Init();
    void Reset() override;

private:
    s32 Read(u8* buffer, s32 size) override;
    s64 Seek(s64 offset, int whence) override;
    u64 GetSize() const override {
        return m_file_size;
    }

    std::filesystem::path m_path;
    Common::FS::IOFile m_file;
    u64 m_position{};
    u64 m_file_size{};
};

} // namespace Libraries::AvPlayer
