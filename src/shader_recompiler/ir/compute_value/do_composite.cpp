// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/cartesian_invoke.h"
#include "shader_recompiler/ir/compute_value/do_composite.h"

namespace Shader::IR::ComputeValue {

static void CommonCompositeConstruct(ImmValueList& inst_values, const ImmValueList& arg0,
                                     const ImmValueList& arg1) {
    const auto op = [](const ImmValue& a, const ImmValue& b) { return ImmValue(a, b); };
    Common::CartesianInvoke(op, std::insert_iterator(inst_values, inst_values.begin()), arg0, arg1);
}

static void CommonCompositeConstruct(ImmValueList& inst_values, const ImmValueList& arg0,
                                     const ImmValueList& arg1, const ImmValueList& arg2) {
    const auto op = [](const ImmValue& a, const ImmValue& b, const ImmValue& c) {
        return ImmValue(a, b, c);
    };
    Common::CartesianInvoke(op, std::insert_iterator(inst_values, inst_values.begin()), arg0, arg1,
                            arg2);
}

static void CommonCompositeConstruct(ImmValueList& inst_values, const ImmValueList& arg0,
                                     const ImmValueList& arg1, const ImmValueList& arg2,
                                     const ImmValueList& arg3) {
    const auto op = [](const ImmValue& a, const ImmValue& b, const ImmValue& c, const ImmValue& d) {
        return ImmValue(a, b, c, d);
    };
    Common::CartesianInvoke(op, std::insert_iterator(inst_values, inst_values.begin()), arg0, arg1,
                            arg2, arg3);
}

void DoCompositeConstructU32x2(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1) {
    CommonCompositeConstruct(inst_values, arg0, arg1);
}

void DoCompositeConstructU32x3(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1, const ImmValueList& arg2) {
    CommonCompositeConstruct(inst_values, arg0, arg1, arg2);
}

void DoCompositeConstructU32x4(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1, const ImmValueList& arg2,
                               const ImmValueList& arg3) {
    CommonCompositeConstruct(inst_values, arg0, arg1, arg2, arg3);
}

void DoCompositeConstructU32x2x2(ImmValueList& inst_values, const ImmValueList& arg0,
                                 const ImmValueList& arg1) {
    Common::CartesianInvoke(ImmValue::CompositeFrom2x2,
                            std::insert_iterator(inst_values, inst_values.begin()), arg0, arg1);
}

void DoCompositeExtractU32x2(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeExtractU32x3(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeExtractU32x4(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeInsertU32x2(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeInsertU32x3(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeInsertU32x4(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeShuffleU32x2(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeShuffleU32x3(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1, const ImmValueList& idx2) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeShuffleU32x4(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1, const ImmValueList& idx2,
                             const ImmValueList& idx3) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeConstructF16x2(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1) {
    CommonCompositeConstruct(inst_values, arg0, arg1);
}

void DoCompositeConstructF16x3(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1, const ImmValueList& arg2) {
    CommonCompositeConstruct(inst_values, arg0, arg1, arg2);
}

void DoCompositeConstructF16x4(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1, const ImmValueList& arg2,
                               const ImmValueList& arg3) {
    CommonCompositeConstruct(inst_values, arg0, arg1, arg2, arg3);
}

void DoCompositeConstructF32x2x2(ImmValueList& inst_values, const ImmValueList& arg0,
                                 const ImmValueList& arg1) {
    Common::CartesianInvoke(ImmValue::CompositeFrom2x2,
                            std::insert_iterator(inst_values, inst_values.begin()), arg0, arg1);
}

void DoCompositeExtractF16x2(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeExtractF16x3(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeExtractF16x4(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeInsertF16x2(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeInsertF16x3(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeInsertF16x4(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeShuffleF16x2(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeShuffleF16x3(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1, const ImmValueList& idx2) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeShuffleF16x4(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1, const ImmValueList& idx2,
                             const ImmValueList& idx3) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeConstructF32x2(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1) {
    CommonCompositeConstruct(inst_values, arg0, arg1);
}

void DoCompositeConstructF32x3(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1, const ImmValueList& arg2) {
    CommonCompositeConstruct(inst_values, arg0, arg1, arg2);
}

void DoCompositeConstructF32x4(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1, const ImmValueList& arg2,
                               const ImmValueList& arg3) {
    CommonCompositeConstruct(inst_values, arg0, arg1, arg2, arg3);
}

void DoCompositeExtractF32x2(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeExtractF32x3(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeExtractF32x4(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeInsertF32x2(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeInsertF32x3(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeInsertF32x4(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeShuffleF32x2(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeShuffleF32x3(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1, const ImmValueList& idx2) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeShuffleF32x4(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1, const ImmValueList& idx2,
                             const ImmValueList& idx3) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeConstructF64x2(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1) {
    CommonCompositeConstruct(inst_values, arg0, arg1);
}

void DoCompositeConstructF64x3(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1, const ImmValueList& arg2) {
    CommonCompositeConstruct(inst_values, arg0, arg1, arg2);
}

void DoCompositeConstructF64x4(ImmValueList& inst_values, const ImmValueList& arg0,
                               const ImmValueList& arg1, const ImmValueList& arg2,
                               const ImmValueList& arg3) {
    CommonCompositeConstruct(inst_values, arg0, arg1, arg2, arg3);
}

void DoCompositeExtractF64x2(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeExtractF64x3(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeExtractF64x4(ImmValueList& inst_values, const ImmValueList& vec,
                             const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Extract,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, idx);
}

void DoCompositeInsertF64x2(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeInsertF64x3(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeInsertF64x4(ImmValueList& inst_values, const ImmValueList& vec,
                            const ImmValueList& val, const ImmValueList& idx) {
    Common::CartesianInvoke(ImmValue::Insert,
                            std::insert_iterator(inst_values, inst_values.begin()), vec, val, idx);
}

void DoCompositeShuffleF64x2(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeShuffleF64x3(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1, const ImmValueList& idx2) {
    UNREACHABLE_MSG("Unimplemented");
}

void DoCompositeShuffleF64x4(ImmValueList& inst_values, const ImmValueList& vec0,
                             const ImmValueList& vec1, const ImmValueList& idx0,
                             const ImmValueList& idx1, const ImmValueList& idx2,
                             const ImmValueList& idx3) {
    UNREACHABLE_MSG("Unimplemented");
}

} // namespace Shader::IR::ComputeValue
