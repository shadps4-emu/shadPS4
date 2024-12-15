// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/page_manager.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/texture_cache/texture_cache.h"

namespace AmdGpu {
struct Liverpool;
}

namespace Core {
class MemoryManager;
}

namespace Vulkan {

class Scheduler;
class RenderState;
class GraphicsPipeline;

class Rasterizer {
public:
    explicit Rasterizer(const Instance& instance, Scheduler& scheduler,
                        AmdGpu::Liverpool* liverpool);
    ~Rasterizer();

    [[nodiscard]] Scheduler& GetScheduler() noexcept {
        return scheduler;
    }

    [[nodiscard]] VideoCore::BufferCache& GetBufferCache() noexcept {
        return buffer_cache;
    }

    [[nodiscard]] VideoCore::TextureCache& GetTextureCache() noexcept {
        return texture_cache;
    }

    void Draw(bool is_indexed, u32 index_offset = 0);
    void DrawIndirect(bool is_indexed, VAddr arg_address, u32 offset, u32 size, u32 max_count,
                      VAddr count_address);

    void DispatchDirect();
    void DispatchIndirect(VAddr address, u32 offset, u32 size);

    void ScopeMarkerBegin(const std::string_view& str);
    void ScopeMarkerEnd();
    void ScopedMarkerInsert(const std::string_view& str);
    void ScopedMarkerInsertColor(const std::string_view& str, const u32 color);

    void InlineData(VAddr address, const void* value, u32 num_bytes, bool is_gds);
    u32 ReadDataFromGds(u32 gsd_offset);
    bool InvalidateMemory(VAddr addr, u64 size);
    bool IsMapped(VAddr addr, u64 size);
    void MapMemory(VAddr addr, u64 size);
    void UnmapMemory(VAddr addr, u64 size);

    void CpSync();
    u64 Flush();
    void Finish();

    PipelineCache& GetPipelineCache() {
        return pipeline_cache;
    }

private:
    RenderState PrepareRenderState(u32 mrt_mask);
    void BeginRendering(const GraphicsPipeline& pipeline, RenderState& state);
    void Resolve();

    void UpdateDynamicState(const GraphicsPipeline& pipeline);
    void UpdateViewportScissorState();

    bool FilterDraw();

    void BindBuffers(const Shader::Info& stage, Shader::Backend::Bindings& binding,
                     Shader::PushData& push_data, Pipeline::DescriptorWrites& set_writes,
                     Pipeline::BufferBarriers& buffer_barriers);

    void BindTextures(const Shader::Info& stage, Shader::Backend::Bindings& binding,
                      Pipeline::DescriptorWrites& set_writes);

    bool BindResources(const Pipeline* pipeline);
    void ResetBindings() {
        for (auto& image_id : bound_images) {
            texture_cache.GetImage(image_id).binding.Reset();
        }
        bound_images.clear();
    }

private:
    const Instance& instance;
    Scheduler& scheduler;
    VideoCore::PageManager page_manager;
    VideoCore::BufferCache buffer_cache;
    VideoCore::TextureCache texture_cache;
    AmdGpu::Liverpool* liverpool;
    Core::MemoryManager* memory;
    boost::icl::interval_set<VAddr> mapped_ranges;
    PipelineCache pipeline_cache;

    boost::container::static_vector<
        std::pair<VideoCore::ImageId, VideoCore::TextureCache::RenderTargetDesc>, 8>
        cb_descs;
    std::optional<std::pair<VideoCore::ImageId, VideoCore::TextureCache::DepthTargetDesc>> db_desc;
    boost::container::static_vector<vk::DescriptorImageInfo, 32> image_infos;
    boost::container::static_vector<vk::BufferView, 8> buffer_views;
    boost::container::static_vector<vk::DescriptorBufferInfo, 32> buffer_infos;
    boost::container::static_vector<VideoCore::ImageId, 64> bound_images;

    Pipeline::DescriptorWrites set_writes;
    Pipeline::BufferBarriers buffer_barriers;

    using BufferBindingInfo = std::pair<VideoCore::BufferId, AmdGpu::Buffer>;
    boost::container::static_vector<BufferBindingInfo, 32> buffer_bindings;
    using TexBufferBindingInfo = std::pair<VideoCore::BufferId, AmdGpu::Buffer>;
    boost::container::static_vector<TexBufferBindingInfo, 32> texbuffer_bindings;
    using ImageBindingInfo = std::pair<VideoCore::ImageId, VideoCore::TextureCache::TextureDesc>;
    boost::container::static_vector<ImageBindingInfo, 32> image_bindings;
};

} // namespace Vulkan
