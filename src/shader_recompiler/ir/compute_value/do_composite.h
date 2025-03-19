// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/compute_value/compute.h"

namespace Shader::IR::ComputeValue {

void DoCompositeConstructU32x2(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1);
void DoCompositeConstructU32x3(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1, const ImmValueList& arg2);
void DoCompositeConstructU32x4(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1, const ImmValueList& arg2, const ImmValueList& arg3);
void DoCompositeConstructU32x2x2(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1);
void DoCompositeExtractU32x2(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeExtractU32x3(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeExtractU32x4(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeInsertU32x2(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeInsertU32x3(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeInsertU32x4(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeShuffleU32x2(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1);
void DoCompositeShuffleU32x3(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1, const ImmValueList& idx2);
void DoCompositeShuffleU32x4(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1, const ImmValueList& idx2, const ImmValueList& idx3);

void DoCompositeConstructF16x2(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1);
void DoCompositeConstructF16x3(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1, const ImmValueList& arg2);
void DoCompositeConstructF16x4(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1, const ImmValueList& arg2, const ImmValueList& arg3);
void DoCompositeExtractF16x2(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeExtractF16x3(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeExtractF16x4(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeInsertF16x2(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeInsertF16x3(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeInsertF16x4(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeShuffleF16x2(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1);
void DoCompositeShuffleF16x3(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1, const ImmValueList& idx2);
void DoCompositeShuffleF16x4(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1, const ImmValueList& idx2, const ImmValueList& idx3);

void DoCompositeConstructF32x2(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1);
void DoCompositeConstructF32x3(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1, const ImmValueList& arg2);
void DoCompositeConstructF32x4(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1, const ImmValueList& arg2, const ImmValueList& arg3);
void DoCompositeConstructF32x2x2(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1);
void DoCompositeExtractF32x2(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeExtractF32x3(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeExtractF32x4(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeInsertF32x2(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeInsertF32x3(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeInsertF32x4(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeShuffleF32x2(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1);
void DoCompositeShuffleF32x3(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1, const ImmValueList& idx2);
void DoCompositeShuffleF32x4(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1, const ImmValueList& idx2, const ImmValueList& idx3);

void DoCompositeConstructF64x2(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1);
void DoCompositeConstructF64x3(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1, const ImmValueList& arg2);
void DoCompositeConstructF64x4(ImmValueList& inst_values, const ImmValueList& arg0, const ImmValueList& arg1, const ImmValueList& arg2, const ImmValueList& arg3);
void DoCompositeExtractF64x2(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeExtractF64x3(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeExtractF64x4(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& idx);
void DoCompositeInsertF64x2(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeInsertF64x3(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeInsertF64x4(ImmValueList& inst_values, const ImmValueList& vec, const ImmValueList& val, const ImmValueList& idx);
void DoCompositeShuffleF64x2(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1);
void DoCompositeShuffleF64x3(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1, const ImmValueList& idx2);
void DoCompositeShuffleF64x4(ImmValueList& inst_values, const ImmValueList& vec0, const ImmValueList& vec1, const ImmValueList& idx0, const ImmValueList& idx1, const ImmValueList& idx2, const ImmValueList& idx3);

} // namespace Shader::IR::ComputeValue
