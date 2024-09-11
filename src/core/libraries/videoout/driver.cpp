// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <pthread.h>

#include "common/assert.h"
#include "common/config.h"
#include "common/debug.h"
#include "common/thread.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/time_management.h"
#include "core/libraries/videoout/driver.h"
#include "core/platform.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"

extern std::unique_ptr<Vulkan::RendererVulkan> renderer;
extern std::unique_ptr<AmdGpu::Liverpool> liverpool;

namespace Libraries::VideoOut {

constexpr static bool Is32BppPixelFormat(PixelFormat format) {
    switch (format) {
    case PixelFormat::A8R8G8B8Srgb:
    case PixelFormat::A8B8G8R8Srgb:
    case PixelFormat::A2R10G10B10:
    case PixelFormat::A2R10G10B10Srgb:
    case PixelFormat::A2R10G10B10Bt2020Pq:
        return true;
    default:
        return false;
    }
}

constexpr u32 PixelFormatBpp(PixelFormat pixel_format) {
    switch (pixel_format) {
    case PixelFormat::A16R16G16B16Float:
        return 8;
    default:
        return 4;
    }
}

VideoOutDriver::VideoOutDriver(u32 width, u32 height) {
    main_port.resolution.fullWidth = width;
    main_port.resolution.fullHeight = height;
    main_port.resolution.paneWidth = width;
    main_port.resolution.paneHeight = height;
    present_thread = std::jthread([&](std::stop_token token) { PresentThread(token); });
}

VideoOutDriver::~VideoOutDriver() = default;

int VideoOutDriver::Open(const ServiceThreadParams* params) {
    if (main_port.is_open) {
        return ORBIS_VIDEO_OUT_ERROR_RESOURCE_BUSY;
    }
    main_port.is_open = true;
    liverpool->SetVoPort(&main_port);
    return 1;
}

void VideoOutDriver::Close(s32 handle) {
    std::scoped_lock lock{mutex};

    main_port.is_open = false;
    main_port.flip_rate = 0;
    ASSERT(main_port.flip_events.empty());
}

VideoOutPort* VideoOutDriver::GetPort(int handle) {
    if (handle != 1) [[unlikely]] {
        return nullptr;
    }
    return &main_port;
}

int VideoOutDriver::RegisterBuffers(VideoOutPort* port, s32 startIndex, void* const* addresses,
                                    s32 bufferNum, const BufferAttribute* attribute) {
    const s32 group_index = port->FindFreeGroup();
    if (group_index >= MaxDisplayBufferGroups) {
        return ORBIS_VIDEO_OUT_ERROR_NO_EMPTY_SLOT;
    }

    if (startIndex + bufferNum > MaxDisplayBuffers || startIndex > MaxDisplayBuffers ||
        bufferNum > MaxDisplayBuffers) {
        LOG_ERROR(Lib_VideoOut,
                  "Attempted to register too many buffers startIndex = {}, bufferNum = {}",
                  startIndex, bufferNum);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_VALUE;
    }

    const s32 end_index = startIndex + bufferNum;
    if (bufferNum > 0 &&
        std::any_of(port->buffer_slots.begin() + startIndex, port->buffer_slots.begin() + end_index,
                    [](auto& buffer) { return buffer.group_index != -1; })) {
        return ORBIS_VIDEO_OUT_ERROR_SLOT_OCCUPIED;
    }

    if (attribute->reserved0 != 0 || attribute->reserved1 != 0) {
        LOG_ERROR(Lib_VideoOut, "Invalid reserved members");
        return ORBIS_VIDEO_OUT_ERROR_INVALID_VALUE;
    }
    if (attribute->aspect_ratio != 0) {
        LOG_ERROR(Lib_VideoOut, "Invalid aspect ratio = {}", attribute->aspect_ratio);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_ASPECT_RATIO;
    }
    if (attribute->width > attribute->pitch_in_pixel) {
        LOG_ERROR(Lib_VideoOut, "Buffer width {} is larger than pitch {}", attribute->width,
                  attribute->pitch_in_pixel);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_PITCH;
    }
    if (attribute->tiling_mode < TilingMode::Tile || attribute->tiling_mode > TilingMode::Linear) {
        LOG_ERROR(Lib_VideoOut, "Invalid tilingMode = {}",
                  static_cast<u32>(attribute->tiling_mode));
        return ORBIS_VIDEO_OUT_ERROR_INVALID_TILING_MODE;
    }

    LOG_INFO(Lib_VideoOut,
             "startIndex = {}, bufferNum = {}, pixelFormat = {}, aspectRatio = {}, "
             "tilingMode = {}, width = {}, height = {}, pitchInPixel = {}, option = {:#x}",
             startIndex, bufferNum, GetPixelFormatString(attribute->pixel_format),
             attribute->aspect_ratio, static_cast<u32>(attribute->tiling_mode), attribute->width,
             attribute->height, attribute->pitch_in_pixel, attribute->option);

    auto& group = port->groups[group_index];
    std::memcpy(&group.attrib, attribute, sizeof(BufferAttribute));
    group.is_occupied = true;

    for (u32 i = 0; i < bufferNum; i++) {
        const uintptr_t address = reinterpret_cast<uintptr_t>(addresses[i]);
        port->buffer_slots[startIndex + i] = VideoOutBuffer{
            .group_index = group_index,
            .address_left = address,
            .address_right = 0,
        };

        renderer->RegisterVideoOutSurface(group, address);
        LOG_INFO(Lib_VideoOut, "buffers[{}] = {:#x}", i + startIndex, address);
    }

    return group_index;
}

int VideoOutDriver::UnregisterBuffers(VideoOutPort* port, s32 attributeIndex) {
    if (attributeIndex >= MaxDisplayBufferGroups || !port->groups[attributeIndex].is_occupied) {
        LOG_ERROR(Lib_VideoOut, "Invalid attribute index {}", attributeIndex);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_VALUE;
    }

    auto& group = port->groups[attributeIndex];
    group.is_occupied = false;

    for (auto& buffer : port->buffer_slots) {
        if (buffer.group_index != attributeIndex) {
            continue;
        }
        buffer.group_index = -1;
    }

    return ORBIS_OK;
}

std::chrono::microseconds VideoOutDriver::Flip(const Request& req) {
    const auto start = std::chrono::high_resolution_clock::now();

    // Whatever the game is rendering show splash if it is active
    if (!renderer->ShowSplash(req.frame)) {
        // Present the frame.
        renderer->Present(req.frame);
    }

    // Update flip status.
    auto* port = req.port;
    {
        std::unique_lock lock{port->port_mutex};
        auto& flip_status = port->flip_status;
        flip_status.count++;
        flip_status.processTime = Libraries::Kernel::sceKernelGetProcessTime();
        flip_status.tsc = Libraries::Kernel::sceKernelReadTsc();
        flip_status.flipArg = req.flip_arg;
        flip_status.currentBuffer = req.index;
        if (req.eop) {
            --flip_status.gcQueueNum;
        }
        --flip_status.flipPendingNum;
    }

    // Trigger flip events for the port.
    for (auto& event : port->flip_events) {
        if (event != nullptr) {
            event->TriggerEvent(SCE_VIDEO_OUT_EVENT_FLIP, Kernel::SceKernelEvent::Filter::VideoOut,
                                reinterpret_cast<void*>(req.flip_arg));
        }
    }

    // Reset flip label
    if (req.index != -1) {
        port->buffer_labels[req.index] = 0;
        port->SignalVoLabel();
    }

    const auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
}

void VideoOutDriver::DrawBlankFrame() {
    const auto empty_frame = renderer->PrepareBlankFrame(false);
    renderer->Present(empty_frame);
}

bool VideoOutDriver::SubmitFlip(VideoOutPort* port, s32 index, s64 flip_arg,
                                bool is_eop /*= false*/) {
    {
        std::unique_lock lock{port->port_mutex};
        if (index != -1 && port->flip_status.flipPendingNum >= port->NumRegisteredBuffers()) {
            LOG_ERROR(Lib_VideoOut, "Flip queue is full");
            return false;
        }

        if (is_eop) {
            ++port->flip_status.gcQueueNum;
        }
        ++port->flip_status.flipPendingNum; // integral GPU and CPU pending flips counter
        port->flip_status.submitTsc = Libraries::Kernel::sceKernelReadTsc();
    }

    if (!is_eop) {
        // Before processing the flip we need to ask GPU thread to flush command list as at this
        // point VO surface is ready to be presented, and we will need have an actual state of
        // Vulkan image at the time of frame presentation.
        liverpool->SendCommand([=, this]() {
            renderer->FlushDraw();
            SubmitFlipInternal(port, index, flip_arg, is_eop);
        });
    } else {
        SubmitFlipInternal(port, index, flip_arg, is_eop);
    }

    return true;
}

void VideoOutDriver::SubmitFlipInternal(VideoOutPort* port, s32 index, s64 flip_arg,
                                        bool is_eop /*= false*/) {
    Vulkan::Frame* frame;
    if (index == -1) {
        frame = renderer->PrepareBlankFrame(is_eop);
    } else {
        const auto& buffer = port->buffer_slots[index];
        const auto& group = port->groups[buffer.group_index];
        frame = renderer->PrepareFrame(group, buffer.address_left, is_eop);
    }

    std::scoped_lock lock{mutex};
    requests.push({
        .frame = frame,
        .port = port,
        .flip_arg = flip_arg,
        .index = index,
        .eop = is_eop,
    });
}

void VideoOutDriver::PresentThread(std::stop_token token) {
    static constexpr std::chrono::milliseconds VblankPeriod{16};
    Common::SetCurrentThreadName("PresentThread");

    const auto receive_request = [this] -> Request {
        std::scoped_lock lk{mutex};
        if (!requests.empty()) {
            const auto request = requests.front();
            requests.pop();
            return request;
        }
        return {};
    };

    auto vblank_period = VblankPeriod / Config::vblankDiv();
    auto delay = std::chrono::microseconds{0};
    while (!token.stop_requested()) {
        // Sleep for most of the vblank duration.
        std::this_thread::sleep_for(vblank_period - delay);

        // Check if it's time to take a request.
        auto& vblank_status = main_port.vblank_status;
        if (vblank_status.count % (main_port.flip_rate + 1) == 0) {
            const auto request = receive_request();
            if (!request) {
                delay = std::chrono::microseconds{0};
                if (!main_port.is_open) {
                    DrawBlankFrame();
                }
            } else {
                delay = Flip(request);
            }
            FRAME_END;
        }

        {
            // Needs lock here as can be concurrently read by `sceVideoOutGetVblankStatus`
            std::unique_lock lock{main_port.vo_mutex};
            vblank_status.count++;
            vblank_status.processTime = Libraries::Kernel::sceKernelGetProcessTime();
            vblank_status.tsc = Libraries::Kernel::sceKernelReadTsc();
            main_port.vblank_cv.notify_all();
        }

        // Trigger flip events for the port.
        for (auto& event : main_port.vblank_events) {
            if (event != nullptr) {
                event->TriggerEvent(SCE_VIDEO_OUT_EVENT_VBLANK,
                                    Kernel::SceKernelEvent::Filter::VideoOut, nullptr);
            }
        }
    }
}

} // namespace Libraries::VideoOut
