// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::EmitDataShare(const GcnInst& inst) {
    switch (inst.opcode) {
    case Opcode::DS_SWIZZLE_B32:
        return DS_SWIZZLE_B32(inst);
    case Opcode::DS_READ_B32:
        return DS_READ(32, false, false, inst);
    case Opcode::DS_READ_B64:
        return DS_READ(64, false, false, inst);
    case Opcode::DS_READ2_B32:
        return DS_READ(32, false, true, inst);
    case Opcode::DS_READ2_B64:
        return DS_READ(64, false, true, inst);
    case Opcode::DS_WRITE_B32:
        return DS_WRITE(32, false, false, inst);
    case Opcode::DS_WRITE_B64:
        return DS_WRITE(64, false, false, inst);
    case Opcode::DS_WRITE2_B32:
        return DS_WRITE(32, false, true, inst);
    case Opcode::DS_WRITE2_B64:
        return DS_WRITE(64, false, true, inst);
    default:
        LogMissingOpcode(inst);
    }
}

void Translator::DS_SWIZZLE_B32(const GcnInst& inst) {
    const u16 offset = (inst.control.ds.offset1 << 8) | inst.control.ds.offset0;
    const IR::U32 src{GetSrc(inst.src[1])};
    const IR::U32 lane_id = ir.LaneId();

    if (offset >= 0xe000) {
        // FFT Mode
        const u8 mask = offset & 0x1f; // offset[4:0]
        IR::U32 j = ir.BitwiseAnd(lane_id, ir.Imm32(0x1f));
        j = ir.ShiftRightLogical(j, ir.BitCount(ir.Imm32(mask)));
        j = ir.BitwiseOr(j, ir.BitwiseAnd(lane_id, ir.Imm32(mask)));
        j = ir.BitwiseOr(j, ir.BitwiseAnd(lane_id, ir.Imm32(0x20)));
        SetDst(inst.dst[0], ir.QuadShuffle(src, j));

    } else if (offset >= 0xc000) {
        // Rotate Mode
        const u8 rotate = (offset >> 5) & 0x1f;           // offset[9:5]
        const u8 mask = offset & 0x1f;                    // offset[4:0]
        const bool rotate_right = (offset & 0x8000) != 0; // offset[15]

        IR::U32 j;
        if (rotate_right) {
            j = ir.BitwiseOr(ir.BitwiseAnd(lane_id, ir.Imm32(mask)),
                             ir.BitwiseAnd(ir.ISub(lane_id, ir.Imm32(mask)), ir.Imm32(~mask)));
            j = ir.BitwiseOr(j, ir.BitwiseAnd(lane_id, ir.Imm32(0x20)));
        } else {
            j = ir.BitwiseOr(ir.BitwiseAnd(lane_id, ir.Imm32(mask)),
                             ir.BitwiseAnd(ir.IAdd(lane_id, ir.Imm32(~mask)), ir.Imm32(~mask)));
            j = ir.BitwiseOr(j, ir.BitwiseAnd(lane_id, ir.Imm32(0x20)));
        }
        SetDst(inst.dst[0], ir.QuadShuffle(src, j));

    } else if (offset & 0x8000) {
        // Full Data Sharing Mode (offset[15] == 1)
        static const std::array<u8, 4> offsets = {0, 1, 2, 3};

        const IR::U32 group_id = ir.BitwiseAnd(lane_id, ir.Imm32(0x3));
        const u32 group_id_value = group_id.F32();

        if (group_id_value < offsets.size()) {
            const u8 base_offset_value = offsets[static_cast<std::size_t>(group_id_value)];
            const IR::U32 base_offset = ir.Imm32(base_offset_value);
            const IR::U32 index = ir.IAdd(ir.BitwiseAnd(lane_id, ir.Imm32(~0x3)), base_offset);

            SetDst(inst.dst[0], ir.QuadShuffle(src, index));
        } else {
            // Handling the case where group_id_value is out of bounds
            // Optionally log an error or handle it in a defined way
            const IR::U32 base_offset = ir.Imm32(0);
            const IR::U32 index = ir.IAdd(ir.BitwiseAnd(lane_id, ir.Imm32(~0x3)), base_offset);

            SetDst(inst.dst[0], ir.QuadShuffle(src, index));
        }
    } else {
        // Limited Data Sharing Mode (offset[15] == 0)
        const u8 xor_mask = (offset >> 10) & 0x1f; // offset[14:10]
        const u8 or_mask = (offset >> 5) & 0x1f;   // offset[9:5]
        const u8 and_mask = offset & 0x1f;         // offset[4:0]

        IR::U32 masked_lanes = ir.BitwiseAnd(lane_id, ir.Imm32(and_mask));
        IR::U32 j = ir.BitwiseOr(masked_lanes, ir.Imm32(or_mask));
        j = ir.BitwiseXor(j, ir.Imm32(xor_mask));
        j = ir.BitwiseOr(j, ir.BitwiseAnd(lane_id, ir.Imm32(0x20)));

        SetDst(inst.dst[0], ir.QuadShuffle(src, j));
    }
}

void Translator::DS_READ(int bit_size, bool is_signed, bool is_pair, const GcnInst& inst) {
    const IR::U32 addr{ir.GetVectorReg(IR::VectorReg(inst.src[0].code))};
    IR::VectorReg dst_reg{inst.dst[0].code};
    if (is_pair) {
        // Pair loads are either 32 or 64-bit
        const IR::U32 addr0 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset0)));
        const IR::Value data0 = ir.LoadShared(bit_size, is_signed, addr0);
        if (bit_size == 32) {
            ir.SetVectorReg(dst_reg++, IR::U32{data0});
        } else {
            ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(data0, 0)});
            ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(data0, 1)});
        }
        const IR::U32 addr1 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset1)));
        const IR::Value data1 = ir.LoadShared(bit_size, is_signed, addr1);
        if (bit_size == 32) {
            ir.SetVectorReg(dst_reg++, IR::U32{data1});
        } else {
            ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(data1, 0)});
            ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(data1, 1)});
        }
    } else if (bit_size == 64) {
        const IR::Value data = ir.LoadShared(bit_size, is_signed, addr);
        ir.SetVectorReg(dst_reg, IR::U32{ir.CompositeExtract(data, 0)});
        ir.SetVectorReg(dst_reg + 1, IR::U32{ir.CompositeExtract(data, 1)});
    } else {
        const IR::U32 data = IR::U32{ir.LoadShared(bit_size, is_signed, addr)};
        ir.SetVectorReg(dst_reg, data);
    }
}

void Translator::DS_WRITE(int bit_size, bool is_signed, bool is_pair, const GcnInst& inst) {
    const IR::U32 addr{ir.GetVectorReg(IR::VectorReg(inst.src[0].code))};
    const IR::VectorReg data0{inst.src[1].code};
    const IR::VectorReg data1{inst.src[2].code};
    if (is_pair) {
        const IR::U32 addr0 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset0)));
        if (bit_size == 32) {
            ir.WriteShared(32, ir.GetVectorReg(data0), addr0);
        } else {
            ir.WriteShared(
                64, ir.CompositeConstruct(ir.GetVectorReg(data0), ir.GetVectorReg(data0 + 1)),
                addr0);
        }
        const IR::U32 addr1 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset1)));
        if (bit_size == 32) {
            ir.WriteShared(32, ir.GetVectorReg(data1), addr1);
        } else {
            ir.WriteShared(
                64, ir.CompositeConstruct(ir.GetVectorReg(data1), ir.GetVectorReg(data1 + 1)),
                addr1);
        }
    } else if (bit_size == 64) {
        const IR::Value data =
            ir.CompositeConstruct(ir.GetVectorReg(data0), ir.GetVectorReg(data0 + 1));
        ir.WriteShared(bit_size, data, addr);
    } else {
        ir.WriteShared(bit_size, ir.GetVectorReg(data0), addr);
    }
}

void Translator::S_BARRIER() {
    ir.Barrier();
}

void Translator::V_READFIRSTLANE_B32(const GcnInst& inst) {
    ASSERT(info.stage != Stage::Compute);
    SetDst(inst.dst[0], GetSrc(inst.src[0]));
}

void Translator::V_READLANE_B32(const GcnInst& inst) {
    ASSERT(info.stage != Stage::Compute);
    SetDst(inst.dst[0], GetSrc(inst.src[0]));
}

void Translator::V_WRITELANE_B32(const GcnInst& inst) {
    ASSERT(info.stage != Stage::Compute);
    SetDst(inst.dst[0], GetSrc(inst.src[0]));
}

} // namespace Shader::Gcn
