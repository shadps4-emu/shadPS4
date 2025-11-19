// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/div_ceil.h"
#include "shader_recompiler/backend/spirv/emit_spirv_bounds.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

Id EmitLoadSharedU16(EmitContext& ctx, Id offset) {
    const Id shift_id{ctx.ConstU32(1U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift_id)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 2u)};

    return AccessBoundsCheck<16>(ctx, index, ctx.ConstU32(num_elements), [&] {
        const Id pointer = ctx.EmitSharedMemoryAccess(ctx.shared_u16, ctx.shared_memory_u16, index);
        return ctx.OpLoad(ctx.U16, pointer);
    });
}

Id EmitLoadSharedU32(EmitContext& ctx, Id offset) {
    const Id shift_id{ctx.ConstU32(2U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift_id)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 4u)};

    return AccessBoundsCheck<32>(ctx, index, ctx.ConstU32(num_elements), [&] {
        const Id pointer = ctx.EmitSharedMemoryAccess(ctx.shared_u32, ctx.shared_memory_u32, index);
        return ctx.OpLoad(ctx.U32[1], pointer);
    });
}

Id EmitLoadSharedU64(EmitContext& ctx, Id offset) {
    const Id shift_id{ctx.ConstU32(3U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift_id)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 8u)};

    return AccessBoundsCheck<64>(ctx, index, ctx.ConstU32(num_elements), [&] {
        const Id pointer = ctx.EmitSharedMemoryAccess(ctx.shared_u64, ctx.shared_memory_u64, index);
        return ctx.OpLoad(ctx.U64, pointer);
    });
}

void EmitWriteSharedU16(EmitContext& ctx, Id offset, Id value) {
    const Id shift{ctx.ConstU32(1U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 2u)};

    AccessBoundsCheck<16>(ctx, index, ctx.ConstU32(num_elements), [&] {
        const Id pointer = ctx.EmitSharedMemoryAccess(ctx.shared_u16, ctx.shared_memory_u16, index);
        ctx.OpStore(pointer, value);
        return Id{0};
    });
}

void EmitWriteSharedU32(EmitContext& ctx, Id offset, Id value) {
    const Id shift{ctx.ConstU32(2U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 4u)};

    AccessBoundsCheck<32>(ctx, index, ctx.ConstU32(num_elements), [&] {
        const Id pointer = ctx.EmitSharedMemoryAccess(ctx.shared_u32, ctx.shared_memory_u32, index);
        ctx.OpStore(pointer, value);
        return Id{0};
    });
}

void EmitWriteSharedU64(EmitContext& ctx, Id offset, Id value) {
    const Id shift{ctx.ConstU32(3U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 8u)};

    AccessBoundsCheck<64>(ctx, index, ctx.ConstU32(num_elements), [&] {
        const Id pointer = ctx.EmitSharedMemoryAccess(ctx.shared_u64, ctx.shared_memory_u64, index);
        ctx.OpStore(pointer, value);
        return Id{0};
    });
}

} // namespace Shader::Backend::SPIRV
