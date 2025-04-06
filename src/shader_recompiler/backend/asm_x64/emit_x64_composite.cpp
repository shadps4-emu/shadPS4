// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_utils.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;

namespace {

template <u32 N>
static const Operand& GetSuffleOperand(const Operands& comp1, const Operands& comp2, u32 index) {
    if (index < N) {
        return comp1[index];
    } else {
        return comp2[index - N];
    }
}
}

void EmitCompositeConstructU32x2(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
}

void EmitCompositeConstructU32x3(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
    MovGP(ctx, dest[2], src3[0]);
}

void EmitCompositeConstructU32x4(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3, const Operands& src4) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
    MovGP(ctx, dest[2], src3[0]);
    MovGP(ctx, dest[3], src4[0]);
}

void EmitCompositeConstructU32x2x2(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
    MovGP(ctx, dest[2], src1[1]);
    MovGP(ctx, dest[3], src2[1]);
}

void EmitCompositeExtractU32x2(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovGP(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractU32x3(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovGP(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractU32x4(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovGP(ctx, dest[0], composite[index]);
}

void EmitCompositeInsertU32x2(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    if (index == 0) {
        MovGP(ctx, dest[0], object[0]);
        MovGP(ctx, dest[1], composite[1]);
    } else {
        MovGP(ctx, dest[0], composite[0]);
        MovGP(ctx, dest[1], object[0]);
    }
}

void EmitCompositeInsertU32x3(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    for (u32 i = 0; i < 3; ++i) {
        if (i == index) {
            MovGP(ctx, dest[i], object[0]);
        } else {
            MovGP(ctx, dest[i], composite[i]);
        }
    }
}

void EmitCompositeInsertU32x4(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    for (u32 i = 0; i < 3; ++i) {
        if (i == index) {
            MovGP(ctx, dest[i], object[0]);
        } else {
            MovGP(ctx, dest[i], composite[i]);
        }
    }
}

void EmitCompositeShuffleU32x2(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2) {
    MovGP(ctx, dest[0], GetSuffleOperand<2>(composite1, composite2, idx1));
    MovGP(ctx, dest[1], GetSuffleOperand<2>(composite1, composite2, idx2));
}

void EmitCompositeShuffleU32x3(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3) {
    MovGP(ctx, dest[0], GetSuffleOperand<3>(composite1, composite2, idx1));
    MovGP(ctx, dest[1], GetSuffleOperand<3>(composite1, composite2, idx2));
    MovGP(ctx, dest[2], GetSuffleOperand<3>(composite1, composite2, idx3));
}

void EmitCompositeShuffleU32x4(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3, u32 idx4) {
    MovGP(ctx, dest[0], GetSuffleOperand<4>(composite1, composite2, idx1));
    MovGP(ctx, dest[1], GetSuffleOperand<4>(composite1, composite2, idx2));
    MovGP(ctx, dest[2], GetSuffleOperand<4>(composite1, composite2, idx3));
    MovGP(ctx, dest[3], GetSuffleOperand<4>(composite1, composite2, idx4));
}

void EmitCompositeConstructF16x2(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
}

void EmitCompositeConstructF16x3(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
    MovGP(ctx, dest[2], src3[0]);
}

void EmitCompositeConstructF16x4(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3, const Operands& src4) {
    MovGP(ctx, dest[0], src1[0]);
    MovGP(ctx, dest[1], src2[0]);
    MovGP(ctx, dest[2], src3[0]);
    MovGP(ctx, dest[3], src4[0]);
}

void EmitCompositeExtractF16x2(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovGP(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractF16x3(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovGP(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractF16x4(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovGP(ctx, dest[0], composite[index]);
}

void EmitCompositeInsertF16x2(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    if (index == 0) {
        MovGP(ctx, dest[0], object[0]);
        MovGP(ctx, dest[1], composite[1]);
    } else {
        MovGP(ctx, dest[0], composite[0]);
        MovGP(ctx, dest[1], object[0]);
    }
}

void EmitCompositeInsertF16x3(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    for (u32 i = 0; i < 3; ++i) {
        if (i == index) {
            MovGP(ctx, dest[i], object[0]);
        } else {
            MovGP(ctx, dest[i], composite[i]);
        }
    }
}

void EmitCompositeInsertF16x4(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    for (u32 i = 0; i < 4; ++i) {
        if (i == index) {
            MovGP(ctx, dest[i], object[0]);
        } else {
            MovGP(ctx, dest[i], composite[i]);
        }
    }
}

void EmitCompositeShuffleF16x2(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2) {
    MovGP(ctx, dest[0], GetSuffleOperand<2>(composite1, composite2, idx1));
    MovGP(ctx, dest[1], GetSuffleOperand<2>(composite1, composite2, idx2));
}

void EmitCompositeShuffleF16x3(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3) {
    MovGP(ctx, dest[0], GetSuffleOperand<3>(composite1, composite2, idx1));
    MovGP(ctx, dest[1], GetSuffleOperand<3>(composite1, composite2, idx2));
    MovGP(ctx, dest[2], GetSuffleOperand<3>(composite1, composite2, idx3));
}

void EmitCompositeShuffleF16x4(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3, u32 idx4) {
    MovGP(ctx, dest[0], GetSuffleOperand<4>(composite1, composite2, idx1));
    MovGP(ctx, dest[1], GetSuffleOperand<4>(composite1, composite2, idx2));
    MovGP(ctx, dest[2], GetSuffleOperand<4>(composite1, composite2, idx3));
    MovGP(ctx, dest[3], GetSuffleOperand<4>(composite1, composite2, idx4));
}

void EmitCompositeConstructF32x2(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2) {
    MovFloat(ctx, dest[0], src1[0]);
    MovFloat(ctx, dest[1], src2[0]);
}

void EmitCompositeConstructF32x3(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3) {
    MovFloat(ctx, dest[0], src1[0]);
    MovFloat(ctx, dest[1], src2[0]);
    MovFloat(ctx, dest[2], src3[0]);
}

void EmitCompositeConstructF32x4(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3, const Operands& src4) {
    MovFloat(ctx, dest[0], src1[0]);
    MovFloat(ctx, dest[1], src2[0]);
    MovFloat(ctx, dest[2], src3[0]);
    MovFloat(ctx, dest[3], src4[0]);
}

void EmitCompositeConstructF32x2x2(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2) {
    MovFloat(ctx, dest[0], src1[0]);
    MovFloat(ctx, dest[1], src2[0]);
    MovFloat(ctx, dest[2], src1[1]);
    MovFloat(ctx, dest[3], src2[1]);
}

void EmitCompositeExtractF32x2(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovFloat(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractF32x3(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovFloat(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractF32x4(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovFloat(ctx, dest[0], composite[index]);
}

void EmitCompositeInsertF32x2(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    if (index == 0) {
        MovFloat(ctx, dest[0], object[0]);
        MovFloat(ctx, dest[1], composite[1]);
    } else {
        MovFloat(ctx, dest[0], composite[0]);
        MovFloat(ctx, dest[1], object[0]);
    }
}

void EmitCompositeInsertF32x3(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    for (u32 i = 0; i < 3; ++i) {
        if (i == index) {
            MovFloat(ctx, dest[i], object[0]);
        } else {
            MovFloat(ctx, dest[i], composite[i]);
        }
    }
}

void EmitCompositeInsertF32x4(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    for (u32 i = 0; i < 4; ++i) {
        if (i == index) {
            MovFloat(ctx, dest[i], object[0]);
        } else {
            MovFloat(ctx, dest[i], composite[i]);
        }
    }
}

void EmitCompositeShuffleF32x2(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2) {
    MovFloat(ctx, dest[0], GetSuffleOperand<2>(composite1, composite2, idx1));
    MovFloat(ctx, dest[1], GetSuffleOperand<2>(composite1, composite2, idx2));
}

void EmitCompositeShuffleF32x3(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3) {
    MovFloat(ctx, dest[0], GetSuffleOperand<3>(composite1, composite2, idx1));
    MovFloat(ctx, dest[1], GetSuffleOperand<3>(composite1, composite2, idx2));
    MovFloat(ctx, dest[2], GetSuffleOperand<3>(composite1, composite2, idx3));
}

void EmitCompositeShuffleF32x4(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3, u32 idx4) {
    MovFloat(ctx, dest[0], GetSuffleOperand<4>(composite1, composite2, idx1));
    MovFloat(ctx, dest[1], GetSuffleOperand<4>(composite1, composite2, idx2));
    MovFloat(ctx, dest[2], GetSuffleOperand<4>(composite1, composite2, idx3));
    MovFloat(ctx, dest[3], GetSuffleOperand<4>(composite1, composite2, idx4));
}

void EmitCompositeConstructF64x2(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2) {
    MovDouble(ctx, dest[0], src1[0]);
    MovDouble(ctx, dest[1], src2[0]);
}

void EmitCompositeConstructF64x3(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3) {
    MovDouble(ctx, dest[0], src1[0]);
    MovDouble(ctx, dest[1], src2[0]);
    MovDouble(ctx, dest[2], src3[0]);
}

void EmitCompositeConstructF64x4(EmitContext& ctx, const Operands& dest, const Operands& src1, const Operands& src2, const Operands& src3, const Operands& src4) {
    MovDouble(ctx, dest[0], src1[0]);
    MovDouble(ctx, dest[1], src2[0]);
    MovDouble(ctx, dest[2], src3[0]);
    MovDouble(ctx, dest[3], src4[0]);
}

void EmitCompositeExtractF64x2(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovDouble(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractF64x3(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovDouble(ctx, dest[0], composite[index]);
}

void EmitCompositeExtractF64x4(EmitContext& ctx, const Operands& dest, const Operands& composite, u32 index) {
    MovDouble(ctx, dest[0], composite[index]);
}

void EmitCompositeInsertF64x2(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    if (index == 0) {
        MovDouble(ctx, dest[0], object[0]);
        MovDouble(ctx, dest[1], composite[1]);
    } else {
        MovDouble(ctx, dest[0], composite[0]);
        MovDouble(ctx, dest[1], object[0]);
    }
}

void EmitCompositeInsertF64x3(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    for (u32 i = 0; i < 3; ++i) {
        if (i == index) {
            MovDouble(ctx, dest[i], object[0]);
        } else {
            MovDouble(ctx, dest[i], composite[i]);
        }
    }
}

void EmitCompositeInsertF64x4(EmitContext& ctx, const Operands& dest, const Operands& composite, const Operands& object, u32 index) {
    for (u32 i = 0; i < 4; ++i) {
        if (i == index) {
            MovDouble(ctx, dest[i], object[0]);
        } else {
            MovDouble(ctx, dest[i], composite[i]);
        }
    }
}

void EmitCompositeShuffleF64x2(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2) {
    MovDouble(ctx, dest[0], GetSuffleOperand<2>(composite1, composite2, idx1));
    MovDouble(ctx, dest[1], GetSuffleOperand<2>(composite1, composite2, idx2));
}

void EmitCompositeShuffleF64x3(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3) {
    MovDouble(ctx, dest[0], GetSuffleOperand<3>(composite1, composite2, idx1));
    MovDouble(ctx, dest[1], GetSuffleOperand<3>(composite1, composite2, idx2));
    MovDouble(ctx, dest[2], GetSuffleOperand<3>(composite1, composite2, idx3));
}

void EmitCompositeShuffleF64x4(EmitContext& ctx, const Operands& dest, const Operands& composite1, const Operands& composite2, u32 idx1, u32 idx2, u32 idx3, u32 idx4) {
    MovDouble(ctx, dest[0], GetSuffleOperand<4>(composite1, composite2, idx1));
    MovDouble(ctx, dest[1], GetSuffleOperand<4>(composite1, composite2, idx2));
    MovDouble(ctx, dest[2], GetSuffleOperand<4>(composite1, composite2, idx3));
    MovDouble(ctx, dest[3], GetSuffleOperand<4>(composite1, composite2, idx4));
}

} // namespace Shader::Backend::X64