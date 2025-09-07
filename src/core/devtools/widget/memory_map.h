//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/memory.h"

namespace Core::Devtools::Widget {

class MemoryMapViewer {
    struct Iterator {
        bool is_vma;
        struct {
            MemoryManager::DMemMap::iterator it;
            MemoryManager::DMemMap::iterator end;
        } dmem;
        struct {
            MemoryManager::VMAMap::iterator it;
            MemoryManager::VMAMap::iterator end;
        } vma;

        bool DrawLine();
    };

    bool showing_vma = true;

public:
    bool open = false;

    void Draw();
};

} // namespace Core::Devtools::Widget
