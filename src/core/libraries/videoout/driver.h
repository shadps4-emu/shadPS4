// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/debug.h"
#include "common/polyfill_thread.h"
//#include "core/libraries/videoout/video_out.h"

#include <condition_variable>
#include <mutex>
#include <queue>

#include "core/libraries/kernel/equeue.h"
#include "core/libraries/videoout/buffer.h"
#include "core/libraries/videoout/flip_status.h"
#include "core/libraries/videoout/sce_video_out_resolution_status.h"
#include "core/libraries/videoout/sce_video_out_vblank_status.h"
#include "video_core/renderer_vulkan/vk_presenter.h"

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
    std::vector<Kernel::EqueueInternal*> flip_events;
    std::vector<Kernel::EqueueInternal*> vblank_events;
    std::mutex vo_mutex;
    std::mutex port_mutex;
    std::condition_variable vo_cv;
    std::condition_variable vblank_cv;
    int flip_rate = 0;
    int prev_index = -1;
    bool is_open = false;
    bool is_hdr = false;

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
    VideoOutDriver(u32 width, u32 height, Vulkan::Presenter& presenter);
    ~VideoOutDriver();

    int Open(const ServiceThreadParams* params);
    void Close(s32 handle);

    VideoOutPort* GetPort(s32 handle);

    int RegisterBuffers(VideoOutPort* port, s32 startIndex, void* const* addresses, s32 bufferNum,
                        const BufferAttribute* attribute);
    int UnregisterBuffers(VideoOutPort* port, s32 attributeIndex);

    bool SubmitFlip(VideoOutPort* port, s32 index, s64 flip_arg, bool is_eop = false);

    std::jthread present_thread;
    std::condition_variable cond_var;

private:
    struct Request {
        Vulkan::Frame* frame;
        VideoOutPort* port;
        s64 flip_arg;
        s32 index;
        bool eop;

        operator bool() const noexcept {
            return frame != nullptr;
        }
    };

    void Flip(const Request& req);
    void DrawBlankFrame(); // Video port out not open
    void DrawLastFrame();  // Used when there is no flip request
    void SubmitFlipInternal(VideoOutPort* port, s32 index, s64 flip_arg, bool is_eop = false);
    void PresentThread(std::stop_token token);

    Vulkan::Presenter& m_presenter;
    std::mutex mutex;
    VideoOutPort main_port{};
    std::queue<Request> requests;
};

} // namespace Libraries::VideoOut
