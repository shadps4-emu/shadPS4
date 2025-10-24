// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/amdgpu/regs.h"

namespace AmdGpu {

// The following values are taken from fpPS4:
// https://github.com/red-prig/fpPS4/blob/436b43064be4c78229500f3d3c054fc76639247d/chip/pm4_pfp.pas#L410
static constexpr std::array REG_ARRAY_DEFAULT = {
    0x00000000u, 0x80000000u, 0x40004000u, 0xdeadbeefu, 0x00000000u, 0x40004000u, 0x00000000u,
    0x40004000u, 0x00000000u, 0x40004000u, 0x00000000u, 0x40004000u, 0xaa99aaaau, 0x00000000u,
    0xdeadbeefu, 0xdeadbeefu, 0x80000000u, 0x40004000u, 0x00000000u, 0x00000000u, 0x80000000u,
    0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u,
    0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u,
    0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u,
    0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u, 0x40004000u, 0x80000000u,
    0x40004000u, 0x80000000u, 0x40004000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u,
    0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u,
    0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u,
    0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u,
    0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u,
    0x2a00161au,
};

void Regs::SetDefaults() {
    std::memset(reg_array.data(), 0, reg_array.size() * sizeof(u32));

    std::memcpy(&reg_array[ContextRegWordOffset + 0x80], REG_ARRAY_DEFAULT.data(),
                REG_ARRAY_DEFAULT.size() * sizeof(u32));

    // Individual context regs values
    reg_array[ContextRegWordOffset + 0x000d] = 0x40004000u;
    reg_array[ContextRegWordOffset + 0x01b6] = 0x00000002u;
    reg_array[ContextRegWordOffset + 0x0204] = 0x00090000u;
    reg_array[ContextRegWordOffset + 0x0205] = 0x00000004u;
    reg_array[ContextRegWordOffset + 0x0295] = 0x00000100u;
    reg_array[ContextRegWordOffset + 0x0296] = 0x00000080u;
    reg_array[ContextRegWordOffset + 0x0297] = 0x00000002u;
    reg_array[ContextRegWordOffset + 0x02aa] = 0x00001000u;
    reg_array[ContextRegWordOffset + 0x02f7] = 0x00001000u;
    reg_array[ContextRegWordOffset + 0x02f9] = 0x00000005u;
    reg_array[ContextRegWordOffset + 0x02fa] = 0x3f800000u;
    reg_array[ContextRegWordOffset + 0x02fb] = 0x3f800000u;
    reg_array[ContextRegWordOffset + 0x02fc] = 0x3f800000u;
    reg_array[ContextRegWordOffset + 0x02fd] = 0x3f800000u;
    reg_array[ContextRegWordOffset + 0x0316] = 0x0000000eu;
    reg_array[ContextRegWordOffset + 0x0317] = 0x00000010u;
}

#define GFX6_3D_REG_INDEX(field_name) (offsetof(AmdGpu::Regs, field_name) / sizeof(u32))

static_assert(GFX6_3D_REG_INDEX(ps_program) == 0x2C08);
static_assert(GFX6_3D_REG_INDEX(vs_program) == 0x2C48);
static_assert(GFX6_3D_REG_INDEX(vs_program.user_data) == 0x2C4C);
static_assert(GFX6_3D_REG_INDEX(gs_program) == 0x2C88);
static_assert(GFX6_3D_REG_INDEX(es_program) == 0x2CC8);
static_assert(GFX6_3D_REG_INDEX(hs_program) == 0x2D08);
static_assert(GFX6_3D_REG_INDEX(ls_program) == 0x2D48);
static_assert(GFX6_3D_REG_INDEX(cs_program) == 0x2E00);
static_assert(GFX6_3D_REG_INDEX(cs_program.dim_z) == 0x2E03);
static_assert(GFX6_3D_REG_INDEX(cs_program.user_data) == 0x2E40);
static_assert(GFX6_3D_REG_INDEX(depth_render_control) == 0xA000);
static_assert(GFX6_3D_REG_INDEX(depth_view) == 0xA002);
static_assert(GFX6_3D_REG_INDEX(depth_htile_data_base) == 0xA005);
static_assert(GFX6_3D_REG_INDEX(screen_scissor) == 0xA00C);
static_assert(GFX6_3D_REG_INDEX(depth_buffer.z_info) == 0xA010);
static_assert(GFX6_3D_REG_INDEX(depth_buffer.depth_slice) == 0xA017);
static_assert(GFX6_3D_REG_INDEX(ta_bc_base) == 0xA020);
static_assert(GFX6_3D_REG_INDEX(window_offset) == 0xA080);
static_assert(GFX6_3D_REG_INDEX(window_scissor) == 0xA081);
static_assert(GFX6_3D_REG_INDEX(color_target_mask) == 0xA08E);
static_assert(GFX6_3D_REG_INDEX(color_shader_mask) == 0xA08F);
static_assert(GFX6_3D_REG_INDEX(generic_scissor) == 0xA090);
static_assert(GFX6_3D_REG_INDEX(viewport_scissors) == 0xA094);
static_assert(GFX6_3D_REG_INDEX(index_offset) == 0xA102);
static_assert(GFX6_3D_REG_INDEX(primitive_restart_index) == 0xA103);
static_assert(GFX6_3D_REG_INDEX(stencil_control) == 0xA10B);
static_assert(GFX6_3D_REG_INDEX(viewports) == 0xA10F);
static_assert(GFX6_3D_REG_INDEX(clip_user_data) == 0xA16F);
static_assert(GFX6_3D_REG_INDEX(ps_inputs) == 0xA191);
static_assert(GFX6_3D_REG_INDEX(vs_output_config) == 0xA1B1);
static_assert(GFX6_3D_REG_INDEX(ps_input_ena) == 0xA1B3);
static_assert(GFX6_3D_REG_INDEX(ps_input_addr) == 0xA1B4);
static_assert(GFX6_3D_REG_INDEX(shader_pos_format) == 0xA1C3);
static_assert(GFX6_3D_REG_INDEX(z_export_format) == 0xA1C4);
static_assert(GFX6_3D_REG_INDEX(color_export_format) == 0xA1C5);
static_assert(GFX6_3D_REG_INDEX(blend_control) == 0xA1E0);
static_assert(GFX6_3D_REG_INDEX(index_base_address) == 0xA1F9);
static_assert(GFX6_3D_REG_INDEX(draw_initiator) == 0xA1FC);
static_assert(GFX6_3D_REG_INDEX(depth_control) == 0xA200);
static_assert(GFX6_3D_REG_INDEX(color_control) == 0xA202);
static_assert(GFX6_3D_REG_INDEX(clipper_control) == 0xA204);
static_assert(GFX6_3D_REG_INDEX(viewport_control) == 0xA206);
static_assert(GFX6_3D_REG_INDEX(vs_output_control) == 0xA207);
static_assert(GFX6_3D_REG_INDEX(line_control) == 0xA282);
static_assert(GFX6_3D_REG_INDEX(hs_clamp) == 0xA287);
static_assert(GFX6_3D_REG_INDEX(vgt_gs_mode) == 0xA290);
static_assert(GFX6_3D_REG_INDEX(mode_control) == 0xA292);
static_assert(GFX6_3D_REG_INDEX(vgt_gs_out_prim_type) == 0xA29B);
static_assert(GFX6_3D_REG_INDEX(index_size) == 0xA29D);
static_assert(GFX6_3D_REG_INDEX(index_buffer_type) == 0xA29F);
static_assert(GFX6_3D_REG_INDEX(enable_primitive_id) == 0xA2A1);
static_assert(GFX6_3D_REG_INDEX(enable_primitive_restart) == 0xA2A5);
static_assert(GFX6_3D_REG_INDEX(vgt_instance_step_rate_0) == 0xA2A8);
static_assert(GFX6_3D_REG_INDEX(vgt_instance_step_rate_1) == 0xA2A9);
static_assert(GFX6_3D_REG_INDEX(vgt_esgs_ring_itemsize) == 0xA2AB);
static_assert(GFX6_3D_REG_INDEX(vgt_gsvs_ring_itemsize) == 0xA2AC);
static_assert(GFX6_3D_REG_INDEX(stage_enable) == 0xA2D5);
static_assert(GFX6_3D_REG_INDEX(vgt_gs_vert_itemsize[0]) == 0xA2D7);
static_assert(GFX6_3D_REG_INDEX(tess_config) == 0xA2DB);
static_assert(GFX6_3D_REG_INDEX(poly_offset) == 0xA2DF);
static_assert(GFX6_3D_REG_INDEX(vgt_gs_instance_cnt) == 0xA2E4);
static_assert(GFX6_3D_REG_INDEX(vgt_strmout_config) == 0xA2E5);
static_assert(GFX6_3D_REG_INDEX(vgt_strmout_buffer_config) == 0xA2E6);
static_assert(GFX6_3D_REG_INDEX(aa_config) == 0xA2F8);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].base_address) == 0xA318);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].pitch) == 0xA319);
static_assert(GFX6_3D_REG_INDEX(color_buffers[0].slice) == 0xA31A);
static_assert(GFX6_3D_REG_INDEX(color_buffers[7].base_address) == 0xA381);
static_assert(GFX6_3D_REG_INDEX(cp_strmout_cntl) == 0xC03F);
static_assert(GFX6_3D_REG_INDEX(primitive_type) == 0xC242);
static_assert(GFX6_3D_REG_INDEX(num_instances) == 0xC24D);
static_assert(GFX6_3D_REG_INDEX(vgt_tf_memory_base) == 0xc250);

#undef GFX6_3D_REG_INDEX

} // namespace AmdGpu
