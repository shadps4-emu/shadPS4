// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <queue>
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "common/arch.h"
#include "common/decoder.h"
#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/signal_context.h"
#include "core/emulator_settings.h"
#include "core/signals.h"
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/dominance.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/srt.h"
#include "shader_recompiler/ir/passes/srt_ir.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/srt_gvn_table.h"
#include "shader_recompiler/ir/topological_sort.h"
#include "shader_recompiler/ir/value.h"

#ifdef ARCH_X86_64

using namespace Xbyak::util;

static Xbyak::CodeGenerator g_srt_codegen(32_MB);
static const u8* g_srt_codegen_start = nullptr;

namespace Shader {

PFN_SrtWalker RegisterWalkerCode(const u8* ptr, size_t size) {
    const auto func_addr = (PFN_SrtWalker)g_srt_codegen.getCurr();
    g_srt_codegen.db(ptr, size);
    g_srt_codegen.ready();
    return func_addr;
}

} // namespace Shader

namespace {

static void DumpSrtProgram(const Shader::Info& info, const u8* code, size_t codesize) {
    using namespace Common::FS;

    const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
    if (!std::filesystem::exists(dump_dir)) {
        std::filesystem::create_directories(dump_dir);
    }
    const auto filename = fmt::format("{}_{:#018x}.srtprogram.txt", info.hw_stage, info.pgm_hash);
    const auto file = IOFile{dump_dir / filename, FileAccessMode::Create, FileType::TextFile};

    u64 address = reinterpret_cast<u64>(code);
    u64 code_end = address + codesize;
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus status = ZYAN_STATUS_SUCCESS;
    while (address < code_end && ZYAN_SUCCESS(Common::Decoder::Instance()->decodeInstruction(
                                     instruction, operands, reinterpret_cast<void*>(address)))) {
        std::string s =
            Common::Decoder::Instance()->disassembleInst(instruction, operands, address);
        s += "\n";
        file.WriteString(s);
        address += instruction.length;
    }
}

static bool SrtWalkerSignalHandler(void* context, void* fault_address) {
    // Only handle if the fault address is within the SRT code range
    const u8* code_start = g_srt_codegen_start;
    const u8* code_end = code_start + g_srt_codegen.getSize();
    const void* code = Common::GetRip(context);
    if (code < code_start || code >= code_end) {
        return false; // Not in SRT code range
    }

    // Patch instruction to zero register
    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus status = Common::Decoder::Instance()->decodeInstruction(instruction, operands,
                                                                       const_cast<void*>(code), 15);

    ASSERT(ZYAN_SUCCESS(status) && instruction.mnemonic == ZYDIS_MNEMONIC_MOV &&
           operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
           operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY);

    size_t len = instruction.length;
    const size_t patch_size = 3;
    u8* code_patch = const_cast<u8*>(reinterpret_cast<const u8*>(code));

    // We can only encounter rdi or r10d as the first operand in a
    // fault memory access for SRT walker.
    switch (operands[0].reg.value) {
    case ZYDIS_REGISTER_RDI:
        // mov rdi, [rdi + (off_dw << 2)] -> xor rdi, rdi
        code_patch[0] = 0x48;
        code_patch[1] = 0x31;
        code_patch[2] = 0xFF;
        break;
    case ZYDIS_REGISTER_R10D:
        // mov r10d, [rdi + (off_dw << 2)] -> xor r10d, r10d
        code_patch[0] = 0x45;
        code_patch[1] = 0x31;
        code_patch[2] = 0xD2;
        break;
    default:
        UNREACHABLE_MSG("Unsupported register for SRT walker patch");
        return false;
    }

    // Fill nops
    memset(code_patch + patch_size, 0x90, len - patch_size);

    LOG_WARNING(Render_Recompiler, "Patched SRT walker at {}, fault address {}", code,
                fault_address);

    return true;
}

} // namespace

namespace Shader::Optimization {

namespace {

struct PassInfo {
    struct PointerListCompare {
        bool operator()(const IR::Value& a, const IR::Value& b) const {
            // Keep order with the following criteria:
            // 1. If immediate, sort by immediate value
            // 2. If not immediate, sort by instruction order
            // 3. Immediates come first, then instructions
            if (a.IsImmediate() && b.IsImmediate()) {
                return a.U32() < b.U32();
            } else if (!a.IsImmediate() && !b.IsImmediate()) {
                auto a_inst = a.Inst();
                auto b_inst = b.Inst();
                auto a_block = a_inst->GetParent();
                auto b_block = b_inst->GetParent();
                if (a_block != b_block) {
                    return a_block < b_block; // Sort by block address
                }

                auto it = a_block->Instructions().iterator_to(*a_inst);
                auto end = a_block->Instructions().end();
                for (; it != end; ++it) {
                    if (&*it == b_inst) {
                        return true;
                    }
                }
                return false;
            } else {
                return a.IsImmediate(); // Immediates come first
            }
        }
    };

    // map offset to inst
    using PtrUserList =
        boost::container::flat_map<IR::Value, Shader::IR::Inst*, PointerListCompare>;

    Optimization::SrtGvnTable gvn_table;
    // keys are GetUserData, ReadConst or ReadConstBuffer instructions that are used as pointers
    std::unordered_map<IR::Inst*, PtrUserList> pointer_uses;
    // GetUserData instructions corresponding to sgpr_base of SRT roots
    boost::container::small_flat_map<IR::ScalarReg, IR::Inst*, 1> srt_roots;

    // pick a single inst for a given value number
    std::unordered_map<u32, IR::Inst*> vn_to_inst;

    using Inputs = boost::container::small_vector<IR::Inst*, 2>;

    struct ReadConstGuard {
        boost::container::small_vector<std::pair<IR::Block*, const IR::Block*>, 4> demand;
        Inputs inputs;
        bool always{};
        bool conditional{};
    };

    std::unordered_set<const IR::Inst*> walker_reads;
    std::unordered_map<const IR::Inst*, ReadConstGuard> guards;
    std::unordered_map<const IR::Inst*, IR::Inst*> parents;
    Optimization::Srt::Program srt;

    // Bumped during codegen to assign offsets to readconsts
    u16 dst_off_dw;

    PtrUserList* GetUsesAsPointer(IR::Inst* inst) {
        auto it = pointer_uses.find(inst);
        if (it != pointer_uses.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Return a single instruction that this instruction is identical to, according
    // to value number
    // The "original" is arbitrary. Here it's the first instruction found for a given value number
    IR::Inst* DeduplicateInstruction(IR::Inst* inst) {
        auto it = vn_to_inst.try_emplace(gvn_table.GetValueNumber(inst), inst);
        return it.first->second;
    }

    IR::Inst* ParentOf(const IR::Inst* inst) const {
        const auto it = parents.find(inst);
        return it != parents.end() ? it->second : nullptr;
    }

    IR::Inst* ChildOnPath(const IR::Inst* parent, IR::Inst* inst) const {
        for (IR::Inst* up = ParentOf(inst); up && up != parent; up = ParentOf(inst)) {
            inst = up;
        }
        return inst;
    }

    bool IsAncestor(const IR::Inst* ancestor, const IR::Inst* inst) const {
        for (const IR::Inst* up = ParentOf(inst); up; up = ParentOf(up)) {
            if (up == ancestor) {
                return true;
            }
        }
        return false;
    }

    Optimization::Srt::Ref GuardOf(const IR::Inst* inst);
};

static inline u16 GetFlatbufOffset(const IR::Inst* inst) {
    if (inst->GetOpcode() == IR::Opcode::ReadConstBuffer) {
        auto inst_info = inst->Flags<IR::BufferInstInfo>();
        return inst_info.flatbuf_off_dw;
    }
    if (inst->GetOpcode() == IR::Opcode::ReadConst) {
        return inst->Flags<u16>();
    }
    UNREACHABLE_MSG("Instruction not supported");
}

static inline void SetFlatbufOffset(IR::Inst* inst, u16 offset) {
    if (inst->GetOpcode() == IR::Opcode::ReadConstBuffer) {
        auto inst_info = inst->Flags<IR::BufferInstInfo>();
        inst_info.flatbuf_off_dw.Assign(offset);
        inst->SetFlags(inst_info);
        return;
    }
    if (inst->GetOpcode() == IR::Opcode::ReadConst) {
        inst->SetFlags(offset);
        return;
    }
    UNREACHABLE_MSG("Instruction not supported");
}

constexpr u32 MaxSrtDepth = 32;
constexpr size_t MaxDemandBlocks = 16;
constexpr auto Near = Xbyak::CodeGenerator::T_NEAR;

std::optional<Srt::Opcode> SrtValueOpcode(IR::Opcode opcode) {
    switch (opcode) {
    case IR::Opcode::IAdd32:
        return Srt::Opcode::IAdd32;
    case IR::Opcode::ISub32:
        return Srt::Opcode::ISub32;
    case IR::Opcode::IMul32:
        return Srt::Opcode::IMul32;
    case IR::Opcode::ShiftLeftLogical32:
        return Srt::Opcode::ShiftLeftLogical32;
    case IR::Opcode::ShiftRightLogical32:
        return Srt::Opcode::ShiftRightLogical32;
    case IR::Opcode::BitwiseAnd32:
        return Srt::Opcode::BitwiseAnd32;
    case IR::Opcode::BitwiseOr32:
        return Srt::Opcode::BitwiseOr32;
    case IR::Opcode::BitwiseXor32:
        return Srt::Opcode::BitwiseXor32;
    case IR::Opcode::BitwiseNot32:
        return Srt::Opcode::BitwiseNot32;
    case IR::Opcode::UMin32:
        return Srt::Opcode::UMin32;
    case IR::Opcode::UMax32:
        return Srt::Opcode::UMax32;
    case IR::Opcode::BitFieldUExtract:
        return Srt::Opcode::BitFieldUExtract;
    default:
        return std::nullopt;
    }
}

std::optional<Srt::Opcode> SrtCompareOpcode(IR::Opcode opcode) {
    switch (opcode) {
    case IR::Opcode::IEqual32:
        return Srt::Opcode::IEqual;
    case IR::Opcode::INotEqual32:
        return Srt::Opcode::INotEqual;
    case IR::Opcode::SLessThan32:
        return Srt::Opcode::SLessThan;
    case IR::Opcode::SLessThanEqual32:
        return Srt::Opcode::SLessThanEqual;
    case IR::Opcode::SGreaterThan32:
        return Srt::Opcode::SGreaterThan;
    case IR::Opcode::SGreaterThanEqual32:
        return Srt::Opcode::SGreaterThanEqual;
    case IR::Opcode::ULessThan32:
        return Srt::Opcode::ULessThan;
    case IR::Opcode::ULessThanEqual32:
        return Srt::Opcode::ULessThanEqual;
    case IR::Opcode::UGreaterThan32:
        return Srt::Opcode::UGreaterThan;
    case IR::Opcode::UGreaterThanEqual32:
        return Srt::Opcode::UGreaterThanEqual;
    case IR::Opcode::FPOrdEqual32:
        return Srt::Opcode::FPOrdEqual;
    case IR::Opcode::FPOrdNotEqual32:
        return Srt::Opcode::FPOrdNotEqual;
    case IR::Opcode::FPOrdLessThan32:
        return Srt::Opcode::FPOrdLessThan;
    case IR::Opcode::FPOrdLessThanEqual32:
        return Srt::Opcode::FPOrdLessThanEqual;
    case IR::Opcode::FPOrdGreaterThan32:
        return Srt::Opcode::FPOrdGreaterThan;
    case IR::Opcode::FPOrdGreaterThanEqual32:
        return Srt::Opcode::FPOrdGreaterThanEqual;
    case IR::Opcode::FPUnordEqual32:
        return Srt::Opcode::FPUnordEqual;
    case IR::Opcode::FPUnordNotEqual32:
        return Srt::Opcode::FPUnordNotEqual;
    case IR::Opcode::FPUnordLessThan32:
        return Srt::Opcode::FPUnordLessThan;
    case IR::Opcode::FPUnordLessThanEqual32:
        return Srt::Opcode::FPUnordLessThanEqual;
    case IR::Opcode::FPUnordGreaterThan32:
        return Srt::Opcode::FPUnordGreaterThan;
    case IR::Opcode::FPUnordGreaterThanEqual32:
        return Srt::Opcode::FPUnordGreaterThanEqual;
    default:
        return std::nullopt;
    }
}

static bool IsAllowedOffsetInstruction(const IR::Inst* inst) {
    switch (inst->GetOpcode()) {
    case IR::Opcode::GetUserData:
    case IR::Opcode::ReadConst:
    case IR::Opcode::ReadConstBuffer:
        return true;
    default:
        return SrtValueOpcode(inst->GetOpcode()).has_value();
    }
}

bool PassesValueThrough(const IR::Inst& inst) {
    if (SrtValueOpcode(inst.GetOpcode()) || SrtCompareOpcode(inst.GetOpcode())) {
        return true;
    }
    switch (inst.GetOpcode()) {
    case IR::Opcode::CompositeConstructU32x2:
    case IR::Opcode::CompositeConstructU32x3:
    case IR::Opcode::CompositeConstructU32x4:
    case IR::Opcode::CompositeConstructU32x2x2:
    case IR::Opcode::CompositeExtractU32x2:
    case IR::Opcode::CompositeExtractU32x3:
    case IR::Opcode::CompositeExtractU32x4:
    case IR::Opcode::ImageHandle:
    case IR::Opcode::BitCastU32F32:
    case IR::Opcode::BitCastF32U32:
    case IR::Opcode::PackUint2x32:
    case IR::Opcode::UnpackUint2x32:
    case IR::Opcode::SelectU1:
    case IR::Opcode::SelectU32:
    case IR::Opcode::SelectF32:
    case IR::Opcode::SelectU64:
    case IR::Opcode::LogicalAnd:
    case IR::Opcode::LogicalOr:
    case IR::Opcode::LogicalNot:
    case IR::Opcode::ReadFirstLane:
    case IR::Opcode::Ballot:
    case IR::Opcode::InverseBallot:
    case IR::Opcode::SetScalarRegister:
    case IR::Opcode::SetVectorRegister:
    case IR::Opcode::SetScc:
    case IR::Opcode::SetExec:
    case IR::Opcode::SetVcc:
    case IR::Opcode::SetVccLo:
    case IR::Opcode::SetVccHi:
    case IR::Opcode::SetM0:
    case IR::Opcode::SetGotoVariable:
        return true;
    default:
        return false;
    }
}

struct Demand {
    boost::container::small_vector<IR::Block*, 4> blocks;
    bool always{};

    void Add(IR::Block* block) {
        if (std::ranges::find(blocks, block) == blocks.end()) {
            blocks.push_back(block);
        }
    }
};

class DemandCollector {
public:
    explicit DemandCollector(const PassInfo& pass_info_) : pass_info{pass_info_} {}

    const Demand& Of(const IR::Inst* inst, u32 depth = 0) {
        if (const auto it = demands.find(inst); it != demands.end()) {
            return it->second;
        }
        Demand demand;
        demand.always = depth >= MaxSrtDepth;
        for (const IR::Use& use : inst->Uses()) {
            if (demand.always) {
                break;
            }
            AddUse(demand, use, depth);
            demand.always |= demand.blocks.size() > MaxDemandBlocks;
        }
        return demands.insert_or_assign(inst, std::move(demand)).first->second;
    }

private:
    void AddUse(Demand& demand, const IR::Use& use, u32 depth) {
        const IR::Inst* const user = use.user;
        switch (user->GetOpcode()) {
        case IR::Opcode::ReadConst:
        case IR::Opcode::ReadConstBuffer:
            if (use.operand != 0 && pass_info.walker_reads.contains(user)) {
                demand.always = true;
                return;
            }
            break;
        default:
            if (PassesValueThrough(*user)) {
                const Demand& through = Of(user, depth + 1);
                demand.always |= through.always;
                for (IR::Block* const block : through.blocks) {
                    demand.Add(block);
                }
                return;
            }
            break;
        }
        demand.Add(user->GetParent());
    }

    const PassInfo& pass_info;
    std::unordered_map<const IR::Inst*, Demand> demands;
};

bool IsNormalOrZero(u32 bits) {
    const u32 exponent = (bits >> 23) & 0xFF;
    return exponent != 0xFF && (exponent != 0 || (bits & 0x7FFFFF) == 0);
}

class SrtBuilder {
public:
    SrtBuilder(PassInfo& pass_info_, Srt::Program& srt_, PassInfo::Inputs* inputs_ = nullptr)
        : pass_info{pass_info_}, srt{srt_}, inputs{inputs_} {}

    std::optional<Srt::Ref> Offset(const IR::Value& value) {
        return Value(value, false, 0);
    }

    Srt::Ref Guard(const PassInfo::ReadConstGuard& guard) {
        if (guard.always) {
            return Srt::TrueRef;
        }
        Srt::Ref result = Srt::FalseRef;
        for (const auto& [block, stop] : guard.demand) {
            result = srt.Or(result, Chain(block, stop));
            if (result == Srt::TrueRef) {
                break;
            }
        }
        return result;
    }

private:
    std::optional<Srt::Ref> Value(const IR::Value& value, bool is_guard, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() == IR::Type::U32) {
                return srt.Imm(value.U32());
            }
            if (is_guard && value.Type() == IR::Type::F32) {
                return srt.Imm(std::bit_cast<u32>(value.F32()));
            }
            return std::nullopt;
        }
        if (depth >= MaxSrtDepth || srt.IsFull()) {
            return std::nullopt;
        }
        IR::Inst* const inst = value.Inst();
        const IR::Opcode opcode = inst->GetOpcode();
        switch (opcode) {
        case IR::Opcode::GetUserData:
            return srt.Load(static_cast<u32>(inst->Arg(0).ScalarReg()));
        case IR::Opcode::ReadConst:
        case IR::Opcode::ReadConstBuffer:
            if (const u16 offset = GetFlatbufOffset(pass_info.DeduplicateInstruction(inst));
                offset != 0) {
                return srt.Load(offset);
            }
            if (inputs && pass_info.walker_reads.contains(inst)) {
                IR::Inst* const input = pass_info.DeduplicateInstruction(inst);
                if (std::ranges::find(*inputs, input) == inputs->end()) {
                    inputs->push_back(input);
                }
                return srt.Load(0);
            }
            return std::nullopt;
        case IR::Opcode::ReadFirstLane:
        case IR::Opcode::BitCastU32F32:
        case IR::Opcode::BitCastF32U32:
            if (is_guard) {
                return Value(inst->Arg(0), is_guard, depth + 1);
            }
            break;
        default:
            break;
        }
        const std::optional<Srt::Opcode> srt_opcode = SrtValueOpcode(opcode);
        if (!srt_opcode) {
            if (!is_guard) {
                LOG_ERROR(Render_Recompiler, "Unexpected instruction for offset computation, {}",
                          magic_enum::enum_name(opcode));
            }
            return std::nullopt;
        }
        std::array<Srt::Ref, 3> args{};
        for (size_t i = 0; i < inst->NumArgs(); ++i) {
            const std::optional<Srt::Ref> arg = Value(inst->Arg(i), is_guard, depth + 1);
            if (!arg) {
                return std::nullopt;
            }
            args[i] = *arg;
        }
        return srt.Op(*srt_opcode, args[0], args[1], args[2]);
    }

    Srt::Ref Chain(IR::Block* block, const IR::Block* stop) {
        Srt::Ref chain = Srt::TrueRef;
        for (IR::Block* current = block; current && current != stop;) {
            chain = srt.And(chain, EdgeCondition(current));
            IR::Block* const idom = current->immediate_dominator;
            current = idom != current ? idom : nullptr;
        }
        return chain;
    }

    Srt::Ref EdgeCondition(const IR::Block* block) {
        const IR::Block* const idom = block->immediate_dominator;
        if (!idom || idom == block || block->ImmPredecessors().size() != 1) {
            return Srt::TrueRef;
        }
        const IR::Block* const pred = block->ImmPredecessors().front();
        const Gcn::Block* const cfg = pred->cfg_block;
        if (pred->branch_cond.IsEmpty() || pred->ImmSuccessors().size() != 2 || !cfg ||
            !cfg->branch_true || !cfg->branch_false) {
            return Srt::TrueRef;
        }
        const IR::Block* const on_true = cfg->branch_true->ir_block;
        const IR::Block* const on_false = cfg->branch_false->ir_block;
        if (on_true == on_false || (block != on_true && block != on_false)) {
            return Srt::TrueRef;
        }
        return Condition(pred->branch_cond, block == on_true, 0);
    }

    Srt::Ref Condition(const IR::Value& value, bool polarity, u32 depth) {
        if (const std::optional<bool> known = Known(value)) {
            return *known == polarity ? Srt::TrueRef : Srt::FalseRef;
        }
        if (value.IsImmediate() || depth >= MaxSrtDepth || srt.IsFull()) {
            return Srt::TrueRef;
        }
        return ConditionInst(*value.Inst(), polarity, depth + 1);
    }

    Srt::Ref ConditionInst(const IR::Inst& inst, bool polarity, u32 depth) {
        switch (inst.GetOpcode()) {
        case IR::Opcode::ConditionRef:
            return Condition(inst.Arg(0), polarity, depth);
        case IR::Opcode::LogicalNot:
            return Condition(inst.Arg(0), !polarity, depth);
        case IR::Opcode::LogicalAnd:
            return Junction(inst, polarity, polarity, depth);
        case IR::Opcode::LogicalOr:
            return Junction(inst, !polarity, polarity, depth);
        case IR::Opcode::SelectU1:
            return Select(inst, polarity, depth);
        default:
            if (const std::optional<Srt::Opcode> opcode = SrtCompareOpcode(inst.GetOpcode())) {
                return Compare(inst, polarity ? *opcode : Srt::Negate(*opcode));
            }
            return Srt::TrueRef;
        }
    }

    Srt::Ref Junction(const IR::Inst& inst, bool is_and, bool polarity, u32 depth) {
        const Srt::Ref lhs = Condition(inst.Arg(0), polarity, depth);
        const Srt::Ref rhs = Condition(inst.Arg(1), polarity, depth);
        return is_and ? srt.And(lhs, rhs) : srt.Or(lhs, rhs);
    }

    Srt::Ref Select(const IR::Inst& inst, bool polarity, u32 depth) {
        const IR::Value cond = inst.Arg(0);
        if (const std::optional<bool> taken = Known(cond)) {
            return Condition(inst.Arg(*taken ? 1 : 2), polarity, depth);
        }
        const Srt::Ref when_true =
            srt.And(Condition(cond, true, depth), Condition(inst.Arg(1), polarity, depth));
        const Srt::Ref when_false =
            srt.And(Condition(cond, false, depth), Condition(inst.Arg(2), polarity, depth));
        return srt.Or(when_true, when_false);
    }

    Srt::Ref Compare(const IR::Inst& inst, Srt::Opcode opcode) {
        const size_t num_inputs = inputs ? inputs->size() : 0;
        const std::optional<Srt::Ref> lhs = Value(inst.Arg(0), true, 0);
        const std::optional<Srt::Ref> rhs = Value(inst.Arg(1), true, 0);
        if (!lhs || !rhs || (IsImm(*lhs) && IsImm(*rhs)) ||
            (Srt::IsFloatCompare(opcode) && (!IsFloatDecidable(*lhs) || !IsFloatDecidable(*rhs)))) {
            if (inputs) {
                inputs->resize(num_inputs);
            }
            return Srt::TrueRef;
        }
        return srt.Op(opcode, *lhs, *rhs);
    }

    bool IsImm(Srt::Ref ref) const {
        return srt[ref].opcode == Srt::Opcode::Imm32;
    }

    bool IsFloatDecidable(Srt::Ref ref) const {
        return !IsImm(ref) || IsNormalOrZero(srt[ref].imm);
    }

    static std::optional<bool> Known(const IR::Value& value) {
        if (value.IsImmediate() && value.Type() == IR::Type::U1) {
            return value.U1();
        }
        return std::nullopt;
    }

    PassInfo& pass_info;
    Srt::Program& srt;
    PassInfo::Inputs* inputs;
};

void EmitValue(Xbyak::CodeGenerator& c, const Srt::Program& srt, Srt::Ref ref,
               const Xbyak::Reg32& reg) {
    ASSERT(reg != ecx && reg != edx);
    const Srt::Inst& inst = srt[ref];
    switch (inst.opcode) {
    case Srt::Opcode::Imm32:
        c.mov(reg, inst.imm);
        return;
    case Srt::Opcode::LoadFlatbuf:
        c.mov(reg, dword[rsi + (inst.imm << 2)]);
        return;
    case Srt::Opcode::BitwiseNot32:
        EmitValue(c, srt, inst.args[0], reg);
        c.not_(reg);
        return;
    case Srt::Opcode::BitFieldUExtract:
        EmitValue(c, srt, inst.args[2], reg);
        c.push(reg.cvt64());
        EmitValue(c, srt, inst.args[1], reg);
        c.push(reg.cvt64());
        EmitValue(c, srt, inst.args[0], reg);
        c.pop(rcx);
        c.shr(reg, cl);
        c.pop(rcx);
        c.mov(edx, 1);
        c.shl(edx, cl);
        c.dec(edx);
        c.and_(reg, edx);
        return;
    default:
        break;
    }
    if (const Srt::Inst& rhs = srt[inst.args[1]]; rhs.opcode == Srt::Opcode::Imm32) {
        EmitValue(c, srt, inst.args[0], reg);
        c.mov(ecx, rhs.imm);
    } else {
        EmitValue(c, srt, inst.args[1], reg);
        c.push(reg.cvt64());
        EmitValue(c, srt, inst.args[0], reg);
        c.pop(rcx);
    }
    switch (inst.opcode) {
    case Srt::Opcode::IAdd32:
        c.add(reg, ecx);
        break;
    case Srt::Opcode::ISub32:
        c.sub(reg, ecx);
        break;
    case Srt::Opcode::IMul32:
        c.imul(reg, ecx);
        break;
    case Srt::Opcode::ShiftLeftLogical32:
        c.shl(reg, cl);
        break;
    case Srt::Opcode::ShiftRightLogical32:
        c.shr(reg, cl);
        break;
    case Srt::Opcode::BitwiseAnd32:
        c.and_(reg, ecx);
        break;
    case Srt::Opcode::BitwiseOr32:
        c.or_(reg, ecx);
        break;
    case Srt::Opcode::BitwiseXor32:
        c.xor_(reg, ecx);
        break;
    case Srt::Opcode::UMin32:
        c.cmp(reg, ecx);
        c.cmova(reg, ecx);
        break;
    case Srt::Opcode::UMax32:
        c.cmp(reg, ecx);
        c.cmovb(reg, ecx);
        break;
    default:
        UNREACHABLE_MSG("Unexpected SRT value opcode {}", static_cast<u32>(inst.opcode));
    }
}

void EmitNormalOrZeroCheck(Xbyak::CodeGenerator& c, const Xbyak::Reg32& reg,
                           const Xbyak::Label& unknown) {
    Xbyak::Label checked;
    c.mov(edx, reg);
    c.and_(edx, 0x7F800000);
    c.cmp(edx, 0x7F800000);
    c.je(unknown, Near);
    c.test(edx, edx);
    c.jnz(checked);
    c.test(reg, 0x7FFFFF);
    c.jnz(unknown, Near);
    c.L(checked);
}

void EmitJumpIfFalse(Xbyak::CodeGenerator& c, Srt::Opcode opcode, const Xbyak::Label& on_false) {
    switch (opcode) {
    case Srt::Opcode::IEqual:
    case Srt::Opcode::FPOrdEqual:
    case Srt::Opcode::FPUnordEqual:
        c.jne(on_false, Near);
        break;
    case Srt::Opcode::INotEqual:
    case Srt::Opcode::FPOrdNotEqual:
    case Srt::Opcode::FPUnordNotEqual:
        c.je(on_false, Near);
        break;
    case Srt::Opcode::SLessThan:
        c.jge(on_false, Near);
        break;
    case Srt::Opcode::SLessThanEqual:
        c.jg(on_false, Near);
        break;
    case Srt::Opcode::SGreaterThan:
        c.jle(on_false, Near);
        break;
    case Srt::Opcode::SGreaterThanEqual:
        c.jl(on_false, Near);
        break;
    case Srt::Opcode::ULessThan:
    case Srt::Opcode::FPOrdLessThan:
    case Srt::Opcode::FPUnordLessThan:
        c.jae(on_false, Near);
        break;
    case Srt::Opcode::ULessThanEqual:
    case Srt::Opcode::FPOrdLessThanEqual:
    case Srt::Opcode::FPUnordLessThanEqual:
        c.ja(on_false, Near);
        break;
    case Srt::Opcode::UGreaterThan:
    case Srt::Opcode::FPOrdGreaterThan:
    case Srt::Opcode::FPUnordGreaterThan:
        c.jbe(on_false, Near);
        break;
    case Srt::Opcode::UGreaterThanEqual:
    case Srt::Opcode::FPOrdGreaterThanEqual:
    case Srt::Opcode::FPUnordGreaterThanEqual:
        c.jb(on_false, Near);
        break;
    default:
        UNREACHABLE_MSG("Unexpected SRT compare opcode {}", static_cast<u32>(opcode));
    }
}

void EmitCondition(Xbyak::CodeGenerator& c, const Srt::Program& srt, Srt::Ref ref,
                   const Xbyak::Label& on_false) {
    const Srt::Inst& inst = srt[ref];
    switch (inst.opcode) {
    case Srt::Opcode::True:
        return;
    case Srt::Opcode::False:
        c.jmp(on_false, Near);
        return;
    case Srt::Opcode::LogicalAnd:
        EmitCondition(c, srt, inst.args[0], on_false);
        EmitCondition(c, srt, inst.args[1], on_false);
        return;
    case Srt::Opcode::LogicalOr: {
        Xbyak::Label on_true;
        Xbyak::Label next;
        EmitCondition(c, srt, inst.args[0], next);
        c.jmp(on_true, Near);
        c.L(next);
        EmitCondition(c, srt, inst.args[1], on_false);
        c.L(on_true);
        return;
    }
    default:
        break;
    }
    Xbyak::Label unknown;
    EmitValue(c, srt, inst.args[0], eax);
    EmitValue(c, srt, inst.args[1], r8d);
    if (Srt::IsFloatCompare(inst.opcode)) {
        if (srt[inst.args[0]].opcode != Srt::Opcode::Imm32) {
            EmitNormalOrZeroCheck(c, eax, unknown);
        }
        if (srt[inst.args[1]].opcode != Srt::Opcode::Imm32) {
            EmitNormalOrZeroCheck(c, r8d, unknown);
        }
        c.movd(xmm0, eax);
        c.movd(xmm1, r8d);
        c.ucomiss(xmm0, xmm1);
    } else {
        c.cmp(eax, r8d);
    }
    EmitJumpIfFalse(c, inst.opcode, on_false);
    c.L(unknown);
}

static void EmitOffset(Xbyak::CodeGenerator& c, const Srt::Program& srt, Srt::Ref offset) {
    EmitValue(c, srt, offset, r10d);
    c.shl(r10d, 2);
    c.mov(r10d, r10d);
}

static inline bool PushPtr(Xbyak::CodeGenerator& c, PassInfo& pass_info, const IR::Value& off_dw) {
    if (off_dw.IsImmediate()) {
        c.push(rdi);
        c.mov(rdi, ptr[rdi + (off_dw.U32() << 2)]);
    } else {
        const std::optional<Srt::Ref> offset = SrtBuilder{pass_info, pass_info.srt}.Offset(off_dw);
        if (!offset) {
            return false;
        }
        c.push(rdi);
        EmitOffset(c, pass_info.srt, *offset);
        c.mov(rdi, ptr[rdi + r10]);
    }
    c.mov(r10, 0xFFFFFFFFFFFFULL);
    c.and_(rdi, r10);
    return true;
}

static inline void PopPtr(Xbyak::CodeGenerator& c) {
    c.pop(rdi);
}

static bool EmitReadConst(Xbyak::CodeGenerator& c, PassInfo& pass_info, const IR::Value& src_off_dw,
                          IR::Inst* use) {
    if (src_off_dw.IsImmediate()) {
        c.mov(r10d, ptr[rdi + (src_off_dw.U32() << 2)]);
    } else {
        const std::optional<Srt::Ref> offset =
            SrtBuilder{pass_info, pass_info.srt}.Offset(src_off_dw);
        if (!offset) {
            return false;
        }
        EmitOffset(c, pass_info.srt, *offset);
        c.mov(r10d, dword[rdi + r10]);
    }
    c.mov(ptr[rsi + (pass_info.dst_off_dw << 2)], r10d);

    SetFlatbufOffset(use, pass_info.dst_off_dw);
    pass_info.dst_off_dw++;
    return true;
}

Srt::Ref PassInfo::GuardOf(const IR::Inst* inst) {
    const auto it = guards.find(inst);
    if (it == guards.end() || !it->second.conditional) {
        return Srt::TrueRef;
    }
    return SrtBuilder{*this, srt}.Guard(it->second);
}

static void EmitReads(std::span<IR::Inst* const> reads, IR::Inst* parent, PassInfo& pass_info,
                      Xbyak::CodeGenerator& c);

static void VisitPointer(const IR::Value& off_dw, IR::Inst* subtree,
                         std::span<IR::Inst* const> reads, PassInfo& pass_info,
                         Xbyak::CodeGenerator& c) {
    if (subtree->GetOpcode() == IR::Opcode::ReadConst && subtree->Flags<u16>() == 0 ||
        subtree->GetOpcode() == IR::Opcode::ReadConstBuffer &&
            subtree->Flags<IR::BufferInstInfo>().flatbuf_off_dw == 0) {
        return;
    }

    if (!PushPtr(c, pass_info, off_dw)) {
        LOG_ERROR(Render_Recompiler, "Failed to compute offset for SRT walker");
        return;
    }
    EmitReads(reads, subtree, pass_info, c);
    PopPtr(c);
}

static void EmitReads(std::span<IR::Inst* const> reads, IR::Inst* parent, PassInfo& pass_info,
                      Xbyak::CodeGenerator& c) {
    for (auto it = reads.begin(); it != reads.end();) {
        IR::Inst* const child = pass_info.ChildOnPath(parent, *it);
        if (child == *it) {
            const Srt::Ref guard = pass_info.GuardOf(child);
            Xbyak::Label skip;
            EmitCondition(c, pass_info.srt, guard, skip);
            do {
                if (!EmitReadConst(c, pass_info, (*it)->Arg(1), *it)) {
                    LOG_ERROR(Render_Recompiler, "Failed to compute offset for SRT walker");
                }
                ++it;
            } while (it != reads.end() && pass_info.ChildOnPath(parent, *it) == *it &&
                     pass_info.GuardOf(*it) == guard);
            c.L(skip);
            continue;
        }
        const auto end = std::find_if(it, reads.end(), [&](IR::Inst* read) {
            return pass_info.ChildOnPath(parent, read) != child;
        });
        const IR::Value off_dw =
            parent ? child->Arg(1) : IR::Value(static_cast<u32>(child->Arg(0).ScalarReg()));
        Xbyak::Label skip;
        EmitCondition(c, pass_info.srt, pass_info.GuardOf(child), skip);
        VisitPointer(off_dw, child, {it, end}, pass_info, c);
        c.L(skip);
        it = end;
    }
}

static void ClassifyReads(PassInfo& pass_info, Srt::Program& scratch, IR::Inst* ptr) {
    for (const auto [src_off_dw, use] : *pass_info.GetUsesAsPointer(ptr)) {
        PassInfo::ReadConstGuard& read = pass_info.guards[use];
        read.conditional = SrtBuilder{pass_info, scratch, &read.inputs}.Guard(read) != Srt::TrueRef;
        if (!read.conditional) {
            read.inputs.clear();
        }
        read.inputs.erase(std::remove(read.inputs.begin(), read.inputs.end(), use),
                          read.inputs.end());
        if (pass_info.GetUsesAsPointer(use)) {
            ClassifyReads(pass_info, scratch, use);
        }
    }
}

static void CollectReadConstGuards(PassInfo& pass_info, std::span<IR::Inst* const> all_readconsts) {
    std::unordered_set<const IR::Inst*> walked;
    for (const auto& [ptr, use_list] : pass_info.pointer_uses) {
        for (const auto& [src_off_dw, use] : use_list) {
            walked.insert(use);
        }
    }
    for (IR::Inst* const inst : all_readconsts) {
        if (walked.contains(pass_info.DeduplicateInstruction(inst))) {
            pass_info.walker_reads.insert(inst);
        }
    }

    DemandCollector demands{pass_info};
    for (IR::Inst* const inst : all_readconsts) {
        if (!pass_info.walker_reads.contains(inst)) {
            continue;
        }
        PassInfo::ReadConstGuard& guard = pass_info.guards[pass_info.DeduplicateInstruction(inst)];
        const Demand& demand = demands.Of(inst);
        guard.always |= demand.always;
        const IR::Block* const stop = inst->Arg(0).Inst()->Arg(0).Inst()->GetParent();
        for (IR::Block* const block : demand.blocks) {
            if (std::ranges::find(guard.demand, std::make_pair(block, stop)) ==
                guard.demand.end()) {
                guard.demand.emplace_back(block, stop);
            }
        }
        guard.always |= guard.demand.size() > MaxDemandBlocks;
    }
    Srt::Program scratch;
    for (const auto& [sgpr_base, root] : pass_info.srt_roots) {
        ClassifyReads(pass_info, scratch, root);
    }
}

static void AddReads(PassInfo& pass_info, IR::Inst* ptr, std::vector<IR::Inst*>& reads) {
    const PassInfo::PtrUserList& use_list = *pass_info.GetUsesAsPointer(ptr);

    // First copy all the src data from this tree level
    // That way, all data that was contiguous in the guest SRT is also contiguous in the
    // flattened buffer.
    // TODO src and dst are contiguous. Optimize with wider loads/stores
    // TODO if this subtree is dynamically indexed, don't compact it (keep it sparse)
    for (const auto& [src_off_dw, use] : use_list) {
        reads.push_back(use);
    }

    // Then visit any children used as pointers
    for (const auto& [src_off_dw, use] : use_list) {
        if (pass_info.GetUsesAsPointer(use)) {
            AddReads(pass_info, use, reads);
        }
    }
}

static std::vector<IR::Inst*> SortReads(PassInfo& pass_info) {
    std::vector<IR::Inst*> reads;
    for (const auto& [sgpr_base, root] : pass_info.srt_roots) {
        AddReads(pass_info, root, reads);
    }
    std::unordered_map<const IR::Inst*, size_t> index;
    for (size_t i = 0; i < reads.size(); ++i) {
        index.emplace(reads[i], i);
    }
    std::vector<boost::container::small_vector<size_t, 4>> successors(reads.size());
    for (size_t i = 0; i < reads.size(); ++i) {
        if (const auto it = index.find(pass_info.ParentOf(reads[i])); it != index.end()) {
            successors[it->second].push_back(i);
        }
        for (IR::Inst* const input : pass_info.guards[reads[i]].inputs) {
            const auto it = index.find(input);
            if (it != index.end() && !pass_info.IsAncestor(reads[i], input)) {
                successors[it->second].push_back(i);
            }
        }
    }
    const auto successors_of = [&](size_t read) -> const auto& { return successors[read]; };
    std::vector<IR::Inst*> order;
    order.reserve(reads.size());
    for (const size_t i : IR::TopologicalSort(reads.size(), successors_of)) {
        order.push_back(reads[i]);
    }
    return order;
}

static void GenerateSrtProgram(Info& info, PassInfo& pass_info) {
    Xbyak::CodeGenerator& c = g_srt_codegen;

    if (pass_info.srt_roots.empty()) {
        return;
    }

    // Register the signal handler for SRT walker, if not already registered
    if (g_srt_codegen_start == nullptr) {
        g_srt_codegen_start = c.getCurr();
        auto* signals = Core::Signals::Instance();
        // Call after the memory invalidation handler
        constexpr u32 priority = 1;
        signals->RegisterAccessViolationHandler(SrtWalkerSignalHandler, priority);
    }

    info.srt_info.walker_func = c.getCurr<PFN_SrtWalker>();
    pass_info.dst_off_dw = NUM_USER_DATA_REGS;
    ASSERT(pass_info.dst_off_dw == info.srt_info.flattened_bufsize_dw);

    const std::vector<IR::Inst*> order = SortReads(pass_info);
    EmitReads(order, nullptr, pass_info, c);

    c.ret();
    c.ready();

    info.srt_info.walker_func_size =
        c.getCurr() - reinterpret_cast<const u8*>(info.srt_info.walker_func);

    if (EmulatorSettings.IsDumpShaders()) {
        DumpSrtProgram(info, reinterpret_cast<const u8*>(info.srt_info.walker_func),
                       info.srt_info.walker_func_size);
    }

    info.srt_info.flattened_bufsize_dw = pass_info.dst_off_dw;
}

static bool IsReadConstSource(const IR::Value base) {
    auto* inst = base.TryInst();
    if (!inst) {
        return false;
    }
    return inst->GetOpcode() == IR::Opcode::GetUserData ||
           inst->GetOpcode() == IR::Opcode::ReadConst ||
           inst->GetOpcode() == IR::Opcode::ReadConstBuffer;
}

void SimplifyReadConstAddressAdd(IR::Inst& inst) {
    // This handles the following pattern by combining the addition with the offset
    // s_add_u32    s12, s2, <offset>
    // s_addc_u32   s13, s3, 0
    // s_load_dword s16, s[12:13], 0x0

    if (inst.GetOpcode() != IR::Opcode::ReadConst) {
        return;
    }
    IR::Inst* addr = inst.Arg(0).Inst();
    IR::Inst* hi = addr->Arg(1).TryInst();
    IR::Inst* lo = addr->Arg(0).TryInst();
    if (!hi || !lo) {
        return;
    }

    if (lo->GetOpcode() != IR::Opcode::IAdd32 || hi->GetOpcode() != IR::Opcode::IAdd32) {
        return;
    }

    IR::Value add_offset;
    IR::Value base;
    if (base = lo->Arg(0); IsReadConstSource(base)) {
        add_offset = lo->Arg(1);
    } else if (base = lo->Arg(1); IsReadConstSource(base)) {
        add_offset = lo->Arg(0);
    } else {
        return;
    }

    IR::Inst* sel = hi->Arg(1).TryInst();
    if (!sel || sel->GetOpcode() != IR::Opcode::SelectU32) {
        return;
    }

    IR::Value sel_true = sel->Arg(1);
    IR::Value sel_false = sel->Arg(2);
    if (!sel_true.IsImmediate() || !sel_false.IsImmediate() || sel_true.U32() != 1 ||
        sel_false.U32() != 0) {
        return;
    }

    addr->SetArg(0, base);
    addr->SetArg(1, hi->Arg(0));
    for (auto [user, operand] : addr->Uses()) {
        ASSERT(user->GetOpcode() == IR::Opcode::ReadConst && operand == 0);
        IR::IREmitter ir{*user->GetParent(), IR::Block::InstructionList::s_iterator_to(*user)};
        IR::U32 offset = IR::U32{user->Arg(1)};
        IR::U32 dw_add_offset =
            add_offset.IsImmediate()
                ? ir.Imm32(add_offset.U32() >> 2u)
                : IR::U32{ir.ShiftRightLogical(IR::U32{add_offset}, ir.Imm32(2u))};
        if (offset.IsImmediate() && dw_add_offset.IsImmediate()) {
            user->SetArg(1, ir.Imm32(offset.U32() + dw_add_offset.U32()));
        } else {
            user->SetArg(1, ir.IAdd(offset, dw_add_offset));
        }
    }
}

} // Anonymous namespace

void FlattenExtendedUserdataPass(IR::Program& program) {
    auto& post_order = program.post_order_blocks;
    PassInfo pass_info;

    // traverse at end and assign offsets to duplicate readconsts, using
    // vn_to_inst as the source
    boost::container::small_vector<IR::Inst*, 32> all_readconsts, visited;
    std::queue<IR::Inst*> queue;

    for (auto it = post_order.rbegin(); it != post_order.rend(); it++) {
        IR::Block* block{*it};
        for (IR::Inst& inst : *block) {
            if (inst.GetOpcode() != IR::Opcode::ReadConst &&
                inst.GetOpcode() != IR::Opcode::ReadConstBuffer) {
                continue;
            }

            if (inst.GetOpcode() == IR::Opcode::ReadConstBuffer) {
                // Only flatten ReadConstBuffer if it was marked as a sharp source in the
                // resource discovery pass
                auto inst_info = inst.Flags<IR::BufferInstInfo>();
                if (!inst_info.sharp_source) {
                    continue;
                }
            }

            SimplifyReadConstAddressAdd(inst);

            all_readconsts.push_back(&inst);

            auto offset = inst.Arg(1);
            if (offset.IsImmediate()) {
                continue;
            }

            queue.push(offset.Inst());
            while (!queue.empty()) {
                IR::Inst* inst{queue.front()};
                queue.pop();

                if (inst->GetOpcode() == IR::Opcode::ReadConstBuffer) {
                    auto buffer_inst_info = inst->Flags<IR::BufferInstInfo>();
                    if (!buffer_inst_info.sharp_source.Value()) {
                        all_readconsts.push_back(inst);
                        auto offset = inst->Arg(1);
                        if (!offset.IsImmediate()) {
                            auto arg_inst = offset.Inst();
                            if (std::ranges::find(visited, arg_inst) == visited.end()) {
                                visited.push_back(arg_inst);
                                queue.push(arg_inst);
                            }
                        }
                    }
                    continue;
                }
                if (inst->GetOpcode() == IR::Opcode::GetUserData ||
                    inst->GetOpcode() == IR::Opcode::ReadConst ||
                    !IsAllowedOffsetInstruction(inst)) {
                    continue;
                }
                for (size_t arg = inst->NumArgs(); arg--;) {
                    const IR::Value arg_value = inst->Arg(arg);
                    if (arg_value.IsImmediate()) {
                        continue;
                    }
                    IR::Inst* arg_inst = arg_value.Inst();
                    if (std::ranges::find(visited, arg_inst) == visited.end()) {
                        visited.push_back(arg_inst);
                        queue.push(arg_inst);
                    }
                }
            }
        }
    }

    for (auto inst : all_readconsts) {
        if (pass_info.DeduplicateInstruction(inst) != inst) {
            // This is a duplicate of a readconst we've already visited
            continue;
        }

        IR::Inst* base = inst->Arg(0).Inst();
        if (auto* inst = base->Arg(0).TryInst();
            inst && inst->GetOpcode() == IR::Opcode::ReadFirstLane) {
            continue;
        }
        ASSERT_MSG(IsReadConstSource(base->Arg(0)), "ReadConst base low not from constant memory");
        ASSERT_MSG(IsReadConstSource(base->Arg(1)), "ReadConst base high not from constant memory");

        IR::Inst* ptr_lo = base->Arg(0).Inst();
        ptr_lo = pass_info.DeduplicateInstruction(ptr_lo);

        auto ptr_uses_kv = pass_info.pointer_uses.try_emplace(ptr_lo, PassInfo::PtrUserList{});
        PassInfo::PtrUserList& user_list = ptr_uses_kv.first->second;

        user_list[inst->Arg(1)] = inst;
        pass_info.parents[inst] = ptr_lo;

        if (ptr_lo->GetOpcode() == IR::Opcode::GetUserData) {
            IR::ScalarReg ud_reg = ptr_lo->Arg(0).ScalarReg();
            pass_info.srt_roots[ud_reg] = ptr_lo;
        }
    }

    if (!program.info.skip_resource_guards) {
        IR::ComputeDominators(program.post_order_blocks);
        CollectReadConstGuards(pass_info, all_readconsts);
    }

    GenerateSrtProgram(program.info, pass_info);

    for (IR::Block* const block : program.blocks) {
        block->immediate_dominator = nullptr;
    }

    // Assign offsets to duplicate readconsts
    for (IR::Inst* readconst : all_readconsts) {
        ASSERT(pass_info.vn_to_inst.contains(pass_info.gvn_table.GetValueNumber(readconst)));
        IR::Inst* original = pass_info.DeduplicateInstruction(readconst);
        SetFlatbufOffset(readconst, GetFlatbufOffset(original));
    }

    program.info.RefreshFlatBuf();
}

} // namespace Shader::Optimization

#else

namespace Shader {

PFN_SrtWalker RegisterWalkerCode(const u8* ptr, size_t size) {
    UNREACHABLE_MSG("RegisterWalkerCode unimplemented for target architecture.");
}

namespace Optimization {

void FlattenExtendedUserdataPass(IR::Program& program) {
    UNREACHABLE_MSG("FlattenExtendedUserdataPass unimplemented for target architecture.");
}

} // namespace Optimization

} // namespace Shader

#endif
