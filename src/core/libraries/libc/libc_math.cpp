// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include "common/assert.h"
#include "core/libraries/libc/libc_math.h"

namespace Libraries::LibC {

float PS4_SYSV_ABI ps4_atan2f(float y, float x) {
    return atan2f(y, x);
}

float PS4_SYSV_ABI ps4_acosf(float num) {
    return acosf(num);
}

float PS4_SYSV_ABI ps4_tanf(float num) {
    return tanf(num);
}

float PS4_SYSV_ABI ps4_asinf(float num) {
    return asinf(num);
}

double PS4_SYSV_ABI ps4_pow(double base, double exponent) {
    return pow(base, exponent);
}

float PS4_SYSV_ABI ps4_powf(float x, float y) {
    return powf(x, y);
}

float PS4_SYSV_ABI ps4_roundf(float arg) {
    return roundf(arg);
}

double PS4_SYSV_ABI ps4__Sin(double x) {
    return sin(x);
}

float PS4_SYSV_ABI ps4__Fsin(float arg, unsigned int m, int n) {
    ASSERT(n == 0);
    if (m != 0) {
        return cosf(arg);
    } else {
        return sinf(arg);
    }
}

double PS4_SYSV_ABI ps4_exp2(double arg) {
    return exp2(arg);
}

} // namespace Libraries::LibC
