// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_headless/headless_backend.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/assert.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/videoout/buffer.h"
#include "core/memory.h"
#include "sdl_window.h"
#include "video_core/amdgpu/liverpool.h"
#include "video_core/page_manager.h"
#include "video_core/renderer/presenter.h"
#include "video_core/renderer/rasterizer.h"

namespace Headless {

namespace {

struct HeadlessFrameInfo {
    Libraries::VideoOut::BufferAttributeGroup attribute{};
    VAddr cpu_address{};
    u64 sequence{};
    bool is_blank{};
    bool used_present_thread{};
};

struct MappedRange {
    VAddr start{};
    VAddr end{};
};

[[nodiscard]] constexpr bool IsValidRange(const VAddr addr, const u64 size) {
    return size != 0 && static_cast<u64>(addr) <= std::numeric_limits<u64>::max() - size;
}

} // namespace

class HeadlessBackend::HeadlessPresenter final : public VideoCore::Render::IPresenter {
public:
    explicit HeadlessPresenter(Frontend::WindowSDL& window_) : window(window_) {
        display_settings = {
            .gamma = 1.0f,
            .fsr_enabled = Config::getFsrEnabled(),
            .rcas_enabled = Config::getRcasEnabled(),
            .rcas_attenuation = static_cast<float>(Config::getRcasAttenuation()) / 1000.0f,
        };
    }

    Frontend::WindowSDL& GetWindow() override {
        return window;
    }

    VideoCore::Render::DisplaySettings GetDisplaySettings() const override {
        std::scoped_lock lock{mutex};
        return display_settings;
    }

    void SetDisplaySettings(const VideoCore::Render::DisplaySettings& settings) override {
        std::scoped_lock lock{mutex};
        display_settings = settings;
    }

    bool IsHDRSupported() const override {
        return false;
    }

    void SetHDR(bool enable) override {
        std::scoped_lock lock{mutex};
        hdr_enabled = enable;
    }

    void RegisterVideoOutSurface(const Libraries::VideoOut::BufferAttributeGroup& attribute,
                                 VAddr cpu_address) override {
        std::scoped_lock lock{mutex};
        surfaces.insert_or_assign(cpu_address, attribute);
    }

    std::unique_ptr<VideoCore::Render::IFrameHandle> PrepareFrame(
        const Libraries::VideoOut::BufferAttributeGroup& attribute, VAddr cpu_address) override {
        std::scoped_lock lock{mutex};
        const auto surface = surfaces.find(cpu_address);
        return std::make_unique<FrameHandle>(HeadlessFrameInfo{
            .attribute = surface != surfaces.end() ? surface->second : attribute,
            .cpu_address = cpu_address,
            .sequence = ++next_sequence,
            .is_blank = false,
            .used_present_thread = false,
        });
    }

    std::unique_ptr<VideoCore::Render::IFrameHandle> PrepareBlankFrame(
        bool present_thread) override {
        std::scoped_lock lock{mutex};
        return std::make_unique<FrameHandle>(HeadlessFrameInfo{
            .sequence = ++next_sequence,
            .is_blank = true,
            .used_present_thread = present_thread,
        });
    }

    std::unique_ptr<VideoCore::Render::IFrameHandle> PrepareLastFrame() override {
        std::scoped_lock lock{mutex};
        if (!last_frame.has_value()) {
            return {};
        }
        return std::make_unique<FrameHandle>(*last_frame);
    }

    void Present(std::unique_ptr<VideoCore::Render::IFrameHandle> frame,
                 bool is_reusing_frame) override {
        if (!frame) {
            return;
        }

        auto* headless_frame = dynamic_cast<FrameHandle*>(frame.get());
        ASSERT_MSG(headless_frame != nullptr,
                   "Received a non-headless frame in the headless presenter");

        std::scoped_lock lock{mutex};
        last_frame = headless_frame->info;
        reusing_last_frame = is_reusing_frame;
    }

private:
    class FrameHandle final : public VideoCore::Render::IFrameHandle {
    public:
        explicit FrameHandle(HeadlessFrameInfo info_) : info(std::move(info_)) {}

        HeadlessFrameInfo info;
    };

    Frontend::WindowSDL& window;
    mutable std::mutex mutex;
    VideoCore::Render::DisplaySettings display_settings{};
    std::unordered_map<VAddr, Libraries::VideoOut::BufferAttributeGroup> surfaces;
    std::optional<HeadlessFrameInfo> last_frame;
    u64 next_sequence{};
    bool hdr_enabled{};
    bool reusing_last_frame{};
};

class HeadlessBackend::HeadlessRasterizer final : public VideoCore::Render::IRasterizer {
public:
    explicit HeadlessRasterizer(AmdGpu::Liverpool& liverpool_) : page_manager(this) {
        auto* memory = Core::Memory::Instance();
        ASSERT_MSG(memory != nullptr, "Memory manager not initialized before headless backend");
        liverpool_.BindRasterizer(this);
        memory->SetRasterizer(this);
    }

    void Draw(bool is_indexed, u32 index_offset) override {
        std::scoped_lock lock{mutex};
        ++draw_call_count;
        if (is_indexed) {
            ++indexed_draw_call_count;
        }
        last_index_offset = index_offset;
    }

    void DrawIndirect(bool is_indexed, VAddr arg_address, u32 offset, u32 stride, u32 max_count,
                      VAddr count_address) override {
        std::scoped_lock lock{mutex};
        ++draw_indirect_count;
        if (is_indexed) {
            ++indexed_draw_indirect_count;
        }
        last_indirect_address = arg_address;
        last_indirect_offset = offset;
        last_indirect_stride = stride;
        last_indirect_max_count = max_count;
        last_indirect_count_address = count_address;
    }

    void DispatchDirect() override {
        std::scoped_lock lock{mutex};
        ++dispatch_direct_count;
    }

    void DispatchIndirect(VAddr address, u32 offset, u32 size) override {
        std::scoped_lock lock{mutex};
        ++dispatch_indirect_count;
        last_dispatch_address = address;
        last_dispatch_offset = offset;
        last_dispatch_size = size;
    }

    void FillBuffer(VAddr address, u32 num_bytes, u32 value, bool is_gds) override {
        std::scoped_lock lock{mutex};
        ++fill_buffer_count;
        if (!is_gds) {
            return;
        }
        EnsureGdsCapacity(address + num_bytes);
        for (u32 offset = 0; offset < num_bytes; offset += sizeof(u32)) {
            const auto chunk_size = std::min<u32>(sizeof(u32), num_bytes - offset);
            std::memcpy(gds_storage.data() + address + offset, &value, chunk_size);
        }
    }

    void CopyBuffer(VAddr dst, VAddr src, u32 num_bytes, bool dst_gds, bool src_gds) override {
        std::scoped_lock lock{mutex};
        ++copy_buffer_count;
        if (!dst_gds && !src_gds) {
            return;
        }

        std::vector<u8> scratch(num_bytes, 0);
        if (src_gds) {
            EnsureGdsCapacity(src + num_bytes);
            std::memcpy(scratch.data(), gds_storage.data() + src, num_bytes);
        }
        if (dst_gds) {
            EnsureGdsCapacity(dst + num_bytes);
            std::memcpy(gds_storage.data() + dst, scratch.data(), num_bytes);
        }
    }

    u32 ReadDataFromGds(u32 gds_offset) override {
        std::scoped_lock lock{mutex};
        const auto end = static_cast<u64>(gds_offset) + sizeof(u32);
        if (end > gds_storage.size()) {
            return 0;
        }
        u32 value{};
        std::memcpy(&value, gds_storage.data() + gds_offset, sizeof(u32));
        return value;
    }

    bool InvalidateMemory(VAddr addr, u64 size) override {
        std::scoped_lock lock{mutex};
        ++invalidate_memory_count;
        return ContainsMappedRange(addr, size);
    }

    bool ReadMemory(VAddr addr, u64 size) override {
        std::scoped_lock lock{mutex};
        ++read_memory_count;
        return ContainsMappedRange(addr, size);
    }

    bool IsMapped(VAddr addr, u64 size) override {
        std::scoped_lock lock{mutex};
        return ContainsMappedRange(addr, size);
    }

    void MapMemory(VAddr addr, u64 size) override {
        if (!IsValidRange(addr, size)) {
            return;
        }

        {
            std::scoped_lock lock{mutex};
            AddMappedRange(addr, addr + size);
            ++map_memory_count;
        }
        page_manager.OnGpuMap(addr, size);
    }

    void UnmapMemory(VAddr addr, u64 size) override {
        if (!IsValidRange(addr, size)) {
            return;
        }

        {
            std::scoped_lock lock{mutex};
            RemoveMappedRange(addr, addr + size);
            ++unmap_memory_count;
        }
        page_manager.OnGpuUnmap(addr, size);
    }

    void CpSync() override {
        std::scoped_lock lock{mutex};
        ++cp_sync_count;
    }

    u64 Flush() override {
        std::scoped_lock lock{mutex};
        ++flush_count;
        return flush_count;
    }

    void Finish() override {
        std::scoped_lock lock{mutex};
        ++finish_count;
    }

    void OnSubmit() override {
        std::scoped_lock lock{mutex};
        ++submit_count;
    }

    void ScopeMarkerBegin(std::string_view str, bool from_guest) override {
        std::scoped_lock lock{mutex};
        marker_depth += 1;
        last_marker = str;
        last_marker_from_guest = from_guest;
    }

    void ScopeMarkerEnd(bool from_guest) override {
        std::scoped_lock lock{mutex};
        marker_depth = std::max<s32>(0, marker_depth - 1);
        last_marker_from_guest = from_guest;
    }

    void ScopedMarkerInsert(std::string_view str, bool from_guest) override {
        std::scoped_lock lock{mutex};
        last_marker = str;
        last_marker_color = std::nullopt;
        last_marker_from_guest = from_guest;
    }

    void ScopedMarkerInsertColor(std::string_view str, u32 color, bool from_guest) override {
        std::scoped_lock lock{mutex};
        last_marker = str;
        last_marker_color = color;
        last_marker_from_guest = from_guest;
    }

private:
    void EnsureGdsCapacity(const u64 size) {
        if (gds_storage.size() < size) {
            gds_storage.resize(size, 0);
        }
    }

    bool ContainsMappedRange(const VAddr addr, const u64 size) const {
        if (!IsValidRange(addr, size)) {
            return false;
        }

        const VAddr range_end = addr + size;
        VAddr covered_until = addr;
        for (const auto& range : mapped_ranges) {
            if (range.end <= covered_until) {
                continue;
            }
            if (range.start > covered_until) {
                return false;
            }
            covered_until = std::max(covered_until, range.end);
            if (covered_until >= range_end) {
                return true;
            }
        }
        return false;
    }

    void AddMappedRange(const VAddr start, const VAddr end) {
        MappedRange merged{start, end};
        auto it = mapped_ranges.begin();
        while (it != mapped_ranges.end() && it->end < merged.start) {
            ++it;
        }
        while (it != mapped_ranges.end() && it->start <= merged.end) {
            merged.start = std::min(merged.start, it->start);
            merged.end = std::max(merged.end, it->end);
            it = mapped_ranges.erase(it);
        }
        mapped_ranges.insert(it, merged);
    }

    void RemoveMappedRange(const VAddr start, const VAddr end) {
        std::vector<MappedRange> updated_ranges;
        updated_ranges.reserve(mapped_ranges.size() + 1);

        for (const auto& range : mapped_ranges) {
            if (range.end <= start || range.start >= end) {
                updated_ranges.push_back(range);
                continue;
            }
            if (range.start < start) {
                updated_ranges.push_back({range.start, start});
            }
            if (range.end > end) {
                updated_ranges.push_back({end, range.end});
            }
        }

        mapped_ranges = std::move(updated_ranges);
    }

    mutable std::mutex mutex;
    VideoCore::PageManager page_manager;
    std::vector<MappedRange> mapped_ranges;
    std::vector<u8> gds_storage;
    std::string last_marker;
    std::optional<u32> last_marker_color;
    u64 draw_call_count{};
    u64 indexed_draw_call_count{};
    u64 draw_indirect_count{};
    u64 indexed_draw_indirect_count{};
    u64 dispatch_direct_count{};
    u64 dispatch_indirect_count{};
    u64 fill_buffer_count{};
    u64 copy_buffer_count{};
    u64 invalidate_memory_count{};
    u64 read_memory_count{};
    u64 map_memory_count{};
    u64 unmap_memory_count{};
    u64 cp_sync_count{};
    u64 flush_count{};
    u64 finish_count{};
    u64 submit_count{};
    VAddr last_indirect_address{};
    VAddr last_indirect_count_address{};
    VAddr last_dispatch_address{};
    u32 last_index_offset{};
    u32 last_indirect_offset{};
    u32 last_indirect_stride{};
    u32 last_indirect_max_count{};
    u32 last_dispatch_offset{};
    u32 last_dispatch_size{};
    s32 marker_depth{};
    bool last_marker_from_guest{};
};

HeadlessBackend::HeadlessBackend(Frontend::WindowSDL& window, AmdGpu::Liverpool& liverpool) {
    presenter = std::make_unique<HeadlessPresenter>(window);
    rasterizer = std::make_unique<HeadlessRasterizer>(liverpool);
    capabilities = {
        .supports_hdr = false,
        .supports_shader_debug = false,
        .supports_overlay_ui = false,
    };
    LOG_INFO(Render, "Headless renderer backend initialized");
}

HeadlessBackend::~HeadlessBackend() = default;

VideoCore::Render::BackendKind HeadlessBackend::GetKind() const {
    return VideoCore::Render::BackendKind::Headless;
}

const VideoCore::Render::BackendCapabilities& HeadlessBackend::GetCapabilities() const {
    return capabilities;
}

VideoCore::Render::IPresenter& HeadlessBackend::GetPresenter() {
    return *presenter;
}

VideoCore::Render::IRasterizer& HeadlessBackend::GetRasterizer() {
    return *rasterizer;
}

VideoCore::Render::IShaderDebugProvider* HeadlessBackend::GetShaderDebugProvider() {
    return nullptr;
}

} // namespace Headless
