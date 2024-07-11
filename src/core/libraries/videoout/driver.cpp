// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <pthread.h>
#include "common/assert.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/time_management.h"
#include "core/libraries/videoout/driver.h"
#include "core/platform.h"

#include "video_core/renderer_vulkan/renderer_vulkan.h"

extern std::unique_ptr<Vulkan::RendererVulkan> renderer;

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
}

VideoOutDriver::~VideoOutDriver() = default;

int VideoOutDriver::Open(const ServiceThreadParams* params) {
    std::scoped_lock lock{mutex};

    if (main_port.is_open) {
        return ORBIS_VIDEO_OUT_ERROR_RESOURCE_BUSY;
    }

    int handle = 1;
    main_port.is_open = true;
    return handle;
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
        LOG_ERROR(Lib_VideoOut, "Invalid reserved memebers");
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
    group.size_in_bytes =
        attribute->height * attribute->pitch_in_pixel * PixelFormatBpp(attribute->pixel_format);
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

void VideoOutDriver::Flip(std::chrono::microseconds timeout) {
    Request req;
    {
        std::unique_lock lock{mutex};
        submit_cond.wait_for(lock, timeout, [&] { return !requests.empty(); });
        if (requests.empty()) {
            renderer->ShowSplash();
            return;
        }

        // Retrieve the request.
        req = requests.front();
        requests.pop();
    }

    // Whatever the game is rendering show splash if it is active
    if (!renderer->ShowSplash(req.frame)) {
        // Present the frame.
        renderer->Present(req.frame);
    }

    std::scoped_lock lock{mutex};

    // Update flip status.
    auto& flip_status = req.port->flip_status;
    flip_status.count++;
    flip_status.processTime = Libraries::Kernel::sceKernelGetProcessTime();
    flip_status.tsc = Libraries::Kernel::sceKernelReadTsc();
    flip_status.submitTsc = Libraries::Kernel::sceKernelReadTsc();
    flip_status.flipArg = req.flip_arg;
    flip_status.currentBuffer = req.index;
    flip_status.flipPendingNum = static_cast<int>(requests.size());

    // Trigger flip events for the port.
    for (auto& event : req.port->flip_events) {
        if (event != nullptr) {
            event->TriggerEvent(SCE_VIDEO_OUT_EVENT_FLIP, Kernel::SceKernelEvent::Filter::VideoOut,
                                reinterpret_cast<void*>(req.flip_arg));
        }
    }

    // Reset flip label
    if (req.index != -1) {
        req.port->buffer_labels[req.index] = 0;
    }
}

bool VideoOutDriver::SubmitFlip(VideoOutPort* port, s32 index, s64 flip_arg,
                                bool is_eop /*= false*/) {
    std::scoped_lock lock{mutex};

    Vulkan::Frame* frame;
    if (index == -1) {
        frame = renderer->PrepareBlankFrame();
    } else {
        const auto& buffer = port->buffer_slots[index];
        const auto& group = port->groups[buffer.group_index];
        frame = renderer->PrepareFrame(group, buffer.address_left);
    }

    if (index != -1 && requests.size() >= port->NumRegisteredBuffers()) {
        LOG_ERROR(Lib_VideoOut, "Flip queue is full");
        return false;
    }

    requests.push({
        .frame = frame,
        .port = port,
        .index = index,
        .flip_arg = flip_arg,
        .submit_tsc = Libraries::Kernel::sceKernelReadTsc(),
        .eop = is_eop,
    });

    port->flip_status.flipPendingNum = static_cast<int>(requests.size());
    port->flip_status.gcQueueNum = 0;
    submit_cond.notify_one();

    return true;
}

void VideoOutDriver::Vblank() {
    std::scoped_lock lock{mutex};

    auto& vblank_status = main_port.vblank_status;
    vblank_status.count++;
    vblank_status.processTime = Libraries::Kernel::sceKernelGetProcessTime();
    vblank_status.tsc = Libraries::Kernel::sceKernelReadTsc();

    // Trigger flip events for the port.
    for (auto& event : main_port.vblank_events) {
        if (event != nullptr) {
            event->TriggerEvent(SCE_VIDEO_OUT_EVENT_VBLANK,
                                Kernel::SceKernelEvent::Filter::VideoOut, nullptr);
        }
    }
}

} // namespace Libraries::VideoOut
