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

    /// Whether this layer currently needs host-only redraws when no guest frame arrived.
    /// Transient layers default to active; permanent layers should report their visibility.
    virtual bool NeedsRender() const {
        return true;
    }
};

} // namespace ImGui
