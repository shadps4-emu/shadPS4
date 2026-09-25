// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <vector>

#include "common/types.h"

std::vector<u32> TranslateToSpirv(u64 raw_gcn_inst);
std::vector<u32> TranslateToSpirv(std::span<const u64> raw_gcn_insts);
std::vector<u32> TranslateFragmentFrontFaceToSpirv(bool front_face_all_bits);
std::vector<u32> TranslateFragmentPullModelToSpirv(bool use_amd_barycentrics);

struct FragmentInterpMovInfo {
    std::vector<u32> attribute_indices;
    u32 fsub_count{};
};

FragmentInterpMovInfo TranslateFragmentInterpMovSelector(u32 src_select, bool flat_shade,
                                                         bool offset5);
