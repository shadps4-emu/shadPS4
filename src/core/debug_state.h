// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <queue>

#include "common/types.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/regs.h"
#include "video_core/renderer_vulkan/vk_common.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
using ThreadID = DWORD;
#else
#include <pthread.h>
#include <signal.h>
using ThreadID = pthread_t;
#endif

namespace Core::Devtools {
class Layer;
namespace Widget {
class FrameGraph;
class ShaderList;
} // namespace Widget
} // namespace Core::Devtools

namespace DebugStateType {

extern bool showing_debug_menu_bar;

enum class QueueType {
    dcb = 0,
    ccb = 1,
    acb = 2,
};

struct QueueDump {
    QueueType type;
    u32 submit_num;
    u32 num2; // acb: queue_num; else: buffer_in_submit
    std::vector<u32> data;
    uintptr_t base_addr;
};

struct PipelineShaderProgramDump {
    std::string name;
    u64 hash;
    AmdGpu::ShaderProgram user_data{};
    std::vector<u32> code{};
};

struct PipelineComputerProgramDump {
    std::string name;
    u64 hash;
    AmdGpu::ComputeProgram cs_program{};
    std::vector<u32> code{};
};

struct RegDump {
    bool is_compute{false};
    static constexpr size_t MaxShaderStages = 5;
    AmdGpu::Regs regs;
    std::array<PipelineShaderProgramDump, MaxShaderStages> stages{};
    PipelineComputerProgramDump cs_data{};
};

struct FrameDump {
    u32 frame_id;
    std::vector<QueueDump> queues;
    std::unordered_map<uintptr_t, RegDump> regs; // address -> reg dump
};

struct ShaderDump {
    std::string name;
    Shader::LogicalStage l_stage;
    vk::ShaderModule module;

    std::vector<u32> spv;
    std::vector<u32> isa;

    std::vector<u32> patch_spv;
    std::string patch_source{};

    bool loaded_data = false;
    bool is_patched = false;
    std::string cache_spv_disasm{};
    std::string cache_isa_disasm{};
    std::string cache_patch_disasm{};

    ShaderDump(std::string name, Shader::LogicalStage l_stage, vk::ShaderModule module,
               std::vector<u32> spv, std::vector<u32> isa, std::vector<u32> patch_spv,
               bool is_patched)
        : name(std::move(name)), l_stage(l_stage), module(module), spv(std::move(spv)),
          isa(std::move(isa)), patch_spv(std::move(patch_spv)), is_patched(is_patched) {}

    ShaderDump(const ShaderDump& other) = delete;
    ShaderDump(ShaderDump&& other) noexcept
        : name{std::move(other.name)}, l_stage(other.l_stage), module{std::move(other.module)},
          spv{std::move(other.spv)}, isa{std::move(other.isa)},
          patch_spv{std::move(other.patch_spv)}, patch_source{std::move(other.patch_source)},
          cache_spv_disasm{std::move(other.cache_spv_disasm)},
          cache_isa_disasm{std::move(other.cache_isa_disasm)},
          cache_patch_disasm{std::move(other.cache_patch_disasm)} {}
    ShaderDump& operator=(const ShaderDump& other) = delete;
    ShaderDump& operator=(ShaderDump&& other) noexcept {
        if (this == &other)
            return *this;
        name = std::move(other.name);
        l_stage = other.l_stage;
        module = std::move(other.module);
        spv = std::move(other.spv);
        isa = std::move(other.isa);
        patch_spv = std::move(other.patch_spv);
        patch_source = std::move(other.patch_source);
        cache_spv_disasm = std::move(other.cache_spv_disasm);
        cache_isa_disasm = std::move(other.cache_isa_disasm);
        cache_patch_disasm = std::move(other.cache_patch_disasm);
        return *this;
    }
};

class DebugStateImpl {
    friend class Core::Devtools::Layer;
    friend class Core::Devtools::Widget::FrameGraph;
    friend class Core::Devtools::Widget::ShaderList;

    std::queue<std::string> debug_message_popup;

    std::mutex guest_threads_mutex{};
    std::vector<ThreadID> guest_threads{};
    std::atomic_bool is_guest_threads_paused = false;
    u64 pause_time{};

    std::atomic_int32_t flip_frame_count = 0;
    std::atomic_int32_t gnm_frame_count = 0;

    s32 gnm_frame_dump_request_count = -1;
    std::unordered_map<size_t, FrameDump*> waiting_reg_dumps;
    std::unordered_map<size_t, std::string> waiting_reg_dumps_dbg;
    bool waiting_submit_pause = false;
    bool should_show_frame_dump = false;

    std::shared_mutex frame_dump_list_mutex;
    std::vector<FrameDump> frame_dump_list{};

    std::vector<ShaderDump> shader_dump_list{};

public:
    float Framerate = 1.0f / 60.0f;
    float FrameDeltaTime;

    std::pair<u32, u32> game_resolution{};
    std::pair<u32, u32> output_resolution{};
    bool is_using_fsr{};

    void ShowDebugMessage(std::string message) {
        if (message.empty()) {
            return;
        }
        debug_message_popup.push(std::move(message));
    }

    bool& IsShowingDebugMenuBar() {
        return showing_debug_menu_bar;
    }

    void AddCurrentThreadToGuestList();

    void RemoveCurrentThreadFromGuestList();

    void PauseGuestThreads();

    void ResumeGuestThreads();

    bool IsGuestThreadsPaused() const {
        return is_guest_threads_paused;
    }

    void IncFlipFrameNum() {
        ++flip_frame_count;
    }

    void IncGnmFrameNum() {
        ++gnm_frame_count;
        --gnm_frame_dump_request_count;
    }

    u32 GetFrameNum() const {
        return flip_frame_count;
    }

    bool DumpingCurrentFrame() const {
        return gnm_frame_dump_request_count > 0;
    }

    bool DumpingCurrentReg() {
        std::shared_lock lock{frame_dump_list_mutex};
        return !waiting_reg_dumps.empty();
    }

    bool ShouldPauseInSubmit() const {
        return waiting_submit_pause && gnm_frame_dump_request_count == 0;
    }

    void RequestFrameDump(s32 count = 1);

    FrameDump& GetFrameDump() {
        return frame_dump_list[frame_dump_list.size() - gnm_frame_dump_request_count];
    }

    void PushQueueDump(QueueDump dump);

    void PushRegsDump(uintptr_t base_addr, uintptr_t header_addr, const AmdGpu::Regs& regs);
    using CsState = AmdGpu::ComputeProgram;
    void PushRegsDumpCompute(uintptr_t base_addr, uintptr_t header_addr, const CsState& cs_state);

    void CollectShader(const std::string& name, Shader::LogicalStage l_stage,
                       vk::ShaderModule module, std::span<const u32> spv,
                       std::span<const u32> raw_code, std::span<const u32> patch_spv,
                       bool is_patched);

private:
    std::optional<RegDump*> GetRegDump(uintptr_t base_addr, uintptr_t header_addr);
};
} // namespace DebugStateType

extern DebugStateType::DebugStateImpl& DebugState;
