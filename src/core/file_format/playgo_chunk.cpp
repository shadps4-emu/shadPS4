// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/io_file.h"

#include "playgo_chunk.h"

bool PlaygoChunk::Open(const std::filesystem::path& filepath) {
    Common::FS::IOFile file(filepath, Common::FS::FileAccessMode::Read);
    if (!file.IsOpen()) {
        return false;
    }
    file.Read(playgoHeader);

    return true;
}