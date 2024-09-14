// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::EmitDataShare(const GcnInst& inst) {
    switch (inst.opcode) {
    case Opcode::DS_SWIZZLE_B32:
        return DS_SWIZZLE_B32(inst);
    case Opcode::DS_READ_B32:
        return DS_READ(32, false, false, false, inst);
    case Opcode::DS_READ2ST64_B32:
        return DS_READ(32, false, true, true, inst);
    case Opcode::DS_READ_B64:
        return DS_READ(64, false, false, false, inst);
    case Opcode::DS_READ2_B32:
        return DS_READ(32, false, true, false, inst);
    case Opcode::DS_READ2_B64:
        return DS_READ(64, false, true, false, inst);
    case Opcode::DS_WRITE_B32:
        return DS_WRITE(32, false, false, false, inst);
    case Opcode::DS_WRITE2ST64_B32:
        return DS_WRITE(32, false, true, true, inst);
    case Opcode::DS_WRITE_B64:
        return DS_WRITE(64, false, false, false, inst);
    case Opcode::DS_WRITE2_B32:
        return DS_WRITE(32, false, true, false, inst);
    case Opcode::DS_WRITE2_B64:
        return DS_WRITE(64, false, true, false, inst);
    case Opcode::DS_ADD_U32:
        return DS_ADD_U32(inst, false);
    case Opcode::DS_MIN_U32:
        return DS_MIN_U32(inst, false, false);
    case Opcode::DS_MIN_I32:
        return DS_MIN_U32(inst, true, false);
    case Opcode::DS_MAX_U32:
        return DS_MAX_U32(inst, false, false);
    case Opcode::DS_MAX_I32:
        return DS_MAX_U32(inst, true, false);
    case Opcode::DS_ADD_RTN_U32:
        return DS_ADD_U32(inst, true);
    case Opcode::DS_MIN_RTN_U32:
        return DS_MIN_U32(inst, false, true);
    case Opcode::DS_MAX_RTN_U32:
        return DS_MAX_U32(inst, false, true);
    case Opcode::DS_APPEND:
        return DS_APPEND(inst);
    case Opcode::DS_CONSUME:
        return DS_CONSUME(inst);
    default:
        LogMissingOpcode(inst);
    }
}

void Translator::DS_SWIZZLE_B32(const GcnInst& inst) {
    const u8 offset0 = inst.control.ds.offset0;
    const u8 offset1 = inst.control.ds.offset1;
    const IR::U32 src{GetSrc(inst.src[1])};
    ASSERT(offset1 & 0x80);
    const IR::U32 lane_id = ir.LaneId();
    const IR::U32 id_in_group = ir.BitwiseAnd(lane_id, ir.Imm32(0b11));
    const IR::U32 base = ir.ShiftLeftLogical(id_in_group, ir.Imm32(1));
    const IR::U32 index =
        ir.IAdd(lane_id, ir.BitFieldExtract(ir.Imm32(offset0), base, ir.Imm32(2)));
    SetDst(inst.dst[0], ir.QuadShuffle(src, index));
}

void Translator::DS_READ(int bit_size, bool is_signed, bool is_pair, bool stride64,
                         const GcnInst& inst) {
    const IR::U32 addr{ir.GetVectorReg(IR::VectorReg(inst.src[0].code))};
    IR::VectorReg dst_reg{inst.dst[0].code};
    if (is_pair) {
        // Pair loads are either 32 or 64-bit
        const u32 adj = (bit_size == 32 ? 4 : 8) * (stride64 ? 64 : 1);
        const IR::U32 addr0 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset0 * adj)));
        const IR::Value data0 = ir.LoadShared(bit_size, is_signed, addr0);
        if (bit_size == 32) {
            ir.SetVectorReg(dst_reg++, IR::U32{data0});
        } else {
            ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(data0, 0)});
            ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(data0, 1)});
        }
        const IR::U32 addr1 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset1 * adj)));
        const IR::Value data1 = ir.LoadShared(bit_size, is_signed, addr1);
        if (bit_size == 32) {
            ir.SetVectorReg(dst_reg++, IR::U32{data1});
        } else {
            ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(data1, 0)});
            ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(data1, 1)});
        }
    } else if (bit_size == 64) {
        const IR::U32 addr0 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset0)));
        const IR::Value data = ir.LoadShared(bit_size, is_signed, addr0);
        ir.SetVectorReg(dst_reg, IR::U32{ir.CompositeExtract(data, 0)});
        ir.SetVectorReg(dst_reg + 1, IR::U32{ir.CompositeExtract(data, 1)});
    } else {
        const IR::U32 addr0 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset0)));
        const IR::U32 data = IR::U32{ir.LoadShared(bit_size, is_signed, addr0)};
        ir.SetVectorReg(dst_reg, data);
    }
}

void Translator::DS_WRITE(int bit_size, bool is_signed, bool is_pair, bool stride64,
                          const GcnInst& inst) {
    const IR::U32 addr{ir.GetVectorReg(IR::VectorReg(inst.src[0].code))};
    const IR::VectorReg data0{inst.src[1].code};
    const IR::VectorReg data1{inst.src[2].code};
    if (is_pair) {
        const u32 adj = (bit_size == 32 ? 4 : 8) * (stride64 ? 64 : 1);
        const IR::U32 addr0 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset0 * adj)));
        if (bit_size == 32) {
            ir.WriteShared(32, ir.GetVectorReg(data0), addr0);
        } else {
            ir.WriteShared(
                64, ir.CompositeConstruct(ir.GetVectorReg(data0), ir.GetVectorReg(data0 + 1)),
                addr0);
        }
        const IR::U32 addr1 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset1 * adj)));
        if (bit_size == 32) {
            ir.WriteShared(32, ir.GetVectorReg(data1), addr1);
        } else {
            ir.WriteShared(
                64, ir.CompositeConstruct(ir.GetVectorReg(data1), ir.GetVectorReg(data1 + 1)),
                addr1);
        }
    } else if (bit_size == 64) {
        const IR::U32 addr0 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset0)));
        const IR::Value data =
            ir.CompositeConstruct(ir.GetVectorReg(data0), ir.GetVectorReg(data0 + 1));
        ir.WriteShared(bit_size, data, addr0);
    } else {
        const IR::U32 addr0 = ir.IAdd(addr, ir.Imm32(u32(inst.control.ds.offset0)));
        ir.WriteShared(bit_size, ir.GetVectorReg(data0), addr0);
    }
}

void Translator::DS_ADD_U32(const GcnInst& inst, bool rtn) {
    const IR::U32 addr{GetSrc(inst.src[0])};
    const IR::U32 data{GetSrc(inst.src[1])};
    const IR::U32 offset = ir.Imm32(u32(inst.control.ds.offset0));
    const IR::U32 addr_offset = ir.IAdd(addr, offset);
    const IR::Value original_val = ir.SharedAtomicIAdd(addr_offset, data);
    if (rtn) {
        SetDst(inst.dst[0], IR::U32{original_val});
    }
}

void Translator::DS_MIN_U32(const GcnInst& inst, bool is_signed, bool rtn) {
    const IR::U32 addr{GetSrc(inst.src[0])};
    const IR::U32 data{GetSrc(inst.src[1])};
    const IR::U32 offset = ir.Imm32(u32(inst.control.ds.offset0));
    const IR::U32 addr_offset = ir.IAdd(addr, offset);
    const IR::Value original_val = ir.SharedAtomicIMin(addr_offset, data, is_signed);
    if (rtn) {
        SetDst(inst.dst[0], IR::U32{original_val});
    }
}

void Translator::DS_MAX_U32(const GcnInst& inst, bool is_signed, bool rtn) {
    const IR::U32 addr{GetSrc(inst.src[0])};
    const IR::U32 data{GetSrc(inst.src[1])};
    const IR::U32 offset = ir.Imm32(u32(inst.control.ds.offset0));
    const IR::U32 addr_offset = ir.IAdd(addr, offset);
    const IR::Value original_val = ir.SharedAtomicIMax(addr_offset, data, is_signed);
    if (rtn) {
        SetDst(inst.dst[0], IR::U32{original_val});
    }
}

void Translator::S_BARRIER() {
    ir.Barrier();
}

void Translator::V_READFIRSTLANE_B32(const GcnInst& inst) {
    const IR::ScalarReg dst{inst.dst[0].code};
    const IR::U32 value{GetSrc(inst.src[0])};

    if (info.stage != Stage::Compute) {
        SetDst(inst.dst[0], value);
    } else {
        SetDst(inst.dst[0], ir.ReadFirstLane(value));
    }
}

void Translator::V_READLANE_B32(const GcnInst& inst) {
    const IR::ScalarReg dst{inst.dst[0].code};
    const IR::U32 value{GetSrc(inst.src[0])};
    const IR::U32 lane{GetSrc(inst.src[1])};
    ir.SetScalarReg(dst, ir.ReadLane(value, lane));
}

void Translator::V_WRITELANE_B32(const GcnInst& inst) {
    const IR::VectorReg dst{inst.dst[0].code};
    const IR::U32 value{GetSrc(inst.src[0])};
    const IR::U32 lane{GetSrc(inst.src[1])};
    const IR::U32 old_value{GetSrc(inst.dst[0])};
    ir.SetVectorReg(dst, ir.WriteLane(old_value, value, lane));
}

void Translator::DS_APPEND(const GcnInst& inst) {
    const u32 inst_offset = inst.control.ds.offset0;
    const IR::U32 gds_offset = ir.IAdd(ir.GetM0(), ir.Imm32(inst_offset));
    const IR::U32 prev = ir.DataAppend(gds_offset);
    SetDst(inst.dst[0], prev);
}

void Translator::DS_CONSUME(const GcnInst& inst) {
    const u32 inst_offset = inst.control.ds.offset0;
    const IR::U32 gds_offset = ir.IAdd(ir.GetM0(), ir.Imm32(inst_offset));
    const IR::U32 prev = ir.DataConsume(gds_offset);
    SetDst(inst.dst[0], prev);
}

} // namespace Shader::Gcn
