// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/texture_cache/image_view.h"
#include "video_core/texture_cache/tile_manager.h"

#include "video_core/host_shaders/detile_m32x1_comp.h"
#include "video_core/host_shaders/detile_m32x2_comp.h"
#include "video_core/host_shaders/detile_m32x4_comp.h"
#include "video_core/host_shaders/detile_m8x1_comp.h"
#include "video_core/host_shaders/detile_m8x2_comp.h"

#include <boost/container/static_vector.hpp>
#include <magic_enum.hpp>
#include <vk_mem_alloc.h>

namespace VideoCore {

class TileManager32 {
public:
    u32 m_macro_tile_height = 0;
    u32 m_bank_height = 0;
    u32 m_num_banks = 0;
    u32 m_num_pipes = 0;
    u32 m_padded_width = 0;
    u32 m_padded_height = 0;
    u32 m_pipe_bits = 0;
    u32 m_bank_bits = 0;

    void Init(u32 width, u32 height, bool is_neo) {
        m_macro_tile_height = (is_neo ? 128 : 64);
        m_bank_height = is_neo ? 2 : 1;
        m_num_banks = is_neo ? 8 : 16;
        m_num_pipes = is_neo ? 16 : 8;
        m_padded_width = width;
        if (height == 1080) {
            m_padded_height = is_neo ? 1152 : 1088;
        }
        if (height == 720) {
            m_padded_height = 768;
        }
        m_pipe_bits = is_neo ? 4 : 3;
        m_bank_bits = is_neo ? 3 : 4;
    }

    static u32 getElementIdx(u32 x, u32 y) {
        u32 elem = 0;
        elem |= ((x >> 0u) & 0x1u) << 0u;
        elem |= ((x >> 1u) & 0x1u) << 1u;
        elem |= ((y >> 0u) & 0x1u) << 2u;
        elem |= ((x >> 2u) & 0x1u) << 3u;
        elem |= ((y >> 1u) & 0x1u) << 4u;
        elem |= ((y >> 2u) & 0x1u) << 5u;

        return elem;
    }

    static u32 getPipeIdx(u32 x, u32 y, bool is_neo) {
        u32 pipe = 0;

        if (!is_neo) {
            pipe |= (((x >> 3u) ^ (y >> 3u) ^ (x >> 4u)) & 0x1u) << 0u;
            pipe |= (((x >> 4u) ^ (y >> 4u)) & 0x1u) << 1u;
            pipe |= (((x >> 5u) ^ (y >> 5u)) & 0x1u) << 2u;
        } else {
            pipe |= (((x >> 3u) ^ (y >> 3u) ^ (x >> 4u)) & 0x1u) << 0u;
            pipe |= (((x >> 4u) ^ (y >> 4u)) & 0x1u) << 1u;
            pipe |= (((x >> 5u) ^ (y >> 5u)) & 0x1u) << 2u;
            pipe |= (((x >> 6u) ^ (y >> 5u)) & 0x1u) << 3u;
        }

        return pipe;
    }

    static u32 getBankIdx(u32 x, u32 y, u32 bank_width, u32 bank_height, u32 num_banks,
                          u32 num_pipes) {
        const u32 x_shift_offset = std::bit_width(bank_width * num_pipes) - 1;
        const u32 y_shift_offset = std::bit_width(bank_height) - 1;
        const u32 xs = x >> x_shift_offset;
        const u32 ys = y >> y_shift_offset;
        u32 bank = 0;
        switch (num_banks) {
        case 8:
            bank |= (((xs >> 3u) ^ (ys >> 5u)) & 0x1u) << 0u;
            bank |= (((xs >> 4u) ^ (ys >> 4u) ^ (ys >> 5u)) & 0x1u) << 1u;
            bank |= (((xs >> 5u) ^ (ys >> 3u)) & 0x1u) << 2u;
            break;
        case 16:
            bank |= (((xs >> 3u) ^ (ys >> 6u)) & 0x1u) << 0u;
            bank |= (((xs >> 4u) ^ (ys >> 5u) ^ (ys >> 6u)) & 0x1u) << 1u;
            bank |= (((xs >> 5u) ^ (ys >> 4u)) & 0x1u) << 2u;
            bank |= (((xs >> 6u) ^ (ys >> 3u)) & 0x1u) << 3u;
            break;
        default:;
        }

        return bank;
    }

    u64 getTiledOffs(u32 x, u32 y, bool is_neo) const {
        u64 element_index = getElementIdx(x, y);

        u32 xh = x;
        u32 yh = y;
        u64 pipe = getPipeIdx(xh, yh, is_neo);
        u64 bank = getBankIdx(xh, yh, 1, m_bank_height, m_num_banks, m_num_pipes);
        u32 tile_bytes = (8 * 8 * 32 + 7) / 8;
        u64 element_offset = (element_index * 32);
        u64 tile_split_slice = 0;

        if (tile_bytes > 512) {
            tile_split_slice = element_offset / (static_cast<u64>(512) * 8);
            element_offset %= (static_cast<u64>(512) * 8);
            tile_bytes = 512;
        }

        u64 macro_tile_bytes =
            (128 / 8) * (m_macro_tile_height / 8) * tile_bytes / (m_num_pipes * m_num_banks);
        u64 macro_tiles_per_row = m_padded_width / 128;
        u64 macro_tile_row_index = y / m_macro_tile_height;
        u64 macro_tile_column_index = x / 128;
        u64 macro_tile_index =
            (macro_tile_row_index * macro_tiles_per_row) + macro_tile_column_index;
        u64 macro_tile_offset = macro_tile_index * macro_tile_bytes;
        u64 macro_tiles_per_slice = macro_tiles_per_row * (m_padded_height / m_macro_tile_height);
        u64 slice_bytes = macro_tiles_per_slice * macro_tile_bytes;
        u64 slice_offset = tile_split_slice * slice_bytes;
        u64 tile_row_index = (y / 8) % m_bank_height;
        u64 tile_index = tile_row_index;
        u64 tile_offset = tile_index * tile_bytes;

        u64 tile_split_slice_rotation = ((m_num_banks / 2) + 1) * tile_split_slice;
        bank ^= tile_split_slice_rotation;
        bank &= (m_num_banks - 1);

        u64 total_offset = (slice_offset + macro_tile_offset + tile_offset) * 8 + element_offset;
        u64 bit_offset = total_offset & 0x7u;
        total_offset /= 8;

        u64 pipe_interleave_offset = total_offset & 0xffu;
        u64 offset = total_offset >> 8u;
        u64 byte_offset = pipe_interleave_offset | (pipe << (8u)) | (bank << (8u + m_pipe_bits)) |
                          (offset << (8u + m_pipe_bits + m_bank_bits));

        return ((byte_offset << 3u) | bit_offset) / 8;
    }
};

void ConvertTileToLinear(u8* dst, const u8* src, u32 width, u32 height, bool is_neo) {
    TileManager32 t;
    t.Init(width, height, is_neo);

    for (u32 y = 0; y < height; y++) {
        u32 x = 0;
        u64 linear_offset = y * width * 4;

        for (; x + 1 < width; x += 2) {
            auto tiled_offset = t.getTiledOffs(x, y, is_neo);

            std::memcpy(dst + linear_offset, src + tiled_offset, sizeof(u64));
            linear_offset += 8;
        }
        if (x < width) {
            auto tiled_offset = t.getTiledOffs(x, y, is_neo);
            std::memcpy(dst + linear_offset, src + tiled_offset, sizeof(u32));
        }
    }
}

vk::Format DemoteImageFormatForDetiling(vk::Format format) {
    switch (format) {
    case vk::Format::eR8Unorm:
        return vk::Format::eR8Uint;
    case vk::Format::eR8G8Unorm:
    case vk::Format::eR16Sfloat:
    case vk::Format::eR16Unorm:
        return vk::Format::eR8G8Uint;
    case vk::Format::eR8G8B8A8Srgb:
    case vk::Format::eB8G8R8A8Srgb:
    case vk::Format::eB8G8R8A8Unorm:
    case vk::Format::eR8G8B8A8Unorm:
    case vk::Format::eR8G8B8A8Uint:
    case vk::Format::eR32Sfloat:
    case vk::Format::eR32Uint:
    case vk::Format::eR16G16Sfloat:
    case vk::Format::eR16G16Unorm:
    case vk::Format::eB10G11R11UfloatPack32:
        return vk::Format::eR32Uint;
    case vk::Format::eBc1RgbaSrgbBlock:
    case vk::Format::eBc1RgbaUnormBlock:
    case vk::Format::eBc4UnormBlock:
    case vk::Format::eR32G32Sfloat:
    case vk::Format::eR32G32Uint:
    case vk::Format::eR16G16B16A16Unorm:
    case vk::Format::eR16G16B16A16Uint:
    case vk::Format::eR16G16B16A16Sfloat:
        return vk::Format::eR32G32Uint;
    case vk::Format::eBc2SrgbBlock:
    case vk::Format::eBc2UnormBlock:
    case vk::Format::eBc3SrgbBlock:
    case vk::Format::eBc3UnormBlock:
    case vk::Format::eBc5UnormBlock:
    case vk::Format::eBc5SnormBlock:
    case vk::Format::eBc7SrgbBlock:
    case vk::Format::eBc7UnormBlock:
    case vk::Format::eBc6HUfloatBlock:
    case vk::Format::eR32G32B32A32Sfloat:
        return vk::Format::eR32G32B32A32Uint;
    default:
        break;
    }

    // Log missing formats only once to avoid spamming the log.
    static constexpr size_t MaxFormatIndex = 256;
    static std::array<bool, MaxFormatIndex> logged_formats{};
    if (const u32 index = u32(format); !logged_formats[index]) {
        LOG_ERROR(Render_Vulkan, "Unexpected format for demotion {}", vk::to_string(format));
        logged_formats[index] = true;
    }
    return format;
}

const DetilerContext* TileManager::GetDetiler(const Image& image) const {
    const auto format = DemoteImageFormatForDetiling(image.info.pixel_format);

    if (image.info.tiling_mode == AmdGpu::TilingMode::Texture_MicroTiled) {
        switch (format) {
        case vk::Format::eR8Uint:
            return &detilers[DetilerType::Micro8x1];
        case vk::Format::eR8G8Uint:
            return &detilers[DetilerType::Micro8x2];
        case vk::Format::eR32Uint:
            return &detilers[DetilerType::Micro32x1];
        case vk::Format::eR32G32Uint:
            return &detilers[DetilerType::Micro32x2];
        case vk::Format::eR32G32B32A32Uint:
            return &detilers[DetilerType::Micro32x4];
        default:
            return nullptr;
        }
    }
    return nullptr;
}

struct DetilerParams {
    u32 num_levels;
    u32 pitch0;
    u32 sizes[14];
};

TileManager::TileManager(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler)
    : instance{instance}, scheduler{scheduler} {
    static const std::array detiler_shaders{
        HostShaders::DETILE_M8X1_COMP,  HostShaders::DETILE_M8X2_COMP,
        HostShaders::DETILE_M32X1_COMP, HostShaders::DETILE_M32X2_COMP,
        HostShaders::DETILE_M32X4_COMP,
    };

    boost::container::static_vector<vk::DescriptorSetLayoutBinding, 2> bindings{
        {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
    };

    const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
    auto desc_layout_result = instance.GetDevice().createDescriptorSetLayoutUnique(desc_layout_ci);
    ASSERT_MSG(desc_layout_result.result == vk::Result::eSuccess,
               "Failed to create descriptor set layout: {}",
               vk::to_string(desc_layout_result.result));
    desc_layout = std::move(desc_layout_result.value);

    const vk::PushConstantRange push_constants = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(DetilerParams),
    };

    for (int pl_id = 0; pl_id < DetilerType::Max; ++pl_id) {
        auto& ctx = detilers[pl_id];

        const auto& module = Vulkan::Compile(
            detiler_shaders[pl_id], vk::ShaderStageFlagBits::eCompute, instance.GetDevice());

        // Set module debug name
        auto module_name = magic_enum::enum_name(static_cast<DetilerType>(pl_id));
        Vulkan::SetObjectName(instance.GetDevice(), module, module_name);

        const vk::PipelineShaderStageCreateInfo shader_ci = {
            .stage = vk::ShaderStageFlagBits::eCompute,
            .module = module,
            .pName = "main",
        };

        const vk::DescriptorSetLayout set_layout = *desc_layout;
        const vk::PipelineLayoutCreateInfo layout_info = {
            .setLayoutCount = 1U,
            .pSetLayouts = &set_layout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &push_constants,
        };
        auto [layout_result, layout] = instance.GetDevice().createPipelineLayoutUnique(layout_info);
        ASSERT_MSG(layout_result == vk::Result::eSuccess, "Failed to create pipeline layout: {}",
                   vk::to_string(layout_result));
        ctx.pl_layout = std::move(layout);

        const vk::ComputePipelineCreateInfo compute_pipeline_ci = {
            .stage = shader_ci,
            .layout = *ctx.pl_layout,
        };
        auto result = instance.GetDevice().createComputePipelineUnique(
            /*pipeline_cache*/ {}, compute_pipeline_ci);
        if (result.result == vk::Result::eSuccess) {
            ctx.pl = std::move(result.value);
        } else {
            UNREACHABLE_MSG("Detiler pipeline creation failed!");
        }

        // Once pipeline is compiled, we don't need the shader module anymore
        instance.GetDevice().destroyShaderModule(module);
    }
}

TileManager::~TileManager() = default;

TileManager::ScratchBuffer TileManager::AllocBuffer(u32 size, bool is_storage /*= false*/) {
    const auto usage = vk::BufferUsageFlagBits::eStorageBuffer |
                       (is_storage ? vk::BufferUsageFlagBits::eTransferSrc
                                   : vk::BufferUsageFlagBits::eTransferDst);
    const vk::BufferCreateInfo buffer_ci{
        .size = size,
        .usage = usage,
    };

    VmaAllocationCreateInfo alloc_info{
        .flags = !is_storage ? VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                             : static_cast<VmaAllocationCreateFlags>(0),
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = !is_storage ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                     : static_cast<VkMemoryPropertyFlags>(0),
    };

    VkBuffer buffer;
    VmaAllocation allocation;
    const auto buffer_ci_unsafe = static_cast<VkBufferCreateInfo>(buffer_ci);
    const auto result = vmaCreateBuffer(instance.GetAllocator(), &buffer_ci_unsafe, &alloc_info,
                                        &buffer, &allocation, nullptr);
    ASSERT(result == VK_SUCCESS);
    return {buffer, allocation};
}

void TileManager::Upload(ScratchBuffer buffer, const void* data, size_t size) {
    VmaAllocationInfo alloc_info{};
    vmaGetAllocationInfo(instance.GetAllocator(), buffer.second, &alloc_info);
    ASSERT(size <= alloc_info.size);
    void* ptr{};
    const auto result = vmaMapMemory(instance.GetAllocator(), buffer.second, &ptr);
    ASSERT(result == VK_SUCCESS);
    std::memcpy(ptr, data, size);
    vmaUnmapMemory(instance.GetAllocator(), buffer.second);
}

void TileManager::FreeBuffer(ScratchBuffer buffer) {
    vmaDestroyBuffer(instance.GetAllocator(), buffer.first, buffer.second);
}

std::pair<vk::Buffer, u32> TileManager::TryDetile(vk::Buffer in_buffer, u32 in_offset,
                                                  Image& image) {
    if (!image.info.props.is_tiled) {
        return {in_buffer, in_offset};
    }

    const auto* detiler = GetDetiler(image);
    if (!detiler) {
        if (image.info.tiling_mode != AmdGpu::TilingMode::Texture_MacroTiled &&
            image.info.tiling_mode != AmdGpu::TilingMode::Display_MacroTiled) {
            LOG_ERROR(Render_Vulkan, "Unsupported tiled image: {} ({})",
                      vk::to_string(image.info.pixel_format), NameOf(image.info.tiling_mode));
        }
        return {in_buffer, in_offset};
    }

    const u32 image_size = image.info.guest_size_bytes;

    // Prepare output buffer
    auto out_buffer = AllocBuffer(image_size, true);
    scheduler.DeferOperation([=, this]() { FreeBuffer(out_buffer); });

    auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *detiler->pl);

    const vk::DescriptorBufferInfo input_buffer_info{
        .buffer = in_buffer,
        .offset = in_offset,
        .range = image_size,
    };

    const vk::DescriptorBufferInfo output_buffer_info{
        .buffer = out_buffer.first,
        .offset = 0,
        .range = image_size,
    };

    std::vector<vk::WriteDescriptorSet> set_writes{
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &input_buffer_info,
        },
        {
            .dstSet = VK_NULL_HANDLE,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &output_buffer_info,
        },
    };
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *detiler->pl_layout, 0,
                                set_writes);

    DetilerParams params;
    params.pitch0 = image.info.pitch >> (image.info.props.is_block ? 2u : 0u);
    params.num_levels = image.info.resources.levels;

    ASSERT(image.info.resources.levels <= 14);
    std::memset(&params.sizes, 0, sizeof(params.sizes));
    for (int m = 0; m < image.info.resources.levels; ++m) {
        params.sizes[m] = image.info.mips_layout[m].size * image.info.resources.layers +
                          (m > 0 ? params.sizes[m - 1] : 0);
    }

    cmdbuf.pushConstants(*detiler->pl_layout, vk::ShaderStageFlagBits::eCompute, 0u, sizeof(params),
                         &params);

    ASSERT((image_size % 64) == 0);
    const auto bpp = image.info.num_bits * (image.info.props.is_block ? 16u : 1u);
    const auto num_tiles = image_size / (64 * (bpp / 8));
    cmdbuf.dispatch(num_tiles, 1, 1);

    const vk::BufferMemoryBarrier post_barrier{
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eTransferRead,
        .buffer = out_buffer.first,
        .size = image_size,
    };
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                           vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion,
                           {}, post_barrier, {});

    return {out_buffer.first, 0};
}

} // namespace VideoCore
