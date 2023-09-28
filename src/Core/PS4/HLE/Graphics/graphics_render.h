#pragma once
#include "graphics_ctx.h"

namespace GPU {

class CommandPool {
  public:
    CommandPool() = default;
    ~CommandPool() {}

    HLE::Libs::Graphics::VulkanCommandPool* getPool(int id) {
        if (m_pool[id] == nullptr) {
            createPool(id);
        }
        return m_pool[id];
    }

  private:
    void createPool(int id);
    void deleteAllPool();

    HLE::Libs::Graphics::VulkanCommandPool* m_pool[11] = {};
};
class CommandBuffer {
  public:
    explicit CommandBuffer(int queue) : m_queue(queue) { allocateBuffer(); }
    virtual ~CommandBuffer() { freeBuffer(); }
    void allocateBuffer();
    void freeBuffer();
    void waitForFence();
    void begin() const;
    void end() const;
    void executeWithSemaphore();
    void execute();
    u32 getIndex() const { return m_index; }
    HLE::Libs::Graphics::VulkanCommandPool* getPool() { return m_pool; }
  private:
    int m_queue = -1;
    u32 m_index = static_cast<u32>(-1);
    HLE::Libs::Graphics::VulkanCommandPool* m_pool = nullptr;
    bool m_execute = false;
};

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
    HLE::Libs::Graphics::GraphicCtx* getGraphicCtx() { return m_graphic_ctx; }

  private:
    Framebuffer* m_framebuffer = nullptr;
    HLE::Libs::Graphics::GraphicCtx* m_graphic_ctx = nullptr;
};

void renderCreateCtx();
};  // namespace GPU