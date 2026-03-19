// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>

#include "common/types.h"

namespace VideoCore::Render {

class IRasterizer {
public:
    virtual ~IRasterizer() = default;

    virtual void Draw(bool is_indexed, u32 index_offset = 0) = 0;
    virtual void DrawIndirect(bool is_indexed, VAddr arg_address, u32 offset, u32 stride,
                              u32 max_count, VAddr count_address) = 0;
    virtual void DispatchDirect() = 0;
    virtual void DispatchIndirect(VAddr address, u32 offset, u32 size) = 0;

    virtual void FillBuffer(VAddr address, u32 num_bytes, u32 value, bool is_gds) = 0;
    virtual void CopyBuffer(VAddr dst, VAddr src, u32 num_bytes, bool dst_gds, bool src_gds) = 0;
    virtual u32 ReadDataFromGds(u32 gds_offset) = 0;

    virtual bool InvalidateMemory(VAddr addr, u64 size) = 0;
    virtual bool ReadMemory(VAddr addr, u64 size) = 0;
    virtual bool IsMapped(VAddr addr, u64 size) = 0;
    virtual void MapMemory(VAddr addr, u64 size) = 0;
    virtual void UnmapMemory(VAddr addr, u64 size) = 0;

    virtual void CpSync() = 0;
    virtual u64 Flush() = 0;
    virtual void Finish() = 0;
    virtual void OnSubmit() = 0;

    virtual void ScopeMarkerBegin(std::string_view str, bool from_guest = false) = 0;
    virtual void ScopeMarkerEnd(bool from_guest = false) = 0;
    virtual void ScopedMarkerInsert(std::string_view str, bool from_guest = false) = 0;
    virtual void ScopedMarkerInsertColor(std::string_view str, u32 color,
                                         bool from_guest = false) = 0;
};

} // namespace VideoCore::Render
