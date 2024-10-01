//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/debug_state.h"

namespace Core::Devtools::Widget {

class FrameDumpViewer {
    DebugStateType::FrameDump frame_dump;
    int id;

public:
    bool is_open = true;

    explicit FrameDumpViewer(DebugStateType::FrameDump frame_dump);

    void Draw();
};

} // namespace Core::Devtools::Widget