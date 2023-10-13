#include "graphics_render.h"

#include "Util/Singleton.h"
#include "emulator.h"

static thread_local GPU::CommandPool g_command_pool;

void GPU::renderCreateCtx() {
    auto* render_ctx = Singleton<RenderCtx>::Instance();

    render_ctx->setGraphicCtx(Emu::getGraphicCtx());
}

void GPU::CommandBuffer::allocateBuffer() {
    m_pool = g_command_pool.getPool(m_queue);

    Lib::LockMutexGuard lock(m_pool->mutex);

    for (uint32_t i = 0; i < m_pool->buffers_count; i++) {
        if (!m_pool->busy[i]) {
            m_pool->busy[i] = true;
            vkResetCommandBuffer(m_pool->buffers[i], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
            m_index = i;
            break;
        }
    }
}

void GPU::CommandBuffer::freeBuffer() {
    Lib::LockMutexGuard lock(m_pool->mutex);

    waitForFence();

    m_pool->busy[m_index] = false;
    vkResetCommandBuffer(m_pool->buffers[m_index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    m_index = static_cast<uint32_t>(-1);
}

void GPU::CommandBuffer::waitForFence() {
    auto* render_ctx = Singleton<RenderCtx>::Instance();

    if (m_execute) {
        auto* device = render_ctx->getGraphicCtx()->m_device;

        vkWaitForFences(device, 1, &m_pool->fences[m_index], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &m_pool->fences[m_index]);

        m_execute = false;
    }
}
void GPU::CommandBuffer::begin() const {
    auto* buffer = m_pool->buffers[m_index];

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    auto result = vkBeginCommandBuffer(buffer, &begin_info);

    if (result != VK_SUCCESS) {
        printf("vkBeginCommandBuffer failed\n");
        std::exit(0);
    }
}
void GPU::CommandBuffer::end() const {
    auto* buffer = m_pool->buffers[m_index];

    auto result = vkEndCommandBuffer(buffer);

    if (result != VK_SUCCESS) {
        printf("vkEndCommandBuffer failed\n");
        std::exit(0);
    }
}
void GPU::CommandBuffer::executeWithSemaphore() {
    auto* buffer = m_pool->buffers[m_index];
    auto* fence = m_pool->fences[m_index];

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &m_pool->semaphores[m_index];

    auto* render_ctx = Singleton<RenderCtx>::Instance();
    const auto& queue = render_ctx->getGraphicCtx()->queues[m_queue];

    if (queue.mutex != nullptr) {
        queue.mutex->LockMutex();
    }

    auto result = vkQueueSubmit(queue.vk_queue, 1, &submit_info, fence);

    if (queue.mutex != nullptr) {
        queue.mutex->LockMutex();
    }

    m_execute = true;

    if (result != VK_SUCCESS) {
        printf("vkQueueSubmit failed\n");
        std::exit(0);
    }
}
void GPU::CommandBuffer::execute() {
    auto* buffer = m_pool->buffers[m_index];
    auto* fence = m_pool->fences[m_index];

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    auto* render_ctx = Singleton<RenderCtx>::Instance();
    const auto& queue = render_ctx->getGraphicCtx()->queues[m_queue];

    if (queue.mutex != nullptr) {
        queue.mutex->LockMutex();
    }

    auto result = vkQueueSubmit(queue.vk_queue, 1, &submit_info, fence);

    if (queue.mutex != nullptr) {
        queue.mutex->UnlockMutex();
    }

    m_execute = true;

    if (result != VK_SUCCESS) {
        printf("vkQueueSubmit failed\n");
        std::exit(0);
    }
}
void GPU::CommandPool::createPool(int id) {
    auto* render_ctx = Singleton<RenderCtx>::Instance();
    auto* ctx = render_ctx->getGraphicCtx();

    m_pool[id] = new HLE::Libs::Graphics::VulkanCommandPool;

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.pNext = nullptr;
    pool_info.queueFamilyIndex = ctx->queues[id].family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(ctx->m_device, &pool_info, nullptr, &m_pool[id]->pool);

    if (m_pool[id]->pool == nullptr) {
        printf("pool is nullptr");
        std::exit(0);
    }

    m_pool[id]->buffers_count = 4;
    m_pool[id]->buffers = new VkCommandBuffer[m_pool[id]->buffers_count];
    m_pool[id]->fences = new VkFence[m_pool[id]->buffers_count];
    m_pool[id]->semaphores = new VkSemaphore[m_pool[id]->buffers_count];
    m_pool[id]->busy = new bool[m_pool[id]->buffers_count];

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = m_pool[id]->pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = m_pool[id]->buffers_count;

    if (vkAllocateCommandBuffers(ctx->m_device, &alloc_info, m_pool[id]->buffers) != VK_SUCCESS) {
        printf("Can't allocate command buffers\n");
        std::exit(0);
    }

    for (uint32_t i = 0; i < m_pool[id]->buffers_count; i++) {
        m_pool[id]->busy[i] = false;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.pNext = nullptr;
        fence_info.flags = 0;

        if (vkCreateFence(ctx->m_device, &fence_info, nullptr, &m_pool[id]->fences[i]) != VK_SUCCESS) {
            printf("Can't create fence\n");
            std::exit(0);
        }

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = nullptr;
        semaphore_info.flags = 0;

        if (vkCreateSemaphore(ctx->m_device, &semaphore_info, nullptr, &m_pool[id]->semaphores[i]) != VK_SUCCESS) {
            printf("Can't create semas\n");
            std::exit(0);
        }
    }
}

void GPU::CommandPool::deleteAllPool() {
    auto* render_ctx = Singleton<RenderCtx>::Instance();
    auto* ctx = render_ctx->getGraphicCtx();

    for (auto& pool : m_pool) {
        if (pool != nullptr) {
            for (uint32_t i = 0; i < pool->buffers_count; i++) {
                vkDestroySemaphore(ctx->m_device, pool->semaphores[i], nullptr);
                vkDestroyFence(ctx->m_device, pool->fences[i], nullptr);
            }

            vkFreeCommandBuffers(ctx->m_device, pool->pool, pool->buffers_count, pool->buffers);

            vkDestroyCommandPool(ctx->m_device, pool->pool, nullptr);

            delete[] pool->semaphores;
            delete[] pool->fences;
            delete[] pool->buffers;
            delete[] pool->busy;

            delete pool;
            pool = nullptr;
        }
    }
}
