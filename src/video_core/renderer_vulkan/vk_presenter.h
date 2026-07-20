// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <queue>

#include "core/libraries/videoout/buffer.h"
#include "imgui/imgui_texture.h"
#include "video_core/renderer_vulkan/host_passes/fsr_pass.h"
#include "video_core/renderer_vulkan/host_passes/pp_pass.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_swapchain.h"
#include "video_core/texture_cache/texture_cache.h"

namespace Frontend {
class WindowSDL;
}

namespace AmdGpu {
struct Liverpool;
}

namespace Vulkan {

struct Frame {
    u32 width{};
    u32 height{};
    VmaAllocation allocation{};
    vk::Image image{};
    vk::ImageView image_view{};
    vk::Fence present_done{};
    vk::Semaphore ready_semaphore{};
    u64 ready_tick{};
    bool is_hdr{false};
    u8 id{};

    ImTextureID imgui_texture{};
};

class Rasterizer;

class Presenter {
public:
    struct FifoTimingFeedback {
        s64 last_present_call_ns;
        s64 present_call_period_ns;
        u32 present_call_samples;
        u64 generation;
        bool is_fifo;
    };

    Presenter(Frontend::WindowSDL& window, AmdGpu::Liverpool* liverpool);
    ~Presenter();

    HostPasses::PostProcessingPass::Settings& GetPPSettingsRef() {
        return pp_settings;
    }

    HostPasses::FsrPass::Settings& GetFsrSettingsRef() {
        return fsr_settings;
    }

    Frontend::WindowSDL& GetWindow() const {
        return window;
    }

    Rasterizer& GetRasterizer() const {
        return *rasterizer.get();
    }

    bool IsHDRSupported() const {
        return swapchain.HasHDR();
    }

    void SetHDR(bool enable) {
        if (!IsHDRSupported()) {
            return;
        }
        const bool changed = swapchain.GetHDR() != enable;
        swapchain.SetHDR(enable);
        if (changed) {
            ResetFifoTimingFeedback();
        }
        pp_settings.hdr = enable ? 1 : 0;
    }

    VideoCore::Image& RegisterVideoOutSurface(
        const Libraries::VideoOut::BufferAttributeGroup& attribute, VAddr cpu_address) {
        vo_buffers_addr.emplace_back(cpu_address);
        auto desc = VideoCore::TextureCache::ImageDesc{attribute, cpu_address};
        const auto image_id = texture_cache.FindImage(desc);
        auto& image = texture_cache.GetImage(image_id);
        image.usage.vo_surface = 1u;
        return image;
    }

    bool IsVideoOutSurface(const AmdGpu::ColorBuffer& color_buffer) const;

    Frame* PrepareFrame(const Libraries::VideoOut::BufferAttributeGroup& attribute,
                        VAddr cpu_address);

    Frame* PrepareBlankFrame(bool present_thread);

    void Present(Frame* frame, bool is_reusing_frame = false, u64 presentation_epoch = 0);
    Frame* PrepareLastFrame();

    /// Returns an unpresented frame once its producer timeline semaphore has completed.
    /// This never waits on the caller and is safe for mailbox replacement on the vblank thread.
    void RecycleFrameAsync(Frame* frame);

    FifoTimingFeedback GetFifoTimingFeedback() const;

    bool UsesMailboxPresentation() const {
        return swapchain.IsMailbox();
    }

    /// Invalidates feedback from work belonging to an older video-out lifecycle.
    void SetPresentationEpoch(u64 epoch);

private:
    Frame* GetRenderFrame();

    void RecreateFrame(Frame* frame, u32 width, u32 height);

    void RecreateSwapchain();

    void ResetFifoTimingFeedback();

    void ResetFifoTimingFeedbackLocked();

    void ClearPresentCallHistoryLocked();

    void RecordPresentCall(u64 epoch);

    void RecycleThread(std::stop_token token);

    void ReturnFrame(Frame* frame);

    /// Waits for host rendering of a submitted source frame on the presentation thread, then
    /// returns it to the producer pool. The guest/GPU thread only ever receives ready frames.
    void RetireSubmittedFrame(Frame* frame);

    void SetExpectedGameSize(s32 width, s32 height);

private:
    float expected_ratio{1920.0 / 1080.0f};
    u32 expected_frame_width{1920};
    u32 expected_frame_height{1080};

    Frontend::WindowSDL& window;
    Instance instance;
    HostPasses::FsrPass fsr_pass;
    HostPasses::FsrPass::Settings fsr_settings{};
    HostPasses::PostProcessingPass::Settings pp_settings{};
    HostPasses::PostProcessingPass pp_pass;
    AmdGpu::Liverpool* liverpool;
    Scheduler draw_scheduler;
    Scheduler present_scheduler;
    Swapchain swapchain;
    std::unique_ptr<Rasterizer> rasterizer;
    VideoCore::TextureCache& texture_cache;
    vk::UniqueCommandPool command_pool;
    std::vector<Frame> present_frames;
    std::queue<Frame*> free_queue;
    Frame* last_submit_frame{};
    std::mutex free_mutex;
    std::condition_variable free_cv;
    std::mutex recycle_mutex;
    std::condition_variable_any recycle_cv;
    std::queue<Frame*> recycle_queue;
    std::jthread recycle_thread;
    std::condition_variable_any frame_cv;
    std::optional<ImGui::RefCountedTexture> splash_img;
    std::vector<VAddr> vo_buffers_addr;
    std::atomic<s64> last_present_call_ns{};
    std::atomic<s64> present_call_period_ns{};
    std::atomic<u32> present_call_samples{};
    std::atomic<u64> timing_generation{};
    std::mutex feedback_mutex;
    static constexpr u32 PresentCallPeriodWindow = 7;
    std::array<s64, PresentCallPeriodWindow> present_call_period_history{};
    u32 present_call_period_history_index{};
    u32 present_call_period_history_size{};
    u64 presentation_epoch{};
};

} // namespace Vulkan
