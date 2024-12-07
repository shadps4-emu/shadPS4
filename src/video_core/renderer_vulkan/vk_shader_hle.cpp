// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/info.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_hle.h"

#include "vk_rasterizer.h"

namespace Vulkan {

static constexpr u64 COPY_SHADER_HASH = 0xfefebf9f;

bool ExecuteCopyShaderHLE(const Shader::Info& info, const AmdGpu::Liverpool::Regs& regs,
                          Rasterizer& rasterizer) {
    auto& scheduler = rasterizer.GetScheduler();
    auto& buffer_cache = rasterizer.GetBufferCache();

    // Copy shader defines three formatted buffers as inputs: control, source, and destination.
    const auto ctl_buf_sharp = info.texture_buffers[0].GetSharp(info);
    const auto src_buf_sharp = info.texture_buffers[1].GetSharp(info);
    const auto dst_buf_sharp = info.texture_buffers[2].GetSharp(info);
    const auto buf_stride = src_buf_sharp.GetStride();
    ASSERT(buf_stride == dst_buf_sharp.GetStride());

    struct CopyShaderControl {
        u32 dst_idx;
        u32 src_idx;
        u32 end;
    };
    static_assert(sizeof(CopyShaderControl) == 12);
    ASSERT(ctl_buf_sharp.GetStride() == sizeof(CopyShaderControl));
    const auto ctl_buf = reinterpret_cast<CopyShaderControl*>(ctl_buf_sharp.base_address);

    // Add list of copies
    boost::container::set<vk::BufferCopy> copies;
    for (u32 i = 0; i < regs.cs_program.dim_x; i++) {
        const auto& [dst_idx, src_idx, end] = ctl_buf[i];
        const u32 local_dst_offset = dst_idx * buf_stride;
        const u32 local_src_offset = src_idx * buf_stride;
        const u32 local_size = (end + 1) * buf_stride;
        copies.emplace(local_src_offset, local_dst_offset, local_size);
    }

    scheduler.EndRendering();

    static constexpr vk::MemoryBarrier READ_BARRIER{
        .srcAccessMask = vk::AccessFlagBits::eMemoryWrite,
        .dstAccessMask = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite,
    };
    static constexpr vk::MemoryBarrier WRITE_BARRIER{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
    };
    scheduler.CommandBuffer().pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlagBits::eByRegion, READ_BARRIER, {}, {});

    static constexpr vk::DeviceSize MaxDistanceForMerge = 64_MB;
    boost::container::small_vector<vk::BufferCopy, 32> batch_copies;
    while (!copies.empty()) {
        // Place first copy into the current batch
        auto it = copies.begin();
        auto src_offset_min = it->srcOffset;
        auto src_offset_max = it->srcOffset + it->size;
        auto dst_offset_min = it->dstOffset;
        auto dst_offset_max = it->dstOffset + it->size;
        batch_copies.emplace_back(*it);
        it = copies.erase(it);

        while (it != copies.end()) {
            // Compute new src and dst bounds if we were to batch this copy
            auto new_src_offset_min = std::min(src_offset_min, it->srcOffset);
            auto new_src_offset_max = std::max(src_offset_max, it->srcOffset + it->size);
            if (new_src_offset_max - new_src_offset_min > MaxDistanceForMerge) {
                ++it;
                continue;
            }

            auto new_dst_offset_min = std::min(dst_offset_min, it->dstOffset);
            auto new_dst_offset_max = std::max(dst_offset_max, it->dstOffset + it->size);
            if (new_dst_offset_max - new_dst_offset_min > MaxDistanceForMerge) {
                ++it;
                continue;
            }

            // We can batch this copy
            src_offset_min = new_src_offset_min;
            src_offset_max = new_src_offset_max;
            dst_offset_min = new_dst_offset_min;
            dst_offset_max = new_dst_offset_max;
            batch_copies.emplace_back(*it);
            it = copies.erase(it);
        }

        // Obtain buffers for the total source and destination ranges.
        const auto [src_buf, src_buf_offset] =
            buffer_cache.ObtainBuffer(src_buf_sharp.base_address + src_offset_min,
                                      src_offset_max - src_offset_min, false, false);
        const auto [dst_buf, dst_buf_offset] =
            buffer_cache.ObtainBuffer(dst_buf_sharp.base_address + dst_offset_min,
                                      dst_offset_max - dst_offset_min, true, false);

        // Apply found buffer base.
        for (auto& vk_copy : batch_copies) {
            vk_copy.srcOffset = vk_copy.srcOffset - src_offset_min + src_buf_offset;
            vk_copy.dstOffset = vk_copy.dstOffset - dst_offset_min + dst_buf_offset;
        }

        // Execute buffer copies.
        LOG_TRACE(Render_Vulkan, "HLE buffer copy: src_size = {}, dst_size = {}",
                  src_offset_max - src_offset_min, dst_offset_max - dst_offset_min);
        scheduler.CommandBuffer().copyBuffer(src_buf->Handle(), dst_buf->Handle(), batch_copies);
        batch_copies.clear();
    }

    scheduler.CommandBuffer().pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlagBits::eByRegion, WRITE_BARRIER, {}, {});

    return true;
}

bool ExecuteShaderHLE(const Shader::Info& info, const AmdGpu::Liverpool::Regs& regs,
                      Rasterizer& rasterizer) {
    switch (info.pgm_hash) {
    case COPY_SHADER_HASH:
        return ExecuteCopyShaderHLE(info, regs, rasterizer);
    default:
        return false;
    }
}

} // namespace Vulkan
