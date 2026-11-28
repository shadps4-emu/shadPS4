//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/memory/kernel.h"

namespace Core::Devtools::Widget {

class MemoryMapViewer {
    bool showing_vma = true;

    void DrawVirtualMemory();
    void DrawDirectMemory();

public:
    bool open = false;

    void Draw();
};

} // namespace Core::Devtools::Widget
