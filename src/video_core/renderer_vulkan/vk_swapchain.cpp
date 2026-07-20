// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <limits>
#include <utility>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "imgui/renderer/imgui_core.h"
#include "sdl_window.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_swapchain.h"

namespace Vulkan {

static constexpr vk::SurfaceFormatKHR SURFACE_FORMAT_HDR = {
    .format = vk::Format::eA2B10G10R10UnormPack32,
    .colorSpace = vk::ColorSpaceKHR::eHdr10St2084EXT,
};

namespace {

struct RetiredSwapchain {
    vk::SwapchainKHR handle{};
    std::vector<vk::ImageView> image_views;
    std::vector<vk::Semaphore> image_acquired;
    std::vector<vk::Semaphore> present_ready;
    std::vector<vk::Fence> present_fences;
    std::vector<u8> present_fence_pending;

    explicit operator bool() const noexcept {
        return handle || !image_views.empty() || !image_acquired.empty() ||
               !present_ready.empty() || !present_fences.empty();
    }
};

void WaitForRetiredSwapchain(const Instance& instance, RetiredSwapchain& retired) {
    if (!instance.HasSwapchainMaintenance1()) {
        std::scoped_lock queue_lock{instance.GetPresentQueueMutex()};
        const auto result = instance.GetPresentQueue().waitIdle();
        if (result != vk::Result::eSuccess) {
            LOG_WARNING(Render_Vulkan, "Failed to wait for the presentation queue: {}",
                        vk::to_string(result));
        }
        return;
    }

    std::vector<vk::Fence> pending_fences;
    pending_fences.reserve(retired.present_fences.size());
    for (std::size_t i = 0; i < retired.present_fences.size(); ++i) {
        if (retired.present_fence_pending[i]) {
            pending_fences.push_back(retired.present_fences[i]);
        }
    }
    if (!pending_fences.empty()) {
        const auto result = instance.GetDevice().waitForFences(pending_fences, true,
                                                               std::numeric_limits<u64>::max());
        ASSERT_MSG(result == vk::Result::eSuccess,
                   "Failed waiting for retired swapchain presents: {}", vk::to_string(result));
    }
}

void DestroyRetiredSwapchain(const Instance& instance, RetiredSwapchain& retired) {
    const vk::Device device = instance.GetDevice();
    WaitForRetiredSwapchain(instance, retired);
    for (const auto image_view : retired.image_views) {
        device.destroyImageView(image_view);
    }
    for (const auto semaphore : retired.image_acquired) {
        device.destroySemaphore(semaphore);
    }
    for (const auto semaphore : retired.present_ready) {
        device.destroySemaphore(semaphore);
    }
    for (const auto fence : retired.present_fences) {
        device.destroyFence(fence);
    }
    if (retired.handle) {
        device.destroySwapchainKHR(retired.handle);
    }
}

} // namespace

Swapchain::Swapchain(const Instance& instance_, const Frontend::WindowSDL& window_)
    : instance{instance_}, window{window_}, surface{CreateSurface(instance.GetInstance(), window)} {
    FindPresentFormat();
    FindPresentMode();

    Create(window.GetWidth(), window.GetHeight());
    ImGui::Core::Initialize(instance, window, image_count, surface_format.format);
}

Swapchain::~Swapchain() {
    Destroy();
    instance.GetInstance().destroySurfaceKHR(surface);
}

void Swapchain::Create(u32 width_, u32 height_) {
    RetiredSwapchain retired{
        .handle = std::exchange(swapchain, {}),
        .image_views = std::move(images_view),
        .image_acquired = std::move(image_acquired),
        .present_ready = std::move(present_ready),
        .present_fences = std::move(present_fences),
        .present_fence_pending = std::move(present_fence_pending),
    };
    images.clear();

    width = width_;
    height = height_;
    needs_recreation = false;
    frame_index = 0;
    image_index = 0;

    SetSurfaceProperties();

    const std::array queue_family_indices = {
        instance.GetGraphicsQueueFamilyIndex(),
        instance.GetPresentQueueFamilyIndex(),
    };

    const bool exclusive = queue_family_indices[0] == queue_family_indices[1];
    const u32 queue_family_indices_count = exclusive ? 1u : 2u;
    const vk::SharingMode sharing_mode =
        exclusive ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
    const auto format = needs_hdr ? SURFACE_FORMAT_HDR : surface_format;
    const vk::SwapchainCreateInfoKHR swapchain_info = {
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                      vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode = sharing_mode,
        .queueFamilyIndexCount = queue_family_indices_count,
        .pQueueFamilyIndices = queue_family_indices.data(),
        .preTransform = transform,
        .compositeAlpha = composite_alpha,
        .presentMode = present_mode,
        .clipped = true,
        .oldSwapchain = retired.handle,
    };

    auto [swapchain_result, chain] = instance.GetDevice().createSwapchainKHR(swapchain_info);
    ASSERT_MSG(swapchain_result == vk::Result::eSuccess, "Failed to create swapchain: {}",
               vk::to_string(swapchain_result));
    swapchain = chain;

    SetupImages();
    RefreshSemaphores();

    if (retired) {
        // Creation uses oldSwapchain and never waits while holding the queue mutex. With
        // swapchain-maintenance1, per-present fences retire only the old presentation resources.
        DestroyRetiredSwapchain(instance, retired);
    }
}

void Swapchain::Recreate(u32 width_, u32 height_) {
    LOG_DEBUG(Render_Vulkan, "Recreate the swapchain: width={} height={} HDR={}", width_, height_,
              needs_hdr);
    Create(width_, height_);
}

void Swapchain::SetHDR(bool hdr) {
    if (needs_hdr == hdr) {
        return;
    }

    needs_hdr = hdr;
    Recreate(width, height);
    ImGui::Core::OnSurfaceFormatChange(needs_hdr ? SURFACE_FORMAT_HDR.format
                                                 : surface_format.format);
}

bool Swapchain::AcquireNextImage() {
    vk::Device device = instance.GetDevice();
    vk::Result result =
        device.acquireNextImageKHR(swapchain, std::numeric_limits<u64>::max(),
                                   image_acquired[frame_index], VK_NULL_HANDLE, &image_index);

    switch (result) {
    case vk::Result::eSuccess:
        break;
    case vk::Result::eSuboptimalKHR:
    case vk::Result::eErrorSurfaceLostKHR:
    case vk::Result::eErrorOutOfDateKHR:
    case vk::Result::eErrorUnknown:
        needs_recreation = true;
        break;
    default:
        LOG_CRITICAL(Render_Vulkan, "Swapchain acquire returned unknown result {}",
                     vk::to_string(result));
        UNREACHABLE();
        break;
    }

    return !needs_recreation;
}

bool Swapchain::Present() {
    vk::SwapchainPresentFenceInfoEXT present_fence_info{};
    if (instance.HasSwapchainMaintenance1()) {
        if (present_fence_pending[image_index]) {
            const auto wait_result = instance.GetDevice().waitForFences(
                present_fences[image_index], true, std::numeric_limits<u64>::max());
            ASSERT_MSG(wait_result == vk::Result::eSuccess,
                       "Failed waiting for reusable swapchain present fence: {}",
                       vk::to_string(wait_result));
            present_fence_pending[image_index] = false;
        }
        const auto reset_result = instance.GetDevice().resetFences(present_fences[image_index]);
        ASSERT_MSG(reset_result == vk::Result::eSuccess,
                   "Failed resetting swapchain present fence: {}", vk::to_string(reset_result));
        present_fence_info = {
            .swapchainCount = 1,
            .pFences = &present_fences[image_index],
        };
    }

    const vk::PresentInfoKHR present_info = {
        .pNext = instance.HasSwapchainMaintenance1() ? &present_fence_info : nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &present_ready[image_index],
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &image_index,
    };

    std::scoped_lock queue_lock{instance.GetPresentQueueMutex()};
    auto result = instance.GetPresentQueue().presentKHR(present_info);
    if (instance.HasSwapchainMaintenance1() &&
        (result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR)) {
        present_fence_pending[image_index] = true;
    }
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        needs_recreation = true;
    } else {
        ASSERT_MSG(result == vk::Result::eSuccess, "Swapchain presentation failed: {}",
                   vk::to_string(result));
    }

    frame_index = (frame_index + 1) % image_count;

    return !needs_recreation;
}

void Swapchain::FindPresentFormat() {
    const auto [formats_result, formats] =
        instance.GetPhysicalDevice().getSurfaceFormatsKHR(surface);
    ASSERT_MSG(formats_result == vk::Result::eSuccess, "Failed to query surface formats: {}",
               vk::to_string(formats_result));

    // Check if the device supports HDR formats. Here we care of Rec.2020 PQ only as it is expected
    // game output. Other variants as e.g. linear Rec.2020 will require additional color space
    // rotation
    supports_hdr =
        std::find_if(formats.begin(), formats.end(), [](const vk::SurfaceFormatKHR& format) {
            return format == SURFACE_FORMAT_HDR;
        }) != formats.end();
    // Also make sure that user allowed us to use HDR
    supports_hdr &= EmulatorSettings.IsHdrAllowed();

    // If there is a single undefined surface format, the device doesn't care, so we'll just use
    // RGBA sRGB.
    if (formats[0].format == vk::Format::eUndefined) {
        surface_format.format = vk::Format::eR8G8B8A8Unorm;
        surface_format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        return;
    }

    // Try to find a suitable format.
    for (const vk::SurfaceFormatKHR& sformat : formats) {
        vk::Format format = sformat.format;
        if (format != vk::Format::eR8G8B8A8Unorm && format != vk::Format::eB8G8R8A8Unorm) {
            continue;
        }

        surface_format.format = format;
        surface_format.colorSpace = sformat.colorSpace;
        return;
    }

    UNREACHABLE_MSG("Unable to find required swapchain format!");
}

void Swapchain::FindPresentMode() {
    const auto [modes_result, modes] =
        instance.GetPhysicalDevice().getSurfacePresentModesKHR(surface);
    if (modes_result != vk::Result::eSuccess) {
        LOG_ERROR(Render, "Failed to query available present modes, falling back to Fifo as "
                          "guaranteed supported option.");
        present_mode = vk::PresentModeKHR::eFifo;
        return;
    }

    const auto requested_mode = EmulatorSettings.GetPresentMode();
    if (requested_mode == "Mailbox") {
        present_mode = vk::PresentModeKHR::eMailbox;
    } else if (requested_mode == "Fifo") {
        present_mode = vk::PresentModeKHR::eFifo;
    } else if (requested_mode == "Immediate") {
        present_mode = vk::PresentModeKHR::eImmediate;
    } else {
        LOG_ERROR(Render_Vulkan, "Unknown present mode {}, defaulting to Mailbox.",
                  EmulatorSettings.GetPresentMode());
        present_mode = vk::PresentModeKHR::eMailbox;
    }

    if (std::ranges::find(modes, present_mode) == modes.cend()) {
        // FIFO is guaranteed to be supported by the Vulkan spec.
        constexpr auto fallback = vk::PresentModeKHR::eFifo;
        LOG_WARNING(Render, "Requested present mode {} is not supported, falling back to {}.",
                    vk::to_string(present_mode), vk::to_string(fallback));
        present_mode = fallback;
    }
}

void Swapchain::SetSurfaceProperties() {
    const auto [capabilities_result, capabilities] =
        instance.GetPhysicalDevice().getSurfaceCapabilitiesKHR(surface);
    ASSERT_MSG(capabilities_result == vk::Result::eSuccess,
               "Failed to query surface capabilities: {}", vk::to_string(capabilities_result));

    extent = capabilities.currentExtent;
    if (capabilities.currentExtent.width == std::numeric_limits<u32>::max()) {
        extent.width = std::max(capabilities.minImageExtent.width,
                                std::min(capabilities.maxImageExtent.width, width));
        extent.height = std::max(capabilities.minImageExtent.height,
                                 std::min(capabilities.maxImageExtent.height, height));
    }

    // Select number of images in swap chain, we prefer one buffer in the background to work on
    image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        image_count = std::min(image_count, capabilities.maxImageCount);
    }

    // Prefer identity transform if possible
    transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    if (!(capabilities.supportedTransforms & transform)) {
        transform = capabilities.currentTransform;
    }

    // Opaque is not supported everywhere.
    composite_alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    if (!(capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)) {
        composite_alpha = vk::CompositeAlphaFlagBitsKHR::eInherit;
    }
}

void Swapchain::Destroy() {
    RetiredSwapchain retired{
        .handle = std::exchange(swapchain, {}),
        .image_views = std::move(images_view),
        .image_acquired = std::move(image_acquired),
        .present_ready = std::move(present_ready),
        .present_fences = std::move(present_fences),
        .present_fence_pending = std::move(present_fence_pending),
    };
    images.clear();
    if (!retired) {
        return;
    }

    DestroyRetiredSwapchain(instance, retired);
}

void Swapchain::RefreshSemaphores() {
    const vk::Device device = instance.GetDevice();
    image_acquired.resize(image_count);
    present_ready.resize(image_count);
    if (instance.HasSwapchainMaintenance1()) {
        present_fences.resize(image_count);
        present_fence_pending.assign(image_count, false);
    }

    for (vk::Semaphore& semaphore : image_acquired) {
        auto [semaphore_result, sem] = device.createSemaphore({});
        ASSERT_MSG(semaphore_result == vk::Result::eSuccess,
                   "Failed to create image acquired semaphore: {}",
                   vk::to_string(semaphore_result));
        semaphore = sem;
    }
    for (vk::Semaphore& semaphore : present_ready) {
        auto [semaphore_result, sem] = device.createSemaphore({});
        ASSERT_MSG(semaphore_result == vk::Result::eSuccess,
                   "Failed to create present ready semaphore: {}", vk::to_string(semaphore_result));
        semaphore = sem;
    }
    for (vk::Fence& fence : present_fences) {
        auto [fence_result, created_fence] =
            device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});
        ASSERT_MSG(fence_result == vk::Result::eSuccess,
                   "Failed to create swapchain present fence: {}", vk::to_string(fence_result));
        fence = created_fence;
    }

    for (u32 i = 0; i < image_count; ++i) {
        SetObjectName(device, image_acquired[i], "Swapchain Semaphore: image_acquired {}", i);
        SetObjectName(device, present_ready[i], "Swapchain Semaphore: present_ready {}", i);
        if (instance.HasSwapchainMaintenance1()) {
            SetObjectName(device, present_fences[i], "Swapchain Fence: present_done {}", i);
        }
    }
}

void Swapchain::SetupImages() {
    vk::Device device = instance.GetDevice();
    auto [images_result, imgs] = device.getSwapchainImagesKHR(swapchain);
    ASSERT_MSG(images_result == vk::Result::eSuccess, "Failed to create swapchain images: {}",
               vk::to_string(images_result));
    images = std::move(imgs);
    image_count = static_cast<u32>(images.size());
    images_view.resize(image_count);
    for (u32 i = 0; i < image_count; ++i) {
        if (images_view[i]) {
            device.destroyImageView(images_view[i]);
        }
        auto [im_view_result, im_view] = device.createImageView(vk::ImageViewCreateInfo{
            .image = images[i],
            .viewType = vk::ImageViewType::e2D,
            .format = needs_hdr ? SURFACE_FORMAT_HDR.format : surface_format.format,
            .subresourceRange =
                {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .levelCount = 1,
                    .layerCount = 1,
                },
        });
        ASSERT_MSG(im_view_result == vk::Result::eSuccess, "Failed to create image view: {}",
                   vk::to_string(im_view_result));
        images_view[i] = im_view;
    }

    for (u32 i = 0; i < image_count; ++i) {
        SetObjectName(device, images[i], "Swapchain Image {}", i);
        SetObjectName(device, images_view[i], "Swapchain ImageView {}", i);
    }
}

} // namespace Vulkan
