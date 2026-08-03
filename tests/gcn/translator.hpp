// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <vector>

#include "common/types.h"

std::vector<u32> TranslateToSpirv(u64 raw_gcn_inst);
std::vector<u32> TranslateToSpirv(u64 raw_gcn_inst, u32 shared_memory_size);
std::vector<u32> TranslateToSpirv(std::span<const u64> raw_gcn_insts, u32 shared_memory_size = 0);
