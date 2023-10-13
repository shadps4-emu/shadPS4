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
    return &m_video_out_ctx;  // assuming that it's the only ctx TODO check if we need more
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

bool FlipQueue::flip(u32 micros) { 
    m_mutex.LockMutex();
    if (m_requests.size() == 0) {
        m_submit_cond.WaitCondVarFor(&m_mutex, micros);

        if (m_requests.size() == 0) {
            m_mutex.UnlockMutex();
            return false;
        }
    }
    auto request = m_requests.at(0);  // proceed first request
    m_mutex.UnlockMutex();

   auto* buffer = request.cfg->buffers[request.index].buffer_render;

    Emu::DrawBuffer(buffer);

    m_mutex.LockMutex();

    request.cfg->m_mutex.LockMutex();
    for (auto& flip_eq : request.cfg->m_flip_evtEq) {
        if (flip_eq != nullptr) {
            flip_eq->triggerEvent(SCE_VIDEO_OUT_EVENT_FLIP, HLE::Kernel::Objects::EVFILT_VIDEO_OUT, reinterpret_cast<void*>(request.flip_arg));
        }
    }
    request.cfg->m_mutex.UnlockMutex();

    m_requests.erase(m_requests.begin());
    m_done_cond.SignalCondVar();

    request.cfg->m_flip_status.count++;
    //TODO request.cfg->m_flip_status.processTime = LibKernel::KernelGetProcessTime();
    request.cfg->m_flip_status.tsc = HLE::Libs::LibKernel::sceKernelReadTsc();
    request.cfg->m_flip_status.submitTsc = request.submit_tsc;
    request.cfg->m_flip_status.flipArg = request.flip_arg;
    request.cfg->m_flip_status.currentBuffer = request.index;
    request.cfg->m_flip_status.flipPendingNum = static_cast<int>(m_requests.size());

    m_mutex.UnlockMutex();

    return false; 
}

};  // namespace HLE::Graphics::Objects
