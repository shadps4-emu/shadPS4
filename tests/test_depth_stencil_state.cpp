// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include <gtest/gtest.h>

#include "video_core/renderer_vulkan/vk_depth_stencil_state.h"

namespace {

std::unique_ptr<AmdGpu::Regs> MakeDepthRegs() {
    auto regs = std::make_unique<AmdGpu::Regs>();
    regs->depth_buffer.z_read_base = 1;
    regs->depth_buffer.z_write_base = 1;
    regs->depth_buffer.z_info.format = AmdGpu::DepthBuffer::ZFormat::Z32Float;
    regs->depth_control.depth_enable = 1;
    regs->depth_control.depth_func = AmdGpu::CompareFunc::Always;
    return regs;
}

void AddStencilTarget(AmdGpu::Regs& regs) {
    regs.depth_buffer.stencil_read_base = 1;
    regs.depth_buffer.stencil_write_base = 1;
    regs.depth_buffer.stencil_info.format = AmdGpu::DepthBuffer::StencilFormat::Stencil8;
    regs.depth_control.stencil_enable = 1;
    regs.depth_control.stencil_ref_func = AmdGpu::CompareFunc::Always;
}

TEST(DepthStencilStateTest, OmitsNoOpDepthAttachment) {
    const auto regs = MakeDepthRegs();
    const auto state = Vulkan::GetEffectiveDepthStencilState(*regs);

    EXPECT_FALSE(state.needs_attachment);
    EXPECT_FALSE(state.depth_test_enable);
    EXPECT_FALSE(state.depth_write_enable);
}

TEST(DepthStencilStateTest, ReadAndClearModesDoNotCreateWrites) {
    const auto regs = MakeDepthRegs();
    regs->depth_render_control.depth_clear_enable = 1;
    regs->depth_render_control.depth_compress_disable = 1;
    regs->depth_render_override.force_z_read = 1;
    regs->depth_render_override.preserve_compression = 1;

    EXPECT_FALSE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);
}

TEST(DepthStencilStateTest, KeepsDepthThatCanAffectRendering) {
    auto regs = MakeDepthRegs();

    regs->depth_control.depth_func = AmdGpu::CompareFunc::Less;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);

    regs = MakeDepthRegs();
    regs->depth_control.depth_write_enable = 1;
    const auto write_state = Vulkan::GetEffectiveDepthStencilState(*regs);
    EXPECT_TRUE(write_state.needs_attachment);
    EXPECT_TRUE(write_state.depth_write_enable);

    regs = MakeDepthRegs();
    regs->depth_control.depth_bounds_enable = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);

    regs = MakeDepthRegs();
    regs->depth_control.disable_color_writes_on_depth_pass = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);
}

TEST(DepthStencilStateTest, AccountsForDepthColorWriteOverrides) {
    auto regs = MakeDepthRegs();

    regs->depth_control.enable_color_writes_on_depth_fail = 1;
    EXPECT_FALSE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);

    regs = MakeDepthRegs();
    regs->depth_control.depth_func = AmdGpu::CompareFunc::Never;
    regs->depth_control.enable_color_writes_on_depth_fail = 1;
    regs->depth_control.disable_color_writes_on_depth_pass = 1;
    EXPECT_FALSE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);

    regs = MakeDepthRegs();
    regs->depth_control.depth_func = AmdGpu::CompareFunc::Less;
    regs->depth_control.enable_color_writes_on_depth_fail = 1;
    EXPECT_FALSE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);
}

TEST(DepthStencilStateTest, OmitsNoOpStencilAttachment) {
    const auto regs = MakeDepthRegs();
    AddStencilTarget(*regs);

    const auto state = Vulkan::GetEffectiveDepthStencilState(*regs);
    EXPECT_FALSE(state.needs_attachment);
    EXPECT_FALSE(state.stencil_test_enable);
}

TEST(DepthStencilStateTest, KeepsStencilThatCanAffectRendering) {
    auto regs = MakeDepthRegs();
    AddStencilTarget(*regs);
    regs->depth_control.stencil_ref_func = AmdGpu::CompareFunc::Less;

    const auto state = Vulkan::GetEffectiveDepthStencilState(*regs);
    EXPECT_TRUE(state.needs_attachment);
    EXPECT_TRUE(state.stencil_test_enable);

    regs = MakeDepthRegs();
    AddStencilTarget(*regs);
    regs->stencil_control.stencil_zpass_front = AmdGpu::StencilFunc::ReplaceTest;
    regs->stencil_ref_front.stencil_write_mask = 0xff;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);

    regs->stencil_ref_front.stencil_write_mask = 0;
    EXPECT_FALSE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);

    regs = MakeDepthRegs();
    AddStencilTarget(*regs);
    regs->depth_control.backface_enable = 1;
    regs->depth_control.stencil_bf_func = AmdGpu::CompareFunc::Less;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);
}

TEST(DepthStencilStateTest, PoliciesDoNotCreateMaintenanceWork) {
    const auto regs = MakeDepthRegs();
    regs->depth_render_control.depth_copy = 1;
    regs->depth_render_control.stencil_copy = 1;
    regs->depth_render_control.decompress_enable = 1;
    regs->depth_render_control.resummarize_enable = 1;
    regs->depth_render_override.force_z_dirty = 1;
    regs->depth_render_override.force_z_limit_summ = AmdGpu::ForceSumm::MinZ;

    EXPECT_FALSE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);
}

TEST(DepthStencilStateTest, KeepsEffectiveMaintenanceOperations) {
    auto regs = MakeDepthRegs();
    regs->depth_render_control.resummarize_enable = 1;
    regs->depth_render_override.force_z_valid = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);

    regs = MakeDepthRegs();
    regs->depth_render_override.force_z_valid = 1;
    regs->depth_render_override.force_z_dirty = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);

    regs = MakeDepthRegs();
    regs->depth_render_override.force_z_valid = 1;
    regs->depth_render_override.force_z_limit_summ = AmdGpu::ForceSumm::MinZ;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);

    regs = MakeDepthRegs();
    AddStencilTarget(*regs);
    regs->depth_control.stencil_enable = 0;
    regs->depth_render_override.force_stencil_valid = 1;
    regs->depth_render_override.force_stencil_dirty = 1;
    EXPECT_TRUE(Vulkan::GetEffectiveDepthStencilState(*regs).needs_attachment);
}

} // namespace
