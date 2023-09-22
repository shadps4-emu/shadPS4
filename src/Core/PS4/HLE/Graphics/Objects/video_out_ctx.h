#pragma once
#include <Core/PS4/HLE/Graphics/video_out.h>
#include <Lib/Threads.h>

using namespace HLE::Libs::Graphics::VideoOut;

namespace HLE::Graphics::Objects {

//class FlipQueue;

struct VideoConfigInternal {
    Lib::Mutex m_mutex;
    SceVideoOutResolutionStatus m_resolution;
    bool isOpened = false;
    SceVideoOutFlipStatus m_flip_status;
    SceVideoOutVblankStatus m_vblank_status;
    std::vector<HLE::Libs::LibKernel::EventQueues::SceKernelEqueue> m_flip_evtEq;
    int m_flip_rate = 0;

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

    Lib::Mutex m_mutex;
    Lib::ConditionVariable m_submit_cond;
    Lib::ConditionVariable m_done_cond;
    std::vector<Request> m_requests;
};

class VideoOutCtx {

  public:
    VideoOutCtx() {}
    virtual ~VideoOutCtx() {}
    void Init(u32 width, u32 height);
    int Open();
    VideoConfigInternal* getCtx(int handle);
    FlipQueue& getFlipQueue() { return m_flip_queue; }
  private:
    Lib::Mutex m_mutex;
    VideoConfigInternal m_video_out_ctx;
    FlipQueue m_flip_queue;
};
};  // namespace HLE::Graphics::Objects