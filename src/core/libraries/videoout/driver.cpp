// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <optional>

#include "common/assert.h"
#include "common/debug.h"
#include "common/thread.h"
#include "core/debug_state.h"
#include "core/emulator_settings.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/videoout/driver.h"
#include "core/libraries/videoout/videoout_error.h"
#include "imgui/renderer/imgui_core.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/renderer_vulkan/vk_presenter.h"

extern std::unique_ptr<Vulkan::Presenter> presenter;
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

VideoOutDriver::VideoOutDriver(u32 width, u32 height)
    : pending_presents{presenter->UsesMailboxPresentation() ? PresentationQueuePolicy::Mailbox
                                                            : PresentationQueuePolicy::Fifo} {
    main_port.resolution.full_width = width;
    main_port.resolution.full_height = height;
    main_port.resolution.pane_width = width;
    main_port.resolution.pane_height = height;
    vblank_thread = std::jthread([this](std::stop_token token) { VblankThread(token); });
    present_thread = std::jthread([this](std::stop_token token) { PresentThread(token); });
}

VideoOutDriver::~VideoOutDriver() {
    vblank_thread.request_stop();
    present_thread.request_stop();
    present_cv.notify_all();
    vblank_thread.join();
    present_thread.join();

    while (!requests.empty()) {
        presenter->RecycleFrameAsync(requests.front().frame);
        requests.pop();
    }
    for (auto& request : pending_presents.Drain()) {
        presenter->RecycleFrameAsync(request.frame);
    }
}

int VideoOutDriver::Open(const ServiceThreadParams* params) {
    std::scoped_lock lock{mutex};
    if (main_port.is_open.load(std::memory_order_acquire)) {
        return ORBIS_VIDEO_OUT_ERROR_RESOURCE_BUSY;
    }
    const u64 generation = main_port.generation.fetch_add(1, std::memory_order_acq_rel) + 1;
    presenter->SetPresentationEpoch(generation);
    liverpool->SetVoPort(&main_port);
    main_port.is_open.store(true, std::memory_order_release);
    return 1;
}

s32 VideoOutDriver::Close(const s32 handle) {
    std::vector<Vulkan::Frame*> discarded_frames;
    std::vector<Kernel::OrbisKernelEqueue> flip_events;
    std::vector<Kernel::OrbisKernelEqueue> vblank_events;
    std::unique_lock lifecycle_lock{mutex};
    {
        if (handle != 1 || !main_port.is_open.load(std::memory_order_acquire)) {
            return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
        }
        main_port.is_open.store(false, std::memory_order_release);
        const u64 generation = main_port.generation.fetch_add(1, std::memory_order_acq_rel) + 1;
        presenter->SetPresentationEpoch(generation);
        main_port.flip_rate.store(0, std::memory_order_release);
        main_port.prev_index = -1;
        while (!requests.empty()) {
            discarded_frames.push_back(requests.front().frame);
            requests.pop();
        }

        {
            std::scoped_lock port_lock{main_port.port_mutex};
            main_port.flip_status = FlipStatus{};
            std::memset(main_port.groups.data(), 0, sizeof(main_port.groups));
            std::memset(main_port.buffer_slots.data(), 0, sizeof(main_port.buffer_slots));
            for (auto& buffer : main_port.buffer_slots) {
                buffer.group_index = -1;
            }
        }
        {
            std::scoped_lock vo_lock{main_port.vo_mutex};
            std::memset(main_port.buffer_labels.data(), 0, sizeof(main_port.buffer_labels));
            std::memset(&main_port.vblank_status, 0, sizeof(main_port.vblank_status));
            main_port.vo_cv.notify_all();
            main_port.vblank_cv.notify_all();
        }
        {
            std::scoped_lock event_lock{main_port.event_mutex};
            flip_events = std::move(main_port.flip_events);
            vblank_events = std::move(main_port.vblank_events);
        }
    }

    {
        std::scoped_lock lock{present_mutex};
        for (auto& request : pending_presents.Drain()) {
            discarded_frames.push_back(request.frame);
        }
        blank_requested = true;
    }
    present_cv.notify_one();

    for (auto* frame : discarded_frames) {
        presenter->RecycleFrameAsync(frame);
    }

    for (auto event : flip_events) {
        auto equeue = Kernel::GetEqueue(event);
        if (equeue != nullptr) {
            equeue->RemoveEvent(static_cast<u64>(OrbisVideoOutInternalEventId::Flip),
                                Kernel::OrbisKernelEvent::Filter::VideoOut);
        }
    }
    for (auto event : vblank_events) {
        auto equeue = Kernel::GetEqueue(event);
        if (equeue != nullptr) {
            equeue->RemoveEvent(static_cast<u64>(OrbisVideoOutInternalEventId::Vblank),
                                Kernel::OrbisKernelEvent::Filter::VideoOut);
        }
    }
    return ORBIS_OK;
}

VideoOutPort* VideoOutDriver::GetPort(int handle) {
    if (handle != 1 || !main_port.is_open.load(std::memory_order_acquire)) [[unlikely]] {
        return nullptr;
    }
    return &main_port;
}

int VideoOutDriver::RegisterBuffers(VideoOutPort* port, s32 startIndex, void* const* addresses,
                                    s32 bufferNum, const BufferAttribute* attribute) {
    std::scoped_lock lifecycle_lock{mutex};
    if (!port->is_open.load(std::memory_order_acquire)) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }
    std::unique_lock port_lock{port->port_mutex};
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

        // Reset flip label also when registering buffer
        port->buffer_labels[startIndex + i] = 0;
        port->SignalVoLabel();

        presenter->RegisterVideoOutSurface(group, address);
        LOG_INFO(Lib_VideoOut, "buffers[{}] = {:#x}", i + startIndex, address);
    }

    return group_index;
}

int VideoOutDriver::UnregisterBuffers(VideoOutPort* port, s32 attributeIndex) {
    std::scoped_lock lifecycle_lock{mutex};
    if (!port->is_open.load(std::memory_order_acquire)) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }
    std::unique_lock port_lock{port->port_mutex};
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

int VideoOutDriver::ChangeBufferAttribute(VideoOutPort* port, s32 attributeIndex,
                                          const BufferAttribute* attribute) {
    std::scoped_lock lifecycle_lock{mutex};
    if (!port->is_open.load(std::memory_order_acquire)) {
        return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
    }
    if (attributeIndex >= MaxDisplayBufferGroups || !port->groups[attributeIndex].is_occupied) {
        LOG_ERROR(Lib_VideoOut, "Invalid attribute index {}", attributeIndex);
        return ORBIS_VIDEO_OUT_ERROR_INVALID_VALUE;
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
             "attributeIndex = {}, pixelFormat = {}, aspectRatio = {}, "
             "tilingMode = {}, width = {}, height = {}, pitchInPixel = {}, option = {:#x}",
             attributeIndex, GetPixelFormatString(attribute->pixel_format), attribute->aspect_ratio,
             static_cast<u32>(attribute->tiling_mode), attribute->width, attribute->height,
             attribute->pitch_in_pixel, attribute->option);

    std::unique_lock lock{port->port_mutex};
    std::memcpy(&port->groups[attributeIndex].attrib, attribute, sizeof(BufferAttribute));
    return 0;
}

bool VideoOutDriver::Flip(const Request& req) {
    std::scoped_lock lifecycle_lock{mutex};
    auto* port = req.port;
    if (!port->is_open.load(std::memory_order_acquire) ||
        port->generation.load(std::memory_order_acquire) != req.generation) {
        return false;
    }

    // Complete every guest-visible flip effect at the vblank latch. Vulkan presentation is
    // intentionally deferred to the independent host presentation thread.
    {
        std::unique_lock lock{port->port_mutex};
        auto& flip_status = port->flip_status;
        flip_status.count++;
        flip_status.process_time = Libraries::Kernel::sceKernelGetProcessTime();
        flip_status.tsc = Libraries::Kernel::sceKernelReadTsc();
        flip_status.flip_arg = req.flip_arg;
        flip_status.current_buffer = req.index;
        if (req.eop) {
            --flip_status.gc_queue_num;
        }
        --flip_status.flip_pending_num;
    }

    // Trigger flip events for the port.
    std::vector<Kernel::OrbisKernelEqueue> flip_events;
    {
        std::scoped_lock event_lock{port->event_mutex};
        flip_events = port->flip_events;
    }
    for (auto event : flip_events) {
        auto equeue = Kernel::GetEqueue(event);
        if (equeue != nullptr) {
            equeue->TriggerEvent(
                static_cast<u64>(OrbisVideoOutInternalEventId::Flip),
                Kernel::OrbisKernelEvent::Filter::VideoOut,
                reinterpret_cast<void*>(static_cast<u64>(OrbisVideoOutInternalEventId::Flip) |
                                        (req.flip_arg << 16)));
        }
    }

    // Reset prev flip label
    {
        std::scoped_lock vo_lock{port->vo_mutex};
        if (port->prev_index != -1) {
            port->buffer_labels[port->prev_index] = 0;
            port->vo_cv.notify_all();
        }
        port->prev_index = req.index;
    }
    return true;
}

void VideoOutDriver::DrawBlankFrame() {
    const auto empty_frame = presenter->PrepareBlankFrame(true);
    presenter->Present(empty_frame);
}

void VideoOutDriver::DrawLastFrame() {
    const auto frame = presenter->PrepareLastFrame();
    if (frame != nullptr) {
        const u64 generation = main_port.is_open.load(std::memory_order_acquire)
                                   ? main_port.generation.load(std::memory_order_acquire)
                                   : 0;
        presenter->Present(frame, true, generation);
    }
}

s32 VideoOutDriver::SubmitFlip(VideoOutPort* port, s32 index, s64 flip_arg,
                               bool is_eop /*= false*/) {
    u64 generation;
    {
        std::scoped_lock lifecycle_lock{mutex};
        if (!port->is_open.load(std::memory_order_acquire)) {
            return ORBIS_VIDEO_OUT_ERROR_INVALID_HANDLE;
        }
        generation = port->generation.load(std::memory_order_acquire);
        std::unique_lock lock{port->port_mutex};
        if (port->flip_status.flip_pending_num >= MaxDisplayBuffers) {
            LOG_ERROR(Lib_VideoOut, "Flip queue is full");
            return ORBIS_VIDEO_OUT_ERROR_FLIP_QUEUE_FULL;
        }

        if (is_eop) {
            ++port->flip_status.gc_queue_num;
        }
        ++port->flip_status.flip_pending_num; // integral GPU and CPU pending flips counter
        port->flip_status.submit_tsc = Libraries::Kernel::sceKernelReadTsc();
    }

    if (!is_eop) {
        // Non EOP flips can arrive from any thread so ask GPU thread to perform them
        liverpool->SendCommand(
            [=, this]() { SubmitFlipInternal(port, index, flip_arg, is_eop, generation); });
    } else {
        SubmitFlipInternal(port, index, flip_arg, is_eop, generation);
    }

    return ORBIS_OK;
}

void VideoOutDriver::SubmitFlipInternal(VideoOutPort* port, s32 index, s64 flip_arg, bool is_eop,
                                        const u64 generation) {
    Vulkan::Frame* frame;
    if (index == -1) {
        frame = presenter->PrepareBlankFrame(false);
    } else {
        VideoOutBuffer buffer;
        BufferAttributeGroup group;
        {
            std::scoped_lock lock{mutex};
            if (!port->is_open.load(std::memory_order_acquire) ||
                port->generation.load(std::memory_order_acquire) != generation) {
                return;
            }
            buffer = port->buffer_slots[index];
            ASSERT_MSG(buffer.group_index >= 0, "Trying to flip an unregistered buffer!");
            group = port->groups[buffer.group_index];
        }
        frame = presenter->PrepareFrame(group, buffer.address_left);
    }

    std::scoped_lock lock{mutex};
    if (!port->is_open.load(std::memory_order_acquire) ||
        port->generation.load(std::memory_order_acquire) != generation) {
        presenter->RecycleFrameAsync(frame);
        return;
    }
    requests.push({
        .frame = frame,
        .port = port,
        .flip_arg = flip_arg,
        .index = index,
        .eop = is_eop,
        .generation = generation,
    });
}

void VideoOutDriver::PublishFrame(PresentRequest request) {
    if (!main_port.is_open.load(std::memory_order_acquire) ||
        main_port.generation.load(std::memory_order_acquire) != request.generation) {
        presenter->RecycleFrameAsync(request.frame);
        return;
    }

    std::optional<PresentRequest> superseded;
    {
        std::scoped_lock lock{present_mutex};
        if (!main_port.is_open.load(std::memory_order_acquire) ||
            main_port.generation.load(std::memory_order_acquire) != request.generation) {
            superseded = request;
        } else {
            superseded = pending_presents.Push(request);
            blank_requested = false;
        }
    }
    if (superseded) {
        presenter->RecycleFrameAsync(superseded->frame);
    }
    present_cv.notify_one();
}

void VideoOutDriver::VblankThread(std::stop_token token) {
    const std::chrono::nanoseconds vblank_period(1000000000 /
                                                 EmulatorSettings.GetVblankFrequency());

    Common::SetCurrentThreadName("shadPS4:VblankThread");
    Common::SetCurrentThreadRealtime(vblank_period);

    // Vblank is an edge, not replayable work. A missed deadline is dropped so a host scheduling
    // stall can never produce a burst of back-to-back guest vblanks.
    Common::AccurateTimer timer{vblank_period, 0, Common::MissedTickPolicy::SkipMissed};
    u64 feedback_generation{};
    std::vector<Kernel::OrbisKernelEqueue> vblank_events;

    while (!token.stop_requested()) {
        timer.Start();

        if (DebugState.IsGuestThreadsPaused()) {
            timer.End();
            continue;
        }

        Request request;
        {
            std::scoped_lock lifecycle_lock{mutex};
            if (main_port.is_open.load(std::memory_order_acquire)) {
                std::scoped_lock vo_lock{main_port.vo_mutex};
                const u64 count = main_port.vblank_status.count;
                const int flip_rate = main_port.flip_rate.load(std::memory_order_acquire);
                if (count % (flip_rate + 1) == 0 && !requests.empty()) {
                    request = requests.front();
                    requests.pop();
                }
            }
        }

        if (request) {
            if (Flip(request)) {
                PublishFrame({
                    .frame = request.frame,
                    .hdr = request.port->is_hdr.load(std::memory_order_acquire),
                    .generation = request.generation,
                });
                FRAME_END;
            } else {
                presenter->RecycleFrameAsync(request.frame);
            }
        }

        {
            std::scoped_lock lifecycle_lock{mutex};
            if (main_port.is_open.load(std::memory_order_acquire)) {
                u64 vblank_count;
                {
                    std::scoped_lock vo_lock{main_port.vo_mutex};
                    auto& vblank_status = main_port.vblank_status;
                    // The SDK defines event data as the total count after port opening, so update
                    // status before notifying waiters and publishing the corresponding event.
                    ++vblank_status.count;
                    vblank_count = vblank_status.count;
                    vblank_status.process_time = Libraries::Kernel::sceKernelGetProcessTime();
                    vblank_status.tsc = Libraries::Kernel::sceKernelReadTsc();
                    main_port.vblank_cv.notify_all();
                }
                {
                    std::scoped_lock event_lock{main_port.event_mutex};
                    vblank_events = main_port.vblank_events;
                }
                for (auto event : vblank_events) {
                    auto equeue = Kernel::GetEqueue(event);
                    if (equeue != nullptr) {
                        equeue->TriggerEvent(
                            static_cast<u64>(OrbisVideoOutInternalEventId::Vblank),
                            Kernel::OrbisKernelEvent::Filter::VideoOut,
                            reinterpret_cast<void*>(
                                static_cast<u64>(OrbisVideoOutInternalEventId::Vblank) |
                                (vblank_count << 16)));
                    }
                }
            }
        }

        timer.End();

        // vkQueuePresentKHR completion is only a proxy for host FIFO cadence, not a physical
        // display timestamp. Use it solely for a bounded phase correction; stale or mismatched
        // samples fall back to the free-running guest timer.
        const auto feedback = presenter->GetFifoTimingFeedback();
        if (feedback.generation != feedback_generation) {
            feedback_generation = feedback.generation;
            timer.Reset();
            continue;
        }
        if (!feedback.is_fifo || feedback.last_present_call_ns == 0 ||
            feedback.present_call_period_ns == 0 || feedback.present_call_samples < 5) {
            continue;
        }

        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        const s64 now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
        const s64 guest_period_ns = vblank_period.count();
        const s64 age_ns = now_ns - feedback.last_present_call_ns;
        const s64 period_error = std::abs(feedback.present_call_period_ns - guest_period_ns);
        if (age_ns < 0 || age_ns >= 2 * guest_period_ns || period_error > guest_period_ns / 100) {
            continue;
        }

        constexpr s64 PhaseLeadNs = 2'000'000;
        constexpr s64 MaxSlewNs = 500'000;
        const s64 expected_deadline = now_ns + std::max<s64>(timer.GetTotalWait().count(), 0);
        const s64 desired_deadline = feedback.last_present_call_ns + guest_period_ns - PhaseLeadNs;
        s64 phase_error = desired_deadline - expected_deadline;
        const s64 half_period = guest_period_ns / 2;
        while (phase_error > half_period) {
            phase_error -= guest_period_ns;
        }
        while (phase_error < -half_period) {
            phase_error += guest_period_ns;
        }
        const s64 correction = std::clamp(phase_error / 10, -MaxSlewNs, MaxSlewNs);
        timer.Adjust(std::chrono::nanoseconds{correction});
    }
}

void VideoOutDriver::PresentThread(std::stop_token token) {
    Common::SetCurrentThreadName("shadPS4:PresentThread");

    using Clock = std::chrono::steady_clock;
    constexpr auto UiRedrawPeriod = std::chrono::milliseconds{33};
    constexpr auto LiveFrameSilence = std::chrono::milliseconds{100};
    Clock::time_point last_guest_present{};
    Clock::time_point last_ui_redraw{};

    while (!token.stop_requested()) {
        PresentRequest request;
        bool draw_blank = false;
        {
            std::unique_lock lock{present_mutex};
            present_cv.wait_for(lock, token, std::chrono::milliseconds{16},
                                [this] { return !pending_presents.Empty() || blank_requested; });
            if (token.stop_requested()) {
                break;
            }
            if (!pending_presents.Empty()) {
                request = pending_presents.Pop();
            } else if (blank_requested) {
                draw_blank = !main_port.is_open.load(std::memory_order_acquire);
                blank_requested = false;
            }
        }

        if (request) {
            if (!main_port.is_open.load(std::memory_order_acquire) ||
                main_port.generation.load(std::memory_order_acquire) != request.generation) {
                presenter->RecycleFrameAsync(request.frame);
                continue;
            }
            presenter->SetHDR(request.hdr);
            if (!main_port.is_open.load(std::memory_order_acquire) ||
                main_port.generation.load(std::memory_order_acquire) != request.generation) {
                presenter->RecycleFrameAsync(request.frame);
                continue;
            }
            presenter->Present(request.frame, false, request.generation);
            last_guest_present = Clock::now();
            continue;
        }

        if (draw_blank) {
            DrawBlankFrame();
            continue;
        }

        const auto now = Clock::now();
        const bool guest_paused = DebugState.IsGuestThreadsPaused();
        const bool guest_is_live = main_port.is_open.load(std::memory_order_acquire) &&
                                   last_guest_present != Clock::time_point{} &&
                                   now - last_guest_present < LiveFrameSilence;
        const bool redraw_due =
            last_ui_redraw == Clock::time_point{} || now - last_ui_redraw >= UiRedrawPeriod;
        if (redraw_due && !guest_is_live && (guest_paused || ImGui::Core::MustKeepDrawing())) {
            DrawLastFrame();
            last_ui_redraw = Clock::now();
        }
    }
}

} // namespace Libraries::VideoOut
