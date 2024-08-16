// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "avplayer.h"

#include "common/types.h"

struct AVIOContext;

namespace Libraries::AvPlayer {

class IDataStreamer {
public:
    virtual ~IDataStreamer() = default;
    virtual AVIOContext* GetContext() = 0;
};

} // namespace Libraries::AvPlayer
