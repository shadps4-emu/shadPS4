// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gcn_test_runner.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <memory>
#include <mutex>
#include <ranges>
#include <string_view>
#include <vector>

#include "shader_recompiler/resource.h"

// Exactly one TU must define the dynamic dispatcher storage.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace gcn_test {
namespace {

constexpr bool kEnableValidation =
#ifdef NDEBUG
    false;
#else
    true;
#endif

auto make_error(Error code, std::string message) {
    return std::unexpected(ErrorInfo{code, std::move(message)});
}

auto find_memory_type(vk::PhysicalDevice pd, std::uint32_t type_filter,
                      vk::MemoryPropertyFlags required) -> std::expected<std::uint32_t, ErrorInfo> {
    auto props = pd.getMemoryProperties();
    for (std::uint32_t i = 0; i < props.memoryTypeCount; ++i) {
        if ((type_filter & (1u << i)) &&
            (props.memoryTypes[i].propertyFlags & required) == required) {
            return i;
        }
    }
    return make_error(Error::BufferAllocationFailed, "no suitable memory type found");
}

struct HostBuffer {
    vk::Device device;
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* mapped = nullptr;

    ~HostBuffer() {
        if (mapped)
            device.unmapMemory(memory);
        if (buffer)
            device.destroyBuffer(buffer);
        if (memory)
            device.freeMemory(memory);
    }
    HostBuffer() = default;
    HostBuffer(const HostBuffer&) = delete;
    HostBuffer& operator=(const HostBuffer&) = delete;
};

auto create_host_buffer(vk::Device dev, vk::PhysicalDevice pd, vk::DeviceSize size,
                        vk::BufferUsageFlags usage)
    -> std::expected<std::unique_ptr<HostBuffer>, ErrorInfo> {
    auto buf = std::make_unique<HostBuffer>();
    buf->device = dev;

    auto [br, buffer] = dev.createBuffer(vk::BufferCreateInfo{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    });
    if (br != vk::Result::eSuccess)
        return make_error(Error::BufferAllocationFailed, "createBuffer");
    buf->buffer = buffer;

    auto req = dev.getBufferMemoryRequirements(buffer);
    auto mt = find_memory_type(pd, req.memoryTypeBits,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent);
    if (!mt)
        return std::unexpected(mt.error());

    auto [mr, mem] = dev.allocateMemory({
        .allocationSize = req.size,
        .memoryTypeIndex = *mt,
    });
    if (mr != vk::Result::eSuccess)
        return make_error(Error::BufferAllocationFailed, "allocateMemory");
    buf->memory = mem;

    if (dev.bindBufferMemory(buffer, mem, 0) != vk::Result::eSuccess)
        return make_error(Error::BufferAllocationFailed, "bindBufferMemory");

    auto [mapr, ptr] = dev.mapMemory(mem, 0, size);
    if (mapr != vk::Result::eSuccess)
        return make_error(Error::BufferAllocationFailed, "mapMemory");
    buf->mapped = ptr;

    return buf;
}

std::mutex g_runner_mutex;
std::unique_ptr<Runner> g_runner;

} // namespace

Runner::~Runner() {
    if (device_) {
        device_.waitIdle();
        if (fence_)
            device_.destroyFence(fence_);
        if (pipeline_layout_)
            device_.destroyPipelineLayout(pipeline_layout_);
        if (descriptor_set_layout_)
            device_.destroyDescriptorSetLayout(descriptor_set_layout_);
        if (command_pool_)
            device_.destroyCommandPool(command_pool_);
        device_.destroy();
    }
    if (instance_)
        instance_.destroy();
}

std::expected<Runner*, ErrorInfo> Runner::instance() {
    std::lock_guard lock{g_runner_mutex};
    if (g_runner)
        return g_runner.get();
    auto r = std::unique_ptr<Runner>(new Runner{});
    if (auto init = r->initialize(); !init)
        return std::unexpected(init.error());
    g_runner = std::move(r);
    return g_runner.get();
}

std::expected<void, ErrorInfo> Runner::initialize() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    // ---- Instance ------------------------------------------------------
    vk::ApplicationInfo app_info{
        .pApplicationName = "gcn_test_runner",
        .applicationVersion = 1,
        .pEngineName = "gcn_test_runner",
        .engineVersion = 1,
        .apiVersion = vk::ApiVersion13,
    };
    std::vector<const char*> layers;
    if (kEnableValidation)
        layers.push_back("VK_LAYER_KHRONOS_validation");

    auto [ir, inst] = vk::createInstance({
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
    });
    if (ir != vk::Result::eSuccess)
        return make_error(Error::InstanceCreationFailed,
                          std::format("createInstance: {}", vk::to_string(ir)));
    instance_ = inst;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_);

    // ---- Pick physical device with the extensions we need -------------
    auto [pr, devs] = instance_.enumeratePhysicalDevices();
    if (pr != vk::Result::eSuccess || devs.empty())
        return make_error(Error::NoSuitableDevice, "no Vulkan devices");

    constexpr std::array required_exts{
        VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_6_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
    };

    for (auto pd : devs) {
        auto [er, exts] = pd.enumerateDeviceExtensionProperties();
        if (er != vk::Result::eSuccess)
            continue;

        auto has_ext = [&](const char* name) {
            return std::ranges::any_of(
                exts, [&](auto& e) { return std::string_view{e.extensionName} == name; });
        };
        if (!std::ranges::all_of(required_exts, has_ext))
            continue;

        auto families = pd.getQueueFamilyProperties();
        for (std::uint32_t i = 0; i < families.size(); ++i) {
            if (families[i].queueFlags & vk::QueueFlagBits::eCompute) {
                physical_device_ = pd;
                queue_family_ = i;
                break;
            }
        }
        if (physical_device_)
            break;
    }
    if (!physical_device_)
        return make_error(Error::NoSuitableDevice,
                          "no device with compute + shader_object + maintenance6 + "
                          "push_descriptor");

    max_push_constant_size_ = sizeof(Shader::PushData);
    // physical_device_.getProperties().limits.maxPushConstantsSize;

    // ---- Device with feature chain ------------------------------------
    float priority = 1.0f;
    vk::DeviceQueueCreateInfo qci{
        .queueFamilyIndex = queue_family_,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    vk::PhysicalDeviceShaderObjectFeaturesEXT so_feat{.shaderObject = VK_TRUE};
    vk::PhysicalDeviceMaintenance6FeaturesKHR m6_feat{
        .pNext = &so_feat,
        .maintenance6 = VK_TRUE,
    };
    vk::PhysicalDeviceVulkan11Features v11_feat{
        .pNext = &m6_feat,
        .uniformAndStorageBuffer16BitAccess = VK_TRUE,
    };
    vk::PhysicalDeviceVulkan12Features v12_feat{
        .pNext = &v11_feat,
        .uniformAndStorageBuffer8BitAccess = VK_TRUE,
        .shaderInt8 = VK_TRUE,
    };
    vk::PhysicalDeviceFeatures phys_feat{
        .shaderInt64 = VK_TRUE,
        .shaderInt16 = VK_TRUE,
    };

    auto [dr, dev] = physical_device_.createDevice({
        .pNext = &v12_feat,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qci,
        .enabledExtensionCount = required_exts.size(),
        .ppEnabledExtensionNames = required_exts.data(),
        .pEnabledFeatures = &phys_feat,
    });
    if (dr != vk::Result::eSuccess)
        return make_error(Error::DeviceCreationFailed,
                          std::format("createDevice: {}", vk::to_string(dr)));
    device_ = dev;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device_);
    queue_ = device_.getQueue(queue_family_, 0);

    // ---- Command pool + cached command buffer -------------------------
    auto [cpr, pool] = device_.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queue_family_,
    });
    if (cpr != vk::Result::eSuccess)
        return make_error(Error::DeviceCreationFailed, "createCommandPool");
    command_pool_ = pool;

    auto [cbr, cbs] = device_.allocateCommandBuffers({
        .commandPool = command_pool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    });
    if (cbr != vk::Result::eSuccess)
        return make_error(Error::DeviceCreationFailed, "allocateCommandBuffers");
    command_buffer_ = cbs[0];

    // ---- Fence (cached, reset per call) --------------------------------
    auto [fr, fence] = device_.createFence({});
    if (fr != vk::Result::eSuccess)
        return make_error(Error::DeviceCreationFailed, "createFence");
    fence_ = fence;

    // ---- Descriptor set layout with push-descriptor flag --------------
    // Single storage buffer at binding 0. No descriptor sets are ever
    // allocated from this layout — the layout is just used to tell the
    // pipeline layout and shader what the push-descriptor shape is.
    vk::DescriptorSetLayoutBinding dsl_binding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
    };
    auto [dslr, dsl] = device_.createDescriptorSetLayout({
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = 1,
        .pBindings = &dsl_binding,
    });
    if (dslr != vk::Result::eSuccess)
        return make_error(Error::DeviceCreationFailed, "createDescriptorSetLayout");
    descriptor_set_layout_ = dsl;

    // ---- Pipeline layout sized to device max push constants -----------
    vk::PushConstantRange pc{
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = max_push_constant_size_,
    };
    auto [plr, pl] = device_.createPipelineLayout({
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout_,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pc,
    });
    if (plr != vk::Result::eSuccess)
        return make_error(Error::DeviceCreationFailed, "createPipelineLayout");
    pipeline_layout_ = pl;

    return {};
}

std::expected<void, ErrorInfo> Runner::run_raw(std::span<const std::uint32_t> spirv,
                                               std::span<const std::byte> push_constants,
                                               std::span<std::byte> output, DispatchSize dispatch) {
    if (push_constants.size() > max_push_constant_size_)
        return make_error(Error::PushConstantTooLarge,
                          std::format("push constants {} exceed device max {}",
                                      push_constants.size(), max_push_constant_size_));
    if (output.empty())
        return make_error(Error::OutputTooLarge, "output buffer is empty");

    // Per-call: output buffer --------------------------------------------
    auto buf_r = create_host_buffer(device_, physical_device_, output.size(),
                                    vk::BufferUsageFlagBits::eStorageBuffer);
    if (!buf_r)
        return std::unexpected(buf_r.error());
    auto& output_buffer = *buf_r;
    std::memset(output_buffer->mapped, 0, output.size());

    // Per-call: shader object --------------------------------------------
    vk::PushConstantRange shader_pc{
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        // .size = static_cast<std::uint32_t>(push_constants.size()),
        .size = sizeof(Shader::PushData),
    };
    vk::ShaderCreateInfoEXT sci{
        .stage = vk::ShaderStageFlagBits::eCompute,
        .codeType = vk::ShaderCodeTypeEXT::eSpirv,
        .codeSize = spirv.size() * sizeof(std::uint32_t),
        .pCode = spirv.data(),
        .pName = "main",
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout_,
        .pushConstantRangeCount = push_constants.empty() ? 0u : 1u,
        .pPushConstantRanges = push_constants.empty() ? nullptr : &shader_pc,
    };
    auto [sr, shaders] = device_.createShadersEXT(sci);
    if (sr != vk::Result::eSuccess)
        return make_error(Error::ShaderCreationFailed,
                          std::format("createShadersEXT: {}", vk::to_string(sr)));
    auto shader = shaders[0];
    struct ShaderGuard {
        vk::Device d;
        vk::ShaderEXT s;
        ~ShaderGuard() {
            if (s)
                d.destroyShaderEXT(s);
        }
    } sg{device_, shader};

    // Reset cached command buffer + fence --------------------------------
    device_.resetFences(fence_);
    command_buffer_.reset();

    if (command_buffer_.begin({
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        }) != vk::Result::eSuccess)
        return make_error(Error::CommandSubmissionFailed, "cmd.begin");

    // Bind shader object -------------------------------------------------
    vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eCompute;
    command_buffer_.bindShadersEXT(1, &stage, &shader);

    // Push descriptor: binding 0 = output SSBO ---------------------------
    vk::DescriptorBufferInfo dbi{
        .buffer = output_buffer->buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    vk::WriteDescriptorSet write{
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &dbi,
    };
    vk::PushDescriptorSetInfoKHR push_desc{
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .layout = pipeline_layout_,
        .set = 0,
        .descriptorWriteCount = 1,
        .pDescriptorWrites = &write,
    };
    command_buffer_.pushDescriptorSet2KHR(push_desc);

    // Push constants -----------------------------------------------------
    if (!push_constants.empty()) {
        vk::PushConstantsInfoKHR pci{
            .layout = pipeline_layout_,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
            .offset = 16, // fall onto ud_regs in PushData
            .size = static_cast<std::uint32_t>(push_constants.size()),
            .pValues = push_constants.data(),
        };
        command_buffer_.pushConstants2KHR(pci);
    }

    command_buffer_.dispatch(dispatch.x, dispatch.y, dispatch.z);

    vk::MemoryBarrier barrier{
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eHostRead,
    };
    command_buffer_.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                    vk::PipelineStageFlagBits::eHost, {}, barrier, {}, {});

    if (command_buffer_.end() != vk::Result::eSuccess)
        return make_error(Error::CommandSubmissionFailed, "cmd.end");

    vk::SubmitInfo si{
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer_,
    };
    if (queue_.submit(si, fence_) != vk::Result::eSuccess)
        return make_error(Error::CommandSubmissionFailed, "queue.submit");
    if (device_.waitForFences(fence_, VK_TRUE, UINT64_MAX) != vk::Result::eSuccess)
        return make_error(Error::ExecutionFailed, "waitForFences");

    std::memcpy(output.data(), output_buffer->mapped, output.size());
    return {};
}

} // namespace gcn_test
