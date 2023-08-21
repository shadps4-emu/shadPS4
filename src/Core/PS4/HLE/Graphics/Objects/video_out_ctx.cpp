#include "video_out_ctx.h"

namespace HLE::Graphics::Objects {

void VideoOutCtx::Init(u32 width, u32 height) {
    for (auto& video_ctx : m_video_out_ctx) {
        video_ctx.m_resolution.fullWidth = width;
        video_ctx.m_resolution.fullHeight = height;
        video_ctx.m_resolution.paneWidth = width;
        video_ctx.m_resolution.paneHeight = height;
    }
}
void VideoOutCtx::Open() {}
};  // namespace HLE::Graphics::Objects