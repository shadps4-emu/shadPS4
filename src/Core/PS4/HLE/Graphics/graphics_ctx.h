#pragma once

#include <types.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <mutex>

namespace HLE::Libs::Graphics {

struct VulkanCommandPool {
    std::mutex mutex;
    VkCommandPool pool = nullptr;
    std::vector<VkCommandBuffer> buffers;
    std::vector<VkFence> fences;
    std::vector<VkSemaphore> semaphores;
    std::vector<bool> busy;
    u32 buffers_count = 0;
};

struct VulkanQueueInfo {
    std::unique_ptr<std::mutex> mutex{};
    u32 family = static_cast<u32>(-1);
    u32 index = static_cast<u32>(-1);
    VkQueue vk_queue = nullptr;
};

struct GraphicCtx {
    u32 screen_width = 0;
    u32 screen_height = 0;
    VkInstance m_instance = nullptr;
    VkPhysicalDevice m_physical_device = nullptr;
    VkDevice m_device = nullptr;
    VulkanQueueInfo queues[11];  // VULKAN_QUEUES_NUM
};

enum class VulkanImageType { Unknown, VideoOut };

struct VulkanMemory {
    VkMemoryRequirements requirements = {};
    VkMemoryPropertyFlags property = 0;
    VkDeviceMemory memory = nullptr;
    VkDeviceSize offset = 0;
    u32 type = 0;
    u64 unique_id = 0;
};

struct VulkanBuffer {
    VkBuffer buffer = nullptr;
    VulkanMemory memory;
    VkBufferUsageFlags usage = 0;
};
struct VulkanImage {
    static constexpr int VIEW_MAX = 4;
    static constexpr int VIEW_DEFAULT = 0;
    static constexpr int VIEW_BGRA = 1;
    static constexpr int VIEW_DEPTH_TEXTURE = 2;

    explicit VulkanImage(VulkanImageType type) : type(type) {}

    VulkanImageType type = VulkanImageType::Unknown;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent = {};
    VkImage image = nullptr;
    VkImageView image_view[VIEW_MAX] = {};
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VulkanMemory memory;
};

struct VideoOutVulkanImage : public VulkanImage {
    VideoOutVulkanImage() : VulkanImage(VulkanImageType::VideoOut) {}
};

}  // namespace HLE::Libs::Graphics
