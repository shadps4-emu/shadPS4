// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/types.h"

namespace Core {

class CpuAffinity {
public:
    int SetThreadAffinity(uintptr_t thread, u64 guest_mask);
    int CurrentGuestCpu(u64 guest_mask);

private:
    void Remap(const std::vector<int>& allowed);

    struct HostCpu {
        int id;
        u8 guest_mask;
    };
    std::vector<HostCpu> host_cpus;
};

} // namespace Core
