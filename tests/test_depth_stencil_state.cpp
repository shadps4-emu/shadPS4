// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "video_core/renderer_vulkan/vk_depth_stencil_state.h"

namespace {

AmdGpu::Regs MakeDepthRegs() {
    AmdGpu::Regs regs{};
    regs.depth_buffer.z_read_base = 1;
    regs.depth_buffer.z_write_base = 1;
    regs.depth_buffer.z_info.format = AmdGpu::DepthBuffer::ZFormat::Z32Float;
    regs.depth_control.depth_enable = 1;
    regs.depth_control.depth_func = AmdGpu::CompareFunc::Always;
    return regs;
}

TEST(DepthStencilStateTest, OmitsNoOpDepthAttachment) {
    const auto state = Vulkan::GetEffectiveDepthStencilState(MakeDepthRegs());

    EXPECT_FALSE(state.needs_attachment);
    EXPECT_FALSE(state.depth_test_enable);
    EXPECT_FALSE(state.depth_write_enable);
}

TEST(DepthStencilStateTest, ReadAndClearModesDoNotCreateWrites) {
    auto regs = MakeDepthRegs();
    regs.depth_render_control.depth_clear_enable = 1;
    regs.depth_render_control.depth_compress_disable = 1;
    regs.depth_render_override.force_z_read = 1;
    regs.depth_render_override.preserve_compression = 1;

    EXPECT_FALSE(Vulkan::GetEffectiveDepthStencilState(regs).needs_attachment);
}

TEST(DepthStencilStateTest, KeepsDepthThatCanAffectRendering) {
    auto regs = MakeDepthRegs();

    regs.depth_control.depth_func = AmdGpu::CompareFunc::Less;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(regs).needs_attachment);

    regs = MakeDepthRegs();
    regs.depth_control.depth_write_enable = 1;
    const auto write_state = Vulkan::GetEffectiveDepthStencilState(regs);
    EXPECT_TRUE(write_state.needs_attachment);
    EXPECT_TRUE(write_state.depth_write_enable);

    regs = MakeDepthRegs();
    regs.depth_control.depth_bounds_enable = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(regs).needs_attachment);

    regs = MakeDepthRegs();
    regs.depth_control.disable_color_writes_on_depth_pass = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(regs).needs_attachment);
}

TEST(DepthStencilStateTest, KeepsStencilAttachment) {
    auto regs = MakeDepthRegs();
    regs.depth_buffer.stencil_read_base = 1;
    regs.depth_buffer.stencil_write_base = 1;
    regs.depth_buffer.stencil_info.format = AmdGpu::DepthBuffer::StencilFormat::Stencil8;
    regs.depth_control.stencil_enable = 1;

    const auto state = Vulkan::GetEffectiveDepthStencilState(regs);
    EXPECT_TRUE(state.needs_attachment);
    EXPECT_TRUE(state.stencil_test_enable);
}

TEST(DepthStencilStateTest, KeepsDepthMaintenanceOperations) {
    auto regs = MakeDepthRegs();
    regs.depth_render_control.depth_copy = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(regs).needs_attachment);

    regs = MakeDepthRegs();
    regs.depth_render_control.resummarize_enable = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(regs).needs_attachment);

    regs = MakeDepthRegs();
    regs.depth_render_control.decompress_enable = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(regs).needs_attachment);

    regs = MakeDepthRegs();
    regs.depth_render_override.force_z_dirty = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(regs).needs_attachment);

    regs = MakeDepthRegs();
    regs.depth_render_override.force_z_limit_summ = AmdGpu::ForceSumm::MinZ;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(regs).needs_attachment);
}

} // namespace
