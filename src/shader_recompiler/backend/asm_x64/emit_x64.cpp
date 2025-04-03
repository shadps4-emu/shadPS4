// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/func_traits.h"
#include "shader_recompiler/backend/asm_x64/emit_x64.h"
#include "shader_recompiler/backend/asm_x64/emit_x64_instructions.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;

namespace {

static void EmitCondition(EmitContext& ctx, const IR::Inst* ref, Label& label, bool invert) {
    IR::Value cond = ref->Arg(0);
    if (cond.IsImmediate()) {
        // If imediate, we evaluate at compile time
        if (cond.U1() != invert) {
            ctx.Code().jmp(label);
        }
    } else {
        const Operand& op = ctx.Def(cond.InstRecursive())[0];
        if (op.isREG()) {
            Reg8 reg = op.getReg().cvt8();
            ctx.Code().test(reg, reg);
        } else {
            ctx.Code().test(op, 0xff);
        }
        if (invert) {
            ctx.Code().jz(label);
        } else {
            ctx.Code().jnz(label);
        }
    }
}

template <typename ArgType>
ArgType Arg(EmitContext& ctx, const IR::Value& arg) {
    if constexpr (std::is_same_v<ArgType, const Operands&>) {
        return ctx.Def(arg);
    } else if constexpr (std::is_same_v<ArgType, const IR::Value&>) {
        return arg;
    } else if constexpr (std::is_same_v<ArgType, u32>) {
        return arg.U32();
    } else if constexpr (std::is_same_v<ArgType, u64>) {
        return arg.U64();
    } else if constexpr (std::is_same_v<ArgType, bool>) {
        return arg.U1();
    } else if constexpr (std::is_same_v<ArgType, IR::Attribute>) {
        return arg.Attribute();
    } else if constexpr (std::is_same_v<ArgType, IR::ScalarReg>) {
        return arg.ScalarReg();
    } else if constexpr (std::is_same_v<ArgType, IR::VectorReg>) {
        return arg.VectorReg();
    } else if constexpr (std::is_same_v<ArgType, const char*>) {
        return arg.StringLiteral();
    } else if constexpr (std::is_same_v<ArgType, IR::Patch>) {
        return arg.Patch();
    }
    UNREACHABLE();
}

template <auto func, bool is_first_arg_inst, bool has_dest, size_t... I>
void Invoke(EmitContext& ctx, IR::Inst* inst, std::index_sequence<I...>) {
    using Traits = Common::FuncTraits<decltype(func)>;
    if constexpr (has_dest) {
        if constexpr (is_first_arg_inst) {
            func(ctx, inst, ctx.Def(inst),
                 Arg<typename Traits::template ArgType<I + 3>>(ctx, inst->Arg(I))...);
        } else {
            func(ctx, ctx.Def(inst),
                 Arg<typename Traits::template ArgType<I + 2>>(ctx, inst->Arg(I))...);
        }
    } else {
        if constexpr (is_first_arg_inst) {
            func(ctx, inst, Arg<typename Traits::template ArgType<I + 2>>(ctx, inst->Arg(I))...);
        } else {
            func(ctx, Arg<typename Traits::template ArgType<I + 1>>(ctx, inst->Arg(I))...);
        }
    }
}

template <auto func, bool has_dest>
void Invoke(EmitContext& ctx, IR::Inst* inst) {
    using Traits = Common::FuncTraits<decltype(func)>;
    static_assert(Traits::NUM_ARGS >= 1, "Insufficient arguments");
    if constexpr (Traits::NUM_ARGS == 1) {
        Invoke<func, false, false>(ctx, inst, std::make_index_sequence<0>{});
    } else {
        using FirstArgType = typename Traits::template ArgType<1>;
        static constexpr bool is_first_arg_inst = std::is_same_v<FirstArgType, IR::Inst*>;
        static constexpr size_t num_inst_args = Traits::NUM_ARGS - (is_first_arg_inst ? 2 : 1);
        if constexpr (num_inst_args > 0 && has_dest) {
            Invoke<func, is_first_arg_inst, true>(ctx, inst,
                                                  std::make_index_sequence<num_inst_args - 1>{});
        } else {
            Invoke<func, is_first_arg_inst, false>(ctx, inst,
                                                   std::make_index_sequence<num_inst_args>{});
        }
    }
}

void EmitInst(EmitContext& ctx, IR::Inst* inst) {
    switch (inst->GetOpcode()) {
#define OPCODE(name, result_type, ...)                                                             \
    case IR::Opcode::name:                                                                         \
        Invoke<&Emit##name, IR::Type::result_type != IR::Type::Void>(ctx, inst);
#include "shader_recompiler/ir/opcodes.inc"
#undef OPCODE
    }
    UNREACHABLE_MSG("Invalid opcode {}", inst->GetOpcode());
}

void Traverse(EmitContext& ctx, const IR::Program& program) {
    CodeGenerator& c = ctx.Code();
    for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
        ctx.ResetTempRegs();
        switch (node.type) {
        case IR::AbstractSyntaxNode::Type::Block: {
            IR::Block* block = node.data.block;
            c.L(ctx.BlockLabel(block));
            for (IR::Inst& inst : *block) {
            }
            const auto& phi_assignments = ctx.PhiAssignments(block);
            if (phi_assignments) {
                for (const auto& [phi, value] : phi_assignments->get()) {
                    MovValue(ctx, ctx.Def(phi), value);
                }
            }
            break;
        }
        case IR::AbstractSyntaxNode::Type::If: {
            IR::Inst* ref = node.data.if_node.cond.InstRecursive();
            Label& merge = ctx.BlockLabel(node.data.if_node.merge);
            EmitCondition(ctx, ref, merge, true);
            break;
        }
        case IR::AbstractSyntaxNode::Type::Repeat: {
            IR::Inst* ref = node.data.repeat.cond.InstRecursive();
            Label& loop_header = ctx.BlockLabel(node.data.repeat.loop_header);
            EmitCondition(ctx, ref, loop_header, false);
            break;
        }
        case IR::AbstractSyntaxNode::Type::Break: {
            IR::Inst* ref = node.data.break_node.cond.InstRecursive();
            Label& merge = ctx.BlockLabel(node.data.break_node.merge);
            EmitCondition(ctx, ref, merge, true);
            +c.jz(merge);
            break;
        }
        case IR::AbstractSyntaxNode::Type::Return: {
            c.jmp(ctx.EndLabel());
            break;
        }
        case IR::AbstractSyntaxNode::Type::Unreachable: {
            c.int3();
            break;
        }
        case IR::AbstractSyntaxNode::Type::Loop:
        case IR::AbstractSyntaxNode::Type::EndIf:
            break;
        }
    }
}

} // Anonymous namespace

void EmitX64(const IR::Program& program, Xbyak::CodeGenerator& c) {
    EmitContext context(program, c);
    Traverse(context, program);
    context.Code().L(context.EndLabel());
    context.Epilogue();
}

void EmitPhi(EmitContext& ctx) {
   
}

void EmitVoid(EmitContext&) {}

void EmitIdentity(EmitContext& ctx) {
    throw NotImplementedException("Forward identity declaration");
}

void EmitConditionRef(EmitContext& ctx) {
    
}

void EmitReference(EmitContext&) {}

void EmitPhiMove(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetScc(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetExec(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetVcc(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetSccLo(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetVccLo(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetVccHi(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetM0(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetScc(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetExec(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetVcc(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetSccLo(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetVccLo(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetVccHi(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetM0(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

} // namespace Shader::Backend::X64