#include "video_out_ctx.h"
#include <Core/PS4/HLE/LibKernel.h>

namespace HLE::Graphics::Objects {

void VideoOutCtx::Init(u32 width, u32 height) {
    m_video_out_ctx.m_resolution.fullWidth = width;
    m_video_out_ctx.m_resolution.fullHeight = height;
    m_video_out_ctx.m_resolution.paneWidth = width;
    m_video_out_ctx.m_resolution.paneHeight = height;
}
int VideoOutCtx::Open() {
    Lib::LockMutexGuard lock(m_mutex);

    int handle = -1;

    if (!m_video_out_ctx.isOpened) {
        handle = 1;  // positive return , should be more than 1 ?
    }

    m_video_out_ctx.isOpened = true;
    m_video_out_ctx.m_flip_status = SceVideoOutFlipStatus();
    m_video_out_ctx.m_flip_status.flipArg = -1;
    m_video_out_ctx.m_flip_status.currentBuffer = -1;
    m_video_out_ctx.m_flip_status.count = 0;
    m_video_out_ctx.m_vblank_status = SceVideoOutVblankStatus();

    return handle;
}

VideoConfigInternal* VideoOutCtx::getCtx(int handle) {
    if (handle != 1) return nullptr;
    return &m_video_out_ctx; // assuming that it's the only ctx TODO check if we need more
}

void FlipQueue::getFlipStatus(VideoConfigInternal* cfg, SceVideoOutFlipStatus* out) {
    Lib::LockMutexGuard lock(m_mutex);

    *out = cfg->m_flip_status;
}

bool FlipQueue::submitFlip(VideoConfigInternal* cfg, s32 index, s64 flip_arg) { 
    Lib::LockMutexGuard lock(m_mutex);

    if (m_requests.size() >= 2) {
        return false;
    }

    Request r{};
    r.cfg = cfg;
    r.index = index;
    r.flip_arg = flip_arg;
    r.submit_tsc = HLE::Libs::LibKernel::sceKernelReadTsc();

    m_requests.push_back(r);

    cfg->m_flip_status.flipPendingNum = static_cast<int>(m_requests.size());
    cfg->m_flip_status.gcQueueNum = 0;

    m_submit_cond.SignalCondVar();

    return true;
}

};  // namespace HLE::Graphics::Objects

