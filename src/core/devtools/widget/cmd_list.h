//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

// Credits to https://github.com/psucien/tlg-emu-tools/

#pragma once

#include <vector>

#include "common/types.h"
#include "video_core/buffer_cache/buffer_cache.h"

namespace AmdGpu {
union PM4Type3Header;
enum class PM4ItOpcode : u32;
} // namespace AmdGpu

namespace Core::Devtools::Widget {

class FrameDumpViewer;

class CmdListViewer {
    /*
     * Generic PM4 header
     */
    union PM4Header {
        struct {
            u32 reserved : 16;
            u32 count : 14;
            u32 type : 2; // PM4_TYPE
        };
        u32 u32All;
    };
    struct BatchInfo {
        std::string marker{};
        size_t start_addr;
        size_t end_addr;
        size_t command_addr;
        AmdGpu::PM4ItOpcode type;
        bool bypass{false};
    };

    FrameDumpViewer* parent;
    std::vector<BatchInfo> batches{};
    uintptr_t cmdb_addr;
    size_t cmdb_size;

    int batch_bp{-1};
    int vqid{255};

    void OnNop(AmdGpu::PM4Type3Header const* header, u32 const* body);
    void OnSetBase(AmdGpu::PM4Type3Header const* header, u32 const* body);
    void OnSetContextReg(AmdGpu::PM4Type3Header const* header, u32 const* body);
    void OnSetShReg(AmdGpu::PM4Type3Header const* header, u32 const* body);
    void OnDispatch(AmdGpu::PM4Type3Header const* header, u32 const* body);

public:
    explicit CmdListViewer(FrameDumpViewer* parent, const std::vector<u32>& cmd_list);

    void Draw();
};

} // namespace Core::Devtools::Widget
