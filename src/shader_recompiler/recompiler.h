// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/object_pool.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/program.h"

namespace Shader {

struct Profile;
struct RuntimeInfo;

struct Pools {
    static constexpr u32 InstPoolSize = 8192;
    static constexpr u32 BlockPoolSize = 32;

    Common::ObjectPool<IR::Inst> inst_pool;
    Common::ObjectPool<IR::Block> block_pool;

    explicit Pools() : inst_pool{InstPoolSize}, block_pool{BlockPoolSize} {}

    void ReleaseContents() {
        inst_pool.ReleaseContents();
        block_pool.ReleaseContents();
    }
};

[[nodiscard]] IR::Program TranslateProgram(const std::span<const u32>& code, Pools& pools,
                                           Info& info, RuntimeInfo& runtime_info,
                                           const Profile& profile);

} // namespace Shader
