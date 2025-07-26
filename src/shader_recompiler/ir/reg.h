// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/enum.h"
#include "common/types.h"
#include "video_core/amdgpu/pixel_format.h"

namespace Shader::IR {

enum class FloatClassFunc : u32 {
    SignalingNan = 1 << 0,
    QuietNan = 1 << 1,
    NegativeInfinity = 1 << 2,
    NegativeNormal = 1 << 3,
    NegativeDenorm = 1 << 4,
    NegativeZero = 1 << 5,
    PositiveZero = 1 << 6,
    PositiveDenorm = 1 << 7,
    PositiveNormal = 1 << 8,
    PositiveInfinity = 1 << 9,

    NaN = SignalingNan | QuietNan,
    Infinity = PositiveInfinity | NegativeInfinity,
    Negative = NegativeInfinity | NegativeNormal | NegativeDenorm | NegativeZero,
    Finite = NegativeNormal | NegativeDenorm | NegativeZero | PositiveNormal | PositiveDenorm |
             PositiveZero,
};
DECLARE_ENUM_FLAG_OPERATORS(FloatClassFunc)

union TextureInstInfo {
    u32 raw;
    BitField<0, 1, u32> is_depth;
    BitField<1, 1, u32> has_bias;
    BitField<2, 1, u32> has_lod_clamp;
    BitField<3, 1, u32> force_level0;
    BitField<4, 1, u32> has_lod;
    BitField<5, 1, u32> has_offset;
    BitField<6, 2, u32> gather_comp;
    BitField<8, 1, u32> has_derivatives;
    BitField<9, 1, u32> is_array;
    BitField<10, 1, u32> is_unnormalized;
    BitField<11, 1, u32> is_gather;
    BitField<12, 1, u32> is_r128;
    BitField<16, 16, u32> pc;
};

union BufferInstInfo {
    u32 raw;
    BitField<0, 1, u32> index_enable;
    BitField<1, 1, u32> offset_enable;
    BitField<2, 12, u32> inst_offset;
    BitField<14, 1, u32> system_coherent;
    BitField<15, 1, u32> globally_coherent;
    BitField<16, 1, u32> typed;
    BitField<17, 4, AmdGpu::DataFormat> inst_data_fmt;
    BitField<21, 3, AmdGpu::NumberFormat> inst_num_fmt;
};

enum class ScalarReg : u32 {
    S0,
    S1,
    S2,
    S3,
    S4,
    S5,
    S6,
    S7,
    S8,
    S9,
    S10,
    S11,
    S12,
    S13,
    S14,
    S15,
    S16,
    S17,
    S18,
    S19,
    S20,
    S21,
    S22,
    S23,
    S24,
    S25,
    S26,
    S27,
    S28,
    S29,
    S30,
    S31,
    S32,
    S33,
    S34,
    S35,
    S36,
    S37,
    S38,
    S39,
    S40,
    S41,
    S42,
    S43,
    S44,
    S45,
    S46,
    S47,
    S48,
    S49,
    S50,
    S51,
    S52,
    S53,
    S54,
    S55,
    S56,
    S57,
    S58,
    S59,
    S60,
    S61,
    S62,
    S63,
    S64,
    S65,
    S66,
    S67,
    S68,
    S69,
    S70,
    S71,
    S72,
    S73,
    S74,
    S75,
    S76,
    S77,
    S78,
    S79,
    S80,
    S81,
    S82,
    S83,
    S84,
    S85,
    S86,
    S87,
    S88,
    S89,
    S90,
    S91,
    S92,
    S93,
    S94,
    S95,
    S96,
    S97,
    S98,
    S99,
    S100,
    S101,
    S102,
    S103,
    Max,
};
static constexpr size_t NumScalarRegs = static_cast<size_t>(ScalarReg::Max);

enum class VectorReg : u32 {
    V0,
    V1,
    V2,
    V3,
    V4,
    V5,
    V6,
    V7,
    V8,
    V9,
    V10,
    V11,
    V12,
    V13,
    V14,
    V15,
    V16,
    V17,
    V18,
    V19,
    V20,
    V21,
    V22,
    V23,
    V24,
    V25,
    V26,
    V27,
    V28,
    V29,
    V30,
    V31,
    V32,
    V33,
    V34,
    V35,
    V36,
    V37,
    V38,
    V39,
    V40,
    V41,
    V42,
    V43,
    V44,
    V45,
    V46,
    V47,
    V48,
    V49,
    V50,
    V51,
    V52,
    V53,
    V54,
    V55,
    V56,
    V57,
    V58,
    V59,
    V60,
    V61,
    V62,
    V63,
    V64,
    V65,
    V66,
    V67,
    V68,
    V69,
    V70,
    V71,
    V72,
    V73,
    V74,
    V75,
    V76,
    V77,
    V78,
    V79,
    V80,
    V81,
    V82,
    V83,
    V84,
    V85,
    V86,
    V87,
    V88,
    V89,
    V90,
    V91,
    V92,
    V93,
    V94,
    V95,
    V96,
    V97,
    V98,
    V99,
    V100,
    V101,
    V102,
    V103,
    V104,
    V105,
    V106,
    V107,
    V108,
    V109,
    V110,
    V111,
    V112,
    V113,
    V114,
    V115,
    V116,
    V117,
    V118,
    V119,
    V120,
    V121,
    V122,
    V123,
    V124,
    V125,
    V126,
    V127,
    V128,
    V129,
    V130,
    V131,
    V132,
    V133,
    V134,
    V135,
    V136,
    V137,
    V138,
    V139,
    V140,
    V141,
    V142,
    V143,
    V144,
    V145,
    V146,
    V147,
    V148,
    V149,
    V150,
    V151,
    V152,
    V153,
    V154,
    V155,
    V156,
    V157,
    V158,
    V159,
    V160,
    V161,
    V162,
    V163,
    V164,
    V165,
    V166,
    V167,
    V168,
    V169,
    V170,
    V171,
    V172,
    V173,
    V174,
    V175,
    V176,
    V177,
    V178,
    V179,
    V180,
    V181,
    V182,
    V183,
    V184,
    V185,
    V186,
    V187,
    V188,
    V189,
    V190,
    V191,
    V192,
    V193,
    V194,
    V195,
    V196,
    V197,
    V198,
    V199,
    V200,
    V201,
    V202,
    V203,
    V204,
    V205,
    V206,
    V207,
    V208,
    V209,
    V210,
    V211,
    V212,
    V213,
    V214,
    V215,
    V216,
    V217,
    V218,
    V219,
    V220,
    V221,
    V222,
    V223,
    V224,
    V225,
    V226,
    V227,
    V228,
    V229,
    V230,
    V231,
    V232,
    V233,
    V234,
    V235,
    V236,
    V237,
    V238,
    V239,
    V240,
    V241,
    V242,
    V243,
    V244,
    V245,
    V246,
    V247,
    V248,
    V249,
    V250,
    V251,
    V252,
    V253,
    V254,
    V255,
    Max,
};
static constexpr size_t NumVectorRegs = static_cast<size_t>(VectorReg::Max);

template <class T>
concept RegT = std::is_same_v<T, ScalarReg> || std::is_same_v<T, VectorReg>;

template <RegT Reg>
[[nodiscard]] constexpr Reg operator+(Reg reg, int num) {
    const int result{static_cast<int>(reg) + num};
    if (result >= static_cast<int>(Reg::Max)) {
        UNREACHABLE_MSG("Overflow on register arithmetic");
    }
    if (result < 0) {
        UNREACHABLE_MSG("Underflow on register arithmetic");
    }
    return static_cast<Reg>(result);
}

template <RegT Reg>
[[nodiscard]] constexpr Reg operator-(Reg reg, int num) {
    return reg + (-num);
}

template <RegT Reg>
constexpr Reg operator++(Reg& reg) {
    reg = reg + 1;
    return reg;
}

template <RegT Reg>
constexpr Reg operator++(Reg& reg, int) {
    const Reg copy{reg};
    reg = reg + 1;
    return copy;
}

template <RegT Reg>
[[nodiscard]] constexpr size_t RegIndex(Reg reg) noexcept {
    return static_cast<size_t>(reg);
}

} // namespace Shader::IR

template <>
struct fmt::formatter<Shader::IR::ScalarReg> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(Shader::IR::ScalarReg reg, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "SGPR{}", static_cast<u32>(reg));
    }
};
template <>
struct fmt::formatter<Shader::IR::VectorReg> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    auto format(Shader::IR::VectorReg reg, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "VGPR{}", static_cast<u32>(reg));
    }
};
