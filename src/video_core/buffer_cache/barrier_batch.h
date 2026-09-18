// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/buffer_cache/interval_set.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"

namespace VideoCore {

struct BarrierBatch {
    BarrierBatch() {
        memory_barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
        memory_barrier.dstAccessMask =
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    }

    void IsRegionAccessedAndFlush(VAddr device_addr, u64 size, Vulkan::Scheduler& scheduler,
                                  bool check_read_access = false) {
        IsRegionAccessed(device_addr, size, check_read_access);
        FlushBarriers(scheduler);
    }

    void IsRegionAccessed(VAddr device_addr, u64 size, bool check_read_access = false) {
        const VAddr end_addr = device_addr + size;
        needs_barrier |= write_ranges.Overlaps(device_addr, end_addr);
        if (check_read_access && !needs_barrier) {
            needs_barrier |= read_ranges.Overlaps(device_addr, end_addr);
        }
    }

    void AccessMemory(VAddr device_addr, u64 size, vk::PipelineStageFlags2 src_stage,
                      vk::AccessFlags2 src_access) {
        const Interval range = {
            .start = device_addr,
            .end = device_addr + size,
        };

        constexpr static vk::AccessFlags2 READ_MASK =
            vk::AccessFlagBits2::eIndexRead | vk::AccessFlagBits2::eVertexAttributeRead |
            vk::AccessFlagBits2::eUniformRead | vk::AccessFlagBits2::eShaderRead |
            vk::AccessFlagBits2::eColorAttachmentRead |
            vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eTransferRead |
            vk::AccessFlagBits2::eMemoryRead;

        constexpr static vk::AccessFlags2 WRITE_MASK =
            vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eColorAttachmentWrite |
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite |
            vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eMemoryWrite;

        if (src_access & WRITE_MASK) {
            write_ranges.Add(range);
        }
        if (src_access & READ_MASK) {
            read_ranges.Add(range);
        }

        memory_barrier.srcStageMask |= src_stage;
        memory_barrier.srcAccessMask |= src_access;
    }

    void FlushBarriers(Vulkan::Scheduler& scheduler) {
        if (!needs_barrier || !memory_barrier.srcStageMask) {
            return;
        }

        vk::DependencyInfo dep_info{};
        dep_info.pMemoryBarriers = &memory_barrier;
        dep_info.memoryBarrierCount = 1U;

        scheduler.EndRendering();
        const auto cmdbuf = scheduler.CommandBuffer();
        cmdbuf.pipelineBarrier2(dep_info);

        memory_barrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
        memory_barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
        read_ranges.Clear();
        write_ranges.Clear();
    }

private:
    using AccessList = IntervalList<Interval>;
    AccessList read_ranges;
    AccessList write_ranges;
    vk::MemoryBarrier2 memory_barrier{};
    bool needs_barrier{};
};

} // namespace VideoCore
