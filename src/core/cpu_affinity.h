// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "common/types.h"

namespace Core {

struct CpuAffinityMap;

class CpuAffinity {
public:
    int SetThreadAffinity(uintptr_t thread, u64 guest_mask);
    int CurrentGuestCpu(u64 guest_mask);

private:
    std::shared_ptr<const CpuAffinityMap> mapping;
};

} // namespace Core
