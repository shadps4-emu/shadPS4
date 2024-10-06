//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "core/debug_state.h"

namespace Core::Devtools::Widget {

class RegView {
    int id;

public:
    bool open = false;

    DebugStateType::RegDump data;

    RegView();

    void Draw();
};

} // namespace Core::Devtools::Widget