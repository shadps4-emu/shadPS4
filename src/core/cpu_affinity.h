// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>
#include "common/types.h"

namespace Core {

class CpuAffinity {
public:
    explicit CpuAffinity(std::vector<int> allowed = {});

    int SetThreadAffinity(uintptr_t thread, u64 guest_mask);
    int CurrentGuestCpu(u64 guest_mask);
    int GetAllowedHostCpus(uintptr_t thread, std::vector<int>& cpus);

private:
    int Refresh(uintptr_t thread);
    void Remap(std::vector<int> allowed);

    std::array<int, 8> host_cpus{};
    std::vector<int> allowed_cpus;
    std::vector<int> applied_cpus;
};

} // namespace Core
