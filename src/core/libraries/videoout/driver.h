// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "common/debug.h"
#include "common/polyfill_thread.h"
#include "core/libraries/videoout/presentation_queue.h"
#include "core/libraries/videoout/video_out.h"

namespace Vulkan {
struct Frame;
}

namespace Libraries::VideoOut {

struct VideoOutPort {
    SceVideoOutResolutionStatus resolution;
    std::array<VideoOutBuffer, MaxDisplayBuffers> buffer_slots;
    std::array<u64, MaxDisplayBuffers> buffer_labels; // should be contiguous in memory
    static_assert(sizeof(buffer_labels[0]) == 8u);
    std::array<BufferAttributeGroup, MaxDisplayBufferGroups> groups;
    FlipStatus flip_status;
    SceVideoOutVblankStatus vblank_status;
    std::vector<Kernel::OrbisKernelEqueue> flip_events;
    std::vector<Kernel::OrbisKernelEqueue> vblank_events;
    std::mutex event_mutex;
    std::mutex vo_mutex;
    std::mutex port_mutex;
    std::condition_variable vo_cv;
    std::condition_variable vblank_cv;
    std::atomic<int> flip_rate{0};
    int prev_index = -1;
    std::atomic_bool is_open{false};
    std::atomic_bool is_hdr{false};
    std::atomic<u64> generation{0};

    s32 FindFreeGroup() const {
        s32 index = 0;
        while (index < groups.size() && groups[index].is_occupied) {
            index++;
        }
        return index;
    }

    bool IsVoLabel(const u64* address) const {
        const u64* start = &buffer_labels[0];
        const u64* end = &buffer_labels[MaxDisplayBuffers - 1];
        return address >= start && address <= end;
    }

    void WaitVoLabel(auto&& pred) {
        std::unique_lock lk{vo_mutex};
        vo_cv.wait(lk, pred);
    }

    void SignalVoLabel() {
        std::scoped_lock lk{vo_mutex};
        vo_cv.notify_one();
    }

    [[nodiscard]] int NumRegisteredBuffers() const {
        return std::count_if(buffer_slots.cbegin(), buffer_slots.cend(),
                             [](auto& buffer) { return buffer.group_index != -1; });
    }
};

struct ServiceThreadParams {
    u32 unknown;
    bool set_priority;
    u32 priority;
    bool set_affinity;
    u64 affinity;
};

class VideoOutDriver {
public:
    VideoOutDriver(u32 width, u32 height);
    ~VideoOutDriver();

    int Open(const ServiceThreadParams* params);
    s32 Close(s32 handle);

    VideoOutPort* GetPort(s32 handle);

    int RegisterBuffers(VideoOutPort* port, s32 startIndex, void* const* addresses, s32 bufferNum,
                        const BufferAttribute* attribute);
    int UnregisterBuffers(VideoOutPort* port, s32 attributeIndex);
    int ChangeBufferAttribute(VideoOutPort* port, s32 bufferIndex,
                              const BufferAttribute* attribute);

    s32 SubmitFlip(VideoOutPort* port, s32 index, s64 flip_arg, bool is_eop = false);

private:
    struct Request {
        Vulkan::Frame* frame{};
        VideoOutPort* port{};
        s64 flip_arg{};
        s32 index{};
        bool eop{};
        u64 generation{};

        operator bool() const noexcept {
            return frame != nullptr;
        }
    };

    struct PresentRequest {
        Vulkan::Frame* frame{};
        bool hdr{};
        u64 generation{};

        operator bool() const noexcept {
            return frame != nullptr;
        }
    };

    bool Flip(const Request& req);
    void DrawBlankFrame(); // Video port out not open
    void DrawLastFrame();  // Used when there is no flip request
    void SubmitFlipInternal(VideoOutPort* port, s32 index, s64 flip_arg, bool is_eop,
                            u64 generation);
    void PublishFrame(PresentRequest request);
    void VblankThread(std::stop_token token);
    void PresentThread(std::stop_token token);

    std::mutex mutex;
    VideoOutPort main_port{};
    std::jthread vblank_thread;
    std::jthread present_thread;
    std::queue<Request> requests;
    std::mutex present_mutex;
    std::condition_variable_any present_cv;
    PresentationQueue<PresentRequest> pending_presents;
    bool blank_requested{true};
};

} // namespace Libraries::VideoOut
