// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/texture_cache/image_view.h"
#include "video_core/texture_cache/texture_cache.h"
#include "video_core/texture_cache/tile_manager.h"

#include "video_core/host_shaders/detile_m32x1_comp.h"
#include "video_core/host_shaders/detile_m32x2_comp.h"
#include "video_core/host_shaders/detile_m32x4_comp.h"
#include "video_core/host_shaders/detile_m8x1_comp.h"
#include "video_core/host_shaders/detile_m8x2_comp.h"

#include <boost/container/static_vector.hpp>
#include <magic_enum.hpp>

namespace VideoCore {

static u32 IntLog2(u32 i) {
    return 31 - __builtin_clz(i | 1u);
}

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
        const u32 x_shift_offset = IntLog2(bank_width * num_pipes);
        const u32 y_shift_offset = IntLog2(bank_height);
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
        return vk::Format::eR8G8Uint;
    case vk::Format::eR8G8B8A8Srgb:
    case vk::Format::eB8G8R8A8Srgb:
    case vk::Format::eB8G8R8A8Unorm:
    case vk::Format::eR8G8B8A8Unorm:
    case vk::Format::eR32Sfloat:
    case vk::Format::eR32Uint:
        return vk::Format::eR32Uint;
    case vk::Format::eBc1RgbaUnormBlock:
    case vk::Format::eBc4UnormBlock:
    case vk::Format::eR32G32Sfloat:
        return vk::Format::eR32G32Uint;
    case vk::Format::eBc2SrgbBlock:
    case vk::Format::eBc2UnormBlock:
    case vk::Format::eBc3SrgbBlock:
    case vk::Format::eBc3UnormBlock:
    case vk::Format::eBc5UnormBlock:
    case vk::Format::eBc7SrgbBlock:
    case vk::Format::eBc7UnormBlock:
        return vk::Format::eR32G32B32A32Uint;
    default:
        break;
    }
    LOG_ERROR(Render_Vulkan, "Unexpected format for demotion {}", vk::to_string(format));
    return format;
}

const DetilerContext* TileManager::GetDetiler(const Image& image) const {
    const auto format = DemoteImageFormatForDetiling(image.info.pixel_format);

    if (image.info.tiling_mode == AmdGpu::TilingMode::Texture_MicroTiled ||
        image.info.tiling_mode == AmdGpu::TilingMode::Depth_MicroTiled) {
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

static constexpr vk::BufferUsageFlags StagingFlags = vk::BufferUsageFlagBits::eTransferDst |
                                                     vk::BufferUsageFlagBits::eUniformBuffer |
                                                     vk::BufferUsageFlagBits::eStorageBuffer;

TileManager::TileManager(const Vulkan::Instance& instance, Vulkan::Scheduler& scheduler)
    : instance{instance}, scheduler{scheduler},
      staging{instance, scheduler, StagingFlags, 128_MB, Vulkan::BufferType::Upload} {

    static const std::array detiler_shaders{
        HostShaders::DETILE_M8X1_COMP,  HostShaders::DETILE_M8X2_COMP,
        HostShaders::DETILE_M32X1_COMP, HostShaders::DETILE_M32X2_COMP,
        HostShaders::DETILE_M32X4_COMP,
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

        boost::container::static_vector<vk::DescriptorSetLayoutBinding, 2> bindings{
            {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eCompute,
            },
            {
                .binding = 1,
                .descriptorType = vk::DescriptorType::eStorageImage,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eCompute,
            },
        };

        const vk::DescriptorSetLayoutCreateInfo desc_layout_ci = {
            .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
            .bindingCount = static_cast<u32>(bindings.size()),
            .pBindings = bindings.data(),
        };
        static auto desc_layout =
            instance.GetDevice().createDescriptorSetLayoutUnique(desc_layout_ci);

        const vk::PushConstantRange push_constants = {
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
            .offset = 0,
            .size = sizeof(u32),
        };

        const vk::DescriptorSetLayout set_layout = *desc_layout;
        const vk::PipelineLayoutCreateInfo layout_info = {
            .setLayoutCount = 1U,
            .pSetLayouts = &set_layout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &push_constants,
        };
        ctx.pl_layout = instance.GetDevice().createPipelineLayoutUnique(layout_info);

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

bool TileManager::TryDetile(Image& image) {
    if (!image.info.is_tiled) {
        return false;
    }

    const auto* detiler = GetDetiler(image);
    if (!detiler) {
        LOG_ERROR(Render_Vulkan, "Unsupported tiled image: {} ({})",
                  vk::to_string(image.info.pixel_format), NameOf(image.info.tiling_mode));
        return false;
    }

    const auto offset =
        staging.Copy(image.cpu_addr, image.info.guest_size_bytes, instance.StorageMinAlignment());
    image.Transit(vk::ImageLayout::eGeneral, vk::AccessFlagBits::eShaderWrite);

    auto cmdbuf = scheduler.CommandBuffer();
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *detiler->pl);

    const vk::DescriptorBufferInfo input_buffer_info{
        .buffer = staging.Handle(),
        .offset = offset,
        .range = image.info.guest_size_bytes,
    };

    ASSERT(image.view_for_detiler.has_value());
    const vk::DescriptorImageInfo output_image_info{
        .imageView = *image.view_for_detiler->image_view,
        .imageLayout = image.layout,
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
            .descriptorType = vk::DescriptorType::eStorageImage,
            .pImageInfo = &output_image_info,
        },
    };
    cmdbuf.pushDescriptorSetKHR(vk::PipelineBindPoint::eCompute, *detiler->pl_layout, 0,
                                set_writes);

    cmdbuf.pushConstants(*detiler->pl_layout, vk::ShaderStageFlagBits::eCompute, 0u,
                         sizeof(image.info.pitch), &image.info.pitch);

    cmdbuf.dispatch((image.info.size.width * image.info.size.height) / 64, 1,
                    1); // round to 64

    return true;
}

} // namespace VideoCore
