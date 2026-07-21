// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::AvPlayer {

class IDataStreamer {
public:
    virtual ~IDataStreamer() = default;

    virtual void Reset() = 0;
    virtual s32 Read(u8* buffer, s32 size) = 0;
    virtual s64 Seek(s64 offset, int whence) = 0;
    virtual u64 GetSize() const = 0;
};

} // namespace Libraries::AvPlayer
