// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include "core/libraries/videoout/video_out.h"

namespace Vulkan {
struct Frame;
class RendererVulkan;
} // namespace Vulkan

namespace Libraries::VideoOut {

struct VideoOutPort {
    bool is_open = false;
    SceVideoOutResolutionStatus resolution;
    std::array<VideoOutBuffer, MaxDisplayBuffers> buffer_slots;
    std::array<uintptr_t, MaxDisplayBuffers> buffer_labels; // should be contiguous in memory
    static_assert(sizeof(buffer_labels[0]) == 8u);
    std::array<BufferAttributeGroup, MaxDisplayBufferGroups> groups;
    FlipStatus flip_status;
    SceVideoOutVblankStatus vblank_status;
    std::vector<Kernel::SceKernelEqueue> flip_events;
    std::vector<Kernel::SceKernelEqueue> vblank_events;
    int flip_rate = 0;

    s32 FindFreeGroup() const {
        s32 index = 0;
        while (index < groups.size() && groups[index].is_occupied) {
            index++;
        }
        return index;
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
    explicit VideoOutDriver(u32 width, u32 height);
    ~VideoOutDriver();

    int Open(const ServiceThreadParams* params);
    void Close(s32 handle);

    VideoOutPort* GetPort(s32 handle);

    int RegisterBuffers(VideoOutPort* port, s32 startIndex, void* const* addresses, s32 bufferNum,
                        const BufferAttribute* attribute);
    int UnregisterBuffers(VideoOutPort* port, s32 attributeIndex);

    void Flip(std::chrono::microseconds timeout);
    bool SubmitFlip(VideoOutPort* port, s32 index, s64 flip_arg, bool is_eop = false);

    void Vblank();

private:
    struct Request {
        Vulkan::Frame* frame;
        VideoOutPort* port;
        s32 index;
        s64 flip_arg;
        u64 submit_tsc;
        bool eop;
    };

    std::mutex mutex;
    VideoOutPort main_port{};
    std::condition_variable_any submit_cond;
    std::condition_variable done_cond;
    std::queue<Request> requests;
    std::unique_ptr<Vulkan::RendererVulkan> renderer;
    bool is_neo{};
};

} // namespace Libraries::VideoOut
