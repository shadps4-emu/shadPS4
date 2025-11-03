// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "avplayer.h"

#include "common/types.h"

#include <string_view>

struct AVIOContext;

namespace Libraries::AvPlayer {

class IDataStreamer {
public:
    virtual ~IDataStreamer() = default;
    virtual bool Init(std::string_view path) = 0;
    virtual void Reset() = 0;
    virtual AVIOContext* GetContext() = 0;
};

} // namespace Libraries::AvPlayer
