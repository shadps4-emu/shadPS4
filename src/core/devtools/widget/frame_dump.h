//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <vector>

#include "cmd_list.h"
#include "core/debug_state.h"

namespace Core::Devtools::Widget {

class CmdListViewer;

class FrameDumpViewer {
    friend class CmdListViewer;

    std::shared_ptr<DebugStateType::FrameDump> frame_dump;
    int id;

    std::vector<CmdListViewer> cmd_list_viewer;
    std::array<bool, 3> has_queue_type;

    DebugStateType::QueueType selected_queue_type;
    s32 selected_submit_num;
    s32 selected_queue_num2;
    s32 selected_cmd = -1;

public:
    bool is_open = true;

    explicit FrameDumpViewer(const DebugStateType::FrameDump& frame_dump);

    ~FrameDumpViewer();

    void Draw();
};

} // namespace Core::Devtools::Widget