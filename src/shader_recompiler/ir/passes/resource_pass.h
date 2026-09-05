// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/small_vector.hpp>

#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/value.h"

namespace Shader {
enum class SharpFetchPostOp : u8;
}

namespace Shader::Optimization {

union PostOpData {
    u32 dw1_mask;
    IR::Value lod_prod{};
};

struct SharpReference {
    std::array<IR::Value, 8> dwords{};
    u32 num_dwords{};
    SharpFetchPostOp post_op{};
    PostOpData post_op_data;
};

struct ResourceDiscovery {
    IR::Inst* user{};
    std::array<SharpReference, 2> sharps;
};
using ResourceDiscoveryList = boost::container::small_vector<ResourceDiscovery, 32>;

inline bool IsBufferAtomic(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::BufferAtomicIAdd32:
    case IR::Opcode::BufferAtomicIAdd64:
    case IR::Opcode::BufferAtomicISub32:
    case IR::Opcode::BufferAtomicSMin32:
    case IR::Opcode::BufferAtomicSMin64:
    case IR::Opcode::BufferAtomicUMin32:
    case IR::Opcode::BufferAtomicUMin64:
    case IR::Opcode::BufferAtomicFMin32:
    case IR::Opcode::BufferAtomicSMax32:
    case IR::Opcode::BufferAtomicSMax64:
    case IR::Opcode::BufferAtomicUMax32:
    case IR::Opcode::BufferAtomicUMax64:
    case IR::Opcode::BufferAtomicFMax32:
    case IR::Opcode::BufferAtomicInc32:
    case IR::Opcode::BufferAtomicDec32:
    case IR::Opcode::BufferAtomicAnd32:
    case IR::Opcode::BufferAtomicOr32:
    case IR::Opcode::BufferAtomicXor32:
    case IR::Opcode::BufferAtomicSwap32:
    case IR::Opcode::BufferAtomicCmpSwap32:
    case IR::Opcode::BufferAtomicFCmpSwap32:
        return true;
    default:
        return false;
    }
}

inline bool IsBufferStore(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::StoreBufferU8:
    case IR::Opcode::StoreBufferU16:
    case IR::Opcode::StoreBufferU32:
    case IR::Opcode::StoreBufferU32x2:
    case IR::Opcode::StoreBufferU32x3:
    case IR::Opcode::StoreBufferU32x4:
    case IR::Opcode::StoreBufferU64:
    case IR::Opcode::StoreBufferF32:
    case IR::Opcode::StoreBufferF32x2:
    case IR::Opcode::StoreBufferF32x3:
    case IR::Opcode::StoreBufferF32x4:
    case IR::Opcode::StoreBufferFormatF32:
        return true;
    default:
        return IsBufferAtomic(inst);
    }
}

inline bool IsDataRingInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::DataAppend:
    case IR::Opcode::DataConsume:
        return true;
    case IR::Opcode::LoadSharedU16:
    case IR::Opcode::LoadSharedU32:
    case IR::Opcode::LoadSharedU64:
    case IR::Opcode::WriteSharedU16:
    case IR::Opcode::WriteSharedU32:
    case IR::Opcode::WriteSharedU64:
    case IR::Opcode::SharedAtomicIAdd32:
    case IR::Opcode::SharedAtomicIAdd64:
    case IR::Opcode::SharedAtomicUMin32:
    case IR::Opcode::SharedAtomicUMin64:
    case IR::Opcode::SharedAtomicSMin32:
    case IR::Opcode::SharedAtomicSMin64:
    case IR::Opcode::SharedAtomicUMax32:
    case IR::Opcode::SharedAtomicUMax64:
    case IR::Opcode::SharedAtomicSMax32:
    case IR::Opcode::SharedAtomicSMax64:
    case IR::Opcode::SharedAtomicAnd32:
    case IR::Opcode::SharedAtomicAnd64:
    case IR::Opcode::SharedAtomicOr32:
    case IR::Opcode::SharedAtomicOr64:
    case IR::Opcode::SharedAtomicXor32:
    case IR::Opcode::SharedAtomicXor64:
    case IR::Opcode::SharedAtomicISub32:
    case IR::Opcode::SharedAtomicISub64:
    case IR::Opcode::SharedAtomicInc32:
    case IR::Opcode::SharedAtomicInc64:
    case IR::Opcode::SharedAtomicDec32:
    case IR::Opcode::SharedAtomicDec64:
        return inst.Flags<bool>(); // is_gds
    default:
        return false;
    }
}

inline bool IsBufferInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadBufferU8:
    case IR::Opcode::LoadBufferU16:
    case IR::Opcode::LoadBufferU32:
    case IR::Opcode::LoadBufferU32x2:
    case IR::Opcode::LoadBufferU32x3:
    case IR::Opcode::LoadBufferU32x4:
    case IR::Opcode::LoadBufferU64:
    case IR::Opcode::LoadBufferF32:
    case IR::Opcode::LoadBufferF32x2:
    case IR::Opcode::LoadBufferF32x3:
    case IR::Opcode::LoadBufferF32x4:
    case IR::Opcode::LoadBufferFormatF32:
    case IR::Opcode::ReadConstBuffer:
        return true;
    default:
        return IsBufferStore(inst);
    }
}

inline bool IsImageAtomicInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::ImageAtomicIAdd32:
    case IR::Opcode::ImageAtomicSMin32:
    case IR::Opcode::ImageAtomicUMin32:
    case IR::Opcode::ImageAtomicSMax32:
    case IR::Opcode::ImageAtomicUMax32:
    case IR::Opcode::ImageAtomicFMax32:
    case IR::Opcode::ImageAtomicFMin32:
    case IR::Opcode::ImageAtomicInc32:
    case IR::Opcode::ImageAtomicDec32:
    case IR::Opcode::ImageAtomicAnd32:
    case IR::Opcode::ImageAtomicOr32:
    case IR::Opcode::ImageAtomicXor32:
    case IR::Opcode::ImageAtomicExchange32:
    case IR::Opcode::ImageAtomicCmpSwap32:
        return true;
    default:
        return false;
    }
}

inline bool IsImageInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::ImageRead:
    case IR::Opcode::ImageWrite:
    case IR::Opcode::ImageQueryDimensions:
    case IR::Opcode::ImageQueryLod:
    case IR::Opcode::ImageSampleRaw:
        return true;
    default:
        return IsImageAtomicInstruction(inst);
    }
}

} // namespace Shader::Optimization