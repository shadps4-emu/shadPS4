#include <cmath>
#include "core/hle/libraries/libc/libc_math.h"

namespace Core::Libraries::LibC {

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

double PS4_SYSV_ABI ps4__Sin(double x) {
    return sin(x);
}

float PS4_SYSV_ABI ps4__Fsin(float arg) {
    return sinf(arg);
}

double PS4_SYSV_ABI ps4_exp2(double arg) {
    return exp2(arg);
}

} // namespace Core::Libraries::LibC
