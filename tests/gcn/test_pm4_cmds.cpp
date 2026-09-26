// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "video_core/amdgpu/pm4_cmds.h"

namespace {

AmdGpu::PM4CmdReleaseMem ReleaseCommand(AmdGpu::DataSelect data_select,
                                        AmdGpu::InterruptSelect interrupt_select, u32* target) {
    AmdGpu::PM4CmdReleaseMem command{AmdGpu::PM4Type3Header{AmdGpu::PM4ItOpcode::ReleaseMem, 5}};
    command.dw2 =
        (static_cast<u32>(data_select) << 29) | (static_cast<u32>(interrupt_select) << 24);
    const u64 address = reinterpret_cast<u64>(target);
    command.address_lo = static_cast<u32>(address);
    command.address_hi = static_cast<u32>(address >> 32);
    return command;
}

} // namespace

TEST(PM4CmdReleaseMem, NoDataAndNoInterrupt) {
    u32 memory = 0x12345678;
    auto command = ReleaseCommand(AmdGpu::DataSelect::None, AmdGpu::InterruptSelect::None, &memory);
    int irq_count = 0;
    bool gds_copied = false;
    command.SignalFence([&irq_count] { ++irq_count; },
                        [&gds_copied](VAddr, u32, u32) { gds_copied = true; });

    EXPECT_EQ(memory, 0x12345678u);
    EXPECT_EQ(irq_count, 0);
    EXPECT_FALSE(gds_copied);
}

TEST(PM4CmdReleaseMem, IrqOnlySignalsWithoutWritingData) {
    u32 memory = 0x12345678;
    auto command =
        ReleaseCommand(AmdGpu::DataSelect::None, AmdGpu::InterruptSelect::IrqOnly, &memory);

    int irq_count = 0;
    bool gds_copied = false;
    command.SignalFence([&irq_count] { ++irq_count; },
                        [&gds_copied](VAddr, u32, u32) { gds_copied = true; });

    EXPECT_EQ(memory, 0x12345678u);
    EXPECT_EQ(irq_count, 1);
    EXPECT_FALSE(gds_copied);
}

TEST(PM4CmdReleaseMem, Data32LowWritesAndSignalsOnConfirmation) {
    u32 memory = 0x12345678;
    auto command = ReleaseCommand(AmdGpu::DataSelect::Data32Low,
                                  AmdGpu::InterruptSelect::IrqWhenWriteConfirm, &memory);
    command.data_lo = 0xabcdef12;
    int irq_count = 0;
    bool gds_copied = false;
    command.SignalFence([&irq_count] { ++irq_count; },
                        [&gds_copied](VAddr, u32, u32) { gds_copied = true; });

    EXPECT_EQ(memory, 0xabcdef12u);
    EXPECT_EQ(irq_count, 1);
    EXPECT_FALSE(gds_copied);
}
