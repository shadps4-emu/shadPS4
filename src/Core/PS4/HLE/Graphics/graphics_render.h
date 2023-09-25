#pragma once
#include "graphics_ctx.h"

namespace GPU {

class Framebuffer {
  public:
    Framebuffer() {}
    virtual ~Framebuffer() {}
};
class RenderCtx {
  public:
    RenderCtx() : m_framebuffer(new Framebuffer) {}

    virtual ~RenderCtx() {}
    void setGraphicCtx(HLE::Libs::Graphics::GraphicCtx* ctx) { m_graphic_ctx = ctx; }
  private:
    Framebuffer* m_framebuffer = nullptr;
    HLE::Libs::Graphics::GraphicCtx* m_graphic_ctx = nullptr;
};


void renderCreateCtx();
};  // namespace GPU