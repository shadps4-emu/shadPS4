// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::LibC {

float PS4_SYSV_ABI ps4_atan2f(float y, float x);
float PS4_SYSV_ABI ps4_acosf(float num);
float PS4_SYSV_ABI ps4_tanf(float num);
float PS4_SYSV_ABI ps4_asinf(float num);
double PS4_SYSV_ABI ps4_pow(double base, double exponent);
double PS4_SYSV_ABI ps4__Sin(double x);
float PS4_SYSV_ABI ps4__Fsin(float arg, unsigned int, int);
double PS4_SYSV_ABI ps4_exp2(double arg);
float PS4_SYSV_ABI ps4_powf(float x, float y);
float PS4_SYSV_ABI ps4_roundf(float arg);

} // namespace Libraries::LibC
