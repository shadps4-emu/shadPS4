// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

struct AVIOContext;

namespace Libraries::AvPlayer {

class IDataStreamer;

class AvioContext {
public:
    AvioContext() = default;
    ~AvioContext();

    AvioContext(const AvioContext&) = delete;
    AvioContext& operator=(const AvioContext&) = delete;

    bool Init(IDataStreamer& streamer);

    AVIOContext* Get() const {
        return m_context;
    }

private:
    static s32 ReadPacket(void* opaque, u8* buffer, s32 size);
    static s64 Seek(void* opaque, s64 offset, int whence);

    AVIOContext* m_context{};
};

} // namespace Libraries::AvPlayer
