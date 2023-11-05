#pragma once

#include <condition_variable>
#include <mutex>
#include <core/PS4/HLE/Graphics/video_out.h>
#include <core/PS4/HLE/Graphics/graphics_ctx.h>
#include <emulator.h>

using namespace HLE::Libs::Graphics::VideoOut;

namespace HLE::Graphics::Objects {

struct VideoOutBufferInfo {
    const void* buffer = nullptr;
    HLE::Libs::Graphics::VideoOutVulkanImage* buffer_render = nullptr;
    u64 buffer_size = 0;
    u64 buffer_pitch = 0;
    int set_id = 0;
};

struct VideoConfigInternal {
    std::mutex m_mutex;
    SceVideoOutResolutionStatus m_resolution;
    bool isOpened = false;
    SceVideoOutFlipStatus m_flip_status;
    SceVideoOutVblankStatus m_vblank_status;
    std::vector<Core::Kernel::SceKernelEqueue> m_flip_evtEq;
    int m_flip_rate = 0;
    VideoOutBufferInfo buffers[16];
    std::vector<VideoOutBufferSetInternal> buffers_sets;
    int buffers_registration_index = 0;
};

class FlipQueue {
  public:
    FlipQueue() {}
    virtual ~FlipQueue() {}

    void getFlipStatus(VideoConfigInternal* cfg, SceVideoOutFlipStatus* out);
    bool submitFlip(VideoConfigInternal* cfg, s32 index, s64 flip_arg);
    bool flip(u32 micros);
  private:
    struct Request {
        VideoConfigInternal* cfg;
        int index;
        int64_t flip_arg;
        uint64_t submit_tsc;
    };

    std::mutex m_mutex;
    std::condition_variable m_submit_cond;
    std::condition_variable m_done_cond;
    std::vector<Request> m_requests;
};

class VideoOutCtx {

  public:
    VideoOutCtx() {}
    virtual ~VideoOutCtx() {}
    void Init(u32 width, u32 height);
    int Open();
    void Close(s32 handle);
    VideoConfigInternal* getCtx(int handle);
    FlipQueue& getFlipQueue() { return m_flip_queue; }
    HLE::Libs::Graphics::GraphicCtx* getGraphicCtx() {
        std::scoped_lock lock{m_mutex};
        
        if (!m_graphic_ctx) {
            m_graphic_ctx = Emu::getGraphicCtx();
        }

        return m_graphic_ctx;
    }
  private:
    std::mutex m_mutex;
    VideoConfigInternal m_video_out_ctx;
    FlipQueue m_flip_queue;
    HLE::Libs::Graphics::GraphicCtx* m_graphic_ctx = nullptr;
};

} // namespace HLE::Graphics::Objects
