// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <span>
#include <string>
#include <type_traits>

namespace gcn_test {

struct DispatchSize {
    std::uint32_t x = 1, y = 1, z = 1;
};

enum class Error {
    NoSuitableDevice,
    InstanceCreationFailed,
    DeviceCreationFailed,
    ShaderCreationFailed,
    BufferAllocationFailed,
    CommandSubmissionFailed,
    PushConstantTooLarge,
    OutputTooLarge,
    ExecutionFailed,
};

struct ErrorInfo {
    Error code;
    std::string message;
};

class Runner {
public:
    static std::expected<Runner*, ErrorInfo> instance();

    std::expected<void, ErrorInfo> run_raw(
        std::span<const std::uint32_t> spirv,
        std::span<const std::byte> push_constants,
        std::span<std::byte> output,
        DispatchSize dispatch = {}
    );

    template <typename OutputT, typename PushT>
    std::expected<OutputT, ErrorInfo> run(
        std::span<const std::uint32_t> spirv,
        const PushT& push,
        DispatchSize dispatch = {}
    ) {
        static_assert(std::is_trivially_copyable_v<PushT>);
        static_assert(std::is_trivially_copyable_v<OutputT>);
        OutputT result{};
        auto r = run_raw(
            spirv,
            {reinterpret_cast<const std::byte*>(&push), sizeof(PushT)},
            {reinterpret_cast<std::byte*>(&result), sizeof(OutputT)},
            dispatch
        );
        if (!r) return std::unexpected(r.error());
        return result;
    }

    template <typename OutputT>
    std::expected<OutputT, ErrorInfo> run(
        std::span<const std::uint32_t> spirv,
        DispatchSize dispatch = {}
    ) {
        static_assert(std::is_trivially_copyable_v<OutputT>);
        OutputT result{};
        auto r = run_raw(
            spirv, {},
            {reinterpret_cast<std::byte*>(&result), sizeof(OutputT)},
            dispatch
        );
        if (!r) return std::unexpected(r.error());
        return result;
    }

    ~Runner();
    Runner(const Runner&) = delete;
    Runner& operator=(const Runner&) = delete;

private:
    Runner() = default;
    std::expected<void, ErrorInfo> initialize();

    vk::Instance            instance_;
    vk::PhysicalDevice      physical_device_;
    vk::Device              device_;
    vk::Queue               queue_;
    std::uint32_t           queue_family_ = 0;
    vk::CommandPool         command_pool_;
    vk::CommandBuffer       command_buffer_;        // cached, reset per call
    vk::Fence               fence_;                 // cached, reset per call
    vk::DescriptorSetLayout descriptor_set_layout_; // push-descriptor
    vk::PipelineLayout      pipeline_layout_;

    std::uint32_t max_push_constant_size_ = 128;
};

} // namespace gcn_test
