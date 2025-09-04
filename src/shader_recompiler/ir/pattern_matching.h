// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::Optimiation::PatternMatching {

// Attempt at pattern matching for Insts and Values
// Needs improvement, mostly a convenience

template <typename Derived>
struct MatchObject {
    inline bool Match(IR::Value v) {
        return static_cast<Derived*>(this)->Match(v);
    }
};

struct MatchValue : MatchObject<MatchValue> {
    MatchValue(IR::Value& return_val_) : return_val(return_val_) {}

    inline bool Match(IR::Value v) {
        return_val = v;
        return true;
    }

private:
    IR::Value& return_val;
};

struct MatchIgnore : MatchObject<MatchIgnore> {
    MatchIgnore() {}

    inline bool Match(IR::Value v) {
        return true;
    }
};

struct MatchImm : MatchObject<MatchImm> {
    MatchImm(IR::Value& v) : return_val(v) {}

    inline bool Match(IR::Value v) {
        if (!v.IsImmediate()) {
            return false;
        }

        return_val = v;
        return true;
    }

private:
    IR::Value& return_val;
};

struct MatchAttribute : MatchObject<MatchAttribute> {
    MatchAttribute(IR::Attribute attribute_) : attribute(attribute_) {}

    inline bool Match(IR::Value v) {
        return v.Type() == IR::Type::Attribute && v.Attribute() == attribute;
    }

private:
    IR::Attribute attribute;
};

struct MatchU32 : MatchObject<MatchU32> {
    MatchU32(u32 imm_) : imm(imm_) {}

    inline bool Match(IR::Value v) {
        return v.IsImmediate() && v.Type() == IR::Type::U32 && v.U32() == imm;
    }

private:
    u32 imm;
};

template <IR::Opcode opcode, typename... Args>
struct MatchInstObject : MatchObject<MatchInstObject<opcode>> {
    static_assert(sizeof...(Args) == IR::NumArgsOf(opcode));
    MatchInstObject(Args&&... args) : pattern(std::forward_as_tuple(args...)) {}

    inline bool Match(IR::Value v) {
        IR::Inst* inst = v.TryInstRecursive();
        if (!inst || inst->GetOpcode() != opcode) {
            return false;
        }

        bool matched = true;

        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((matched = matched && std::get<Is>(pattern).Match(inst->Arg(Is))), ...);
        }(std::make_index_sequence<sizeof...(Args)>{});

        return matched;
    }

private:
    using MatchArgs = std::tuple<Args&...>;
    MatchArgs pattern;
};

template <IR::Opcode opcode, typename... Args>
inline auto MakeInstPattern(Args&&... args) {
    return MatchInstObject<opcode, Args...>(std::forward<Args>(args)...);
}

// Conveniences. TODO probably simpler way of doing this
#define M_READCONST(...) MakeInstPattern<IR::Opcode::ReadConst>(__VA_ARGS__)
#define M_GETUSERDATA(...) MakeInstPattern<IR::Opcode::GetUserData>(__VA_ARGS__)
#define M_BITFIELDUEXTRACT(...) MakeInstPattern<IR::Opcode::BitFieldUExtract>(__VA_ARGS__)
#define M_BITFIELDSEXTRACT(...) MakeInstPattern<IR::Opcode::BitFieldSExtract>(__VA_ARGS__)
#define M_GETATTRIBUTEU32(...) MakeInstPattern<IR::Opcode::GetAttributeU32>(__VA_ARGS__)
#define M_UMOD32(...) MakeInstPattern<IR::Opcode::UMod32>(__VA_ARGS__)
#define M_SHIFTRIGHTLOGICAL32(...) MakeInstPattern<IR::Opcode::ShiftRightLogical32>(__VA_ARGS__)
#define M_IADD32(...) MakeInstPattern<IR::Opcode::IAdd32>(__VA_ARGS__)
#define M_IMUL32(...) MakeInstPattern<IR::Opcode::IMul32>(__VA_ARGS__)
#define M_BITWISEAND32(...) MakeInstPattern<IR::Opcode::BitwiseAnd32>(__VA_ARGS__)
#define M_GETTESSGENERICATTRIBUTE(...)                                                             \
    MakeInstPattern<IR::Opcode::GetTessGenericAttribute>(__VA_ARGS__)
#define M_SETTCSGENERICATTRIBUTE(...)                                                              \
    MakeInstPattern<IR::Opcode::SetTcsGenericAttribute>(__VA_ARGS__)
#define M_COMPOSITECONSTRUCTU32X2(...)                                                             \
    MakeInstPattern<IR::Opcode::CompositeConstructU32x2>(__VA_ARGS__)
#define M_COMPOSITECONSTRUCTU32X3(...)                                                             \
    MakeInstPattern<IR::Opcode::CompositeConstructU32x3>(__VA_ARGS__)
#define M_COMPOSITECONSTRUCTU32X4(...)                                                             \
    MakeInstPattern<IR::Opcode::CompositeConstructU32x4>(__VA_ARGS__)

} // namespace Shader::Optimiation::PatternMatching