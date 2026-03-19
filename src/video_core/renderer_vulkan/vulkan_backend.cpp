// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/vulkan_backend.h"

#include "common/assert.h"
#include "common/config.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_presenter.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"

namespace Vulkan {

class VulkanBackend::FrameHandle final : public VideoCore::Render::IFrameHandle {
public:
    explicit FrameHandle(Frame* frame_) : frame(frame_) {}

    Frame* frame;
};

class VulkanBackend::PresenterProxy final : public VideoCore::Render::IPresenter {
public:
    explicit PresenterProxy(Presenter& presenter_) : presenter(presenter_) {}

    Frontend::WindowSDL& GetWindow() override {
        return presenter.GetWindow();
    }

    VideoCore::Render::DisplaySettings GetDisplaySettings() const override {
        const auto& pp = presenter.GetPPSettingsRef();
        const auto& fsr = presenter.GetFsrSettingsRef();
        return {
            .gamma = pp.gamma,
            .fsr_enabled = fsr.enable,
            .rcas_enabled = fsr.use_rcas,
            .rcas_attenuation = fsr.rcas_attenuation,
        };
    }

    void SetDisplaySettings(const VideoCore::Render::DisplaySettings& settings) override {
        auto& pp = presenter.GetPPSettingsRef();
        auto& fsr = presenter.GetFsrSettingsRef();
        pp.gamma = settings.gamma;
        fsr.enable = settings.fsr_enabled;
        fsr.use_rcas = settings.rcas_enabled;
        fsr.rcas_attenuation = settings.rcas_attenuation;
    }

    bool IsHDRSupported() const override {
        return presenter.IsHDRSupported();
    }

    void SetHDR(bool enable) override {
        presenter.SetHDR(enable);
    }

    void RegisterVideoOutSurface(const Libraries::VideoOut::BufferAttributeGroup& attribute,
                                 VAddr cpu_address) override {
        presenter.RegisterVideoOutSurface(attribute, cpu_address);
    }

    std::unique_ptr<VideoCore::Render::IFrameHandle> PrepareFrame(
        const Libraries::VideoOut::BufferAttributeGroup& attribute, VAddr cpu_address) override {
        return std::make_unique<FrameHandle>(presenter.PrepareFrame(attribute, cpu_address));
    }

    std::unique_ptr<VideoCore::Render::IFrameHandle> PrepareBlankFrame(
        bool present_thread) override {
        return std::make_unique<FrameHandle>(presenter.PrepareBlankFrame(present_thread));
    }

    std::unique_ptr<VideoCore::Render::IFrameHandle> PrepareLastFrame() override {
        if (auto* frame = presenter.PrepareLastFrame(); frame != nullptr) {
            return std::make_unique<FrameHandle>(frame);
        }
        return {};
    }

    void Present(std::unique_ptr<VideoCore::Render::IFrameHandle> frame,
                 bool is_reusing_frame) override {
        if (!frame) {
            return;
        }
        auto* vk_frame = dynamic_cast<FrameHandle*>(frame.get());
        ASSERT_MSG(vk_frame != nullptr, "Received a non-Vulkan frame in the Vulkan presenter");
        presenter.Present(vk_frame->frame, is_reusing_frame);
    }

private:
    Presenter& presenter;
};

class VulkanBackend::RasterizerProxy final : public VideoCore::Render::IRasterizer {
public:
    explicit RasterizerProxy(Rasterizer& rasterizer_) : rasterizer(rasterizer_) {}

    void Draw(bool is_indexed, u32 index_offset) override {
        rasterizer.Draw(is_indexed, index_offset);
    }

    void DrawIndirect(bool is_indexed, VAddr arg_address, u32 offset, u32 stride, u32 max_count,
                      VAddr count_address) override {
        rasterizer.DrawIndirect(is_indexed, arg_address, offset, stride, max_count, count_address);
    }

    void DispatchDirect() override {
        rasterizer.DispatchDirect();
    }

    void DispatchIndirect(VAddr address, u32 offset, u32 size) override {
        rasterizer.DispatchIndirect(address, offset, size);
    }

    void FillBuffer(VAddr address, u32 num_bytes, u32 value, bool is_gds) override {
        rasterizer.FillBuffer(address, num_bytes, value, is_gds);
    }

    void CopyBuffer(VAddr dst, VAddr src, u32 num_bytes, bool dst_gds, bool src_gds) override {
        rasterizer.CopyBuffer(dst, src, num_bytes, dst_gds, src_gds);
    }

    u32 ReadDataFromGds(u32 gds_offset) override {
        return rasterizer.ReadDataFromGds(gds_offset);
    }

    bool InvalidateMemory(VAddr addr, u64 size) override {
        return rasterizer.InvalidateMemory(addr, size);
    }

    bool ReadMemory(VAddr addr, u64 size) override {
        return rasterizer.ReadMemory(addr, size);
    }

    bool IsMapped(VAddr addr, u64 size) override {
        return rasterizer.IsMapped(addr, size);
    }

    void MapMemory(VAddr addr, u64 size) override {
        rasterizer.MapMemory(addr, size);
    }

    void UnmapMemory(VAddr addr, u64 size) override {
        rasterizer.UnmapMemory(addr, size);
    }

    void CpSync() override {
        rasterizer.CpSync();
    }

    u64 Flush() override {
        return rasterizer.Flush();
    }

    void Finish() override {
        rasterizer.Finish();
    }

    void OnSubmit() override {
        rasterizer.OnSubmit();
    }

    void ScopeMarkerBegin(std::string_view str, bool from_guest) override {
        rasterizer.ScopeMarkerBegin(str, from_guest);
    }

    void ScopeMarkerEnd(bool from_guest) override {
        rasterizer.ScopeMarkerEnd(from_guest);
    }

    void ScopedMarkerInsert(std::string_view str, bool from_guest) override {
        rasterizer.ScopedMarkerInsert(str, from_guest);
    }

    void ScopedMarkerInsertColor(std::string_view str, u32 color, bool from_guest) override {
        rasterizer.ScopedMarkerInsertColor(str, color, from_guest);
    }

private:
    Rasterizer& rasterizer;
};

class VulkanBackend::ShaderDebugProvider final : public VideoCore::Render::IShaderDebugProvider {
public:
    explicit ShaderDebugProvider(Rasterizer& rasterizer_) : rasterizer(rasterizer_) {}

    std::string GetShaderName(Shader::Stage stage, u64 hash,
                              std::optional<size_t> perm) const override {
        return PipelineCache::GetShaderName(stage, hash, perm);
    }

    std::optional<u64> ReplaceShader(u64 module_handle,
                                     std::span<const u32> backend_binary) override {
        auto& cache = rasterizer.GetPipelineCache();
        const auto module = vk::ShaderModule{
            reinterpret_cast<VkShaderModule>(static_cast<uintptr_t>(module_handle))};
        if (const auto replacement = cache.ReplaceShader(module, backend_binary); replacement) {
            return reinterpret_cast<uint64_t>(static_cast<VkShaderModule>(*replacement));
        }
        return std::nullopt;
    }

private:
    Rasterizer& rasterizer;
};

VulkanBackend::VulkanBackend(Frontend::WindowSDL& window, AmdGpu::Liverpool& liverpool)
    : presenter(std::make_unique<Presenter>(window, &liverpool)) {
    presenter_proxy = std::make_unique<PresenterProxy>(*presenter);
    rasterizer_proxy = std::make_unique<RasterizerProxy>(presenter->GetRasterizer());
    shader_debug_provider = std::make_unique<ShaderDebugProvider>(presenter->GetRasterizer());
    capabilities = {
        .supports_hdr = presenter->IsHDRSupported(),
        .supports_shader_debug = true,
        .supports_overlay_ui = true,
    };
}

VulkanBackend::~VulkanBackend() = default;

VideoCore::Render::BackendKind VulkanBackend::GetKind() const {
    return VideoCore::Render::BackendKind::Vulkan;
}

const VideoCore::Render::BackendCapabilities& VulkanBackend::GetCapabilities() const {
    return capabilities;
}

VideoCore::Render::IPresenter& VulkanBackend::GetPresenter() {
    return *presenter_proxy;
}

VideoCore::Render::IRasterizer& VulkanBackend::GetRasterizer() {
    return *rasterizer_proxy;
}

VideoCore::Render::IShaderDebugProvider* VulkanBackend::GetShaderDebugProvider() {
    return shader_debug_provider.get();
}

} // namespace Vulkan
