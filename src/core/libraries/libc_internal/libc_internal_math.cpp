// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal_mspace.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI internal_abs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI internal_acos(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acos(x);
}

float PS4_SYSV_ABI internal_acosf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acosf(x);
}

float PS4_SYSV_ABI internal_acosh(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acosh(x);
}

float PS4_SYSV_ABI internal_acoshf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acoshf(x);
}

float PS4_SYSV_ABI internal_acoshl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acoshl(x);
}

float PS4_SYSV_ABI internal_acosl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acosl(x);
}

double PS4_SYSV_ABI internal_asin(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asin(x);
}

float PS4_SYSV_ABI internal_asinf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinf(x);
}

float PS4_SYSV_ABI internal_asinh(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinh(x);
}

float PS4_SYSV_ABI internal_asinhf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinhf(x);
}

float PS4_SYSV_ABI internal_asinhl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinhl(x);
}

float PS4_SYSV_ABI internal_asinl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinl(x);
}

double PS4_SYSV_ABI internal_atan(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::atan(x);
}

double PS4_SYSV_ABI internal_atan2(double y, double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::atan2(y, x);
}

s32 PS4_SYSV_ABI internal_atan2f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atan2l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atanh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atanhf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atanhl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atanl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ceil() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ceilf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ceill() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI internal_cos(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::cos(x);
}

float PS4_SYSV_ABI internal_cosf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::cosf(x);
}

s32 PS4_SYSV_ABI internal_cosh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_coshf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_coshl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_cosl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI internal_exp(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::exp(x);
}

double PS4_SYSV_ABI internal_exp2(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::exp2(x);
}

float PS4_SYSV_ABI internal_exp2f(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::exp2f(x);
}

float PS4_SYSV_ABI internal_exp2l(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::exp2l(x);
}

float PS4_SYSV_ABI internal_expf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::expf(x);
}

s32 PS4_SYSV_ABI internal_expl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_expm1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_expm1f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_expm1l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fabs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fabsf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fabsl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_floor() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_floorf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_floorl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fmax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fmaxf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fmaxl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fmin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fminf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fminl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fmod() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fmodf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fmodl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_frexp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_frexpf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_frexpl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ilogb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ilogbf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ilogbl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_imaxabs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_imaxdiv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isinf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_islower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isnan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isnanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_log() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_log10() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

float PS4_SYSV_ABI internal_log10f(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::log10f(x);
}

s32 PS4_SYSV_ABI internal_log10l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_log1p() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_log1pf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_log1pl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_log2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_log2f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_log2l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_logb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_logbf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_logbl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_logf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_logl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lround() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lroundf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lroundl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nanl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI internal_pow(double x, double y) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::pow(x, y);
}

float PS4_SYSV_ABI internal_powf(float x, float y) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::powf(x, y);
}

s32 PS4_SYSV_ABI internal_powl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_remainder() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI internal_sin(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sin(x);
}

void PS4_SYSV_ABI internal_sincos(double x, double* sinp, double* cosp) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    *sinp = std::sin(x);
    *cosp = std::cos(x);
}

void PS4_SYSV_ABI internal_sincosf(double x, double* sinp, double* cosp) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    *sinp = std::sinf(x);
    *cosp = std::cosf(x);
}

float PS4_SYSV_ABI internal_sinf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinf(x);
}

float PS4_SYSV_ABI internal_sinh(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinh(x);
}

float PS4_SYSV_ABI internal_sinhf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinhf(x);
}

float PS4_SYSV_ABI internal_sinhl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinhl(x);
}

float PS4_SYSV_ABI internal_sinl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinl(x);
}

s32 PS4_SYSV_ABI internal_sqrt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sqrtf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sqrtl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_srand() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tanh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tanhf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tanhl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tanl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

float PS4_SYSV_ABI internal__Fsin(float arg, unsigned int m, int n) {
    ASSERT(n == 0);
    if (m != 0) {
        return cosf(arg);
    } else {
        return sinf(arg);
    }
}

double PS4_SYSV_ABI internal__Sin(double x) {
    return sin(x);
}

void RegisterlibSceLibcInternalMath(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("Ye20uNnlglA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_abs);
    LIB_FUNCTION("JBcgYuW8lPU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_acos);
    LIB_FUNCTION("QI-x0SL8jhw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_acosf);
    LIB_FUNCTION("Fk7-KFKZi-8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_acosh);
    LIB_FUNCTION("XJp2C-b0tRU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_acoshf);
    LIB_FUNCTION("u14Y1HFh0uY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_acoshl);
    LIB_FUNCTION("iH4YAIRcecA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_acosl);
    LIB_FUNCTION("7Ly52zaL44Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_asin);
    LIB_FUNCTION("GZWjF-YIFFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_asinf);
    LIB_FUNCTION("2eQpqTjJ5Y4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_asinh);
    LIB_FUNCTION("yPPtp1RMihw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_asinhf);
    LIB_FUNCTION("iCl-Z-g-uuA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_asinhl);
    LIB_FUNCTION("Nx-F5v0-qU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_asinl);
    LIB_FUNCTION("OXmauLdQ8kY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_atan);
    LIB_FUNCTION("HUbZmOnT-Dg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atan2);
    LIB_FUNCTION("EH-x713A99c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atan2f);
    LIB_FUNCTION("9VeY8wiqf8M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atan2l);
    LIB_FUNCTION("weDug8QD-lE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atanf);
    LIB_FUNCTION("YjbpxXpi6Zk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atanh);
    LIB_FUNCTION("cPGyc5FGjy0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atanhf);
    LIB_FUNCTION("a3BNqojL4LM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atanhl);
    LIB_FUNCTION("KvOHPTz595Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atanl);
    LIB_FUNCTION("gacfOmO8hNs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_ceil);
    LIB_FUNCTION("GAUuLKGhsCw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ceilf);
    LIB_FUNCTION("aJKn6X+40Z8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ceill);
    LIB_FUNCTION("2WE3BTYVwKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_cos);
    LIB_FUNCTION("-P6FNMzk2Kc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_cosf);
    LIB_FUNCTION("m7iLTaO9RMs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_cosh);
    LIB_FUNCTION("RCQAffkEh9A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_coshf);
    LIB_FUNCTION("XK2R46yx0jc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_coshl);
    LIB_FUNCTION("x8dc5Y8zFgc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_cosl);
    LIB_FUNCTION("NVadfnzQhHQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_exp);
    LIB_FUNCTION("dnaeGXbjP6E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_exp2);
    LIB_FUNCTION("wuAQt-j+p4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_exp2f);
    LIB_FUNCTION("9O1Xdko-wSo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_exp2l);
    LIB_FUNCTION("8zsu04XNsZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_expf);
    LIB_FUNCTION("qMp2fTDCyMo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_expl);
    LIB_FUNCTION("gqKfOiJaCOo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_expm1);
    LIB_FUNCTION("3EgxfDRefdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_expm1f);
    LIB_FUNCTION("jVS263HH1b0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_expm1l);
    LIB_FUNCTION("388LcMWHRCA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_fabs);
    LIB_FUNCTION("fmT2cjPoWBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fabsf);
    LIB_FUNCTION("w-AryX51ObA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fabsl);
    LIB_FUNCTION("mpcTgMzhUY8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_floor);
    LIB_FUNCTION("mKhVDmYciWA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_floorf);
    LIB_FUNCTION("06QaR1Cpn-k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_floorl);
    LIB_FUNCTION("fiOgmWkP+Xc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_fmax);
    LIB_FUNCTION("Lyx2DzUL7Lc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fmaxf);
    LIB_FUNCTION("0H5TVprQSkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fmaxl);
    LIB_FUNCTION("iU0z6SdUNbI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_fmin);
    LIB_FUNCTION("uVRcM2yFdP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fminf);
    LIB_FUNCTION("DQ7K6s8euWY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fminl);
    LIB_FUNCTION("pKwslsMUmSk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_fmod);
    LIB_FUNCTION("88Vv-AzHVj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fmodf);
    LIB_FUNCTION("A1R5T0xOyn8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fmodl);
    LIB_FUNCTION("kA-TdiOCsaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_frexp);
    LIB_FUNCTION("aaDMGGkXFxo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_frexpf);
    LIB_FUNCTION("YZk9sHO0yNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_frexpl);
    LIB_FUNCTION("h6pVBKjcLiU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ilogb);
    LIB_FUNCTION("0dQrYWd7g94", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ilogbf);
    LIB_FUNCTION("wXs12eD3uvA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ilogbl);
    LIB_FUNCTION("UgZ7Rhk60cQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_imaxabs);
    LIB_FUNCTION("V0X-mrfdM9E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_imaxdiv);
    LIB_FUNCTION("2q5PPh7HsKE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isinf);
    LIB_FUNCTION("KqYTqtSfGos", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_islower);
    LIB_FUNCTION("20qj+7O69XY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isnan);
    LIB_FUNCTION("3pF7bUSIH8o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isnanf);
    LIB_FUNCTION("GcFKlTJEMkI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isupper);
    LIB_FUNCTION("rtV7-jWC6Yg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_log);
    LIB_FUNCTION("WuMbPBKN1TU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log10);
    LIB_FUNCTION("lhpd6Wk6ccs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log10f);
    LIB_FUNCTION("CT4aR0tBgkQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log10l);
    LIB_FUNCTION("VfsML+n9cDM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log1p);
    LIB_FUNCTION("MFe91s8apQk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log1pf);
    LIB_FUNCTION("77qd0ksTwdI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log1pl);
    LIB_FUNCTION("Y5DhuDKGlnQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_log2);
    LIB_FUNCTION("hsi9drzHR2k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log2f);
    LIB_FUNCTION("CfOrGjBj-RY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_log2l);
    LIB_FUNCTION("owKuegZU4ew", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_logb);
    LIB_FUNCTION("RWqyr1OKuw4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_logbf);
    LIB_FUNCTION("txJTOe0Db6M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_logbl);
    LIB_FUNCTION("RQXLbdT2lc4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_logf);
    LIB_FUNCTION("EiHf-aLDImI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_logl);
    LIB_FUNCTION("J3XuGS-cC0Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lround);
    LIB_FUNCTION("C6gWCWJKM+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lroundf);
    LIB_FUNCTION("4ITASgL50uc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lroundl);
    LIB_FUNCTION("zck+6bVj5pA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_nan);
    LIB_FUNCTION("DZU+K1wozGI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_nanf);
    LIB_FUNCTION("ZUvemFIkkhQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_nanl);
    LIB_FUNCTION("9LCjpWyQ5Zc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_pow);
    LIB_FUNCTION("1D0H2KNjshE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_powf);
    LIB_FUNCTION("95V3PF0kUEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_powl);
    LIB_FUNCTION("pv2etu4pocs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_remainder);
    LIB_FUNCTION("H8ya2H00jbI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_sin);
    LIB_FUNCTION("jMB7EFyu30Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sincos);
    LIB_FUNCTION("pztV4AF18iI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sincosf);
    LIB_FUNCTION("Q4rRL34CEeE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_sinf);
    LIB_FUNCTION("ZjtRqSMJwdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_sinh);
    LIB_FUNCTION("1t1-JoZ0sZQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sinhf);
    LIB_FUNCTION("lYdqBvDgeHU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sinhl);
    LIB_FUNCTION("vxgqrJxDPHo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_sinl);
    LIB_FUNCTION("MXRNWnosNlM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_sqrt);
    LIB_FUNCTION("Q+xU11-h0xQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sqrtf);
    LIB_FUNCTION("RIkUZRadZgc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sqrtl);
    LIB_FUNCTION("VPbJwTCgME0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_srand);
    LIB_FUNCTION("T7uyNqP7vQA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_tan);
    LIB_FUNCTION("ZE6RNL+eLbk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_tanf);
    LIB_FUNCTION("JM4EBvWT9rc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_tanh);
    LIB_FUNCTION("SAd0Z3wKwLA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_tanhf);
    LIB_FUNCTION("JCmHsYVc2eo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_tanhl);
    LIB_FUNCTION("QL+3q43NfEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_tanl);
    LIB_FUNCTION("ZtjspkJQ+vw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FSin);
    LIB_FUNCTION("cCXjU72Z0Ow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Sin);
}

} // namespace Libraries::LibcInternal
