// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/compute_value/compute.h"

namespace Shader::IR::ComputeValue {

void DoFPAbs32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPAbs64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPAdd32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoFPAdd64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoFPSub32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoFPFma32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
               const ImmValueList& args2);
void DoFPFma64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
               const ImmValueList& args2);
void DoFPMax32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
               const ImmValueList& args_legacy);
void DoFPMax64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoFPMin32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
               const ImmValueList& args_legacy);
void DoFPMin64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoFPMinTri32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1, 
                  const ImmValueList& args2);
void DoFPMaxTri32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
                  const ImmValueList& args2);
void DoFPMedTri32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1,
                  const ImmValueList& args2);
void DoFPMul32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoFPMul64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoFPDiv32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoFPDiv64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoFPNeg32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPNeg64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPRecip32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPRecip64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPRecipSqrt32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPRecipSqrt64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPSqrt(ImmValueList& inst_values, const ImmValueList& args);
void DoFPSin(ImmValueList& inst_values, const ImmValueList& args);
void DoFPExp2(ImmValueList& inst_values, const ImmValueList& args);
void DoFPLdexp(ImmValueList& inst_values, const ImmValueList& args, const ImmValueList& exponents);
void DoFPCos(ImmValueList& inst_values, const ImmValueList& args);
void DoFPLog2(ImmValueList& inst_values, const ImmValueList& args);
void DoFPSaturate32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPSaturate64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPClamp32(ImmValueList& inst_values, const ImmValueList& args, const ImmValueList& mins,
                 const ImmValueList& maxs);
void DoFPClamp64(ImmValueList& inst_values, const ImmValueList& args, const ImmValueList& mins,
                 const ImmValueList& maxs);
void DoFPRoundEven32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPRoundEven64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPFloor32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPFloor64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPCeil32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPCeil64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPTrunc32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPTrunc64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPFract32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPFract64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPFrexpSig32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPFrexpSig64(ImmValueList& inst_values, const ImmValueList& args);
void DoFPFrexpExp32(ImmValueList& inst_values, const ImmValueList& args);
void DoFPFrexpExp64(ImmValueList& inst_values, const ImmValueList& args);

} // namespace Shader::IR::ComputeValue