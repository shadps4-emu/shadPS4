#include "video_out_buffer.h"

static void* create_func(HLE::Libs::Graphics::GraphicCtx* ctx, const u64* params, const u64* vaddr, const u64* size, int vaddr_num,
                         HLE::Libs::Graphics::VulkanMemory* mem) {
    return nullptr;//TODO
}

GPU::GPUObject::create_func_t GPU::VideoOutBufferObj::getCreateFunc() const { return create_func; }