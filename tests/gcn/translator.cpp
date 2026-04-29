// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "translator.hpp"

#include <iostream>

#include "common/io_file.h"
#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/post_order.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/recompiler.h"

using namespace Shader;

namespace Shader::Optimization {
void ResourceTrackingPassStub(IR::Program& program, const Profile& profile);
}

std::vector<u32> TranslateToSpirv(u64 raw_gcn_inst) {
    std::array<u32, 2> provided_inst{static_cast<u32>(raw_gcn_inst & 0xFFFFFFFFU),
                                     static_cast<u32>(raw_gcn_inst >> 32)};
    std::array<u32, 2> store{
        0xe0700000,
        0x80000000 // buffer_store_dword v0, v0, s[0:3], 0
    };
    Gcn::GcnCodeSlice first(provided_inst.data(), provided_inst.data() + provided_inst.size());
    Gcn::GcnCodeSlice second(store.data(), store.data() + store.size());

    Gcn::GcnDecodeContext decoder;
    Gcn::GcnInst inst = decoder.decodeInstruction(first);
    Gcn::GcnInst store_inst = decoder.decodeInstruction(second);

    Shader::Info info{};
    info.stage = Stage::Compute;
    info.l_stage = LogicalStage::Compute;
    info.flattened_ud_buf.resize(4);
    AmdGpu::Buffer buf = AmdGpu::Buffer::Null();
    std::memcpy(info.flattened_ud_buf.data(), &buf, sizeof(buf));

    IR::Program program{info};
    Pools pools{};

    IR::Block* block = pools.block_pool.Create(pools.inst_pool);
    program.blocks.push_back(block);

    program.syntax_list.emplace_back();
    program.syntax_list.back().type = IR::AbstractSyntaxNode::Type::Block;
    program.syntax_list.back().data.block = block;
    program.syntax_list.emplace_back();
    program.syntax_list.back().type = IR::AbstractSyntaxNode::Type::Return;
    program.post_order_blocks = Shader::IR::PostOrder(program.syntax_list.front());

    Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.subgroup_size = 32;
    profile.supports_robust_buffer_access = true;

    RuntimeInfo runtime_info{};
    runtime_info.Initialize(Stage::Compute);
    runtime_info.num_user_data = 4;
    runtime_info.cs_info.workgroup_size = {1, 1, 1};

    Gcn::Translator translator(program.info, runtime_info, profile);
    translator.EmitPrologue(block);

    for (int i = 0; i < 4; ++i) {
        // copy user data from SGPR to VGPR as (most?) instructions cannot access
        // two SGPRs
        Shader::Gcn::GcnInst mov{};
        mov.src[0].field = Shader::Gcn::OperandField::ScalarGPR;
        mov.src[0].code = i;
        mov.dst[0].field = Shader::Gcn::OperandField::VectorGPR;
        mov.dst[0].code = i;
        translator.S_MOV(mov);
    }
    translator.TranslateInstruction(inst);
    translator.TranslateInstruction(store_inst);

    Shader::Optimization::SsaRewritePass(program.post_order_blocks);
    Shader::Optimization::IdentityRemovalPass(program.blocks);
    Shader::Optimization::ResourceTrackingPassStub(program, profile);
    Shader::Optimization::ConstantPropagationPass(program.blocks);
    Shader::Optimization::DeadCodeEliminationPass(program);
    Shader::Optimization::CollectShaderInfoPass(program, profile);

    Backend::Bindings bindings{};

    const auto spirv = Backend::SPIRV::EmitSPIRV(profile, runtime_info, program, bindings);

    return spirv;
}
