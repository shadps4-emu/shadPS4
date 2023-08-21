#pragma once
#include <Core/PS4/HLE/Graphics/video_out.h>
#include <Lib/Threads.h>

using namespace HLE::Libs::Graphics::VideoOut;

namespace HLE::Graphics::Objects {

struct VideoConfigInternal {
    Lib::Mutex m_mutex;
    SceVideoOutResolutionStatus m_resolution;
    bool isOpened = false;
};

class VideoOutCtx {
    static constexpr int MAX_VIDEO_OUT = 2;

  public:
    VideoOutCtx() {}
    virtual ~VideoOutCtx() {}
    void Init(u32 width, u32 height);
    void Open();
  private:
    Lib::Mutex m_mutex;
    VideoConfigInternal m_video_out_ctx[MAX_VIDEO_OUT];
};
};  // namespace HLE::Graphics::Objects