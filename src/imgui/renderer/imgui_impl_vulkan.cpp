// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on imgui_impl_vulkan.cpp from Dear ImGui repository

#include <cstdio>
#include <imgui.h>

#include "imgui_impl_vulkan.h"

#ifndef IM_MAX
#define IM_MAX(A, B) (((A) >= (B)) ? (A) : (B))
#endif

#define IDX_SIZE sizeof(ImDrawIdx)

namespace ImGui::Vulkan {

struct RenderBuffer {
    vk::DeviceMemory buffer_memory{};
    vk::DeviceSize buffer_size{};
    vk::Buffer buffer{};
};

// Reusable buffers used for rendering 1 current in-flight frame, for RenderDrawData()
struct FrameRenderBuffers {
    RenderBuffer vertex;
    RenderBuffer index;
};

// Each viewport will hold 1 WindowRenderBuffers
struct WindowRenderBuffers {
    uint32_t index{};
    uint32_t count{};
    std::vector<FrameRenderBuffers> frame_render_buffers{};
};

// Vulkan data
struct VkData {
    const InitInfo init_info;
    vk::DeviceSize buffer_memory_alignment = 256;
    vk::PipelineCreateFlags pipeline_create_flags{};
    vk::DescriptorPool descriptor_pool{};
    vk::DescriptorSetLayout descriptor_set_layout{};
    vk::PipelineLayout pipeline_layout{};
    vk::Pipeline pipeline{};
    vk::ShaderModule shader_module_vert{};
    vk::ShaderModule shader_module_frag{};

    // Font data
    vk::Sampler font_sampler{};
    vk::DeviceMemory font_memory{};
    vk::Image font_image{};
    vk::ImageView font_view{};
    vk::DescriptorSet font_descriptor_set{};
    vk::CommandPool font_command_pool{};
    vk::CommandBuffer font_command_buffer{};

    // Render buffers
    WindowRenderBuffers render_buffers{};

    VkData(const InitInfo init_info) : init_info(init_info) {
        render_buffers.count = init_info.image_count;
        render_buffers.frame_render_buffers.resize(render_buffers.count);
    }
};

//-----------------------------------------------------------------------------
// SHADERS
//-----------------------------------------------------------------------------

// backends/vulkan/glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static uint32_t glsl_shader_vert_spv[] = {
    0x07230203, 0x00010000, 0x00080001, 0x0000002e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x000a000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x0000000f, 0x00000015,
    0x0000001b, 0x0000001c, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
    0x00000000, 0x00030005, 0x00000009, 0x00000000, 0x00050006, 0x00000009, 0x00000000, 0x6f6c6f43,
    0x00000072, 0x00040006, 0x00000009, 0x00000001, 0x00005655, 0x00030005, 0x0000000b, 0x0074754f,
    0x00040005, 0x0000000f, 0x6c6f4361, 0x0000726f, 0x00030005, 0x00000015, 0x00565561, 0x00060005,
    0x00000019, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x00000019, 0x00000000,
    0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x0000001b, 0x00000000, 0x00040005, 0x0000001c,
    0x736f5061, 0x00000000, 0x00060005, 0x0000001e, 0x73755075, 0x6e6f4368, 0x6e617473, 0x00000074,
    0x00050006, 0x0000001e, 0x00000000, 0x61635375, 0x0000656c, 0x00060006, 0x0000001e, 0x00000001,
    0x61725475, 0x616c736e, 0x00006574, 0x00030005, 0x00000020, 0x00006370, 0x00040047, 0x0000000b,
    0x0000001e, 0x00000000, 0x00040047, 0x0000000f, 0x0000001e, 0x00000002, 0x00040047, 0x00000015,
    0x0000001e, 0x00000001, 0x00050048, 0x00000019, 0x00000000, 0x0000000b, 0x00000000, 0x00030047,
    0x00000019, 0x00000002, 0x00040047, 0x0000001c, 0x0000001e, 0x00000000, 0x00050048, 0x0000001e,
    0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x0000001e, 0x00000001, 0x00000023, 0x00000008,
    0x00030047, 0x0000001e, 0x00000002, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
    0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040017,
    0x00000008, 0x00000006, 0x00000002, 0x0004001e, 0x00000009, 0x00000007, 0x00000008, 0x00040020,
    0x0000000a, 0x00000003, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000003, 0x00040015,
    0x0000000c, 0x00000020, 0x00000001, 0x0004002b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040020,
    0x0000000e, 0x00000001, 0x00000007, 0x0004003b, 0x0000000e, 0x0000000f, 0x00000001, 0x00040020,
    0x00000011, 0x00000003, 0x00000007, 0x0004002b, 0x0000000c, 0x00000013, 0x00000001, 0x00040020,
    0x00000014, 0x00000001, 0x00000008, 0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00040020,
    0x00000017, 0x00000003, 0x00000008, 0x0003001e, 0x00000019, 0x00000007, 0x00040020, 0x0000001a,
    0x00000003, 0x00000019, 0x0004003b, 0x0000001a, 0x0000001b, 0x00000003, 0x0004003b, 0x00000014,
    0x0000001c, 0x00000001, 0x0004001e, 0x0000001e, 0x00000008, 0x00000008, 0x00040020, 0x0000001f,
    0x00000009, 0x0000001e, 0x0004003b, 0x0000001f, 0x00000020, 0x00000009, 0x00040020, 0x00000021,
    0x00000009, 0x00000008, 0x0004002b, 0x00000006, 0x00000028, 0x00000000, 0x0004002b, 0x00000006,
    0x00000029, 0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
    0x00000005, 0x0004003d, 0x00000007, 0x00000010, 0x0000000f, 0x00050041, 0x00000011, 0x00000012,
    0x0000000b, 0x0000000d, 0x0003003e, 0x00000012, 0x00000010, 0x0004003d, 0x00000008, 0x00000016,
    0x00000015, 0x00050041, 0x00000017, 0x00000018, 0x0000000b, 0x00000013, 0x0003003e, 0x00000018,
    0x00000016, 0x0004003d, 0x00000008, 0x0000001d, 0x0000001c, 0x00050041, 0x00000021, 0x00000022,
    0x00000020, 0x0000000d, 0x0004003d, 0x00000008, 0x00000023, 0x00000022, 0x00050085, 0x00000008,
    0x00000024, 0x0000001d, 0x00000023, 0x00050041, 0x00000021, 0x00000025, 0x00000020, 0x00000013,
    0x0004003d, 0x00000008, 0x00000026, 0x00000025, 0x00050081, 0x00000008, 0x00000027, 0x00000024,
    0x00000026, 0x00050051, 0x00000006, 0x0000002a, 0x00000027, 0x00000000, 0x00050051, 0x00000006,
    0x0000002b, 0x00000027, 0x00000001, 0x00070050, 0x00000007, 0x0000002c, 0x0000002a, 0x0000002b,
    0x00000028, 0x00000029, 0x00050041, 0x00000011, 0x0000002d, 0x0000001b, 0x0000000d, 0x0003003e,
    0x0000002d, 0x0000002c, 0x000100fd, 0x00010038};

// backends/vulkan/glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t glsl_shader_frag_spv[] = {
    0x07230203, 0x00010000, 0x00080001, 0x0000001e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00030010,
    0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
    0x00000000, 0x00040005, 0x00000009, 0x6c6f4366, 0x0000726f, 0x00030005, 0x0000000b, 0x00000000,
    0x00050006, 0x0000000b, 0x00000000, 0x6f6c6f43, 0x00000072, 0x00040006, 0x0000000b, 0x00000001,
    0x00005655, 0x00030005, 0x0000000d, 0x00006e49, 0x00050005, 0x00000016, 0x78655473, 0x65727574,
    0x00000000, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000d, 0x0000001e,
    0x00000000, 0x00040047, 0x00000016, 0x00000022, 0x00000000, 0x00040047, 0x00000016, 0x00000021,
    0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
    0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003,
    0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017, 0x0000000a, 0x00000006,
    0x00000002, 0x0004001e, 0x0000000b, 0x00000007, 0x0000000a, 0x00040020, 0x0000000c, 0x00000001,
    0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000001, 0x00040015, 0x0000000e, 0x00000020,
    0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040020, 0x00000010, 0x00000001,
    0x00000007, 0x00090019, 0x00000013, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0x00000001, 0x00000000, 0x0003001b, 0x00000014, 0x00000013, 0x00040020, 0x00000015, 0x00000000,
    0x00000014, 0x0004003b, 0x00000015, 0x00000016, 0x00000000, 0x0004002b, 0x0000000e, 0x00000018,
    0x00000001, 0x00040020, 0x00000019, 0x00000001, 0x0000000a, 0x00050036, 0x00000002, 0x00000004,
    0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000010, 0x00000011, 0x0000000d,
    0x0000000f, 0x0004003d, 0x00000007, 0x00000012, 0x00000011, 0x0004003d, 0x00000014, 0x00000017,
    0x00000016, 0x00050041, 0x00000019, 0x0000001a, 0x0000000d, 0x00000018, 0x0004003d, 0x0000000a,
    0x0000001b, 0x0000001a, 0x00050057, 0x00000007, 0x0000001c, 0x00000017, 0x0000001b, 0x00050085,
    0x00000007, 0x0000001d, 0x00000012, 0x0000001c, 0x0003003e, 0x00000009, 0x0000001d, 0x000100fd,
    0x00010038};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui
// contexts It is STRONGLY preferred that you use docking branch with multi-viewports (== single
// Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static VkData* GetBackendData() {
    return ImGui::GetCurrentContext() ? (VkData*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

static uint32_t FindMemoryType(vk::MemoryPropertyFlags properties, uint32_t type_bits) {
    VkData* bd = GetBackendData();
    const InitInfo& v = bd->init_info;
    const auto prop = v.physical_device.getMemoryProperties();
    for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
        if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
            return i;
    return 0xFFFFFFFF; // Unable to find memoryType
}

template <typename T>
static T CheckVkResult(vk::ResultValue<T> res) {
    if (res.result == vk::Result::eSuccess) {
        return res.value;
    }
    const VkData* bd = GetBackendData();
    if (!bd) {
        return res.value;
    }
    const InitInfo& v = bd->init_info;
    if (v.check_vk_result_fn) {
        v.check_vk_result_fn(res.result);
    }
    return res.value;
}

static void CheckVkErr(vk::Result res) {
    if (res == vk::Result::eSuccess) {
        return;
    }
    const VkData* bd = GetBackendData();
    if (!bd) {
        return;
    }
    const InitInfo& v = bd->init_info;
    if (v.check_vk_result_fn) {
        v.check_vk_result_fn(res);
    }
}

// Same as IM_MEMALIGN(). 'alignment' must be a power of two.
static inline vk::DeviceSize AlignBufferSize(vk::DeviceSize size, vk::DeviceSize alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Register a texture
vk::DescriptorSet AddTexture(vk::Sampler sampler, vk::ImageView image_view,
                             vk::ImageLayout image_layout) {
    VkData* bd = GetBackendData();
    const InitInfo& v = bd->init_info;

    // Create Descriptor Set:
    vk::DescriptorSet descriptor_set;
    {
        vk::DescriptorSetAllocateInfo alloc_info{
            .sType = vk::StructureType::eDescriptorSetAllocateInfo,
            .descriptorPool = bd->descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &bd->descriptor_set_layout,
        };
        descriptor_set = CheckVkResult(v.device.allocateDescriptorSets(alloc_info)).front();
    }

    // Update the Descriptor Set:
    {
        vk::DescriptorImageInfo desc_image[1]{
            {
                .sampler = sampler,
                .imageView = image_view,
                .imageLayout = image_layout,
            },
        };
        vk::WriteDescriptorSet write_desc[1]{
            {
                .sType = vk::StructureType::eWriteDescriptorSet,
                .dstSet = descriptor_set,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = desc_image,
            },
        };
        v.device.updateDescriptorSets({write_desc}, {});
    }
    return descriptor_set;
}

void RemoveTexture(vk::DescriptorSet descriptor_set) {
    VkData* bd = GetBackendData();
    const InitInfo& v = bd->init_info;
    v.device.freeDescriptorSets(bd->descriptor_pool, {descriptor_set});
}

static void CreateOrResizeBuffer(RenderBuffer& rb, size_t new_size, vk::BufferUsageFlagBits usage) {
    VkData* bd = GetBackendData();
    IM_ASSERT(bd != nullptr);
    const InitInfo& v = bd->init_info;
    if (rb.buffer != VK_NULL_HANDLE) {
        v.device.destroyBuffer(rb.buffer, v.allocator);
    }
    if (rb.buffer_memory != VK_NULL_HANDLE) {
        v.device.freeMemory(rb.buffer_memory, v.allocator);
    }

    const vk::DeviceSize buffer_size_aligned =
        AlignBufferSize(IM_MAX(v.min_allocation_size, new_size), bd->buffer_memory_alignment);
    vk::BufferCreateInfo buffer_info{
        .sType = vk::StructureType::eBufferCreateInfo,
        .size = buffer_size_aligned,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    rb.buffer = CheckVkResult(v.device.createBuffer(buffer_info, v.allocator));

    const vk::MemoryRequirements req = v.device.getBufferMemoryRequirements(rb.buffer);
    bd->buffer_memory_alignment = IM_MAX(bd->buffer_memory_alignment, req.alignment);
    vk::MemoryAllocateInfo alloc_info{
        .sType = vk::StructureType::eMemoryAllocateInfo,
        .allocationSize = req.size,
        .memoryTypeIndex =
            FindMemoryType(vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits),
    };
    rb.buffer_memory = CheckVkResult(v.device.allocateMemory(alloc_info, v.allocator));

    CheckVkErr(v.device.bindBufferMemory(rb.buffer, rb.buffer_memory, 0));
    rb.buffer_size = buffer_size_aligned;
}

static void SetupRenderState(ImDrawData& draw_data, vk::Pipeline pipeline, vk::CommandBuffer cmdbuf,
                             FrameRenderBuffers& frb, int fb_width, int fb_height) {
    VkData* bd = GetBackendData();

    // Bind pipeline:
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    // Bind Vertex And Index Buffer:
    if (draw_data.TotalVtxCount > 0) {
        vk::Buffer vertex_buffers[1] = {frb.vertex.buffer};
        vk::DeviceSize vertex_offset[1] = {0};
        cmdbuf.bindVertexBuffers(0, {vertex_buffers}, vertex_offset);
        cmdbuf.bindIndexBuffer(frb.index.buffer, 0,
                               IDX_SIZE == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
    }

    // Setup viewport:
    {
        vk::Viewport viewport{
            .x = 0,
            .y = 0,
            .width = (float)fb_width,
            .height = (float)fb_height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        cmdbuf.setViewport(0, {viewport});
    }

    // Setup scale and translation:
    // Our visible imgui space lies from draw_data->DisplayPps (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single
    // viewport apps.
    {
        float scale[2];
        scale[0] = 2.0f / draw_data.DisplaySize.x;
        scale[1] = 2.0f / draw_data.DisplaySize.y;
        float translate[2];
        translate[0] = -1.0f - draw_data.DisplayPos.x * scale[0];
        translate[1] = -1.0f - draw_data.DisplayPos.y * scale[1];
        cmdbuf.pushConstants(bd->pipeline_layout, vk::ShaderStageFlagBits::eVertex,
                             sizeof(float) * 0, sizeof(float) * 2, scale);
        cmdbuf.pushConstants(bd->pipeline_layout, vk::ShaderStageFlagBits::eVertex,
                             sizeof(float) * 2, sizeof(float) * 2, translate);
    }
}

// Render function
void RenderDrawData(ImDrawData& draw_data, vk::CommandBuffer command_buffer,
                    vk::Pipeline pipeline) {
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates !=
    // framebuffer coordinates)
    int fb_width = (int)(draw_data.DisplaySize.x * draw_data.FramebufferScale.x);
    int fb_height = (int)(draw_data.DisplaySize.y * draw_data.FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }

    VkData* bd = GetBackendData();
    const InitInfo& v = bd->init_info;
    if (pipeline == VK_NULL_HANDLE) {
        pipeline = bd->pipeline;
    }

    // Allocate array to store enough vertex/index buffers
    WindowRenderBuffers& wrb = bd->render_buffers;
    wrb.index = (wrb.index + 1) % wrb.count;
    FrameRenderBuffers& frb = wrb.frame_render_buffers[wrb.index];

    if (draw_data.TotalVtxCount > 0) {
        // Create or resize the vertex/index buffers
        size_t vertex_size = AlignBufferSize(draw_data.TotalVtxCount * sizeof(ImDrawVert),
                                             bd->buffer_memory_alignment);
        size_t index_size =
            AlignBufferSize(draw_data.TotalIdxCount * IDX_SIZE, bd->buffer_memory_alignment);
        if (frb.vertex.buffer == VK_NULL_HANDLE || frb.vertex.buffer_size < vertex_size) {
            CreateOrResizeBuffer(frb.vertex, vertex_size, vk::BufferUsageFlagBits::eVertexBuffer);
        }
        if (frb.index.buffer == VK_NULL_HANDLE || frb.index.buffer_size < index_size) {
            CreateOrResizeBuffer(frb.index, index_size, vk::BufferUsageFlagBits::eIndexBuffer);
        }

        // Upload vertex/index data into a single contiguous GPU buffer
        ImDrawVert* vtx_dst = nullptr;
        ImDrawIdx* idx_dst = nullptr;
        vtx_dst = (ImDrawVert*)CheckVkResult(
            v.device.mapMemory(frb.vertex.buffer_memory, 0, vertex_size, vk::MemoryMapFlags{}));
        idx_dst = (ImDrawIdx*)CheckVkResult(
            v.device.mapMemory(frb.index.buffer_memory, 0, index_size, vk::MemoryMapFlags{}));
        for (int n = 0; n < draw_data.CmdListsCount; n++) {
            const ImDrawList* cmd_list = draw_data.CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data,
                   cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * IDX_SIZE);
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        vk::MappedMemoryRange range[2]{
            {
                .sType = vk::StructureType::eMappedMemoryRange,
                .memory = frb.vertex.buffer_memory,
                .size = VK_WHOLE_SIZE,
            },
            {
                .sType = vk::StructureType::eMappedMemoryRange,
                .memory = frb.index.buffer_memory,
                .size = VK_WHOLE_SIZE,
            },
        };
        CheckVkErr(v.device.flushMappedMemoryRanges({range}));
        v.device.unmapMemory(frb.vertex.buffer_memory);
        v.device.unmapMemory(frb.index.buffer_memory);
    }

    // Setup desired Vulkan state
    SetupRenderState(draw_data, pipeline, command_buffer, frb, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space

    // (0,0) unless using multi-viewports
    ImVec2 clip_off = draw_data.DisplayPos;
    // (1,1) unless using retina display which are often (2,2)
    ImVec2 clip_scale = draw_data.FramebufferScale;

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data.CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data.CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to
                // request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
                    SetupRenderState(draw_data, pipeline, command_buffer, frb, fb_width, fb_height);
                } else {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

                // Clamp to viewport as vk::CmdSetScissor() won't accept values that are off bounds
                if (clip_min.x < 0.0f) {
                    clip_min.x = 0.0f;
                }
                if (clip_min.y < 0.0f) {
                    clip_min.y = 0.0f;
                }
                if (clip_max.x > fb_width) {
                    clip_max.x = (float)fb_width;
                }
                if (clip_max.y > fb_height) {
                    clip_max.y = (float)fb_height;
                }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle
                vk::Rect2D scissor{
                    .offset{
                        .x = (int32_t)(clip_min.x),
                        .y = (int32_t)(clip_min.y),
                    },
                    .extent{
                        .width = (uint32_t)(clip_max.x - clip_min.x),
                        .height = (uint32_t)(clip_max.y - clip_min.y),
                    },
                };
                command_buffer.setScissor(0, 1, &scissor);

                // Bind DescriptorSet with font or user texture
                vk::DescriptorSet desc_set[1]{(VkDescriptorSet)pcmd->TextureId};
                if (sizeof(ImTextureID) < sizeof(ImU64)) {
                    // We don't support texture switches if ImTextureID hasn't been redefined to be
                    // 64-bit. Do a flaky check that other textures haven't been used.
                    IM_ASSERT(pcmd->TextureId == (ImTextureID)bd->font_descriptor_set);
                    desc_set[0] = bd->font_descriptor_set;
                }
                command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                  bd->pipeline_layout, 0, {desc_set}, {});

                // Draw
                command_buffer.drawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset,
                                           pcmd->VtxOffset + global_vtx_offset, 0);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
    //    vk::Rect2D scissor = {{0, 0}, {(uint32_t)fb_width, (uint32_t)fb_height}};
    //    command_buffer.setScissor(0, 1, &scissor);
}

static void DestroyFontsTexture();

static bool CreateFontsTexture() {
    ImGuiIO& io = ImGui::GetIO();
    VkData* bd = GetBackendData();
    const InitInfo& v = bd->init_info;

    // Destroy existing texture (if any)
    if (bd->font_view || bd->font_image || bd->font_memory || bd->font_descriptor_set) {
        CheckVkErr(v.queue.waitIdle());
        DestroyFontsTexture();
    }

    // Create command pool/buffer
    if (bd->font_command_pool == VK_NULL_HANDLE) {
        vk::CommandPoolCreateInfo info{
            .sType = vk::StructureType::eCommandPoolCreateInfo,
            .flags = vk::CommandPoolCreateFlags{},
            .queueFamilyIndex = v.queue_family,
        };
        bd->font_command_pool = CheckVkResult(v.device.createCommandPool(info, v.allocator));
    }
    if (bd->font_command_buffer == VK_NULL_HANDLE) {
        vk::CommandBufferAllocateInfo info{
            .sType = vk::StructureType::eCommandBufferAllocateInfo,
            .commandPool = bd->font_command_pool,
            .commandBufferCount = 1,
        };
        bd->font_command_buffer = CheckVkResult(v.device.allocateCommandBuffers(info)).front();
    }

    // Start command buffer
    {
        CheckVkErr(v.device.resetCommandPool(bd->font_command_pool, vk::CommandPoolResetFlags{}));
        vk::CommandBufferBeginInfo begin_info{};
        begin_info.sType = vk::StructureType::eCommandBufferBeginInfo;
        begin_info.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        CheckVkErr(bd->font_command_buffer.begin(&begin_info));
    }

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    size_t upload_size = width * height * 4 * sizeof(char);

    // Create the Image:
    {
        vk::ImageCreateInfo info{
            .sType = vk::StructureType::eImageCreateInfo,
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .extent{
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height),
                .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined,
        };
        bd->font_image = CheckVkResult(v.device.createImage(info, v.allocator));
        vk::MemoryRequirements req = v.device.getImageMemoryRequirements(bd->font_image);
        vk::MemoryAllocateInfo alloc_info{
            .sType = vk::StructureType::eMemoryAllocateInfo,
            .allocationSize = IM_MAX(v.min_allocation_size, req.size),
            .memoryTypeIndex =
                FindMemoryType(vk::MemoryPropertyFlagBits::eDeviceLocal, req.memoryTypeBits),
        };
        bd->font_memory = CheckVkResult(v.device.allocateMemory(alloc_info, v.allocator));
        CheckVkErr(v.device.bindImageMemory(bd->font_image, bd->font_memory, 0));
    }

    // Create the Image View:
    {
        vk::ImageViewCreateInfo info{
            .sType = vk::StructureType::eImageViewCreateInfo,
            .image = bd->font_image,
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .subresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        bd->font_view = CheckVkResult(v.device.createImageView(info, v.allocator));
    }

    // Create the Descriptor Set:
    bd->font_descriptor_set =
        AddTexture(bd->font_sampler, bd->font_view, vk::ImageLayout::eShaderReadOnlyOptimal);

    // Create the Upload Buffer:
    vk::DeviceMemory upload_buffer_memory{};
    vk::Buffer upload_buffer{};
    {
        vk::BufferCreateInfo buffer_info{
            .sType = vk::StructureType::eBufferCreateInfo,
            .size = upload_size,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive,
        };
        upload_buffer = CheckVkResult(v.device.createBuffer(buffer_info, v.allocator));
        vk::MemoryRequirements req = v.device.getBufferMemoryRequirements(upload_buffer);
        bd->buffer_memory_alignment = IM_MAX(bd->buffer_memory_alignment, req.alignment);
        vk::MemoryAllocateInfo alloc_info{
            .sType = vk::StructureType::eMemoryAllocateInfo,
            .allocationSize = IM_MAX(v.min_allocation_size, req.size),
            .memoryTypeIndex =
                FindMemoryType(vk::MemoryPropertyFlagBits::eHostVisible, req.memoryTypeBits),
        };
        upload_buffer_memory = CheckVkResult(v.device.allocateMemory(alloc_info, v.allocator));
        CheckVkErr(v.device.bindBufferMemory(upload_buffer, upload_buffer_memory, 0));
    }

    // Upload to Buffer:
    {
        char* map = (char*)CheckVkResult(v.device.mapMemory(upload_buffer_memory, 0, upload_size));
        memcpy(map, pixels, upload_size);
        vk::MappedMemoryRange range[1]{
            {
                .sType = vk::StructureType::eMappedMemoryRange,
                .memory = upload_buffer_memory,
                .size = upload_size,
            },
        };
        CheckVkErr(v.device.flushMappedMemoryRanges({range}));
        v.device.unmapMemory(upload_buffer_memory);
    }

    // Copy to Image:
    {
        vk::ImageMemoryBarrier copy_barrier[1]{
            {
                .sType = vk::StructureType::eImageMemoryBarrier,
                .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = bd->font_image,
                .subresourceRange{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .levelCount = 1,
                    .layerCount = 1,
                },
            },
        };
        bd->font_command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost,
                                                vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                                                {copy_barrier});

        vk::BufferImageCopy region{
            .imageSubresource{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .layerCount = 1,
            },
            .imageExtent{
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height),
                .depth = 1,
            },
        };
        bd->font_command_buffer.copyBufferToImage(upload_buffer, bd->font_image,
                                                  vk::ImageLayout::eTransferDstOptimal, {region});

        vk::ImageMemoryBarrier use_barrier[1]{{
            .sType = vk::StructureType::eImageMemoryBarrier,
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = bd->font_image,
            .subresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .levelCount = 1,
                .layerCount = 1,
            },
        }};
        bd->font_command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                                vk::PipelineStageFlagBits::eFragmentShader, {}, {},
                                                {}, {use_barrier});
    }

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)bd->font_descriptor_set);

    // End command buffer
    vk::SubmitInfo end_info = {};
    end_info.sType = vk::StructureType::eSubmitInfo;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &bd->font_command_buffer;
    CheckVkErr(bd->font_command_buffer.end());
    CheckVkErr(v.queue.submit({end_info}));

    CheckVkErr(v.queue.waitIdle());

    v.device.destroyBuffer(upload_buffer, v.allocator);
    v.device.freeMemory(upload_buffer_memory, v.allocator);

    return true;
}

// You probably never need to call this, as it is called by CreateFontsTexture()
// and Shutdown().
static void DestroyFontsTexture() {
    ImGuiIO& io = ImGui::GetIO();
    VkData* bd = GetBackendData();
    const InitInfo& v = bd->init_info;

    if (bd->font_descriptor_set) {
        RemoveTexture(bd->font_descriptor_set);
        bd->font_descriptor_set = VK_NULL_HANDLE;
        io.Fonts->SetTexID(nullptr);
    }

    if (bd->font_view) {
        v.device.destroyImageView(bd->font_view, v.allocator);
        bd->font_view = VK_NULL_HANDLE;
    }
    if (bd->font_image) {
        v.device.destroyImage(bd->font_image, v.allocator);
        bd->font_image = VK_NULL_HANDLE;
    }
    if (bd->font_memory) {
        v.device.freeMemory(bd->font_memory, v.allocator);
        bd->font_memory = VK_NULL_HANDLE;
    }
}

static void DestroyFrameRenderBuffers(vk::Device device, RenderBuffer& rb,
                                      const vk::AllocationCallbacks* allocator) {
    if (rb.buffer) {
        device.destroyBuffer(rb.buffer, allocator);
        rb.buffer = VK_NULL_HANDLE;
    }
    if (rb.buffer_memory) {
        device.freeMemory(rb.buffer_memory, allocator);
        rb.buffer_memory = VK_NULL_HANDLE;
    }
    rb.buffer_size = 0;
}

static void DestroyWindowRenderBuffers(vk::Device device, WindowRenderBuffers& buffers,
                                       const vk::AllocationCallbacks* allocator) {
    for (uint32_t n = 0; n < buffers.count; n++) {
        auto& frb = buffers.frame_render_buffers[n];
        DestroyFrameRenderBuffers(device, frb.index, allocator);
        DestroyFrameRenderBuffers(device, frb.vertex, allocator);
    }
    buffers = {};
}

static void CreateShaderModules(vk::Device device, const vk::AllocationCallbacks* allocator) {
    // Create the shader modules
    VkData* bd = GetBackendData();
    if (bd->shader_module_vert == VK_NULL_HANDLE) {
        vk::ShaderModuleCreateInfo vert_info{
            .sType = vk::StructureType::eShaderModuleCreateInfo,
            .codeSize = sizeof(glsl_shader_vert_spv),
            .pCode = (uint32_t*)glsl_shader_vert_spv,
        };
        bd->shader_module_vert = CheckVkResult(device.createShaderModule(vert_info, allocator));
    }
    if (bd->shader_module_frag == VK_NULL_HANDLE) {
        vk::ShaderModuleCreateInfo frag_info{
            .sType = vk::StructureType::eShaderModuleCreateInfo,
            .codeSize = sizeof(glsl_shader_frag_spv),
            .pCode = (uint32_t*)glsl_shader_frag_spv,
        };
        bd->shader_module_frag = CheckVkResult(device.createShaderModule(frag_info, allocator));
    }
}

static void CreatePipeline(vk::Device device, const vk::AllocationCallbacks* allocator,
                           vk::PipelineCache pipeline_cache, vk::RenderPass render_pass,
                           vk::Pipeline* pipeline, uint32_t subpass) {
    VkData* bd = GetBackendData();
    const InitInfo& v = bd->init_info;

    CreateShaderModules(device, allocator);

    vk::PipelineShaderStageCreateInfo stage[2]{
        {
            .sType = vk::StructureType::ePipelineShaderStageCreateInfo,
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = bd->shader_module_vert,
            .pName = "main",
        },
        {
            .sType = vk::StructureType::ePipelineShaderStageCreateInfo,
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = bd->shader_module_frag,
            .pName = "main",
        },
    };

    vk::VertexInputBindingDescription binding_desc[1]{
        {
            .stride = sizeof(ImDrawVert),
            .inputRate = vk::VertexInputRate::eVertex,
        },
    };

    vk::VertexInputAttributeDescription attribute_desc[3]{
        {
            .location = 0,
            .binding = binding_desc[0].binding,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(ImDrawVert, pos),
        },
        {
            .location = 1,
            .binding = binding_desc[0].binding,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(ImDrawVert, uv),
        },
        {
            .location = 2,
            .binding = binding_desc[0].binding,
            .format = vk::Format::eR8G8B8A8Unorm,
            .offset = offsetof(ImDrawVert, col),
        },
    };

    vk::PipelineVertexInputStateCreateInfo vertex_info{
        .sType = vk::StructureType::ePipelineVertexInputStateCreateInfo,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = binding_desc,
        .vertexAttributeDescriptionCount = 3,
        .pVertexAttributeDescriptions = attribute_desc,
    };

    vk::PipelineInputAssemblyStateCreateInfo ia_info{
        .sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo,
        .topology = vk::PrimitiveTopology::eTriangleList,
    };

    vk::PipelineViewportStateCreateInfo viewport_info{
        .sType = vk::StructureType::ePipelineViewportStateCreateInfo,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    vk::PipelineRasterizationStateCreateInfo raster_info{
        .sType = vk::StructureType::ePipelineRasterizationStateCreateInfo,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo ms_info{
        .sType = vk::StructureType::ePipelineMultisampleStateCreateInfo,
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
    };

    vk::PipelineColorBlendAttachmentState color_attachment[1]{
        {
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eOne,
            .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .alphaBlendOp = vk::BlendOp::eAdd,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        },
    };

    vk::PipelineDepthStencilStateCreateInfo depth_info{
        .sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo,
    };

    vk::PipelineColorBlendStateCreateInfo blend_info{
        .sType = vk::StructureType::ePipelineColorBlendStateCreateInfo,
        .attachmentCount = 1,
        .pAttachments = color_attachment,
    };

    vk::DynamicState dynamic_states[2]{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state{
        .sType = vk::StructureType::ePipelineDynamicStateCreateInfo,
        .dynamicStateCount = (uint32_t)IM_ARRAYSIZE(dynamic_states),
        .pDynamicStates = dynamic_states,
    };

    vk::GraphicsPipelineCreateInfo info{
        .sType = vk::StructureType::eGraphicsPipelineCreateInfo,
        .pNext = &v.pipeline_rendering_create_info,
        .flags = bd->pipeline_create_flags,
        .stageCount = 2,
        .pStages = stage,
        .pVertexInputState = &vertex_info,
        .pInputAssemblyState = &ia_info,
        .pViewportState = &viewport_info,
        .pRasterizationState = &raster_info,
        .pMultisampleState = &ms_info,
        .pDepthStencilState = &depth_info,
        .pColorBlendState = &blend_info,
        .pDynamicState = &dynamic_state,
        .layout = bd->pipeline_layout,
        .renderPass = render_pass,
        .subpass = subpass,
    };

    *pipeline =
        CheckVkResult(device.createGraphicsPipelines(pipeline_cache, {info}, allocator)).front();
}

bool CreateDeviceObjects() {
    VkData* bd = GetBackendData();
    const InitInfo& v = bd->init_info;
    vk::Result err;

    if (!bd->descriptor_pool) {
        // large enough descriptor pool
        vk::DescriptorPoolSize pool_sizes[]{
            {vk::DescriptorType::eSampler, 1000},
            {vk::DescriptorType::eCombinedImageSampler, 1000},
            {vk::DescriptorType::eSampledImage, 1000},
            {vk::DescriptorType::eStorageImage, 1000},
            {vk::DescriptorType::eUniformTexelBuffer, 1000},
            {vk::DescriptorType::eStorageTexelBuffer, 1000},
            {vk::DescriptorType::eUniformBuffer, 1000},
            {vk::DescriptorType::eStorageBuffer, 1000},
            {vk::DescriptorType::eUniformBufferDynamic, 1000},
            {vk::DescriptorType::eStorageBufferDynamic, 1000},
            {vk::DescriptorType::eInputAttachment, 1000},
        };

        vk::DescriptorPoolCreateInfo pool_info{
            .sType = vk::StructureType::eDescriptorPoolCreateInfo,
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1000,
            .poolSizeCount = std::size(pool_sizes),
            .pPoolSizes = pool_sizes,
        };

        bd->descriptor_pool = CheckVkResult(v.device.createDescriptorPool(pool_info));
    }

    if (!bd->font_sampler) {
        // Bilinear sampling is required by default. Set 'io.Fonts->Flags |=
        // ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow
        // point/nearest sampling.
        vk::SamplerCreateInfo info{
            .sType = vk::StructureType::eSamplerCreateInfo,
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eRepeat,
            .addressModeV = vk::SamplerAddressMode::eRepeat,
            .addressModeW = vk::SamplerAddressMode::eRepeat,
            .maxAnisotropy = 1.0f,
            .minLod = -1000,
            .maxLod = 1000,
        };
        bd->font_sampler = CheckVkResult(v.device.createSampler(info, v.allocator));
    }

    if (!bd->descriptor_set_layout) {
        vk::DescriptorSetLayoutBinding binding[1]{
            {
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            },
        };
        vk::DescriptorSetLayoutCreateInfo info{
            .sType = vk::StructureType::eDescriptorSetLayoutCreateInfo,
            .bindingCount = 1,
            .pBindings = binding,
        };
        bd->descriptor_set_layout =
            CheckVkResult(v.device.createDescriptorSetLayout(info, v.allocator));
    }

    if (!bd->pipeline_layout) {
        // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection
        // matrix
        vk::PushConstantRange push_constants[1]{
            {
                .stageFlags = vk::ShaderStageFlagBits::eVertex,
                .offset = sizeof(float) * 0,
                .size = sizeof(float) * 4,
            },
        };
        vk::DescriptorSetLayout set_layout[1] = {bd->descriptor_set_layout};
        vk::PipelineLayoutCreateInfo layout_info{
            .sType = vk::StructureType::ePipelineLayoutCreateInfo,
            .setLayoutCount = 1,
            .pSetLayouts = set_layout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = push_constants,
        };
        bd->pipeline_layout =
            CheckVkResult(v.device.createPipelineLayout(layout_info, v.allocator));
    }

    CreatePipeline(v.device, v.allocator, v.pipeline_cache, nullptr, &bd->pipeline, v.subpass);

    return true;
}

void ImGuiImplVulkanDestroyDeviceObjects() {
    VkData* bd = GetBackendData();
    const InitInfo& v = bd->init_info;
    DestroyWindowRenderBuffers(v.device, bd->render_buffers, v.allocator);
    DestroyFontsTexture();

    if (bd->font_command_buffer) {
        v.device.freeCommandBuffers(bd->font_command_pool, {bd->font_command_buffer});
        bd->font_command_buffer = VK_NULL_HANDLE;
    }
    if (bd->font_command_pool) {
        v.device.destroyCommandPool(bd->font_command_pool, v.allocator);
        bd->font_command_pool = VK_NULL_HANDLE;
    }
    if (bd->shader_module_vert) {
        v.device.destroyShaderModule(bd->shader_module_vert, v.allocator);
        bd->shader_module_vert = VK_NULL_HANDLE;
    }
    if (bd->shader_module_frag) {
        v.device.destroyShaderModule(bd->shader_module_frag, v.allocator);
        bd->shader_module_frag = VK_NULL_HANDLE;
    }
    if (bd->font_sampler) {
        v.device.destroySampler(bd->font_sampler, v.allocator);
        bd->font_sampler = VK_NULL_HANDLE;
    }
    if (bd->descriptor_set_layout) {
        v.device.destroyDescriptorSetLayout(bd->descriptor_set_layout, v.allocator);
        bd->descriptor_set_layout = VK_NULL_HANDLE;
    }
    if (bd->pipeline_layout) {
        v.device.destroyPipelineLayout(bd->pipeline_layout, v.allocator);
        bd->pipeline_layout = VK_NULL_HANDLE;
    }
    if (bd->pipeline) {
        v.device.destroyPipeline(bd->pipeline, v.allocator);
        bd->pipeline = VK_NULL_HANDLE;
    }
}

bool Init(InitInfo info) {

    IM_ASSERT(info.instance != VK_NULL_HANDLE);
    IM_ASSERT(info.physical_device != VK_NULL_HANDLE);
    IM_ASSERT(info.device != VK_NULL_HANDLE);
    IM_ASSERT(info.image_count >= 2);

    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    auto* bd = IM_NEW(VkData)(info);
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_vulkan_shadps4";
    // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    CreateDeviceObjects();
    CreateFontsTexture();

    return true;
}

void Shutdown() {
    VkData* bd = GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGuiImplVulkanDestroyDeviceObjects();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    IM_DELETE(bd);
}

void NewFrame() {
    VkData* bd = GetBackendData();
    IM_ASSERT(bd != nullptr &&
              "Context or backend not initialized! Did you call ImGuiImplVulkanInit()?");

    if (!bd->font_descriptor_set)
        CreateFontsTexture();
}

} // namespace ImGui::Vulkan
