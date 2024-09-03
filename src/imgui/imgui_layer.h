// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace ImGui {

class Layer {
public:
    virtual ~Layer() = default;
    static void AddLayer(Layer* layer);
    static void RemoveLayer(Layer* layer);

    virtual void Draw() = 0;

    virtual bool ShouldGrabGamepad() {
        return false;
    }
};

} // namespace ImGui