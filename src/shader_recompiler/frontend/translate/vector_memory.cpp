// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::EmitVectorMemory(const GcnInst& inst) {
    switch (inst.opcode) {
        // MUBUF / MTBUF

        // Buffer load operations
    case Opcode::TBUFFER_LOAD_FORMAT_X:
        return BUFFER_LOAD(1, true, false, inst);
    case Opcode::TBUFFER_LOAD_FORMAT_XY:
        return BUFFER_LOAD(2, true, false, inst);
    case Opcode::TBUFFER_LOAD_FORMAT_XYZ:
        return BUFFER_LOAD(3, true, false, inst);
    case Opcode::TBUFFER_LOAD_FORMAT_XYZW:
        return BUFFER_LOAD(4, true, false, inst);

    case Opcode::BUFFER_LOAD_FORMAT_X:
        return BUFFER_LOAD(1, false, true, inst);
    case Opcode::BUFFER_LOAD_FORMAT_XY:
        return BUFFER_LOAD(2, false, true, inst);
    case Opcode::BUFFER_LOAD_FORMAT_XYZ:
        return BUFFER_LOAD(3, false, true, inst);
    case Opcode::BUFFER_LOAD_FORMAT_XYZW:
        return BUFFER_LOAD(4, false, true, inst);

    case Opcode::BUFFER_LOAD_UBYTE:
        return BUFFER_LOAD(1, false, false, inst, 8, false);
    case Opcode::BUFFER_LOAD_SBYTE:
        return BUFFER_LOAD(1, false, false, inst, 8, true);
    case Opcode::BUFFER_LOAD_USHORT:
        return BUFFER_LOAD(1, false, false, inst, 16, false);
    case Opcode::BUFFER_LOAD_SSHORT:
        return BUFFER_LOAD(1, false, false, inst, 16, true);

    case Opcode::BUFFER_LOAD_DWORD:
        return BUFFER_LOAD(1, false, false, inst);
    case Opcode::BUFFER_LOAD_DWORDX2:
        return BUFFER_LOAD(2, false, false, inst);
    case Opcode::BUFFER_LOAD_DWORDX3:
        return BUFFER_LOAD(3, false, false, inst);
    case Opcode::BUFFER_LOAD_DWORDX4:
        return BUFFER_LOAD(4, false, false, inst);

        // Buffer store operations
    case Opcode::BUFFER_STORE_FORMAT_X:
        return BUFFER_STORE(1, false, true, inst);
    case Opcode::BUFFER_STORE_FORMAT_XY:
        return BUFFER_STORE(2, false, true, inst);
    case Opcode::BUFFER_STORE_FORMAT_XYZ:
        return BUFFER_STORE(3, false, true, inst);
    case Opcode::BUFFER_STORE_FORMAT_XYZW:
        return BUFFER_STORE(4, false, true, inst);

    case Opcode::TBUFFER_STORE_FORMAT_X:
        return BUFFER_STORE(1, true, false, inst);
    case Opcode::TBUFFER_STORE_FORMAT_XY:
        return BUFFER_STORE(2, true, false, inst);
    case Opcode::TBUFFER_STORE_FORMAT_XYZ:
        return BUFFER_STORE(3, true, false, inst);
    case Opcode::TBUFFER_STORE_FORMAT_XYZW:
        return BUFFER_STORE(4, true, false, inst);

    case Opcode::BUFFER_STORE_BYTE:
        return BUFFER_STORE(1, false, false, inst, 8);
    case Opcode::BUFFER_STORE_SHORT:
        return BUFFER_STORE(1, false, false, inst, 16);

    case Opcode::BUFFER_STORE_DWORD:
        return BUFFER_STORE(1, false, false, inst);
    case Opcode::BUFFER_STORE_DWORDX2:
        return BUFFER_STORE(2, false, false, inst);
    case Opcode::BUFFER_STORE_DWORDX3:
        return BUFFER_STORE(3, false, false, inst);
    case Opcode::BUFFER_STORE_DWORDX4:
        return BUFFER_STORE(4, false, false, inst);

        // Buffer atomic operations
    case Opcode::BUFFER_ATOMIC_ADD:
        return BUFFER_ATOMIC(AtomicOp::Add, inst);
    case Opcode::BUFFER_ATOMIC_SWAP:
        return BUFFER_ATOMIC(AtomicOp::Swap, inst);
    case Opcode::BUFFER_ATOMIC_CMPSWAP:
        return BUFFER_ATOMIC(AtomicOp::CmpSwap, inst);
    case Opcode::BUFFER_ATOMIC_SMIN:
        return BUFFER_ATOMIC(AtomicOp::Smin, inst);
    case Opcode::BUFFER_ATOMIC_SMIN_X2:
        return BUFFER_ATOMIC<IR::U64>(AtomicOp::Smin, inst);
    case Opcode::BUFFER_ATOMIC_UMIN:
        return BUFFER_ATOMIC(AtomicOp::Umin, inst);
    case Opcode::BUFFER_ATOMIC_UMIN_X2:
        return BUFFER_ATOMIC<IR::U64>(AtomicOp::Umin, inst);
    case Opcode::BUFFER_ATOMIC_SMAX:
        return BUFFER_ATOMIC(AtomicOp::Smax, inst);
    case Opcode::BUFFER_ATOMIC_SMAX_X2:
        return BUFFER_ATOMIC<IR::U64>(AtomicOp::Smax, inst);
    case Opcode::BUFFER_ATOMIC_UMAX:
        return BUFFER_ATOMIC(AtomicOp::Umax, inst);
    case Opcode::BUFFER_ATOMIC_UMAX_X2:
        return BUFFER_ATOMIC<IR::U64>(AtomicOp::Umax, inst);
    case Opcode::BUFFER_ATOMIC_AND:
        return BUFFER_ATOMIC(AtomicOp::And, inst);
    case Opcode::BUFFER_ATOMIC_OR:
        return BUFFER_ATOMIC(AtomicOp::Or, inst);
    case Opcode::BUFFER_ATOMIC_XOR:
        return BUFFER_ATOMIC(AtomicOp::Xor, inst);
    case Opcode::BUFFER_ATOMIC_INC:
        return BUFFER_ATOMIC(AtomicOp::Inc, inst);
    case Opcode::BUFFER_ATOMIC_DEC:
        return BUFFER_ATOMIC(AtomicOp::Dec, inst);
    case Opcode::BUFFER_ATOMIC_FMIN:
        return BUFFER_ATOMIC(AtomicOp::Fmin, inst);
    case Opcode::BUFFER_ATOMIC_FMAX:
        return BUFFER_ATOMIC(AtomicOp::Fmax, inst);

        // MIMG
        // Image load operations
    case Opcode::IMAGE_LOAD:
        return IMAGE_LOAD(false, inst);
    case Opcode::IMAGE_LOAD_MIP:
        return IMAGE_LOAD(true, inst);

        // Buffer store operations
    case Opcode::IMAGE_STORE:
        return IMAGE_STORE(false, inst);
    case Opcode::IMAGE_STORE_MIP:
        return IMAGE_STORE(true, inst);

        // Image misc operations
    case Opcode::IMAGE_GET_RESINFO:
        return IMAGE_GET_RESINFO(inst);

        // Image atomic operations
    case Opcode::IMAGE_ATOMIC_SWAP:
        return IMAGE_ATOMIC(AtomicOp::Swap, inst);
    case Opcode::IMAGE_ATOMIC_ADD:
        return IMAGE_ATOMIC(AtomicOp::Add, inst);
    case Opcode::IMAGE_ATOMIC_SMIN:
        return IMAGE_ATOMIC(AtomicOp::Smin, inst);
    case Opcode::IMAGE_ATOMIC_UMIN:
        return IMAGE_ATOMIC(AtomicOp::Umin, inst);
    case Opcode::IMAGE_ATOMIC_FMIN:
        return IMAGE_ATOMIC(AtomicOp::Fmin, inst);
    case Opcode::IMAGE_ATOMIC_SMAX:
        return IMAGE_ATOMIC(AtomicOp::Smax, inst);
    case Opcode::IMAGE_ATOMIC_FMAX:
        return IMAGE_ATOMIC(AtomicOp::Fmax, inst);
    case Opcode::IMAGE_ATOMIC_UMAX:
        return IMAGE_ATOMIC(AtomicOp::Umax, inst);
    case Opcode::IMAGE_ATOMIC_AND:
        return IMAGE_ATOMIC(AtomicOp::And, inst);
    case Opcode::IMAGE_ATOMIC_OR:
        return IMAGE_ATOMIC(AtomicOp::Or, inst);
    case Opcode::IMAGE_ATOMIC_XOR:
        return IMAGE_ATOMIC(AtomicOp::Xor, inst);
    case Opcode::IMAGE_ATOMIC_INC:
        return IMAGE_ATOMIC(AtomicOp::Inc, inst);
    case Opcode::IMAGE_ATOMIC_DEC:
        return IMAGE_ATOMIC(AtomicOp::Dec, inst);

    case Opcode::IMAGE_SAMPLE:
    case Opcode::IMAGE_SAMPLE_D:
    case Opcode::IMAGE_SAMPLE_L:
    case Opcode::IMAGE_SAMPLE_B:
    case Opcode::IMAGE_SAMPLE_LZ:
    case Opcode::IMAGE_SAMPLE_C:
    case Opcode::IMAGE_SAMPLE_C_LZ:
    case Opcode::IMAGE_SAMPLE_O:
    case Opcode::IMAGE_SAMPLE_L_O:
    case Opcode::IMAGE_SAMPLE_B_O:
    case Opcode::IMAGE_SAMPLE_LZ_O:
    case Opcode::IMAGE_SAMPLE_C_O:
    case Opcode::IMAGE_SAMPLE_C_LZ_O:
    case Opcode::IMAGE_SAMPLE_CD:
        return IMAGE_SAMPLE(inst);

        // Image gather operations
    case Opcode::IMAGE_GATHER4:
    case Opcode::IMAGE_GATHER4_L:
    case Opcode::IMAGE_GATHER4_LZ:
    case Opcode::IMAGE_GATHER4_C:
    case Opcode::IMAGE_GATHER4_O:
    case Opcode::IMAGE_GATHER4_C_O:
    case Opcode::IMAGE_GATHER4_C_LZ:
    case Opcode::IMAGE_GATHER4_LZ_O:
    case Opcode::IMAGE_GATHER4_C_LZ_O:
        return IMAGE_GATHER(inst);

        // Image misc operations
    case Opcode::IMAGE_GET_LOD:
        return IMAGE_GET_LOD(inst);

    default:
        LogMissingOpcode(inst);
    }
}

void Translator::BUFFER_LOAD(u32 num_dwords, bool is_inst_typed, bool is_buffer_typed,
                             const GcnInst& inst, u32 scalar_width, bool is_signed) {
    const auto& mubuf = inst.control.mubuf;
    const bool is_ring = mubuf.glc && mubuf.slc;
    const IR::VectorReg vaddr{inst.src[0].code};
    const IR::ScalarReg sharp{inst.src[2].code * 4};
    const IR::Value soffset{GetSrc(inst.src[3])};
    const bool has_soffset = !soffset.IsImmediate() || soffset.U32() != 0;
    if (info.stage != Stage::Geometry) {
        ASSERT_MSG(!has_soffset || !mubuf.offen,
                   "Having both scalar and vector offsets is not supported");
    }

    const IR::Value address = [&] -> IR::Value {
        if (is_ring) {
            return ir.CompositeConstruct(ir.GetVectorReg(vaddr), soffset);
        }
        if (mubuf.idxen && mubuf.offen) {
            return ir.CompositeConstruct(ir.GetVectorReg(vaddr), ir.GetVectorReg(vaddr + 1));
        }
        if (mubuf.idxen && has_soffset) {
            return ir.CompositeConstruct(ir.GetVectorReg(vaddr), soffset);
        }
        if (mubuf.idxen || mubuf.offen) {
            return ir.GetVectorReg(vaddr);
        }
        if (has_soffset) {
            return soffset;
        }
        return {};
    }();

    IR::BufferInstInfo buffer_info{};
    buffer_info.index_enable.Assign(mubuf.idxen);
    buffer_info.offset_enable.Assign(mubuf.offen || has_soffset);
    buffer_info.inst_offset.Assign(mubuf.offset);
    buffer_info.globally_coherent.Assign(mubuf.glc);
    buffer_info.system_coherent.Assign(mubuf.slc);
    buffer_info.typed.Assign(is_inst_typed || is_buffer_typed);
    if (is_inst_typed) {
        const auto& mtbuf = inst.control.mtbuf;
        buffer_info.inst_data_fmt.Assign(static_cast<AmdGpu::DataFormat>(mtbuf.dfmt));
        buffer_info.inst_num_fmt.Assign(static_cast<AmdGpu::NumberFormat>(mtbuf.nfmt));
    } else {
        buffer_info.inst_data_fmt.Assign(AmdGpu::DataFormat::FormatInvalid);
    }

    const IR::Value handle =
        ir.CompositeConstruct(ir.GetScalarReg(sharp), ir.GetScalarReg(sharp + 1),
                              ir.GetScalarReg(sharp + 2), ir.GetScalarReg(sharp + 3));
    const IR::VectorReg dst_reg{inst.src[1].code};
    if (buffer_info.typed) {
        const IR::Value value = ir.LoadBufferFormat(handle, address, buffer_info);
        for (u32 i = 0; i < num_dwords; i++) {
            ir.SetVectorReg(dst_reg + i, IR::F32{ir.CompositeExtract(value, i)});
        }
    } else {
        IR::Value value;
        switch (scalar_width) {
        case 8: {
            IR::U8 byte_val = ir.LoadBufferU8(handle, address, buffer_info);
            value = is_signed ? ir.SConvert(32, byte_val) : ir.UConvert(32, byte_val);
            break;
        }
        case 16: {
            IR::U16 short_val = ir.LoadBufferU16(handle, address, buffer_info);
            value = is_signed ? ir.SConvert(32, short_val) : ir.UConvert(32, short_val);
            break;
        }
        case 32:
            value = ir.LoadBufferU32(num_dwords, handle, address, buffer_info);
            break;

        default:
            UNREACHABLE();
        }

        if (num_dwords == 1) {
            ir.SetVectorReg(dst_reg, IR::U32{value});
            return;
        }
        for (u32 i = 0; i < num_dwords; i++) {
            ir.SetVectorReg(dst_reg + i, IR::U32{ir.CompositeExtract(value, i)});
        }
    }
}

void Translator::BUFFER_STORE(u32 num_dwords, bool is_inst_typed, bool is_buffer_typed,
                              const GcnInst& inst, u32 scalar_width) {
    const auto& mubuf = inst.control.mubuf;
    const bool is_ring = mubuf.glc && mubuf.slc;
    const IR::VectorReg vaddr{inst.src[0].code};
    const IR::ScalarReg sharp{inst.src[2].code * 4};
    const IR::Value soffset{GetSrc(inst.src[3])};

    if (info.stage != Stage::Export && info.stage != Stage::Hull && info.stage != Stage::Geometry) {
        ASSERT_MSG(soffset.IsImmediate() && soffset.U32() == 0,
                   "Non immediate offset not supported");
    }

    IR::Value address = [&] -> IR::Value {
        if (is_ring) {
            return ir.CompositeConstruct(ir.GetVectorReg(vaddr), soffset);
        }
        if (mubuf.idxen && mubuf.offen) {
            return ir.CompositeConstruct(ir.GetVectorReg(vaddr), ir.GetVectorReg(vaddr + 1));
        }
        if (mubuf.idxen || mubuf.offen) {
            return ir.GetVectorReg(vaddr);
        }
        return {};
    }();

    IR::BufferInstInfo buffer_info{};
    buffer_info.index_enable.Assign(mubuf.idxen);
    buffer_info.offset_enable.Assign(mubuf.offen);
    buffer_info.inst_offset.Assign(mubuf.offset);
    buffer_info.globally_coherent.Assign(mubuf.glc);
    buffer_info.system_coherent.Assign(mubuf.slc);
    buffer_info.typed.Assign(is_inst_typed || is_buffer_typed);
    if (is_inst_typed) {
        const auto& mtbuf = inst.control.mtbuf;
        buffer_info.inst_data_fmt.Assign(static_cast<AmdGpu::DataFormat>(mtbuf.dfmt));
        buffer_info.inst_num_fmt.Assign(static_cast<AmdGpu::NumberFormat>(mtbuf.nfmt));
    } else {
        buffer_info.inst_data_fmt.Assign(AmdGpu::DataFormat::FormatInvalid);
    }

    const IR::Value handle =
        ir.CompositeConstruct(ir.GetScalarReg(sharp), ir.GetScalarReg(sharp + 1),
                              ir.GetScalarReg(sharp + 2), ir.GetScalarReg(sharp + 3));
    const IR::VectorReg src_reg{inst.src[1].code};

    boost::container::static_vector<IR::Value, 4> comps;
    for (u32 i = 0; i < num_dwords; i++) {
        const auto src_reg_i = src_reg + i;
        if (buffer_info.typed) {
            comps.push_back(ir.GetVectorReg<IR::F32>(src_reg_i));
        } else {
            comps.push_back(ir.GetVectorReg<IR::U32>(src_reg_i));
        }
    }
    if (buffer_info.typed) {
        for (u32 i = num_dwords; i < 4; i++) {
            comps.push_back(ir.Imm32(0.f));
        }
        ir.StoreBufferFormat(handle, address, ir.CompositeConstruct(comps), buffer_info);
    } else {
        IR::Value value = num_dwords == 1 ? comps[0] : ir.CompositeConstruct(comps);
        if (scalar_width != 32) {
            value = ir.UConvert(scalar_width, IR::U32{value});
        }
        switch (scalar_width) {
        case 8:
            ir.StoreBufferU8(handle, address, IR::U8{value}, buffer_info);
            break;
        case 16:
            ir.StoreBufferU16(handle, address, IR::U16{value}, buffer_info);
            break;
        case 32:
            ir.StoreBufferU32(num_dwords, handle, address, value, buffer_info);
            break;
        default:
            UNREACHABLE();
        }
    }
}

template <typename T>
void Translator::BUFFER_ATOMIC(AtomicOp op, const GcnInst& inst) {
    const auto& mubuf = inst.control.mubuf;
    const IR::VectorReg vaddr{inst.src[0].code};
    const IR::VectorReg vdata{inst.src[1].code};
    const IR::ScalarReg srsrc{inst.src[2].code * 4};
    const IR::Value address = [&] -> IR::Value {
        if (mubuf.idxen && mubuf.offen) {
            return ir.CompositeConstruct(ir.GetVectorReg(vaddr), ir.GetVectorReg(vaddr + 1));
        }
        if (mubuf.idxen || mubuf.offen) {
            return ir.GetVectorReg(vaddr);
        }
        return {};
    }();
    const IR::U32 soffset{GetSrc(inst.src[3])};
    ASSERT_MSG(soffset.IsImmediate() && soffset.U32() == 0, "Non immediate offset not supported");

    IR::BufferInstInfo buffer_info{};
    buffer_info.index_enable.Assign(mubuf.idxen);
    buffer_info.offset_enable.Assign(mubuf.offen);
    buffer_info.inst_offset.Assign(mubuf.offset);
    buffer_info.globally_coherent.Assign(mubuf.glc);
    buffer_info.system_coherent.Assign(mubuf.slc);

    IR::Value vdata_val = [&] {
        if constexpr (std::is_same_v<T, IR::U32>) {
            return ir.GetVectorReg<Shader::IR::U32>(vdata);
        } else if constexpr (std::is_same_v<T, IR::U64>) {
            return ir.PackUint2x32(
                ir.CompositeConstruct(ir.GetVectorReg<Shader::IR::U32>(vdata),
                                      ir.GetVectorReg<Shader::IR::U32>(vdata + 1)));
        } else {
            static_assert(false, "buffer_atomic: type not supported");
        }
    }();
    const IR::Value handle =
        ir.CompositeConstruct(ir.GetScalarReg(srsrc), ir.GetScalarReg(srsrc + 1),
                              ir.GetScalarReg(srsrc + 2), ir.GetScalarReg(srsrc + 3));

    const IR::Value original_val = [&] {
        switch (op) {
        case AtomicOp::Swap:
            return ir.BufferAtomicSwap(handle, address, vdata_val, buffer_info);
        case AtomicOp::CmpSwap: {
            const IR::Value cmp_val = ir.GetVectorReg(vdata + 1);
            return ir.BufferAtomicCmpSwap(handle, address, vdata_val, cmp_val, buffer_info);
        }
        case AtomicOp::Add:
            return ir.BufferAtomicIAdd(handle, address, vdata_val, buffer_info);
        case AtomicOp::Smin:
            return ir.BufferAtomicIMin(handle, address, vdata_val, true, buffer_info);
        case AtomicOp::Umin:
            return ir.BufferAtomicIMin(handle, address, vdata_val, false, buffer_info);
        case AtomicOp::Smax:
            return ir.BufferAtomicIMax(handle, address, vdata_val, true, buffer_info);
        case AtomicOp::Umax:
            return ir.BufferAtomicIMax(handle, address, vdata_val, false, buffer_info);
        case AtomicOp::And:
            return ir.BufferAtomicAnd(handle, address, vdata_val, buffer_info);
        case AtomicOp::Or:
            return ir.BufferAtomicOr(handle, address, vdata_val, buffer_info);
        case AtomicOp::Xor:
            return ir.BufferAtomicXor(handle, address, vdata_val, buffer_info);
        case AtomicOp::Inc:
            return ir.BufferAtomicInc(handle, address, buffer_info);
        case AtomicOp::Dec:
            return ir.BufferAtomicDec(handle, address, buffer_info);
        case AtomicOp::Fmin:
            return ir.BufferAtomicFMin(handle, address, vdata_val, buffer_info);
        case AtomicOp::Fmax:
            return ir.BufferAtomicFMax(handle, address, vdata_val, buffer_info);
        default:
            UNREACHABLE();
        }
    }();

    if (mubuf.glc) {
        ir.SetVectorReg(vdata, IR::U32{original_val});
    }
}

// Image Memory
// MIMG

void Translator::IMAGE_LOAD(bool has_mip, const GcnInst& inst) {
    const auto& mimg = inst.control.mimg;
    IR::VectorReg addr_reg{inst.src[0].code};
    IR::VectorReg dest_reg{inst.dst[0].code};
    const IR::ScalarReg tsharp_reg{inst.src[2].code * 4};

    const IR::Value handle = ir.GetScalarReg(tsharp_reg);
    const IR::Value body =
        ir.CompositeConstruct(ir.GetVectorReg(addr_reg), ir.GetVectorReg(addr_reg + 1),
                              ir.GetVectorReg(addr_reg + 2), ir.GetVectorReg(addr_reg + 3));

    IR::TextureInstInfo info{};
    info.has_lod.Assign(has_mip);
    info.is_array.Assign(mimg.da);
    info.is_r128.Assign(mimg.r128);
    const IR::Value texel = ir.ImageRead(handle, body, {}, {}, info);

    for (u32 i = 0; i < 4; i++) {
        if (((mimg.dmask >> i) & 1) == 0) {
            continue;
        }
        IR::F32 value = IR::F32{ir.CompositeExtract(texel, i)};
        ir.SetVectorReg(dest_reg++, value);
    }
}

void Translator::IMAGE_STORE(bool has_mip, const GcnInst& inst) {
    const auto& mimg = inst.control.mimg;
    IR::VectorReg addr_reg{inst.src[0].code};
    IR::VectorReg data_reg{inst.dst[0].code};
    const IR::ScalarReg tsharp_reg{inst.src[2].code * 4};

    const IR::Value handle = ir.GetScalarReg(tsharp_reg);
    const IR::Value body =
        ir.CompositeConstruct(ir.GetVectorReg(addr_reg), ir.GetVectorReg(addr_reg + 1),
                              ir.GetVectorReg(addr_reg + 2), ir.GetVectorReg(addr_reg + 3));

    IR::TextureInstInfo info{};
    info.has_lod.Assign(has_mip);
    info.is_array.Assign(mimg.da);

    boost::container::static_vector<IR::F32, 4> comps;
    for (u32 i = 0; i < 4; i++) {
        if (((mimg.dmask >> i) & 1) == 0) {
            comps.push_back(ir.Imm32(0.f));
            continue;
        }
        comps.push_back(ir.GetVectorReg<IR::F32>(data_reg++));
    }
    const IR::Value value = ir.CompositeConstruct(comps[0], comps[1], comps[2], comps[3]);
    ir.ImageWrite(handle, body, {}, {}, value, info);
}

void Translator::IMAGE_GET_RESINFO(const GcnInst& inst) {
    const auto& mimg = inst.control.mimg;
    IR::VectorReg dst_reg{inst.dst[0].code};
    const IR::ScalarReg tsharp_reg{inst.src[2].code * 4};
    const auto flags = ImageResFlags(inst.control.mimg.dmask);
    const bool has_mips = flags.test(ImageResComponent::MipCount);
    const IR::U32 lod = ir.GetVectorReg(IR::VectorReg(inst.src[0].code));
    const IR::Value tsharp = ir.GetScalarReg(tsharp_reg);

    IR::TextureInstInfo info{};
    info.is_array.Assign(mimg.da);
    info.is_r128.Assign(mimg.r128);

    const IR::Value size = ir.ImageQueryDimension(tsharp, lod, ir.Imm1(has_mips), info);

    if (flags.test(ImageResComponent::Width)) {
        ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(size, 0)});
    }
    if (flags.test(ImageResComponent::Height)) {
        ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(size, 1)});
    }
    if (flags.test(ImageResComponent::Depth)) {
        ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(size, 2)});
    }
    if (has_mips) {
        ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(size, 3)});
    }
}

void Translator::IMAGE_ATOMIC(AtomicOp op, const GcnInst& inst) {
    const auto& mimg = inst.control.mimg;
    IR::VectorReg val_reg{inst.dst[0].code};
    IR::VectorReg addr_reg{inst.src[0].code};
    const IR::ScalarReg tsharp_reg{inst.src[2].code * 4};

    IR::TextureInstInfo info{};
    info.is_array.Assign(mimg.da);
    info.is_r128.Assign(mimg.r128);

    const IR::Value value = ir.GetVectorReg(val_reg);
    const IR::Value handle = ir.GetScalarReg(tsharp_reg);
    const IR::Value body =
        ir.CompositeConstruct(ir.GetVectorReg(addr_reg), ir.GetVectorReg(addr_reg + 1),
                              ir.GetVectorReg(addr_reg + 2), ir.GetVectorReg(addr_reg + 3));
    const IR::Value prev = [&] {
        switch (op) {
        case AtomicOp::Swap:
            return ir.ImageAtomicExchange(handle, body, value, {});
        case AtomicOp::Add:
            return ir.ImageAtomicIAdd(handle, body, value, info);
        case AtomicOp::Smin:
            return ir.ImageAtomicIMin(handle, body, value, true, info);
        case AtomicOp::Umin:
            return ir.ImageAtomicUMin(handle, body, value, info);
        case AtomicOp::Smax:
            return ir.ImageAtomicIMax(handle, body, value, true, info);
        case AtomicOp::Umax:
            return ir.ImageAtomicUMax(handle, body, value, info);
        case AtomicOp::Fmax:
            return ir.ImageAtomicFMax(handle, body, value, info);
        case AtomicOp::Fmin:
            return ir.ImageAtomicFMin(handle, body, value, info);
        case AtomicOp::And:
            return ir.ImageAtomicAnd(handle, body, value, info);
        case AtomicOp::Or:
            return ir.ImageAtomicOr(handle, body, value, info);
        case AtomicOp::Xor:
            return ir.ImageAtomicXor(handle, body, value, info);
        case AtomicOp::Inc:
            return ir.ImageAtomicInc(handle, body, value, info);
        case AtomicOp::Dec:
            return ir.ImageAtomicDec(handle, body, value, info);
        default:
            UNREACHABLE();
        }
    }();
    if (mimg.glc) {
        ir.SetVectorReg(val_reg, IR::U32{prev});
    }
}

IR::Value EmitImageSample(IR::IREmitter& ir, const GcnInst& inst, const IR::ScalarReg tsharp_reg,
                          const IR::ScalarReg sampler_reg, const IR::VectorReg addr_reg,
                          bool gather, u32 pc) {
    const auto& mimg = inst.control.mimg;
    const auto flags = MimgModifierFlags(mimg.mod);

    IR::TextureInstInfo info{};
    info.is_depth.Assign(flags.test(MimgModifier::Pcf));
    info.has_bias.Assign(flags.test(MimgModifier::LodBias));
    info.has_lod_clamp.Assign(flags.test(MimgModifier::LodClamp));
    info.force_level0.Assign(flags.test(MimgModifier::Level0));
    info.has_offset.Assign(flags.test(MimgModifier::Offset));
    info.has_lod.Assign(flags.any(MimgModifier::Lod));
    info.is_array.Assign(mimg.da);
    info.is_unnormalized.Assign(mimg.unrm);
    info.is_r128.Assign(mimg.r128);
    info.pc.Assign(pc);

    if (gather) {
        info.gather_comp.Assign(std::bit_width(mimg.dmask) - 1);
        info.is_gather.Assign(true);
    } else {
        info.has_derivatives.Assign(flags.test(MimgModifier::Derivative));
    }

    // Load first dword of T# and the full S#. We will use them as the handle that will guide
    // resource tracking pass where to read the sharps. This will later also get patched to the
    // backend texture binding index.
    const IR::Value image_handle = ir.GetScalarReg(tsharp_reg);
    const IR::Value sampler_handle =
        ir.CompositeConstruct(ir.GetScalarReg(sampler_reg), ir.GetScalarReg(sampler_reg + 1),
                              ir.GetScalarReg(sampler_reg + 2), ir.GetScalarReg(sampler_reg + 3));

    // Determine how many address registers need to be passed.
    // The image type is unknown, so add all 4 possible base registers and resolve later.
    int num_addr_regs = 4;
    if (info.has_offset) {
        ++num_addr_regs;
    }
    if (info.has_bias) {
        ++num_addr_regs;
    }
    if (info.is_depth) {
        ++num_addr_regs;
    }
    if (info.has_derivatives) {
        // The image type is unknown, so add all 6 possible derivative registers and resolve later.
        num_addr_regs += 6;
    }

    // Fetch all the address registers to pass in the IR instruction. There can be up to 13
    // registers.
    const auto get_addr_reg = [&](int index) -> IR::F32 {
        if (index >= num_addr_regs) {
            return ir.Imm32(0.f);
        }
        return ir.GetVectorReg<IR::F32>(addr_reg + index);
    };
    const IR::Value address1 =
        ir.CompositeConstruct(get_addr_reg(0), get_addr_reg(1), get_addr_reg(2), get_addr_reg(3));
    const IR::Value address2 =
        ir.CompositeConstruct(get_addr_reg(4), get_addr_reg(5), get_addr_reg(6), get_addr_reg(7));
    const IR::Value address3 =
        ir.CompositeConstruct(get_addr_reg(8), get_addr_reg(9), get_addr_reg(10), get_addr_reg(11));
    const IR::Value address4 = get_addr_reg(12);

    // Issue the placeholder IR instruction.
    IR::Value texel = ir.ImageSampleRaw(image_handle, sampler_handle, address1, address2, address3,
                                        address4, info);
    if (info.is_depth && !gather) {
        // For non-gather depth sampling, only return a single value.
        texel = ir.CompositeExtract(texel, 0);
    }
    return texel;
}

void Translator::IMAGE_SAMPLE(const GcnInst& inst) {
    const auto& mimg = inst.control.mimg;
    IR::VectorReg addr_reg{inst.src[0].code};
    IR::VectorReg dest_reg{inst.dst[0].code};
    const IR::ScalarReg tsharp_reg{inst.src[2].code * 4};
    const IR::ScalarReg sampler_reg{inst.src[3].code * 4};
    const auto flags = MimgModifierFlags(mimg.mod);

    const IR::Value texel = EmitImageSample(ir, inst, tsharp_reg, sampler_reg, addr_reg, false, pc);
    for (u32 i = 0; i < 4; i++) {
        if (((mimg.dmask >> i) & 1) == 0) {
            continue;
        }
        IR::F32 value;
        if (flags.test(MimgModifier::Pcf)) {
            value = i < 3 ? IR::F32{texel} : ir.Imm32(1.0f);
        } else {
            value = IR::F32{ir.CompositeExtract(texel, i)};
        }
        ir.SetVectorReg(dest_reg++, value);
    }
}

void Translator::IMAGE_GATHER(const GcnInst& inst) {
    const auto& mimg = inst.control.mimg;
    IR::VectorReg addr_reg{inst.src[0].code};
    IR::VectorReg dest_reg{inst.dst[0].code};
    const IR::ScalarReg tsharp_reg{inst.src[2].code * 4};
    const IR::ScalarReg sampler_reg{inst.src[3].code * 4};
    const auto flags = MimgModifierFlags(mimg.mod);

    // For gather4 instructions dmask selects which component to read and must have
    // only one bit set to 1
    ASSERT_MSG(std::popcount(mimg.dmask) == 1, "Unexpected bits in gather dmask");
    // should be always 1st (R) component for depth
    ASSERT(!flags.test(MimgModifier::Pcf) || mimg.dmask & 1);

    const IR::Value texel = EmitImageSample(ir, inst, tsharp_reg, sampler_reg, addr_reg, true, pc);
    for (u32 i = 0; i < 4; i++) {
        const IR::F32 value = IR::F32{ir.CompositeExtract(texel, i)};
        ir.SetVectorReg(dest_reg++, value);
    }
}

void Translator::IMAGE_GET_LOD(const GcnInst& inst) {
    const auto& mimg = inst.control.mimg;
    IR::VectorReg dst_reg{inst.dst[0].code};
    IR::VectorReg addr_reg{inst.src[0].code};
    const IR::ScalarReg tsharp_reg{inst.src[2].code * 4};

    IR::TextureInstInfo info{};
    info.is_array.Assign(mimg.da);
    info.is_r128.Assign(mimg.r128);

    const IR::Value handle = ir.GetScalarReg(tsharp_reg);
    const IR::Value body = ir.CompositeConstruct(
        ir.GetVectorReg<IR::F32>(addr_reg), ir.GetVectorReg<IR::F32>(addr_reg + 1),
        ir.GetVectorReg<IR::F32>(addr_reg + 2), ir.GetVectorReg<IR::F32>(addr_reg + 3));
    const IR::Value lod = ir.ImageQueryLod(handle, body, info);
    ir.SetVectorReg(dst_reg++, IR::F32{ir.CompositeExtract(lod, 0)});
    ir.SetVectorReg(dst_reg++, IR::F32{ir.CompositeExtract(lod, 1)});
}

} // namespace Shader::Gcn
