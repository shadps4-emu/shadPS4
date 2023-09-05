#pragma once
#include <Core/PS4/HLE/Graphics/video_out.h>
#include <Lib/Threads.h>

using namespace HLE::Libs::Graphics::VideoOut;

namespace HLE::Graphics::Objects {

struct VideoConfigInternal {
    Lib::Mutex m_mutex;
    SceVideoOutResolutionStatus m_resolution;
    bool isOpened = false;
    SceVideoOutFlipStatus m_flip_status;
    SceVideoOutVblankStatus m_vblank_status;
    std::vector<HLE::Libs::LibKernel::EventQueues::SceKernelEqueue> m_flip_evtEq;
};

class VideoOutCtx {

  public:
    VideoOutCtx() {}
    virtual ~VideoOutCtx() {}
    void Init(u32 width, u32 height);
    int Open();
    VideoConfigInternal* getCtx(int handle);
  private:
    Lib::Mutex m_mutex;
    VideoConfigInternal m_video_out_ctx;
};
};  // namespace HLE::Graphics::Objects