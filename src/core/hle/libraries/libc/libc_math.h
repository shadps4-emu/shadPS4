#pragma once

#include "common/types.h"

namespace Core::Libraries::LibC {
float PS4_SYSV_ABI atan2f(float y, float x);
float PS4_SYSV_ABI acosf(float num);
float PS4_SYSV_ABI tanf(float num);
float PS4_SYSV_ABI asinf(float num);
double PS4_SYSV_ABI pow(double base, double exponent);
double PS4_SYSV_ABI _Sin(double x);
float PS4_SYSV_ABI _Fsin(float arg);
double PS4_SYSV_ABI exp2(double arg);
}  // namespace Core::Libraries::LibC