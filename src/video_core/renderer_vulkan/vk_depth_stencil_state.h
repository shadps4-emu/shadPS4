// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/amdgpu/regs.h"

namespace Vulkan {

struct EffectiveDepthStencilState {
    bool needs_attachment{};
    bool depth_test_enable{};
    bool depth_write_enable{};
    bool depth_bounds_enable{};
    bool stencil_test_enable{};
};

[[nodiscard]] constexpr EffectiveDepthStencilState GetEffectiveDepthStencilState(
    const AmdGpu::Regs& regs) {
    const auto& control = regs.depth_control;
    const auto& render_control = regs.depth_render_control;
    const auto& render_override = regs.depth_render_override;

    const bool depth_valid = regs.depth_buffer.DepthValid();
    const bool stencil_valid = regs.depth_buffer.StencilValid();
    const bool depth_enabled = control.depth_enable && depth_valid;
    const bool stencil_enabled = control.stencil_enable && stencil_valid;

    const bool depth_write = depth_enabled && control.depth_write_enable;
    const bool depth_can_reject =
        depth_enabled &&
        (control.depth_func != AmdGpu::CompareFunc::Always ||
         control.enable_color_writes_on_depth_fail || control.disable_color_writes_on_depth_pass);
    const bool depth_bounds = depth_valid && control.depth_bounds_enable;

    // These operations modify depth/stencil data or metadata independently of an ordinary
    // comparison. Read/valid overrides alone are not observable when no test or write occurs.
    const bool depth_maintenance =
        depth_valid && (render_control.depth_copy || render_control.resummarize_enable ||
                        render_control.decompress_enable || render_override.force_z_dirty ||
                        render_override.force_z_limit_summ != AmdGpu::ForceSumm::Off);
    const bool stencil_maintenance =
        stencil_valid && (render_control.stencil_copy || render_override.force_stencil_dirty);

    const bool needs_depth = depth_write || depth_can_reject || depth_bounds || depth_maintenance;
    const bool needs_stencil = stencil_enabled || stencil_maintenance;
    const bool needs_attachment = needs_depth || needs_stencil;

    return {
        .needs_attachment = needs_attachment,
        .depth_test_enable = needs_attachment && depth_enabled,
        .depth_write_enable = needs_attachment && depth_write,
        .depth_bounds_enable = needs_attachment && depth_bounds,
        .stencil_test_enable = needs_attachment && stencil_enabled,
    };
}

} // namespace Vulkan
