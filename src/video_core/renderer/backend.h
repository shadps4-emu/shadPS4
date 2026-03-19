// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace VideoCore::Render {

enum class BackendKind {
    Vulkan,
    Headless,
};

struct BackendCapabilities {
    bool supports_hdr{};
    bool supports_shader_debug{};
    bool supports_overlay_ui{};
};

struct DisplaySettings {
    float gamma{1.0f};
    bool fsr_enabled{};
    bool rcas_enabled{};
    float rcas_attenuation{};
};

} // namespace VideoCore::Render
