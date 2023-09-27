#include "video_out_buffer.h"

#include <Util/log.h>

#include "debug.h"
#include <vulkan_util.h>

constexpr bool log_file_videoOutBuffer = true;  // disable it to disable logging

static void update_func(HLE::Libs::Graphics::GraphicCtx* ctx, const u64* params, void* obj, const u64* virtual_addr, const u64* size,
                        int virtual_addr_num) {

    auto pitch = params[GPU::VideoOutBufferObj::PITCH_PARAM];

    auto* vk_obj = static_cast<HLE::Libs::Graphics::VideoOutVulkanImage*>(obj);

    vk_obj->layout = VK_IMAGE_LAYOUT_UNDEFINED;

    Graphics::Vulkan::vulkanFillImage(ctx, vk_obj, reinterpret_cast<void*>(*virtual_addr), *size, pitch,
                                    static_cast<uint64_t>(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
}

static void* create_func(HLE::Libs::Graphics::GraphicCtx* ctx, const u64* params, const u64* virtual_addr, const u64* size, int virtual_addr_num,
                         HLE::Libs::Graphics::VulkanMemory* mem) {
    auto pixel_format = params[GPU::VideoOutBufferObj::PIXEL_FORMAT_PARAM];
    auto width = params[GPU::VideoOutBufferObj::WIDTH_PARAM];
    auto height = params[GPU::VideoOutBufferObj::HEIGHT_PARAM];

    auto* vk_obj = new HLE::Libs::Graphics::VideoOutVulkanImage;

    VkFormat vk_format = VK_FORMAT_UNDEFINED;

    switch (pixel_format) {
        case static_cast<uint64_t>(GPU::VideoOutBufferFormat::R8G8B8A8Srgb): vk_format = VK_FORMAT_R8G8B8A8_SRGB; break;
        case static_cast<uint64_t>(GPU::VideoOutBufferFormat::B8G8R8A8Srgb): vk_format = VK_FORMAT_B8G8R8A8_SRGB; break;
        default: LOG_CRITICAL_IF(log_file_videoOutBuffer, "unknown pixelFormat  = {}\n", pixel_format); std::exit(0);
    }

    vk_obj->extent.width = width;
    vk_obj->extent.height = height;
    vk_obj->format = vk_format;
    vk_obj->image = nullptr;
    vk_obj->layout = VK_IMAGE_LAYOUT_UNDEFINED;

    for (auto& view : vk_obj->image_view) {
        view = nullptr;
    }

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = vk_obj->extent.width;
    image_info.extent.height = vk_obj->extent.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = vk_obj->format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = vk_obj->layout;
    image_info.usage =
        static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT) | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;

    vkCreateImage(ctx->m_device, &image_info, nullptr, &vk_obj->image);

    if (vk_obj->image == nullptr) {
        LOG_CRITICAL_IF(log_file_videoOutBuffer, "vk_obj->image is null\n");
        std::exit(0);
    }

    vkGetImageMemoryRequirements(ctx->m_device, vk_obj->image, &mem->requirements);

    mem->property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    bool allocated = GPU::vulkanAllocateMemory(ctx, mem);

    if (!allocated) {
        LOG_CRITICAL_IF(log_file_videoOutBuffer, "can't allocate vulkan memory\n");
        std::exit(0);
    }

    vkBindImageMemory(ctx->m_device, vk_obj->image, mem->memory, mem->offset);

    vk_obj->memory = *mem;

    LOG_INFO_IF(log_file_videoOutBuffer, "videoOutBuffer create object\n");
    LOG_INFO_IF(log_file_videoOutBuffer, "mem  requirements.size = {}\n", mem->requirements.size);
    LOG_INFO_IF(log_file_videoOutBuffer, "width                  = {}\n", width);
    LOG_INFO_IF(log_file_videoOutBuffer, "height                 = {}\n", height);
    LOG_INFO_IF(log_file_videoOutBuffer, "size                   = {}\n", *size);

    update_func(ctx, params, vk_obj, virtual_addr, size, virtual_addr_num);

    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.image = vk_obj->image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = vk_obj->format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.layerCount = 1;
    create_info.subresourceRange.levelCount = 1;

    vkCreateImageView(ctx->m_device, &create_info, nullptr, &vk_obj->image_view[HLE::Libs::Graphics::VulkanImage::VIEW_DEFAULT]);

    if (vk_obj->image_view[HLE::Libs::Graphics::VulkanImage::VIEW_DEFAULT] == nullptr) {
        LOG_CRITICAL_IF(log_file_videoOutBuffer, "vk_obj->image_view is null\n");
        std::exit(0);
    }

    return vk_obj;
}

GPU::GPUObject::create_func_t GPU::VideoOutBufferObj::getCreateFunc() const { return create_func; }

GPU::GPUObject::update_func_t GPU::VideoOutBufferObj::getUpdateFunc() const { return update_func; }
