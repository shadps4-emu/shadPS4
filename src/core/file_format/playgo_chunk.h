// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <filesystem>
#include "common/types.h"

struct PlaygoHeader {
    u32 magic;

    u16 version_major;
    u16 version_minor;
    u16 image_count;
    u16 chunk_count;
    u16 mchunk_count;
    u16 scenario_count;
    // TODO fill the rest
};
class PlaygoChunk {
public:
    PlaygoChunk() = default;
    ~PlaygoChunk() = default;

    bool Open(const std::filesystem::path& filepath);
    PlaygoHeader GetPlaygoHeader() {
        return playgoHeader;
    }

private:
    PlaygoHeader playgoHeader;
};