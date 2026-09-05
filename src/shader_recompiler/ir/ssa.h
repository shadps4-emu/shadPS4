// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::IR {

enum class RegType {
    ScalarReg,
    VectorReg = ScalarReg + NumScalarRegs,
    ThreadBitReg = VectorReg + NumVectorRegs,
    Scc = ThreadBitReg + NumScalarRegs,
    Exec,
    Vcc,
    VccLo,
    VccHi,
    M0,
    IntrusiveEnd,
    GotoVariable,
    MaskLaneVariable,
    VirtualReg,
    None,
};

struct SsaState {
    static constexpr size_t NumValues = static_cast<size_t>(RegType::IntrusiveEnd);

    std::array<Value, NumValues> values;

    void Reset() {
        std::memset(this, 0, sizeof(SsaState));
    }
};

struct MaskLaneReg {
    IR::VectorReg vreg;
    u32 lane;

    u32 Key() const {
        return (RegIndex(vreg) << 6) | lane;
    }
};

/// Unique identier for guest registers participating in SSA passes
struct RegTag {
    RegType type{RegType::None};
    union {
        u32 index{};
        IR::ScalarReg sreg;
        IR::VectorReg vreg;
        IR::VirtualReg reg;
        MaskLaneReg lane_reg;
    };

    RegTag() = default;
    RegTag(IR::ScalarReg reg, bool is_sbit = false)
        : type{is_sbit ? RegType::ThreadBitReg : RegType::ScalarReg}, sreg{reg} {}
    RegTag(IR::VectorReg reg) : type{RegType::VectorReg}, vreg{reg} {}
    RegTag(IR::VectorReg reg, u32 lane) : type{RegType::MaskLaneVariable}, lane_reg{reg, lane} {}
    RegTag(IR::VirtualReg reg_) : type{RegType::VirtualReg}, reg{reg_} {}
    RegTag(RegType type_, u32 index_ = 0) : type{type_}, index{index_} {}

    bool IsIntrusive() const {
        return type < RegType::IntrusiveEnd;
    }

    u32 Index() const {
        DEBUG_ASSERT(IsIntrusive());
        return static_cast<u32>(type) + index;
    }

    bool operator==(const RegTag& other) const {
        return type == other.type && index == other.index;
    }

    explicit operator bool() const {
        return type != RegType::None;
    }
};
static constexpr RegTag EMPTY_REG_TAG{};

} // namespace Shader::IR