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
    const bool depth_can_pass = control.depth_func != AmdGpu::CompareFunc::Never;
    const bool depth_can_fail = control.depth_func != AmdGpu::CompareFunc::Always;
    const bool depth_can_reject =
        depth_enabled && ((depth_can_fail && !control.enable_color_writes_on_depth_fail) ||
                          (depth_can_pass && control.disable_color_writes_on_depth_pass));
    const bool depth_bounds = depth_valid && control.depth_bounds_enable;

    const auto stencil_op_writes = [](AmdGpu::StencilFunc fail, AmdGpu::StencilFunc zpass,
                                      AmdGpu::StencilFunc zfail, u8 write_mask) {
        return write_mask != 0 &&
               (fail != AmdGpu::StencilFunc::Keep || zpass != AmdGpu::StencilFunc::Keep ||
                zfail != AmdGpu::StencilFunc::Keep);
    };
    const bool stencil_can_reject =
        stencil_enabled &&
        (control.stencil_ref_func != AmdGpu::CompareFunc::Always ||
         (control.backface_enable && control.stencil_bf_func != AmdGpu::CompareFunc::Always));
    const bool stencil_can_write =
        stencil_enabled &&
        (stencil_op_writes(
             regs.stencil_control.stencil_fail_front, regs.stencil_control.stencil_zpass_front,
             regs.stencil_control.stencil_zfail_front, regs.stencil_ref_front.stencil_write_mask) ||
         (control.backface_enable && stencil_op_writes(regs.stencil_control.stencil_fail_back,
                                                       regs.stencil_control.stencil_zpass_back,
                                                       regs.stencil_control.stencil_zfail_back,
                                                       regs.stencil_ref_back.stencil_write_mask)));
    const bool stencil_has_effect = stencil_can_reject || stencil_can_write;

    // DEPTH_COPY/STENCIL_COPY are compression write-back policies in Gnm, and
    // FORCE_DEPTH_DECOMPRESS is unsupported on PS4 hardware. Effective maintenance work needs a
    // forced read of valid data before it can update the surface or its HTILE metadata.
    const bool depth_maintenance =
        depth_valid && render_override.force_z_valid &&
        (render_control.resummarize_enable || render_override.force_z_dirty ||
         render_override.force_z_limit_summ != AmdGpu::ForceSumm::Off);
    const bool stencil_maintenance =
        stencil_valid && render_override.force_stencil_valid && render_override.force_stencil_dirty;

    const bool needs_depth = depth_write || depth_can_reject || depth_bounds || depth_maintenance;
    const bool needs_stencil = stencil_has_effect || stencil_maintenance;
    const bool needs_attachment = needs_depth || needs_stencil;

    return {
        .needs_attachment = needs_attachment,
        .depth_test_enable = needs_attachment && depth_enabled,
        .depth_write_enable = needs_attachment && depth_write,
        .depth_bounds_enable = needs_attachment && depth_bounds,
        .stencil_test_enable = needs_attachment && stencil_has_effect,
    };
}

} // namespace Vulkan
