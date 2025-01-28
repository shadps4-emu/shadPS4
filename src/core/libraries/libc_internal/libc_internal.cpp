// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include <csetjmp>
#include <cstdarg>
#include <cstdio>
#include <string>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libc_internal.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI sceLibcHeapGetTraceInfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __absvdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __absvsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __absvti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __adddf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __addsf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __addvdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __addvsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __addvti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ashldi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ashlti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ashrdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ashrti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_compare_exchange() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_compare_exchange_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_compare_exchange_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_compare_exchange_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_compare_exchange_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_compare_exchange_n() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_exchange() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_exchange_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_exchange_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_exchange_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_exchange_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_exchange_n() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_add_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_add_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_add_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_add_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_and_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_and_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_and_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_and_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_or_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_or_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_or_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_or_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_sub_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_sub_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_sub_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_sub_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_xor_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_xor_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_xor_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_fetch_xor_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_is_lock_free() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_load() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_load_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_load_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_load_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_load_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_load_n() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_store() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_store_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_store_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_store_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_store_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __atomic_store_n() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cleanup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __clzdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __clzsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __clzti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cmpdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cmpti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ctzdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ctzsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ctzti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_allocate_dependent_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_allocate_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_atexit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_bad_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_bad_typeid() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_begin_catch() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_call_unexpected() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_current_exception_type() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_current_primary_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_decrement_exception_refcount() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_demangle() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_end_catch() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_finalize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_free_dependent_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_free_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_get_exception_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_get_globals() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_get_globals_fast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_guard_abort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_guard_acquire() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_guard_release() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_increment_exception_refcount() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_pure_virtual() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_rethrow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_rethrow_primary_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __cxa_throw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divdc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divdf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divmoddi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divmodsi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divsc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divsf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __divxc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __dynamic_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __eqdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __eqsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __extendsfdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fe_dfl_env() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fedisableexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __feenableexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fflush() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ffsdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ffsti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixdfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixdfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixdfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixsfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixsfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixsfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixunsdfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixunsdfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixunsdfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixunssfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixunssfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixunssfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixunsxfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixunsxfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixunsxfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixxfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fixxfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatdidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatdisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatdixf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatsidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatsisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floattidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floattisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floattixf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatundidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatundisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatundixf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatunsidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatunsisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatuntidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatuntisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __floatuntixf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fpclassifyd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fpclassifyf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __fpclassifyl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __gedf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __gesf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __gtdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __gtsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __gxx_personality_v0() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __inet_addr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __inet_aton() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __inet_ntoa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __inet_ntoa_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isfinite() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isfinitef() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isfinitel() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isinf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isinff() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isinfl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isnan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isnanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isnanl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isnormal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isnormalf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isnormall() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __isthreaded() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __kernel_cos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __kernel_cosdf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __kernel_rem_pio2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __kernel_sin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __kernel_sindf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ledf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __lesf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __longjmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __lshrdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __lshrti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ltdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ltsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mb_cur_max() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mb_sb_limit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __moddi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __modsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __modti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __muldc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __muldf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __muldi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mulodi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mulosi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __muloti4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mulsc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mulsf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __multi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mulvdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mulvsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mulvti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __mulxc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __nedf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __negdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __negdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __negsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __negti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __negvdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __negvsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __negvti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __nesf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __opendir2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __paritydi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __paritysi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __parityti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __popcountdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __popcountsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __popcountti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __powidf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __powisf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __powixf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __signbit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __signbitf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __signbitl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __srefill() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __srget() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __stderrp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __stdinp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __stdoutp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __subdf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __subsf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __subvdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __subvsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __subvti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __swbuf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __sync_fetch_and_add_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __sync_fetch_and_and_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __sync_fetch_and_or_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __sync_fetch_and_sub_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __sync_fetch_and_xor_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __sync_lock_test_and_set_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __truncdfsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ucmpdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __ucmpti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __udivdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __udivmoddi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __udivmodsi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __udivmodti4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __udivsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __udivti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __umoddi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __umodsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __umodti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __unorddf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __unordsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI __vfprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Assert() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_strong() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_strong_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_strong_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_strong_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_strong_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_weak() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_weak_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_weak_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_weak_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_compare_exchange_weak_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_copy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_exchange() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_exchange_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_exchange_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_exchange_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_exchange_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_add_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_add_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_add_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_add_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_and_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_and_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_and_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_and_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_or_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_or_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_or_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_or_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_sub_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_sub_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_sub_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_sub_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_xor_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_xor_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_xor_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_fetch_xor_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_flag_clear() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_flag_test_and_set() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_is_lock_free_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_is_lock_free_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_is_lock_free_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_is_lock_free_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_load_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_load_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_load_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_load_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_signal_fence() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_store_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_store_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_store_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_store_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atomic_thread_fence() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atqexit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Atthreadexit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Btowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Call_once() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Call_onceEx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Clocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Closreg() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_broadcast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_destroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_do_broadcast_at_thread_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_init_with_name() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_register_at_thread_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_signal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_timedwait() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_unregister_at_thread_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cnd_wait() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Cosh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Costate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _CStrftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _CStrxfrm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _CTinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Ctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _CurrentRuneLocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _CWcsxfrm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Daysto() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Dbl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Dclass() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _DefaultRuneLocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Deletegloballocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Denorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Dint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Divide() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Dnorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Do_call() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Dscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Dsign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Dtento() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Dtest() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Dunscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Eps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Erf_one() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Erf_small() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Erfc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _err() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Errno() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Exp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fac_tidy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fail_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FAtan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FCosh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDclass() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDenorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDivide() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDnorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDsign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDtento() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDtest() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FDunscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FEps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Feraise() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FErf_one() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FErf_small() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FErfc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_add_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_and_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_and_seq_cst_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_and_seq_cst_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_and_seq_cst_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_or_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_or_seq_cst_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_or_seq_cst_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_or_seq_cst_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_xor_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_xor_seq_cst_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_xor_seq_cst_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fetch_xor_seq_cst_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FExp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FFpcomp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FGamma_big() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fgpos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FHypot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Files() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FInf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FLog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FLogpoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Flt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fltrounds() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FNan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fofind() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fofree() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Foprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fpcomp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FPlsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FPmsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FPoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FPow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FQuad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FQuadph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FRecip() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FRint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Frprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FRteps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FSin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FSincos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FSinh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FSnan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fspos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FTan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FTgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Fwprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXbig() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_addh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_addx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_getw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_invx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_ldexpx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_movx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_mulh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_mulx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_setn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_setw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_sqrtx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FXp_subx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _FZero() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Gamma_big() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Genld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Gentime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getcloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getctyptab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getdst() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Geterrno() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getfld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getfloat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getgloballocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getmbcurmax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getpcostate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getpctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getpmbstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _getprogname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getptimes() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getptolower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getptoupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getpwcostate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getpwcstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getpwctrtab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getpwctytab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Gettime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Getzone() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Hugeval() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Hypot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Inf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _init_env() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _init_tls() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Isdst() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Iswctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LAtan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LCosh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Ldbl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDclass() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDenorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDivide() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDnorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDsign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDtento() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDtest() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Ldtob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LDunscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LEps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LErf_one() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LErf_small() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LErfc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LExp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LFpcomp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LGamma_big() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LHypot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LInf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Litob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LLog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LLogpoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LNan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Lock_shared_ptr_spin_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Lock_spin_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Lockfilelock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Locksyslock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Locsum() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Loctab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Locterm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Locvar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Log() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Logpoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LPlsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LPmsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LPoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LPow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LQuad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LQuadph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LRecip() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LRint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LRteps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LSin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LSincos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LSinh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LSnan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LTan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LTgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXbig() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_addh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_addx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_getw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_invx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_ldexpx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_movx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_mulh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_mulx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_setn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_setw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_sqrtx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LXp_subx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _LZero() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Makeloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Makestab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Makewct() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _malloc_finalize_lv2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _malloc_fini() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _malloc_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _malloc_init_lv2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _malloc_postfork() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _malloc_prefork() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _malloc_thread_cleanup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mbcurmax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mbstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mbtowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mbtowcx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtx_current_owns() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtx_destroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtx_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtx_init_with_name() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtx_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtx_timedlock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtx_trylock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtx_unlock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtxdst() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtxinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtxlock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Mtxunlock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Nan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _new_setup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Nnl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _PathLocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _PJP_C_Copyright() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _PJP_CPP_Copyright() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Plsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Pmsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Poly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Pow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Printf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Putfld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Putstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Puttxt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Quad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Quadph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Randseed() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _readdir_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Readloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Recip() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _reclaim_telldir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Restore_state() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Rint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Rteps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _rtld_addr_phdr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _rtld_atfork_post() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _rtld_atfork_pre() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _rtld_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _rtld_get_stack_prot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _rtld_thread_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Save_state() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Scanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _SceLibcDebugOut() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _sceLibcGetMallocParam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _SceLibcTelemetoryOut() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _seekdir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Setgloballocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Shared_ptr_flag() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Sin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Sincos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Sinh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Skip() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Snan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stderr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stdin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stdout() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stod() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stodx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stoflt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stofx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stold() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stoldx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stollx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stolx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stopfx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stoul() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stoull() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stoullx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stoulx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Stoxflt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Strcollx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Strerror() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Strxfrmx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_abort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_create() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_current() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_detach() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_equal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_id() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_join() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_lt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_sleep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_start() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_start_with_attr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_start_with_name() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_start_with_name_attr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Thrd_yield() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _thread_autoinit_dummy_decl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _thread_autoinit_dummy_decl_stub() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _thread_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _thread_init_stub() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Times() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Costate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Ctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Errno() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Mbcurmax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Mbstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Times() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Tolotab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Touptab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__WCostate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Wcstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Wctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tls_setup__Wctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tolotab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Touptab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Towctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tss_create() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tss_delete() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tss_get() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tss_set() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Ttotm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Tzoff() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Unlock_shared_ptr_spin_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Unlock_spin_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Unlockfilelock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Unlocksyslock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Unwind_Backtrace() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Unwind_GetIP() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Unwind_Resume() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Unwind_Resume_or_Rethrow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Vacopy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _warn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WCostate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Wcscollx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Wcsftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Wcstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Wcsxfrmx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Wctob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Wctomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Wctombx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Wctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Wctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WFrprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WFwprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WGenld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WGetfld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WGetfloat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WGetint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WGetstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WLdtob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WLitob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WPrintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WPutfld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WPutstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WPuttxt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WScanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStod() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStodx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStoflt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStofx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStold() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStoldx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStopfx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStoul() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStoull() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _WStoxflt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xbig() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_addh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_addx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_getw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_invx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_ldexpx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_movx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_mulh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_mulx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_setn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_setw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_sqrtx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xp_subx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xtime_diff_to_ts() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xtime_get_ticks() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Xtime_to_ts() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdaPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdaPvm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdaPvmRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdaPvmSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdaPvRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdaPvS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdaPvSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdaPvSt11align_val_tRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdlPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdlPvm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdlPvmRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdlPvmSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdlPvRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdlPvS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdlPvSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZdlPvSt11align_val_tRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Zero() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt10moneypunctIcLb0EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt10moneypunctIcLb1EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt10moneypunctIwLb0EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt10moneypunctIwLb1EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt14_Error_objectsIiE14_System_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt14_Error_objectsIiE15_Generic_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt14_Error_objectsIiE16_Iostream_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt20_Future_error_objectIiE14_Future_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt7codecvtIcc9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt7collateIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt7collateIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt8messagesIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt8messagesIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt8numpunctIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt8numpunctIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZGVNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZGVZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZGVZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv116__enum_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv116__enum_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv116__enum_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv117__array_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv117__array_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv117__array_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv117__class_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv117__class_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv117__class_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv117__pbase_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv117__pbase_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv117__pbase_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv119__pointer_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv119__pointer_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv119__pointer_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv120__function_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv120__function_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv120__function_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv120__si_class_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv120__si_class_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv120__si_class_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv121__vmi_class_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv121__vmi_class_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv121__vmi_class_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv123__fundamental_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv123__fundamental_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv123__fundamental_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv129__pointer_to_member_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv129__pointer_to_member_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN10__cxxabiv129__pointer_to_member_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN6Dinkum7codecvt10_Cvt_checkEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN6Dinkum7threads10lock_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN6Dinkum7threads10lock_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN6Dinkum7threads17_Throw_lock_errorEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN6Dinkum7threads21_Throw_resource_errorEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN6Dinkum7threads21thread_resource_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZN6Dinkum7threads21thread_resource_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Znam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZnamRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZnamSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZnamSt11align_val_tRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSbIwSt11char_traitsIwESaIwEE5_XlenEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSbIwSt11char_traitsIwESaIwEE5_XranEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSs5_XlenEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSs5_XranEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt10bad_typeid4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt10bad_typeid8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt11logic_error4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt11logic_error8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt12bad_weak_ptr4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt12codecvt_base11do_encodingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt12codecvt_base13do_max_lengthEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt12future_error4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt12future_error8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt12system_error8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt13bad_exception8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt13runtime_error4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt14error_category10equivalentEiRKSt15error_condition() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt14error_category10equivalentERKSt10error_codei() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt14error_category23default_error_conditionEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt17bad_function_call4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt18bad_variant_access4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt19istreambuf_iteratorIcSt11char_traitsIcEE5equalERKS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt19istreambuf_iteratorIwSt11char_traitsIwEE5equalERKS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt22_Future_error_category4nameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt22_Future_error_category7messageEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt22_System_error_category23default_error_conditionEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt22_System_error_category4nameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt22_System_error_category7messageEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt23_Generic_error_category4nameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt23_Generic_error_category7messageEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt24_Iostream_error_category4nameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt24_Iostream_error_category7messageEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIcE10do_tolowerEc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIcE10do_tolowerEPcPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIcE10do_toupperEc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIcE10do_toupperEPcPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIcE8do_widenEc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIcE8do_widenEPKcS2_Pc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIcE9do_narrowEcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIcE9do_narrowEPKcS2_cPc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE10do_scan_isEsPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE10do_tolowerEPwPKw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE10do_tolowerEw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE10do_toupperEPwPKw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE10do_toupperEw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE11do_scan_notEsPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE5do_isEPKwS2_Ps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE5do_isEsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE8do_widenEc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE8do_widenEPKcS2_Pw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE9do_narrowEPKwS2_cPc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt5ctypeIwE9do_narrowEwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIcE11do_groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIcE13do_neg_formatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIcE13do_pos_formatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIcE14do_curr_symbolEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIcE14do_frac_digitsEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIcE16do_decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIcE16do_negative_signEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIcE16do_positive_signEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIcE16do_thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIwE11do_groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIwE13do_neg_formatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIwE13do_pos_formatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIwE14do_curr_symbolEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIwE14do_frac_digitsEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIwE16do_decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIwE16do_negative_signEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIwE16do_positive_signEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7_MpunctIwE16do_thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIcc9_MbstatetE10do_unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIcc9_MbstatetE16do_always_noconvEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIcc9_MbstatetE2inERS0_PKcS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIcc9_MbstatetE3outERS0_PKcS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIcc9_MbstatetE5do_inERS0_PKcS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIcc9_MbstatetE6do_outERS0_PKcS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIcc9_MbstatetE6lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIcc9_MbstatetE7unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIcc9_MbstatetE9do_lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDic9_MbstatetE10do_unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDic9_MbstatetE11do_encodingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDic9_MbstatetE13do_max_lengthEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDic9_MbstatetE16do_always_noconvEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDic9_MbstatetE5do_inERS0_PKcS4_RS4_PDiS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDic9_MbstatetE6do_outERS0_PKDiS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDic9_MbstatetE9do_lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDsc9_MbstatetE10do_unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDsc9_MbstatetE11do_encodingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDsc9_MbstatetE13do_max_lengthEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDsc9_MbstatetE16do_always_noconvEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDsc9_MbstatetE5do_inERS0_PKcS4_RS4_PDsS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDsc9_MbstatetE6do_outERS0_PKDsS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIDsc9_MbstatetE9do_lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIwc9_MbstatetE10do_unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIwc9_MbstatetE11do_encodingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIwc9_MbstatetE13do_max_lengthEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIwc9_MbstatetE16do_always_noconvEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIwc9_MbstatetE5do_inERS0_PKcS4_RS4_PwS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIwc9_MbstatetE6do_outERS0_PKwS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7codecvtIwc9_MbstatetE9do_lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIcE10do_compareEPKcS2_S2_S2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIcE12do_transformEPKcS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIcE4hashEPKcS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIcE7compareEPKcS2_S2_S2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIcE7do_hashEPKcS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIcE9transformEPKcS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIwE10do_compareEPKwS2_S2_S2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIwE12do_transformEPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIwE4hashEPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIwE7compareEPKwS2_S2_S2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIwE7do_hashEPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7collateIwE9transformEPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetffldEPcRS3_S6_RSt8ios_basePi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE9_GetffldxEPcRS3_S6_RSt8ios_basePi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetffldEPcRS3_S6_RSt8ios_basePi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE9_GetffldxEPcRS3_S6_RSt8ios_basePi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basece() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecPKv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE4_PutES3_PKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE4_RepES3_cm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_FfmtEPccNSt5_IosbIiE9_FmtflagsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_FputES3_RSt8ios_basecPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_FputES3_RSt8ios_basecPKcmmmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_IfmtEPcPKcNSt5_IosbIiE9_FmtflagsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_IputES3_RSt8ios_basecPcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basece() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecPKv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewPKv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE4_PutES3_PKwm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE4_RepES3_wm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_FfmtEPccNSt5_IosbIiE9_FmtflagsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_FputES3_RSt8ios_basewPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_FputES3_RSt8ios_basewPKcmmmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_IfmtEPcPKcNSt5_IosbIiE9_FmtflagsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_IputES3_RSt8ios_basewPcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewPKv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8bad_cast4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8bad_cast8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8ios_base7failure8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIcE3getEiiiRKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIcE4openERKSsRKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIcE5closeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIcE6do_getEiiiRKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIcE7do_openERKSsRKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIcE8do_closeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIwE3getEiiiRKSbIwSt11char_traitsIwESaIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIwE4openERKSsRKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIwE5closeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIwE6do_getEiiiRKSbIwSt11char_traitsIwESaIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIwE7do_openERKSsRKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8messagesIwE8do_closeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE11do_groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE11do_truenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE12do_falsenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE13decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE13thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE16do_decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE16do_thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE8groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE8truenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIcE9falsenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE11do_groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE11do_truenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE12do_falsenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE13decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE13thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE16do_decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE16do_thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE8groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE8truenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8numpunctIwE9falsenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE10date_orderEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE11do_get_dateES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE11do_get_timeES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE11do_get_yearES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE11get_weekdayES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE13do_date_orderEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE13get_monthnameES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE14do_get_weekdayES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE16do_get_monthnameES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmPKcSE_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetfmtES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetintERS3_S5_iiRiRKSt5ctypeIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8get_dateES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8get_timeES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8get_yearES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE10date_orderEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE11do_get_dateES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE11do_get_timeES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE11do_get_yearES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE11get_weekdayES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE13do_date_orderEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE13get_monthnameES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE14do_get_weekdayES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE16do_get_monthnameES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmPKwSE_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetfmtES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetintERS3_S5_iiRiRKSt5ctypeIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8get_dateES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8get_timeES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8get_yearES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecPK2tmcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecPK2tmPKcSB_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecPK2tmcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewPK2tmcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewPK2tmPKwSB_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewPK2tmcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt9bad_alloc4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt9bad_alloc8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt9exception4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt9exception6_RaiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNKSt9exception8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetmfldERS3_S5_bRSt8ios_basePc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERSbIwS2_SaIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERSbIwS2_SaIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetmfldERS3_S5_bRSt8ios_basePw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_bRSt8ios_basece() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_bRSt8ios_basecRKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_bRSt8ios_basece() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_bRSt8ios_basecRKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE8_PutmfldES3_bRSt8ios_basecbSsc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_bRSt8ios_basewe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_bRSt8ios_basewRKSbIwS2_SaIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_bRSt8ios_basewe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_bRSt8ios_basewRKSbIwS2_SaIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE8_PutmfldES3_bRSt8ios_basewbSbIwS2_SaIwEEw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSbIwSt11char_traitsIwESaIwEE5_CopyEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSbIwSt11char_traitsIwESaIwEE5eraseEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSbIwSt11char_traitsIwESaIwEE6appendEmw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSbIwSt11char_traitsIwESaIwEE6appendERKS2_mm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSbIwSt11char_traitsIwESaIwEE6assignEmw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSbIwSt11char_traitsIwESaIwEE6assignEPKwm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSbIwSt11char_traitsIwESaIwEE6assignERKS2_mm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSbIwSt11char_traitsIwESaIwEE6insertEmmw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSiD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSiD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSo6sentryC2ERSo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSo6sentryD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSs5_CopyEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSs5eraseEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSs6appendEmc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSs6appendERKSsmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSs6assignEmc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSs6assignEPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSs6assignERKSsmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSs6insertEmmc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10bad_typeidD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10bad_typeidD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10bad_typeidD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem10_Close_dirEPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem10_Copy_fileEPKcS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem10_File_sizeEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem11_EquivalentEPKcS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem11_Remove_dirEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem12_Current_getERA260_c() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem12_Current_setEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem16_Last_write_timeEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem18_Xfilesystem_errorEPKcRKNS_4pathES4_St10error_code() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem18_Xfilesystem_errorEPKcRKNS_4pathESt10error_code() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem18_Xfilesystem_errorEPKcSt10error_code() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem20_Set_last_write_timeEPKcl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem5_StatEPKcPNS_5permsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem6_ChmodEPKcNS_5permsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem6_LstatEPKcPNS_5permsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem7_RenameEPKcS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem7_ResizeEPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem7_UnlinkEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem8_StatvfsEPKcRNS_10space_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem9_Make_dirEPKcS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem9_Open_dirERA260_cPKcRiRNS_9file_typeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10filesystem9_Read_dirERA260_cPvRNS_9file_typeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EE4intlE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb0EED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EE4intlE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIcLb1EED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EE4intlE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb0EED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EE4intlE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt10moneypunctIwLb1EED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt11logic_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt11logic_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt11logic_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt11range_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt11range_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt11range_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt11regex_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt11regex_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt11regex_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12bad_weak_ptrD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12bad_weak_ptrD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12bad_weak_ptrD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12domain_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12domain_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12domain_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12future_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12future_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12future_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12length_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12length_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12length_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12out_of_rangeD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12out_of_rangeD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12out_of_rangeD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders2_1E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders2_2E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders2_3E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders2_4E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders2_5E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders2_6E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders2_7E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders2_8E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders2_9E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_11E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_12E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_13E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_14E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_15E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_16E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_17E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_18E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_19E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12placeholders3_20E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12system_errorC2ESt10error_codePKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12system_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12system_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt12system_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13_Num_int_base10is_boundedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13_Num_int_base10is_integerE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13_Num_int_base14is_specializedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13_Num_int_base5radixE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13_Num_int_base8is_exactE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13_Num_int_base9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13_Regex_traitsIcE6_NamesE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13_Regex_traitsIwE6_NamesE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13bad_exceptionD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13bad_exceptionD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13bad_exceptionD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE4syncEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE5_LockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE5imbueERKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE5uflowEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE6setbufEPci() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE7_UnlockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt13basic_filebufIcSt11char_traitsIcEE7seekoffElNSt5_IosbIiE8_SeekdirENS4_9_OpenmodeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt13basic_filebufIcSt11char_traitsIcEE7seekposESt4fposI9_MbstatetENSt5_IosbIiE9_OpenmodeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE8overflowEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE9_EndwriteEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE9pbackfailEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEE9underflowEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIcSt11char_traitsIcEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE4syncEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE5_LockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE5imbueERKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE5uflowEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE6setbufEPwi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE7_UnlockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt13basic_filebufIwSt11char_traitsIwEE7seekoffElNSt5_IosbIiE8_SeekdirENS4_9_OpenmodeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt13basic_filebufIwSt11char_traitsIwEE7seekposESt4fposI9_MbstatetENSt5_IosbIiE9_OpenmodeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE8overflowEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE9_EndwriteEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE9pbackfailEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEE9underflowEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_filebufIwSt11char_traitsIwEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_istreamIwSt11char_traitsIwEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_istreamIwSt11char_traitsIwEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_ostreamIwSt11char_traitsIwEE6sentryC2ERS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_ostreamIwSt11char_traitsIwEE6sentryD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_ostreamIwSt11char_traitsIwEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13basic_ostreamIwSt11char_traitsIwEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13runtime_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13runtime_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt13runtime_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Error_objectsIiE14_System_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Error_objectsIiE15_Generic_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Error_objectsIiE16_Iostream_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base10has_denormE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base10is_boundedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base10is_integerE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base11round_styleE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base12has_infinityE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base13has_quiet_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base14is_specializedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base15has_denorm_lossE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base15tinyness_beforeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base17has_signaling_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base5radixE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base5trapsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base8is_exactE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base9is_iec559E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14_Num_ldbl_base9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14error_categoryD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIaE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIaE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIaE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIbE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIbE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIbE9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIbE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIcE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIcE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIcE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIdE12max_digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIdE12max_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIdE12min_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIdE14max_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIdE14min_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIdE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIdE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIDiE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIDiE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIDiE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIDsE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIDsE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIDsE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIeE12max_digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIeE12max_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIeE12min_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIeE14max_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIeE14min_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIeE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIeE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIfE12max_digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIfE12max_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIfE12min_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIfE14max_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIfE14min_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIfE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIfE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIhE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIhE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIhE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIiE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIiE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIiE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIjE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIjE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIjE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIlE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIlE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIlE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsImE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsImE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsImE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIsE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIsE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIsE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsItE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsItE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsItE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIwE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIwE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIwE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIxE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIxE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIxE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIyE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIyE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14numeric_limitsIyE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14overflow_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14overflow_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt14overflow_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base10has_denormE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base10is_boundedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base10is_integerE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base11round_styleE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base12has_infinityE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base13has_quiet_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base14is_specializedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base15has_denorm_lossE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base15tinyness_beforeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base17has_signaling_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base5radixE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base5trapsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base8is_exactE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base9is_iec559E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15_Num_float_base9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15basic_streambufIcSt11char_traitsIcEE6xsgetnEPci() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15basic_streambufIcSt11char_traitsIcEE6xsputnEPKci() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15basic_streambufIcSt11char_traitsIcEE9showmanycEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15basic_streambufIwSt11char_traitsIwEE6xsgetnEPwi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15basic_streambufIwSt11char_traitsIwEE6xsputnEPKwi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15basic_streambufIwSt11char_traitsIwEE9showmanycEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15underflow_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15underflow_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt15underflow_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt16invalid_argumentD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt16invalid_argumentD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt16invalid_argumentD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt16nested_exceptionD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt16nested_exceptionD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt16nested_exceptionD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt17bad_function_callD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt17bad_function_callD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt17bad_function_callD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt18bad_variant_accessD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt18bad_variant_accessD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt20_Future_error_objectIiE14_Future_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt20bad_array_new_lengthD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt20bad_array_new_lengthD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt20bad_array_new_lengthD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt22_Future_error_categoryD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt22_Future_error_categoryD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt22_System_error_categoryD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt22_System_error_categoryD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt23_Generic_error_categoryD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt23_Generic_error_categoryD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt24_Iostream_error_categoryD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt24_Iostream_error_categoryD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt3pmr19new_delete_resourceEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt3pmr20get_default_resourceEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt3pmr20null_memory_resourceEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt3pmr20set_default_resourceEPNS_15memory_resourceE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt4_Pad7_LaunchEPKcPP12pthread_attrPP7pthread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt4_Pad7_LaunchEPKcPP7pthread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt4_Pad7_LaunchEPP12pthread_attrPP7pthread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt4_Pad7_LaunchEPP7pthread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt4_Pad8_ReleaseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt4_PadC2EPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt4_PadC2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt4_PadD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt4_PadD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt5ctypeIcE10table_sizeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt5ctypeIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt5ctypeIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt5ctypeIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt5ctypeIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt5ctypeIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt5ctypeIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_Mutex5_LockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_Mutex7_UnlockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_MutexC1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_MutexC2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_MutexD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_MutexD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_Winit9_Init_cntE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_WinitC1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_WinitC2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_WinitD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6_WinitD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6chrono12steady_clock12is_monotonicE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6chrono12steady_clock9is_steadyE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6chrono12system_clock12is_monotonicE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6chrono12system_clock9is_steadyE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale16_GetgloballocaleEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale16_SetgloballocaleEPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale2id7_Id_cntE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale5_InitEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale5emptyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale5facet7_DecrefEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale5facet7_IncrefEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale5facet9_RegisterEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale6globalERKS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_Locimp7_AddfacEPNS_5facetEm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_Locimp8_ClocptrE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_Locimp8_MakelocERKSt8_LocinfoiPS0_PKS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_Locimp9_MakewlocERKSt8_LocinfoiPS0_PKS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_Locimp9_MakexlocERKSt8_LocinfoiPS0_PKS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_LocimpC1Eb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_LocimpC1ERKS0_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_LocimpC2Eb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_LocimpC2ERKS0_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_LocimpD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_LocimpD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7_LocimpD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6locale7classicEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6localeD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt6thread20hardware_concurrencyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIcE5_InitERKSt8_Locinfob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIcEC2Emb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIcEC2EPKcmbb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIwE5_InitERKSt8_Locinfob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIwEC2Emb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIwEC2EPKcmbb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7_MpunctIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetE7_GetcatEPPKNSt6locale5facetEPKS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIcc9_MbstatetED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIDic9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIDic9_MbstatetED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIDic9_MbstatetED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIDsc9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIDsc9_MbstatetED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIDsc9_MbstatetED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIwc9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIwc9_MbstatetED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7codecvtIwc9_MbstatetED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIcED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7collateIwED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8_Locinfo8_AddcatsEiPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8_LocinfoC1EiPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8_LocinfoC1EPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8_LocinfoC1ERKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8_LocinfoC2EiPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8_LocinfoC2EPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8_LocinfoC2ERKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8_LocinfoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8_LocinfoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8bad_castD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8bad_castD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8bad_castD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base4Init9_Init_cntE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base4InitC1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base4InitC2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base4InitD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base4InitD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base5_SyncE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base5clearENSt5_IosbIiE8_IostateEb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base6_IndexE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base7_AddstdEPS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base7failureD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base7failureD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_base7failureD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_baseD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_baseD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8ios_baseD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIcED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8messagesIwED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcE5_InitERKSt8_Locinfob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcE5_TidyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcEC1EPKcmb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcEC2EPKcmb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIcED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwE5_InitERKSt8_Locinfob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwE5_TidyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwEC1EPKcmb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwEC2EPKcmb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8numpunctIwED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE5_TidyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE5_TidyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base10has_denormE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base10is_boundedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base10is_integerE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base11round_styleE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base12has_infinityE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base12max_digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base12max_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base12min_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base13has_quiet_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base14is_specializedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base14max_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base14min_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base15has_denorm_lossE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base15tinyness_beforeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base17has_signaling_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base5radixE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base5trapsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base8is_exactE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base9is_iec559E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9_Num_base9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9bad_allocD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9bad_allocD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9bad_allocD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_Eb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9basic_iosIwSt11char_traitsIwEE4initEPSt15basic_streambufIwS1_Eb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9exception18_Set_raise_handlerEPFvRKS_E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9exceptionD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9exceptionD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9exceptionD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE4_PutES3_St22_String_const_iteratorISt11_String_valISt13_Simple_typesIcEEEm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE4_RepES3_cm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE4_PutES3_St22_String_const_iteratorISt11_String_valISt13_Simple_typesIwEEEm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE4_RepES3_wm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZNSt9type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _Znwm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZnwmRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZnwmSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZnwmSt11align_val_tRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt10_GetloctxtIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEiRT0_S5_mPKT_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt10_GetloctxtIcSt19istreambuf_iteratorIwSt11char_traitsIwEEEiRT0_S5_mPKT_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt10_GetloctxtIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEiRT0_S5_mPKT_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt10_Rng_abortPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt10adopt_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt10defer_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt10unexpectedv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt11_Xbad_allocv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt11setiosflagsNSt5_IosbIiE9_FmtflagsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt11try_to_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt12setprecisioni() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt13_Cl_charnames() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt13_Execute_onceRSt9once_flagPFiPvS1_PS1_ES1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt13_Syserror_mapi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt13_Xregex_errorNSt15regex_constants10error_typeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt13get_terminatev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt13resetiosflagsNSt5_IosbIiE9_FmtflagsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt13set_terminatePFvvE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14_Atomic_assertPKcS0_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14_Cl_wcharnames() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14_Debug_messagePKcS0_j() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14_Raise_handler() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14_Random_devicev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14_Throw_C_errori() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14_Xlength_errorPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14_Xout_of_rangePKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14get_unexpectedv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt14set_unexpectedPFvvE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt15_sceLibcLocinfoPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt15_Xruntime_errorPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt15future_categoryv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt15get_new_handlerv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt15set_new_handlerPFvvE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt15system_categoryv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt16_Throw_Cpp_errori() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt16_Xoverflow_errorPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt16generic_categoryv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt17_Future_error_mapi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt17iostream_categoryv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt18_String_cpp_unused() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt18_Xinvalid_argumentPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt18uncaught_exceptionv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt19_Throw_future_errorRKSt10error_code() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt19_Xbad_function_callv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt21_sceLibcClassicLocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt22_Get_future_error_whati() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt22_Random_device_entropyv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt25_Rethrow_future_exceptionSt13exception_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt3cin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt4_Fpz() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt4cerr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt4clog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt4cout() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt4setwi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt4wcin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt5wcerr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt5wclog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt5wcout() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt6_ThrowRKSt9exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt6ignore() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt7_BADOFF() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt7_FiopenPKcNSt5_IosbIiE9_OpenmodeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt7_FiopenPKwNSt5_IosbIiE9_OpenmodeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt7_MP_AddPyy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt7_MP_GetPy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt7_MP_MulPyyy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt7_MP_RemPyy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt7nothrow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt7setbasei() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt8_XLgammad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt8_XLgammae() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt8_XLgammaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt9_LStrcollIcEiPKT_S2_S2_S2_PKSt8_Collvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt9_LStrcollIwEiPKT_S2_S2_S2_PKSt8_Collvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt9_LStrxfrmIcEmPT_S1_PKS0_S3_PKSt8_Collvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt9_LStrxfrmIwEmPT_S1_PKS0_S3_PKSt8_Collvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZSt9terminatev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTId() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIDh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv116__enum_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv117__array_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv117__class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv117__pbase_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv119__pointer_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv120__function_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv120__si_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv121__vmi_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv123__fundamental_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN10__cxxabiv129__pointer_to_member_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN6Dinkum7threads10lock_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIN6Dinkum7threads21thread_resource_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTINSt6locale5facetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTINSt6locale7_LocimpE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTINSt8ios_base7failureE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPDh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKDh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPKy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIPy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt10bad_typeid() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt10ctype_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt10money_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt10moneypunctIcLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt10moneypunctIcLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt10moneypunctIwLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt10moneypunctIwLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt11_Facet_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt11logic_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt11range_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt11regex_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt12bad_weak_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt12codecvt_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt12domain_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt12future_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt12length_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt12out_of_range() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt12system_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt13bad_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt13basic_filebufIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt13basic_filebufIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt13basic_istreamIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt13basic_ostreamIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt13messages_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt13runtime_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt14error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt14overflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt15basic_streambufIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt15basic_streambufIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt15underflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt16invalid_argument() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt16nested_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt17bad_function_call() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt18bad_variant_access() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt20bad_array_new_length() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt22_Future_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt22_System_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt23_Generic_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt24_Iostream_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt4_Pad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt5_IosbIiE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt5ctypeIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt5ctypeIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7_MpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7_MpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7codecvtIcc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7codecvtIDic9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7codecvtIDsc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7codecvtIwc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7collateIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7collateIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8bad_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8ios_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8messagesIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8messagesIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8numpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8numpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9bad_alloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9basic_iosIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9basic_iosIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9time_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTISt9type_info() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTIy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv116__enum_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv117__array_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv117__class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv117__pbase_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv119__pointer_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv120__function_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv120__si_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv121__vmi_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv123__fundamental_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN10__cxxabiv129__pointer_to_member_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN6Dinkum7threads10lock_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSN6Dinkum7threads21thread_resource_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSNSt6locale5facetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSNSt6locale7_LocimpE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSNSt8ios_base7failureE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPKy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSPy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt10bad_typeid() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt10ctype_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt10money_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt10moneypunctIcLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt10moneypunctIcLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt10moneypunctIwLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt10moneypunctIwLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt11_Facet_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt11logic_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt11range_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt11regex_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt12bad_weak_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt12codecvt_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt12domain_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt12future_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt12length_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt12out_of_range() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt12system_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt13bad_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt13basic_filebufIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt13basic_filebufIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt13basic_istreamIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt13basic_ostreamIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt13messages_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt13runtime_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt14error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt14overflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt15basic_streambufIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt15basic_streambufIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt15underflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt16invalid_argument() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt16nested_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt17bad_function_call() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt18bad_variant_access() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt20bad_array_new_length() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt22_Future_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt22_System_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt23_Generic_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt24_Iostream_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt4_Pad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt5_IosbIiE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt5ctypeIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt5ctypeIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7_MpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7_MpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7codecvtIcc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7codecvtIDic9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7codecvtIDsc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7codecvtIwc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7collateIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7collateIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8bad_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8ios_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8messagesIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8messagesIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8numpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8numpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9bad_alloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9basic_iosIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9basic_iosIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9time_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSSt9type_info() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTSy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTv0_n24_NSiD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTv0_n24_NSiD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTv0_n24_NSoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTv0_n24_NSoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTv0_n24_NSt13basic_istreamIwSt11char_traitsIwEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTv0_n24_NSt13basic_istreamIwSt11char_traitsIwEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTv0_n24_NSt13basic_ostreamIwSt11char_traitsIwEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTv0_n24_NSt13basic_ostreamIwSt11char_traitsIwEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv116__enum_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv117__array_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv117__class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv117__pbase_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv119__pointer_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv120__function_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv120__si_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv121__vmi_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv123__fundamental_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN10__cxxabiv129__pointer_to_member_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN6Dinkum7threads10lock_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVN6Dinkum7threads21thread_resource_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVNSt6locale7_LocimpE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVNSt8ios_base7failureE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt10bad_typeid() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt10moneypunctIcLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt10moneypunctIcLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt10moneypunctIwLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt10moneypunctIwLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt11logic_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt11range_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt11regex_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt12bad_weak_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt12domain_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt12future_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt12length_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt12out_of_range() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt12system_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt13bad_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt13basic_filebufIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt13basic_filebufIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt13basic_istreamIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt13basic_ostreamIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt13runtime_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt14error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt14overflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt15underflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt16invalid_argument() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt16nested_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt17bad_function_call() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt18bad_variant_access() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt20bad_array_new_length() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt22_Future_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt22_System_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt23_Generic_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt24_Iostream_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt4_Pad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt5ctypeIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt5ctypeIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7_MpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7_MpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7codecvtIcc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7codecvtIDic9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7codecvtIDsc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7codecvtIwc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7collateIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7collateIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8bad_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8ios_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8messagesIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8messagesIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8numpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8numpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt9bad_alloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt9exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI _ZTVSt9type_info() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetffldEPcRS3_S6_RSt8ios_basePiE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6localeE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE9_GetffldxEPcRS3_S6_RSt8ios_basePiE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetffldEPcRS3_S6_RSt8ios_basePiE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6localeE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE9_GetffldxEPcRS3_S6_RSt8ios_basePiE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetmfldERS3_S5_bRSt8ios_basePcE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetmfldERS3_S5_bRSt8ios_basePwE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_bRSt8ios_basecRKSsE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_bRSt8ios_basewRKSbIwS2_SaIwEEE4_Src() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
_ZZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI abort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI abort_handler_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI abs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI acos(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acos(x);
}

float PS4_SYSV_ABI acosf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acosf(x);
}

float PS4_SYSV_ABI acosh(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acosh(x);
}

float PS4_SYSV_ABI acoshf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acoshf(x);
}

float PS4_SYSV_ABI acoshl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acoshl(x);
}

float PS4_SYSV_ABI acosl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::acosl(x);
}

s32 PS4_SYSV_ABI alarm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI aligned_alloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI asctime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI asctime_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI asin(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asin(x);
}

float PS4_SYSV_ABI asinf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinf(x);
}

float PS4_SYSV_ABI asinh(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinh(x);
}

float PS4_SYSV_ABI asinhf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinhf(x);
}

float PS4_SYSV_ABI asinhl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinhl(x);
}

float PS4_SYSV_ABI asinl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::asinl(x);
}

s32 PS4_SYSV_ABI asprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI at_quick_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI atan(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::atan(x);
}

double PS4_SYSV_ABI atan2(double y, double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::atan2(y, x);
}

s32 PS4_SYSV_ABI atan2f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atan2l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atanh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atanhf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atanhl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atanl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atexit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atoi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atol() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI atoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI basename() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI basename_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI bcmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI bcopy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI bsearch() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI bsearch_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI btowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI bzero() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI c16rtomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI c32rtomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI calloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI cbrt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI cbrtf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI cbrtl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ceil() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ceilf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ceill() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI clearerr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI clearerr_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI clock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI clock_1700() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI closedir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI copysign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI copysignf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI copysignl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI cos(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::cos(x);
}

float PS4_SYSV_ABI cosf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::cosf(x);
}

s32 PS4_SYSV_ABI cosh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI coshf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI coshl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI cosl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ctime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ctime_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI daemon() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI daylight() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI devname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI devname_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI difftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI dirname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI div() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI drand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI drem() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI dremf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI erand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI erf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI erfc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI erfcf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI erfcl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI erff() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI erfl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI err() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI err_set_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI err_set_file() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI errc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI errx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI exp(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::exp(x);
}

double PS4_SYSV_ABI exp2(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::exp2(x);
}

float PS4_SYSV_ABI exp2f(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::exp2f(x);
}

float PS4_SYSV_ABI exp2l(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::exp2l(x);
}

float PS4_SYSV_ABI expf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::expf(x);
}

s32 PS4_SYSV_ABI expl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI expm1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI expm1f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI expm1l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fabs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fabsf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fabsl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fclose() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fcloseall() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fdim() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fdimf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fdiml() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fdopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fdopendir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI feclearexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fedisableexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI feenableexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fegetenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fegetexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fegetexceptflag() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fegetround() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fegettrapenable() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI feholdexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI feof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI feof_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI feraiseexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ferror() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ferror_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fesetenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fesetexceptflag() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fesetround() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fesettrapenable() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fetestexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI feupdateenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fflush() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fgetc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fgetln() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fgetpos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fgets() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fgetwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fgetws() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fileno() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fileno_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI finite() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI finitef() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI flockfile() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI floor() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI floorf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI floorl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI flsl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fmaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fmal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fmax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fmaxf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fmaxl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fmin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fminf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fminl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fmod() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fmodf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fmodl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fopen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fpurge() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fputc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fputs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fputwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fputws() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI free() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI freeifaddrs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI freopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI freopen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI frexp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI frexpf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI frexpl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fseek() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fseeko() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fsetpos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fstatvfs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ftell() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ftello() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ftrylockfile() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI funlockfile() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fwide() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fwprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fwrite() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fwscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI fwscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI gamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI gamma_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI gammaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI gammaf_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getc_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getchar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getchar_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getcwd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI gethostname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getifaddrs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getopt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getopt_long() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getopt_long_only() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getprogname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI gets() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI gets_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getwchar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI gmtime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI gmtime_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI hypot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI hypot3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI hypot3f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI hypot3l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI hypotf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI hypotl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ignore_handler_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ilogb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ilogbf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ilogbl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI imaxabs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI imaxdiv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI index() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI inet_addr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI inet_aton() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI inet_ntoa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI inet_ntoa_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI initstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isalnum() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isalpha() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isblank() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iscntrl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isdigit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isgraph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isinf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI islower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isnan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isnanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isprint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ispunct() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isspace() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswalnum() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswalpha() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswblank() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswcntrl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswdigit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswgraph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswlower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswprint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswpunct() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswspace() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI iswxdigit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI isxdigit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI j0() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI j0f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI j1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI j1f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI jn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI jnf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI jrand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI labs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lcong48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ldexp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ldexpf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ldexpl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ldiv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lgamma_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lgammaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lgammaf_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lgammal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI llabs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lldiv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI llrint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI llrintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI llrintl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI llround() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI llroundf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI llroundl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI localeconv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI localtime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI localtime_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI log() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI log10() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

float PS4_SYSV_ABI log10f(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::log10f(x);
}

s32 PS4_SYSV_ABI log10l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI log1p() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI log1pf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI log1pl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI log2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI log2f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI log2l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI logb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI logbf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI logbl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI logf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI logl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI longjmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lrand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lrint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lrintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lrintl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lround() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lroundf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI lroundl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI makecontext() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc_check_memory_bounds() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc_finalize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc_get_footer_value() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc_get_malloc_state() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc_initialize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc_report_memory_blocks() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc_stats() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc_stats_fast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI malloc_usable_size() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mblen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbrlen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbrtoc16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbrtoc32() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbrtowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbsinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbsrtowcs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbsrtowcs_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbstowcs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbstowcs_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mbtowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI memalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI memchr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI memcmp(const void* s1, const void* s2, size_t n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::memcmp(s1, s2, n);
}

void* PS4_SYSV_ABI memcpy(void* dest, const void* src, size_t n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::memcpy(dest, src, n);
}

s32 PS4_SYSV_ABI memcpy_s(void* dest, size_t destsz, const void* src, size_t count) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _WIN64
    return memcpy_s(dest, destsz, src, count);
#else
    std::memcpy(dest, src, count);
    return 0; // ALL OK
#endif
}

s32 PS4_SYSV_ABI memmove(void* d, void* s, size_t n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    std::memmove(d, s, n);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI memmove_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI memrchr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

void* PS4_SYSV_ABI memset(void* s, int c, size_t n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::memset(s, c, n);
}

s32 PS4_SYSV_ABI memset_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mergesort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mktime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI modf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI modff() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI modfl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI mrand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nanl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nearbyint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nearbyintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nearbyintl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Need_sceLibcInternal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nextafter() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nextafterf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nextafterl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nexttoward() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nexttowardf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nexttowardl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI nrand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI opendir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI optarg() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI opterr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI optind() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI optopt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI optreset() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI perror() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_memalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawn_file_actions_addclose() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawn_file_actions_adddup2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawn_file_actions_addopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawn_file_actions_destroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawn_file_actions_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_destroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_getflags() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_getpgroup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_getschedparam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_getschedpolicy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_getsigdefault() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_getsigmask() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_setflags() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_setpgroup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_setschedparam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_setschedpolicy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_setsigdefault() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnattr_setsigmask() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI posix_spawnp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI pow(double x, double y) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::pow(x, y);
}

float PS4_SYSV_ABI powf(float x, float y) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::powf(x, y);
}

s32 PS4_SYSV_ABI powl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI printf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI printf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI psignal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI putc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI putc_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI putchar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI putchar_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI putenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI puts() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI putw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI putwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI putwchar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI qsort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI qsort_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI quick_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI rand() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI rand_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI random() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI readdir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI readdir_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI realloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI reallocalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI reallocf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI realpath() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI remainder() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI remainderf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI remainderl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI remove() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI remquo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI remquof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI remquol() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI rewind() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI rewinddir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI rindex() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI rint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI rintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI rintl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI round() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI roundf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI roundl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scalb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scalbf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scalbln() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scalblnf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scalblnl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scalbn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scalbnf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scalbnl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI scanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcDebugOut() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcHeapGetAddressRanges() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcHeapMutexCalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcHeapMutexFree() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcHeapSetAddressRangeCallback() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcHeapSetTraceMarker() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcHeapUnsetTraceMarker() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcInternalMemoryGetWakeAddr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcInternalMemoryMutexEnable() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcInternalSetMallocCallback() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceAlignedAlloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceCalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceCreate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceDestroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceFree() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceGetAddressRanges() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceIsHeapEmpty() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMallocStats() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMallocStatsFast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMallocUsableSize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceMemalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspacePosixMemalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceRealloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceReallocalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcMspaceSetMallocCallback() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcOnce() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceCalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceCheckMemoryBounds() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceCreate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceDestroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceFree() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceGetFooterValue() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceIsHeapEmpty() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMallocStats() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMallocStatsFast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMallocUsableSize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceMemalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspacePosixMemalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceRealloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceReallocalign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceReportMemoryBlocks() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceLibcPafMspaceTrim() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI seed48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI seekdir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI set_constraint_handler_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI setbuf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI setenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI setjmp(std::jmp_buf* buf) {
    LOG_ERROR(Lib_LibcInternal, "(TEST) called");
    return setjmp(buf); // todo this feels platform specific but maybe not
}

s32 PS4_SYSV_ABI setlocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI setstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI setvbuf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sigblock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI siginterrupt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI signalcontext() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI signgam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI significand() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI significandf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sigsetmask() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sigvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

double PS4_SYSV_ABI sin(double x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sin(x);
}

void PS4_SYSV_ABI sincos(double x, double* sinp, double* cosp) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    *sinp = std::sin(x);
    *cosp = std::cos(x);
}

void PS4_SYSV_ABI sincosf(double x, double* sinp, double* cosp) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    *sinp = std::sinf(x);
    *cosp = std::cosf(x);
}

float PS4_SYSV_ABI sinf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinf(x);
}

float PS4_SYSV_ABI sinh(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinh(x);
}

float PS4_SYSV_ABI sinhf(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinhf(x);
}

float PS4_SYSV_ABI sinhl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinhl(x);
}

float PS4_SYSV_ABI sinl(float x) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::sinl(x);
}

s32 PS4_SYSV_ABI snprintf(char* s, size_t n, const char* format, ...) {
    va_list args;
    va_start(args, format);

    // Calculate the required buffer size
    va_list args_copy;
    va_copy(args_copy, args);
    int size = std::vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        // Handle vsnprintf error
        LOG_ERROR(Lib_LibcInternal, "vsnprintf failed to calculate size");
        return size;
    }

    // Ensure that the size doesn't exceed the given buffer size
    size = std::min(size, static_cast<int>(n - 1)); // -1 to leave space for null terminator

    // Format the string into the provided buffer
    int result = std::vsnprintf(s, n, format, args);
    if (result >= 0) {
        // Null-terminate the buffer manually if needed
        s[size] = '\0';  // Ensures that s is null-terminated
        LOG_DEBUG(Lib_LibcInternal, "Formatted result: {}", s);
    } else {
        LOG_ERROR(Lib_LibcInternal, "vsnprintf failed during formatting");
    }

    va_end(args);

    return result;
}

s32 PS4_SYSV_ABI snprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI snwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sqrt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sqrtf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sqrtl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI srand() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI srand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI srandom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI srandomdev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI statvfs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI stderr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI stdin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI stdout() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI stpcpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strcasecmp(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _WIN32
    return _stricmp(str1, str2);
#else
    return strcasecmp(str1, str2);
#endif
}

char* PS4_SYSV_ABI strcat(char* dest, const char* src) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strcat(dest, src);
}

s32 PS4_SYSV_ABI strcat_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

const char* PS4_SYSV_ABI strchr(const char* str, int c) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strchr(str, c);
}

s32 PS4_SYSV_ABI strcmp(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strcmp(str1, str2);
}

s32 PS4_SYSV_ABI strcoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

char* PS4_SYSV_ABI strcpy(char* dest, const char* src) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strcpy(dest, src);
}

char* PS4_SYSV_ABI strcpy_s(char* dest, u64 len, const char* src) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strncpy(dest, src, len);
}

s32 PS4_SYSV_ABI strcspn(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strcspn(str1, str2);
}

s32 PS4_SYSV_ABI strdup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strerror() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strerror_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strerror_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strerrorlen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strlcat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strlcpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

size_t PS4_SYSV_ABI strlen(const char* str) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strlen(str);
}

s32 PS4_SYSV_ABI strncasecmp(const char* str1, const char* str2, size_t num) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _WIN32
    return _strnicmp(str1, str2, num);
#else
    return strncasecmp(str1, str2, num);
#endif
}

s32 PS4_SYSV_ABI strncat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strncat_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strncmp(const char* str1, const char* str2, size_t num) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strncmp(str1, str2, num);
}

char* PS4_SYSV_ABI strncpy(char* dest, const char* src, std::size_t count) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strncpy(dest, src, count);
}

s32 PS4_SYSV_ABI strncpy_s(char* dest, size_t destsz, const char* src, size_t count) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _WIN64
    return strncpy_s(dest, destsz, src, count);
#else
    std::strcpy(dest, src);
    return 0;
#endif
}

s32 PS4_SYSV_ABI strndup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strnlen(const char* str, size_t maxlen) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::min(std::strlen(str), maxlen);
}

s32 PS4_SYSV_ABI strnlen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strnstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

const char* PS4_SYSV_ABI strpbrk(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strpbrk(str1, str2);
}

const char* PS4_SYSV_ABI strrchr(const char* str, int c) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strrchr(str, c);
}

char* PS4_SYSV_ABI strsep(char** strp, const char* delim) {
    LOG_DEBUG(Lib_LibcInternal, "called");
#ifdef _GNU_SOURCE
    return strsep(strp, delim);
#else
    if (!*strp) return nullptr;
    char* token = *strp;
    *strp = std::strpbrk(token, delim);
    if (*strp) *(*strp)++ = '\0';
    return token;
#endif
}

s32 PS4_SYSV_ABI strspn(const char* str1, const char* str2) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strspn(str1, str2);
}

char* PS4_SYSV_ABI strstr(char* h, char* n) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strstr(h, n);
}

double PS4_SYSV_ABI strtod(const char* str, char** endptr) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strtod(str, endptr);
}

float PS4_SYSV_ABI strtof(const char* str, char** endptr) {
    LOG_DEBUG(Lib_LibcInternal, "called");
    return std::strtof(str, endptr);
}

s32 PS4_SYSV_ABI strtoimax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtok() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtok_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtok_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtol() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtold() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtoul() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtoull() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtoumax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strtouq() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI strxfrm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI swprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI swprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI swscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI swscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sys_nsig() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sys_siglist() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sys_signame() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI syslog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tanh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tanhf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tanhl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tanl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI telldir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tgammaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tgammal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI time() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI timezone() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tolower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI toupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI towctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI towlower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI towupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI trunc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI truncf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI truncl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tzname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI tzset() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ungetc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ungetwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI unsetenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI utime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vasprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI verr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI verrc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI verrx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vfprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vfprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vfscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vfscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vfwprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vfwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vfwscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vfwscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vprintf(const char *format, va_list args) {
    // Copy the va_list because vsnprintf consumes it
    va_list args_copy;
    va_copy(args_copy, args);

    // Calculate the required buffer size
    int size = std::vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        // Handle vsnprintf error
        LOG_ERROR(Lib_LibcInternal, "vsnprintf failed to calculate size");
        return size;
    }

    // Create a string with the required size
    std::string buffer(size, '\0');

    // Format the string into the buffer
    int result = std::vsnprintf(buffer.data(), buffer.size() + 1, format, args); // +1 for null terminator
    if (result >= 0) {
        // Log the formatted result
        LOG_INFO(Lib_LibcInternal, "{}", buffer);
    } else {
        LOG_ERROR(Lib_LibcInternal, "vsnprintf failed during formatting");
    }

    return result;
}

s32 PS4_SYSV_ABI vprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vsnprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vsnprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vsnwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vsprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vsprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vsscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vsscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vswprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vswprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vswscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vswscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vsyslog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vwarn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vwarnc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vwarnx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vwprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vwprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vwscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI vwscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI warn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI warnc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI warnx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcrtomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcrtomb_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcscat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcscat_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcschr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcscmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcscoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcscpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcscpy_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcscspn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcslen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsncat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsncat_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsncmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsncpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsncpy_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsnlen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcspbrk() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsrchr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsrtombs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsrtombs_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsspn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstod() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstoimax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstok() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstok_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstol() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstold() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstombs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstombs_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstoul() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstoull() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcstoumax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wcsxfrm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wctob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wctomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wctomb_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wmemchr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wmemcmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wmemcpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wmemcpy_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wmemmove() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wmemmove_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wmemset() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wprintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wprintf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wscanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI wscanf_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI xtime_get() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI y0() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI y0f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI y1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI y1f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI yn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI ynf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_186EB8E3525D6240() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_419F5881393ECAB1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_6C6B8377791654A4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7FD2D5C8DF0ACBC8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_C14A89D29B148C3A() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceLibcInternal(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapGetTraceInfo);
    LIB_FUNCTION("ys1W6EwuVw4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __absvdi2);
    LIB_FUNCTION("2HED9ow7Zjc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __absvsi2);
    LIB_FUNCTION("v9XNTmsmz+M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __absvti2);
    LIB_FUNCTION("3CAYAjL-BLs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __adddf3);
    LIB_FUNCTION("mhIInD5nz8I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __addsf3);
    LIB_FUNCTION("8gG-+co6LfM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __addvdi3);
    LIB_FUNCTION("gsnW-FWQqZo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __addvsi3);
    LIB_FUNCTION("IjlonFkCFDs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __addvti3);
    LIB_FUNCTION("CS91br93fag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ashldi3);
    LIB_FUNCTION("ECUHmdEfhic", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ashlti3);
    LIB_FUNCTION("fSZ+gbf8Ekc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ashrdi3);
    LIB_FUNCTION("7+0ouwmGDww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ashrti3);
    LIB_FUNCTION("ClfCoK1Zeb4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_compare_exchange);
    LIB_FUNCTION("ZwapHUAcijE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_compare_exchange_1);
    LIB_FUNCTION("MwiKdf6QFvI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_compare_exchange_2);
    LIB_FUNCTION("lku-VgKK0RE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_compare_exchange_4);
    LIB_FUNCTION("tnlAgPCKyTk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_compare_exchange_8);
    LIB_FUNCTION("hsn2TaF3poY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_compare_exchange_n);
    LIB_FUNCTION("5i8mTQeo9hs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_exchange);
    LIB_FUNCTION("z8lecpCHpqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_exchange_1);
    LIB_FUNCTION("HDvFM0iZYXo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_exchange_2);
    LIB_FUNCTION("yit-Idli5gU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_exchange_4);
    LIB_FUNCTION("UOz27kgch8k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_exchange_8);
    LIB_FUNCTION("oCH4efUlxZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_exchange_n);
    LIB_FUNCTION("Qb86Y5QldaE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_add_1);
    LIB_FUNCTION("wEImmi0YYQM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_add_2);
    LIB_FUNCTION("U8pDVMfBDUY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_add_4);
    LIB_FUNCTION("SqcnaljoFBw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_add_8);
    LIB_FUNCTION("Q3-0HGD3Y48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_and_1);
    LIB_FUNCTION("A71XWS1kKqA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_and_2);
    LIB_FUNCTION("E-XEmpL9i1A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_and_4);
    LIB_FUNCTION("xMksIr3nXug", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_and_8);
    LIB_FUNCTION("LvLuiirFk8U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_or_1);
    LIB_FUNCTION("aSNAf0kxC+Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_or_2);
    LIB_FUNCTION("AFRS4-8aOSo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_or_4);
    LIB_FUNCTION("5ZKavcBG7eM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_or_8);
    LIB_FUNCTION("HWBJOsgJBT8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_sub_1);
    LIB_FUNCTION("yvhjR7PTRgc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_sub_2);
    LIB_FUNCTION("-mUC21i8WBQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_sub_4);
    LIB_FUNCTION("K+k1HlhjyuA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_sub_8);
    LIB_FUNCTION("aWc+LyHD1vk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_xor_1);
    LIB_FUNCTION("PZoM-Yn6g2Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_xor_2);
    LIB_FUNCTION("pPdYDr1KDsI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_xor_4);
    LIB_FUNCTION("Dw3ieb2rMmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_fetch_xor_8);
    LIB_FUNCTION("JZWEhLSIMoQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_is_lock_free);
    LIB_FUNCTION("+iy+BecyFVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_load);
    LIB_FUNCTION("cWgvLiSJSOQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_load_1);
    LIB_FUNCTION("ufqiLmjiBeM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_load_2);
    LIB_FUNCTION("F+m2tOMgeTo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_load_4);
    LIB_FUNCTION("8KwflkOtvZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_load_8);
    LIB_FUNCTION("Q6oqEnefZQ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_load_n);
    LIB_FUNCTION("sV6ry-Fd-TM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_store);
    LIB_FUNCTION("ZF6hpsTZ2m8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_store_1);
    LIB_FUNCTION("-JjkEief9No", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_store_2);
    LIB_FUNCTION("4tDF0D+qdWk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_store_4);
    LIB_FUNCTION("DEQmHCl-EGU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_store_8);
    LIB_FUNCTION("GdwuPYbVpP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __atomic_store_n);
    LIB_FUNCTION("XGNIEdRyYPo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __cleanup);
    LIB_FUNCTION("gCf7+aGEhnU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __clzdi2);
    LIB_FUNCTION("ptL8XWgpGS4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __clzsi2);
    LIB_FUNCTION("jPywoVsPVR8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __clzti2);
    LIB_FUNCTION("OvbYtSGnzFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __cmpdi2);
    LIB_FUNCTION("u2kPEkUHfsg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __cmpti2);
    LIB_FUNCTION("yDPuV0SXp7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ctzdi2);
    LIB_FUNCTION("2NvhgiBTcVE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ctzsi2);
    LIB_FUNCTION("olBDzD1rX2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ctzti2);
    LIB_FUNCTION("IJKVjsmxxWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_allocate_dependent_exception);
    LIB_FUNCTION("cfAXurvfl5o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_allocate_exception);
    LIB_FUNCTION("tsvEmnenz48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __cxa_atexit);
    LIB_FUNCTION("pBxafllkvt0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_bad_cast);
    LIB_FUNCTION("xcc6DTcL8QA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_bad_typeid);
    LIB_FUNCTION("3cUUypQzMiI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_begin_catch);
    LIB_FUNCTION("usKbuvy2hQg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_call_unexpected);
    LIB_FUNCTION("BxPeH9TTcs4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_current_exception_type);
    LIB_FUNCTION("RY8mQlhg7mI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_current_primary_exception);
    LIB_FUNCTION("MQFPAqQPt1s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_decrement_exception_refcount);
    LIB_FUNCTION("zMCYAqNRllc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_demangle);
    LIB_FUNCTION("lX+4FNUklF0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_end_catch);
    LIB_FUNCTION("H2e8t5ScQGc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_finalize);
    LIB_FUNCTION("kBxt5LwtLA4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_free_dependent_exception);
    LIB_FUNCTION("nOIEswYD4Ig", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_free_exception);
    LIB_FUNCTION("Y6Sl4Xw7gfA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_get_exception_ptr);
    LIB_FUNCTION("3rJJb81CDM4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_get_globals);
    LIB_FUNCTION("uCRed7SvX5E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_get_globals_fast);
    LIB_FUNCTION("2emaaluWzUw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_guard_abort);
    LIB_FUNCTION("3GPpjQdAMTw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_guard_acquire);
    LIB_FUNCTION("9rAeANT2tyE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_guard_release);
    LIB_FUNCTION("PsrRUg671K0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_increment_exception_refcount);
    LIB_FUNCTION("zr094EQ39Ww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_pure_virtual);
    LIB_FUNCTION("ZL9FV4mJXxo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_rethrow);
    LIB_FUNCTION("qKQiNX91IGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __cxa_rethrow_primary_exception);
    LIB_FUNCTION("vkuuLfhnSZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __cxa_throw);
    LIB_FUNCTION("eTP9Mz4KkY4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divdc3);
    LIB_FUNCTION("mdGgLADsq8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divdf3);
    LIB_FUNCTION("9daYeu+0Y-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divdi3);
    LIB_FUNCTION("1rs4-h7Fq9U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divmoddi4);
    LIB_FUNCTION("rtBENmz8Iwc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divmodsi4);
    LIB_FUNCTION("dcaiFCKtoDg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divsc3);
    LIB_FUNCTION("nufufTB4jcI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divsf3);
    LIB_FUNCTION("zdJ3GXAcI9M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divsi3);
    LIB_FUNCTION("XU4yLKvcDh0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divti3);
    LIB_FUNCTION("SNdBm+sNfM4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __divxc3);
    LIB_FUNCTION("hMAe+TWS9mQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __dynamic_cast);
    LIB_FUNCTION("8F52nf7VDS8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __eqdf2);
    LIB_FUNCTION("LmXIpdHppBM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __eqsf2);
    LIB_FUNCTION("6zU++1tayjA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __extendsfdf2);
    LIB_FUNCTION("CVoT4wFYleE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fe_dfl_env);
    LIB_FUNCTION("1IB0U3rUtBw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __fedisableexcept);
    LIB_FUNCTION("NDOLSTFT1ns", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __feenableexcept);
    LIB_FUNCTION("E1iwBYkG3CM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fflush);
    LIB_FUNCTION("r3tNGoVJ2YA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ffsdi2);
    LIB_FUNCTION("b54DvYZEHj4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ffsti2);
    LIB_FUNCTION("q9SHp+5SOOQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixdfdi);
    LIB_FUNCTION("saNCRNfjeeg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixdfsi);
    LIB_FUNCTION("cY4yCWdcTXE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixdfti);
    LIB_FUNCTION("0eoyU-FoNyk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixsfdi);
    LIB_FUNCTION("3qQmz11yFaA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixsfsi);
    LIB_FUNCTION("IHq2IaY4UGg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixsfti);
    LIB_FUNCTION("h8nbSvw0s+M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixunsdfdi);
    LIB_FUNCTION("6WwFtNvnDag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixunsdfsi);
    LIB_FUNCTION("rLuypv9iADw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixunsdfti);
    LIB_FUNCTION("Qa6HUR3h1k4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixunssfdi);
    LIB_FUNCTION("NcZqFTG-RBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixunssfsi);
    LIB_FUNCTION("mCESRUqZ+mw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixunssfti);
    LIB_FUNCTION("DG8dDx9ZV70", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixunsxfdi);
    LIB_FUNCTION("dtMu9zCDn3s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixunsxfsi);
    LIB_FUNCTION("l0qC0BR1F44", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixunsxfti);
    LIB_FUNCTION("31g+YJf1fHk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixxfdi);
    LIB_FUNCTION("usQDRS-1HZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __fixxfti);
    LIB_FUNCTION("BMVIEbwpP+8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __floatdidf);
    LIB_FUNCTION("2SSK3UFPqgQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __floatdisf);
    LIB_FUNCTION("MVPtIf3MtL8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __floatdixf);
    LIB_FUNCTION("X7A21ChFXPQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __floatsidf);
    LIB_FUNCTION("rdht7pwpNfM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __floatsisf);
    LIB_FUNCTION("EtpM9Qdy8D4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __floattidf);
    LIB_FUNCTION("VlDpPYOXL58", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __floattisf);
    LIB_FUNCTION("dJvVWc2jOP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __floattixf);
    LIB_FUNCTION("1RNxpXpVWs4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __floatundidf);
    LIB_FUNCTION("9tnIVFbvOrw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __floatundisf);
    LIB_FUNCTION("3A9RVSwG8B0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __floatundixf);
    LIB_FUNCTION("OdvMJCV7Oxo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __floatunsidf);
    LIB_FUNCTION("RC3VBr2l94o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __floatunsisf);
    LIB_FUNCTION("ibs6jIR0Bw0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __floatuntidf);
    LIB_FUNCTION("KLfd8g4xp+c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __floatuntisf);
    LIB_FUNCTION("OdzLUcBLhb4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __floatuntixf);
    LIB_FUNCTION("qlWiRfOJx1A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __fpclassifyd);
    LIB_FUNCTION("z7aCCd9hMsI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __fpclassifyf);
    LIB_FUNCTION("zwV79ZJ9qAU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __fpclassifyl);
    LIB_FUNCTION("hXA24GbAPBk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __gedf2);
    LIB_FUNCTION("mdLGxBXl6nk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __gesf2);
    LIB_FUNCTION("1PvImz6yb4M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __gtdf2);
    LIB_FUNCTION("ICY0Px6zjjo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __gtsf2);
    LIB_FUNCTION("XwLA5cTHjt4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __gxx_personality_v0);
    LIB_FUNCTION("7p7kTAJcuGg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __inet_addr);
    LIB_FUNCTION("a7ToDPsIQrc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __inet_aton);
    LIB_FUNCTION("6i5aLrxRhG0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __inet_ntoa);
    LIB_FUNCTION("H2QD+kNpa+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __inet_ntoa_r);
    LIB_FUNCTION("dhK16CKwhQg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isfinite);
    LIB_FUNCTION("Q8pvJimUWis", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isfinitef);
    LIB_FUNCTION("3-zCDXatSU4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isfinitel);
    LIB_FUNCTION("V02oFv+-JzA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isinf);
    LIB_FUNCTION("rDMyAf1Jhug", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isinff);
    LIB_FUNCTION("gLGmR9aan4c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isinfl);
    LIB_FUNCTION("GfxAp9Xyiqs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isnan);
    LIB_FUNCTION("lA94ZgT+vMM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isnanf);
    LIB_FUNCTION("YBRHNH4+dDo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isnanl);
    LIB_FUNCTION("fGPRa6T+Cu8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isnormal);
    LIB_FUNCTION("WkYnBHFsmW4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isnormalf);
    LIB_FUNCTION("S3nFV6TR1Dw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isnormall);
    LIB_FUNCTION("q1OvUam0BJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __isthreaded);
    LIB_FUNCTION("-m7FIvSBbMM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __kernel_cos);
    LIB_FUNCTION("7ruwcMCJVGI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __kernel_cosdf);
    LIB_FUNCTION("GLNDoAYNlLQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __kernel_rem_pio2);
    LIB_FUNCTION("zpy7LnTL5p0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __kernel_sin);
    LIB_FUNCTION("2Lvc7KWtErs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __kernel_sindf);
    LIB_FUNCTION("F78ECICRxho", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ledf2);
    LIB_FUNCTION("hbiV9vHqTgo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __lesf2);
    LIB_FUNCTION("9mKjVppFsL0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __longjmp);
    LIB_FUNCTION("18E1gOH7cmk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __lshrdi3);
    LIB_FUNCTION("1iRAqEqEL0Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __lshrti3);
    LIB_FUNCTION("tcBJa2sYx0w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ltdf2);
    LIB_FUNCTION("259y57ZdZ3I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ltsf2);
    LIB_FUNCTION("77pL1FoD4I4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __mb_cur_max);
    LIB_FUNCTION("fGYLBr2COwA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __mb_sb_limit);
    LIB_FUNCTION("gQFVRFgFi48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __moddi3);
    LIB_FUNCTION("k0vARyJi9oU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __modsi3);
    LIB_FUNCTION("J8JRHcUKWP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __modti3);
    LIB_FUNCTION("D4Hf-0ik5xU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __muldc3);
    LIB_FUNCTION("O+Bv-zodKLw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __muldf3);
    LIB_FUNCTION("Hf8hPlDoVsw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __muldi3);
    LIB_FUNCTION("wVbBBrqhwdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __mulodi4);
    LIB_FUNCTION("DDxNvs1a9jM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __mulosi4);
    LIB_FUNCTION("+X-5yNFPbDw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __muloti4);
    LIB_FUNCTION("y+E+IUZYVmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __mulsc3);
    LIB_FUNCTION("BXmn6hA5o0M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __mulsf3);
    LIB_FUNCTION("zhAIFVIN1Ds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __multi3);
    LIB_FUNCTION("Uyfpss5cZDE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __mulvdi3);
    LIB_FUNCTION("tFgzEdfmEjI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __mulvsi3);
    LIB_FUNCTION("6gc1Q7uu244", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __mulvti3);
    LIB_FUNCTION("gZWsDrmeBsg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __mulxc3);
    LIB_FUNCTION("ocyIiJnJW24", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __nedf2);
    LIB_FUNCTION("tWI4Ej9k9BY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __negdf2);
    LIB_FUNCTION("Rj4qy44yYUw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __negdi2);
    LIB_FUNCTION("4f+Q5Ka3Ex0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __negsf2);
    LIB_FUNCTION("Zofiv1PMmR4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __negti2);
    LIB_FUNCTION("fh54IRxGBUQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __negvdi2);
    LIB_FUNCTION("7xnsvjuqtZQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __negvsi2);
    LIB_FUNCTION("QW-f9vYgI7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __negvti2);
    LIB_FUNCTION("OWZ3ZLkgye8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __nesf2);
    LIB_FUNCTION("KOy7MeQ7OAU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __opendir2);
    LIB_FUNCTION("RDeUB6JGi1U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __paritydi2);
    LIB_FUNCTION("9xUnIQ53Ao4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __paritysi2);
    LIB_FUNCTION("vBP4ytNRXm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __parityti2);
    LIB_FUNCTION("m4S+lkRvTVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __popcountdi2);
    LIB_FUNCTION("IBn9qjWnXIw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __popcountsi2);
    LIB_FUNCTION("l1wz5R6cIxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __popcountti2);
    LIB_FUNCTION("H+8UBOwfScI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __powidf2);
    LIB_FUNCTION("EiMkgQsOfU0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __powisf2);
    LIB_FUNCTION("DSI7bz2Jt-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __powixf2);
    LIB_FUNCTION("Rw4J-22tu1U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __signbit);
    LIB_FUNCTION("CjQROLB88a4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __signbitf);
    LIB_FUNCTION("Cj81LPErPCc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __signbitl);
    LIB_FUNCTION("fIskTFX9p68", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __srefill);
    LIB_FUNCTION("yDnwZsMnX0Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __srget);
    LIB_FUNCTION("as8Od-tH1BI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __stderrp);
    LIB_FUNCTION("bgAcsbcEznc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __stdinp);
    LIB_FUNCTION("zqJhBxAKfsc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __stdoutp);
    LIB_FUNCTION("HLDcfGUMNWY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __subdf3);
    LIB_FUNCTION("FeyelHfQPzo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __subsf3);
    LIB_FUNCTION("+kvyBGa+5VI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __subvdi3);
    LIB_FUNCTION("y8j-jP6bHW4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __subvsi3);
    LIB_FUNCTION("cbyLM5qrvHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __subvti3);
    LIB_FUNCTION("TP6INgQ6N4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __swbuf);
    LIB_FUNCTION("+WLgzxv5xYA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __sync_fetch_and_add_16);
    LIB_FUNCTION("XmAquprnaGM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __sync_fetch_and_and_16);
    LIB_FUNCTION("GE4I2XAd4G4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __sync_fetch_and_or_16);
    LIB_FUNCTION("Z2I0BWPANGY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __sync_fetch_and_sub_16);
    LIB_FUNCTION("d5Q-h2wF+-E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __sync_fetch_and_xor_16);
    LIB_FUNCTION("ufZdCzu8nME", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 __sync_lock_test_and_set_16);
    LIB_FUNCTION("2M9VZGYPHLI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __truncdfsf2);
    LIB_FUNCTION("SZk+FxWXdAs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ucmpdi2);
    LIB_FUNCTION("dLmvQfG8am4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __ucmpti2);
    LIB_FUNCTION("tX8ED4uIAsQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __udivdi3);
    LIB_FUNCTION("EWWEBA+Ldw8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __udivmoddi4);
    LIB_FUNCTION("PPdIvXwUQwA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __udivmodsi4);
    LIB_FUNCTION("lcNk3Ar5rUQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __udivmodti4);
    LIB_FUNCTION("PxP1PFdu9OQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __udivsi3);
    LIB_FUNCTION("802pFCwC9w0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __udivti3);
    LIB_FUNCTION("+wj27DzRPpo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __umoddi3);
    LIB_FUNCTION("p4vYrlsVpDE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __umodsi3);
    LIB_FUNCTION("ELSr5qm4K1M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __umodti3);
    LIB_FUNCTION("EDvkw0WaiOw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __unorddf2);
    LIB_FUNCTION("z0OhwgG3Bik", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __unordsf2);
    LIB_FUNCTION("yAZ5vOpmBus", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, __vfprintf);
    LIB_FUNCTION("-QgqOT5u2Vk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Assert);
    LIB_FUNCTION("FHErahnajkw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Atan);
    LIB_FUNCTION("kBpWlgVZLm4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_strong);
    LIB_FUNCTION("SwJ-E2FImAo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_strong_1);
    LIB_FUNCTION("qXkZo1LGnfk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_strong_2);
    LIB_FUNCTION("s+LfDF7LKxM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_strong_4);
    LIB_FUNCTION("SZrEVfvcHuA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_strong_8);
    LIB_FUNCTION("FOe7cAuBjh8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_weak);
    LIB_FUNCTION("rBbtKToRRq4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_weak_1);
    LIB_FUNCTION("sDOFamOKWBI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_weak_2);
    LIB_FUNCTION("0AgCOypbQ90", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_weak_4);
    LIB_FUNCTION("bNFLV9DJxdc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_compare_exchange_weak_8);
    LIB_FUNCTION("frx6Ge5+Uco", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Atomic_copy);
    LIB_FUNCTION("qvTpLUKwq7Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_exchange);
    LIB_FUNCTION("KHJflcH9s84", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_exchange_1);
    LIB_FUNCTION("TbuLWpWuJmc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_exchange_2);
    LIB_FUNCTION("-EgDt569OVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_exchange_4);
    LIB_FUNCTION("+xoGf-x7nJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_exchange_8);
    LIB_FUNCTION("cO0ldEk3Uko", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_add_1);
    LIB_FUNCTION("9kSWQ8RGtVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_add_2);
    LIB_FUNCTION("iPBqs+YUUFw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_add_4);
    LIB_FUNCTION("QVsk3fWNbp0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_add_8);
    LIB_FUNCTION("UVDWssRNEPM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_and_1);
    LIB_FUNCTION("PnfhEsZ-5uk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_and_2);
    LIB_FUNCTION("Pn2dnvUmbRA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_and_4);
    LIB_FUNCTION("O6LEoHo2qSQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_and_8);
    LIB_FUNCTION("K49mqeyzLSk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_or_1);
    LIB_FUNCTION("SVIiJg5eppY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_or_2);
    LIB_FUNCTION("R5X1i1zcapI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_or_4);
    LIB_FUNCTION("++In3PHBZfw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_or_8);
    LIB_FUNCTION("-Zfr0ZQheg4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_sub_1);
    LIB_FUNCTION("ovtwh8IO3HE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_sub_2);
    LIB_FUNCTION("2HnmKiLmV6s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_sub_4);
    LIB_FUNCTION("T8lH8xXEwIw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_sub_8);
    LIB_FUNCTION("Z9gbzf7fkMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_xor_1);
    LIB_FUNCTION("rpl4rhpUhfg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_xor_2);
    LIB_FUNCTION("-GVEj2QODEI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_xor_4);
    LIB_FUNCTION("XKenFBsoh1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_fetch_xor_8);
    LIB_FUNCTION("4CVc6G8JrvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_flag_clear);
    LIB_FUNCTION("Ou6QdDy1f7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_flag_test_and_set);
    LIB_FUNCTION("RBPhCcRhyGI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_is_lock_free_1);
    LIB_FUNCTION("QhORYaNkS+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_is_lock_free_2);
    LIB_FUNCTION("cRYyxdZo1YQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_is_lock_free_4);
    LIB_FUNCTION("-3ZujD7JX9c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_is_lock_free_8);
    LIB_FUNCTION("XAqAE803zMg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_load_1);
    LIB_FUNCTION("aYVETR3B8wk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_load_2);
    LIB_FUNCTION("cjZEuzHkgng", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_load_4);
    LIB_FUNCTION("ea-rVHyM3es", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_load_8);
    LIB_FUNCTION("HfKQ6ZD53sM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_signal_fence);
    LIB_FUNCTION("VRX+Ul1oSgE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_store_1);
    LIB_FUNCTION("6WR6sFxcd40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_store_2);
    LIB_FUNCTION("HMRMLOwOFIQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_store_4);
    LIB_FUNCTION("2uKxXHAKynI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_store_8);
    LIB_FUNCTION("-7vr7t-uto8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atomic_thread_fence);
    LIB_FUNCTION("M6nCy6H8Hs4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Atqexit);
    LIB_FUNCTION("IHiK3lL7CvI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Atthreadexit);
    LIB_FUNCTION("aMucxariNg8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Btowc);
    LIB_FUNCTION("fttiF7rDdak", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Call_once);
    LIB_FUNCTION("G1kDk+5L6dU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Call_onceEx);
    LIB_FUNCTION("myTyhGbuDBw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Clocale);
    LIB_FUNCTION("mgNGxmJltOY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Closreg);
    LIB_FUNCTION("VsP3daJgmVA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Cnd_broadcast);
    LIB_FUNCTION("7yMFgcS8EPA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Cnd_destroy);
    LIB_FUNCTION("vyLotuB6AS4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Cnd_do_broadcast_at_thread_exit);
    LIB_FUNCTION("SreZybSRWpU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Cnd_init);
    LIB_FUNCTION("2B+V3qCqz4s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Cnd_init_with_name);
    LIB_FUNCTION("DV2AdZFFEh8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Cnd_register_at_thread_exit);
    LIB_FUNCTION("0uuqgRz9qfo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Cnd_signal);
    LIB_FUNCTION("McaImWKXong", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Cnd_timedwait);
    LIB_FUNCTION("wpuIiVoCWcM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Cnd_unregister_at_thread_exit);
    LIB_FUNCTION("vEaqE-7IZYc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Cnd_wait);
    LIB_FUNCTION("KeOZ19X8-Ug", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Cosh);
    LIB_FUNCTION("gguxDbgbG74", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Costate);
    LIB_FUNCTION("ykNF6P3ZsdA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _CStrftime);
    LIB_FUNCTION("we-vQBAugV8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _CStrxfrm);
    LIB_FUNCTION("YUKO57czb+0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _CTinfo);
    LIB_FUNCTION("eul2MC3gaYs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Ctype);
    LIB_FUNCTION("chlN6g6UbGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _CurrentRuneLocale);
    LIB_FUNCTION("6aEXAPYpaEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _CWcsxfrm);
    LIB_FUNCTION("ZlsoRa7pcuI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Daysto);
    LIB_FUNCTION("e+hi-tOrDZU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Dbl);
    LIB_FUNCTION("+5OuLYpRB28", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Dclass);
    LIB_FUNCTION("lWGF8NHv880", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _DefaultRuneLocale);
    LIB_FUNCTION("H0FQnSWp1es", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Deletegloballocale);
    LIB_FUNCTION("COSADmn1ROg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Denorm);
    LIB_FUNCTION("-vyIrREaQ0g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Dint);
    LIB_FUNCTION("VGhcd0QwhhY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Divide);
    LIB_FUNCTION("NApYynEzlco", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Dnorm);
    LIB_FUNCTION("4QwxZ3U0OK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Do_call);
    LIB_FUNCTION("FMU7jRhYCRU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Dscale);
    LIB_FUNCTION("zvl6nrvd0sE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Dsign);
    LIB_FUNCTION("vCQLavj-3CM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Dtento);
    LIB_FUNCTION("b-xTWRgI1qw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Dtest);
    LIB_FUNCTION("4Wt5uzHO98o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Dunscale);
    LIB_FUNCTION("E4SYYdwWV28", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Eps);
    LIB_FUNCTION("HmdaOhdCr88", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Erf_one);
    LIB_FUNCTION("DJXyKhVrAD8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Erf_small);
    LIB_FUNCTION("aQURHgjHo-w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Erfc);
    LIB_FUNCTION("UhKI6z9WWuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _err);
    LIB_FUNCTION("u4FNPlIIAtw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Errno);
    LIB_FUNCTION("wZi5ly2guNw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Exit);
    LIB_FUNCTION("yL91YD-WTBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Exp);
    LIB_FUNCTION("chzmnjxxVtk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fac_tidy);
    LIB_FUNCTION("D+fkILS7EK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fail_s);
    LIB_FUNCTION("us3bDnDzd70", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FAtan);
    LIB_FUNCTION("PdnFCFqKGqA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FCosh);
    LIB_FUNCTION("LSp+r7-JWwc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDclass);
    LIB_FUNCTION("JG1MkIFKnT8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDenorm);
    LIB_FUNCTION("HcpmBnY1RGE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDint);
    LIB_FUNCTION("fuzzBVdpRG0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDivide);
    LIB_FUNCTION("0NwCmZv7XcU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDnorm);
    LIB_FUNCTION("SSvY6pTRAgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDscale);
    LIB_FUNCTION("6ei1eQn2WIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDsign);
    LIB_FUNCTION("SoNnx4Ejxw8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDtento);
    LIB_FUNCTION("mnufPlYbnN0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDtest);
    LIB_FUNCTION("41SqJvOe8lA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FDunscale);
    LIB_FUNCTION("OviE3yVSuTU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FEps);
    LIB_FUNCTION("z1EfxK6E0ts", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Feraise);
    LIB_FUNCTION("dST+OsSWbno", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FErf_one);
    LIB_FUNCTION("qEvDssa4tOE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FErf_small);
    LIB_FUNCTION("qwR1gtp-WS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FErfc);
    LIB_FUNCTION("JbQw6W62UwI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fetch_add_8);
    LIB_FUNCTION("pxFnS1okTFE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fetch_and_8);
    LIB_FUNCTION("zQQIrnpCoFA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Fetch_and_seq_cst_1);
    LIB_FUNCTION("xROUuk7ItMM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Fetch_and_seq_cst_2);
    LIB_FUNCTION("jQuruQuMlyo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Fetch_and_seq_cst_4);
    LIB_FUNCTION("ixWEOmOBavk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fetch_or_8);
    LIB_FUNCTION("2+6K-2tWaok", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Fetch_or_seq_cst_1);
    LIB_FUNCTION("-egu08GJ0lw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Fetch_or_seq_cst_2);
    LIB_FUNCTION("gI9un1H-fZk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Fetch_or_seq_cst_4);
    LIB_FUNCTION("YhaOeniKcoA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fetch_xor_8);
    LIB_FUNCTION("E2YhT7m79kM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Fetch_xor_seq_cst_1);
    LIB_FUNCTION("fgXJvOSrqfg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Fetch_xor_seq_cst_2);
    LIB_FUNCTION("cqcY17uV3dI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Fetch_xor_seq_cst_4);
    LIB_FUNCTION("-3pU5y1utmI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FExp);
    LIB_FUNCTION("EBkab3s8Jto", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FFpcomp);
    LIB_FUNCTION("cNGg-Y7JQQw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FGamma_big);
    LIB_FUNCTION("dYJJbxnyb74", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fgpos);
    LIB_FUNCTION("DS03EjPDtFo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FHypot);
    LIB_FUNCTION("qG50MWOiS-Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Files);
    LIB_FUNCTION("QWCTbYI14dA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FInf);
    LIB_FUNCTION("jjjRS7l1MPM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FLog);
    LIB_FUNCTION("OAE3YU396YQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FLogpoly);
    LIB_FUNCTION("+SeQg8c1WC0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Flt);
    LIB_FUNCTION("Jo9ON-AX9eU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fltrounds);
    LIB_FUNCTION("VVgqI3B2bfk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FNan);
    LIB_FUNCTION("xGT4Mc55ViQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fofind);
    LIB_FUNCTION("jVDuvE3s5Bs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fofree);
    LIB_FUNCTION("sQL8D-jio7U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fopen);
    LIB_FUNCTION("dREVnZkAKRE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Foprep);
    LIB_FUNCTION("vhPKxN6zs+A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fpcomp);
    LIB_FUNCTION("cfpRP3h9F6o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FPlsw);
    LIB_FUNCTION("IdWhZ0SM7JA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FPmsw);
    LIB_FUNCTION("5AN3vhTZ7f8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FPoly);
    LIB_FUNCTION("A98W3Iad6xE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FPow);
    LIB_FUNCTION("y9Ur6T0A0p8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FQuad);
    LIB_FUNCTION("PDQbEFcV4h0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FQuadph);
    LIB_FUNCTION("lP9zfrhtpBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FRecip);
    LIB_FUNCTION("TLvAYmLtdVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FRint);
    LIB_FUNCTION("9s3P+LCvWP8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Frprep);
    LIB_FUNCTION("XwRd4IpNEss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FRteps);
    LIB_FUNCTION("ZtjspkJQ+vw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FSin);
    LIB_FUNCTION("fQ+SWrQUQBg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FSincos);
    LIB_FUNCTION("O4L+0oCN9zA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FSinh);
    LIB_FUNCTION("UCjpTas5O3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FSnan);
    LIB_FUNCTION("A+Y3xfrWLLo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fspos);
    LIB_FUNCTION("iBrTJkDlQv8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FTan);
    LIB_FUNCTION("odPHnVL-rFg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FTgamma);
    LIB_FUNCTION("4F9pQjbh8R8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Fwprep);
    LIB_FUNCTION("3uW2ESAzsKo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXbig);
    LIB_FUNCTION("1EyHxzcz6AM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_addh);
    LIB_FUNCTION("1b+IhPTX0nk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_addx);
    LIB_FUNCTION("e1y7KVAySrY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_getw);
    LIB_FUNCTION("OVqW4uElSrc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_invx);
    LIB_FUNCTION("7GgGIxmwA6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_ldexpx);
    LIB_FUNCTION("DVZmEd0ipSg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_movx);
    LIB_FUNCTION("W+lrIwAQVUk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_mulh);
    LIB_FUNCTION("o1B4dkvesMc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_mulx);
    LIB_FUNCTION("ikHTMeh60B0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_setn);
    LIB_FUNCTION("5zWUVRtR8xg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_setw);
    LIB_FUNCTION("pNWIpeE5Wv4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_sqrtx);
    LIB_FUNCTION("HD9vSXqj6zI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FXp_subx);
    LIB_FUNCTION("LrXu7E+GLDY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _FZero);
    LIB_FUNCTION("7FjitE7KKm4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Gamma_big);
    LIB_FUNCTION("vakoyx9nkqo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Genld);
    LIB_FUNCTION("bRN9BzEkm4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Gentime);
    LIB_FUNCTION("2MfMa3456FI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getcloc);
    LIB_FUNCTION("i1N28hWcD-4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getctyptab);
    LIB_FUNCTION("Jcu0Wl1-XbE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getdst);
    LIB_FUNCTION("M1xC101lsIU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Geterrno);
    LIB_FUNCTION("bItEABINEm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getfld);
    LIB_FUNCTION("7iFNNuNyXxw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getfloat);
    LIB_FUNCTION("8Jr4cvRM6EM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Getgloballocale);
    LIB_FUNCTION("PWmDp8ZTS9k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getint);
    LIB_FUNCTION("U52BlHBvYvE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getmbcurmax);
    LIB_FUNCTION("bF4eWOM5ouo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getpcostate);
    LIB_FUNCTION("sUP1hBaouOw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getpctype);
    LIB_FUNCTION("QxqK-IdpumU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getpmbstate);
    LIB_FUNCTION("iI6kGxgXzcU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _getprogname);
    LIB_FUNCTION("8xXiEPby8h8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getptimes);
    LIB_FUNCTION("1uJgoVq3bQU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getptolower);
    LIB_FUNCTION("rcQCUr0EaRU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getptoupper);
    LIB_FUNCTION("hzsdjKbFD7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Getpwcostate);
    LIB_FUNCTION("zS94yyJRSUs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getpwcstate);
    LIB_FUNCTION("RLdcWoBjmT4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getpwctrtab);
    LIB_FUNCTION("uF8hDs1CqWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getpwctytab);
    LIB_FUNCTION("i2yN6xBwooo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getstr);
    LIB_FUNCTION("g8ozp2Zrsj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Gettime);
    LIB_FUNCTION("Wz9CvcF5jn4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Getzone);
    LIB_FUNCTION("ac102y6Rjjc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Hugeval);
    LIB_FUNCTION("wUa+oPQvFZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Hypot);
    LIB_FUNCTION("HIhqigNaOns", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Inf);
    LIB_FUNCTION("bzQExy189ZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _init_env);
    LIB_FUNCTION("6NCOqr3cD74", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _init_tls);
    LIB_FUNCTION("Sb26PiOiFtE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Isdst);
    LIB_FUNCTION("CyXs2l-1kNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Iswctype);
    LIB_FUNCTION("2Aw366Jn07s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LAtan);
    LIB_FUNCTION("moDSeLQGJFQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LCosh);
    LIB_FUNCTION("43u-nm1hQc8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Ldbl);
    LIB_FUNCTION("hdcGjNpcr4w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDclass);
    LIB_FUNCTION("O7zxyNnSDDA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDenorm);
    LIB_FUNCTION("alNWe8glQH4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDint);
    LIB_FUNCTION("HPGLb8Qo6as", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDivide);
    LIB_FUNCTION("ldbrWsQk+2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDnorm);
    LIB_FUNCTION("s-Ml8NxBKf4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDscale);
    LIB_FUNCTION("islhay8zGWo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDsign);
    LIB_FUNCTION("PEU-SAfo5+0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDtento);
    LIB_FUNCTION("A+1YXWOGpuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDtest);
    LIB_FUNCTION("3BbBNPjfkYI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Ldtob);
    LIB_FUNCTION("ArZF2KISb5k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LDunscale);
    LIB_FUNCTION("DzkYNChIvmw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LEps);
    LIB_FUNCTION("urrv9v-Ge6g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LErf_one);
    LIB_FUNCTION("MHyK+d+72V0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LErf_small);
    LIB_FUNCTION("PG4HVe4X+Oc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LErfc);
    LIB_FUNCTION("se3uU7lRMHY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LExp);
    LIB_FUNCTION("-BwLPxElT7U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LFpcomp);
    LIB_FUNCTION("-svZFUiG3T4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LGamma_big);
    LIB_FUNCTION("IRUFZywbTT0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LHypot);
    LIB_FUNCTION("2h3jY75zMkI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LInf);
    LIB_FUNCTION("+GxvfSLhoAo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Litob);
    LIB_FUNCTION("4iFgTDd9NFs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LLog);
    LIB_FUNCTION("niPixjs0l2w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LLogpoly);
    LIB_FUNCTION("zqASRvZg6d0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LNan);
    LIB_FUNCTION("JHvEnCQLPQI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Locale);
    LIB_FUNCTION("fRWufXAccuI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Lock_shared_ptr_spin_lock);
    LIB_FUNCTION("Cv-8x++GS9A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Lock_spin_lock);
    LIB_FUNCTION("vZkmJmvqueY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Lockfilelock);
    LIB_FUNCTION("kALvdgEv5ME", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Locksyslock);
    LIB_FUNCTION("sYz-SxY8H6M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Locsum);
    LIB_FUNCTION("rvNWRuHY6AQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Loctab);
    LIB_FUNCTION("4ifo9QGrO5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Locterm);
    LIB_FUNCTION("ElU3kEY8q+I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Locvar);
    LIB_FUNCTION("zXCi78bYrEI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Log);
    LIB_FUNCTION("bqcStLRGIXw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Logpoly);
    LIB_FUNCTION("W-T-amhWrkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LPlsw);
    LIB_FUNCTION("gso5X06CFkI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LPmsw);
    LIB_FUNCTION("c6Qa0P3XKYQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LPoly);
    LIB_FUNCTION("3Z8jD-HTKr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LPow);
    LIB_FUNCTION("fXCPTDS0tD0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LQuad);
    LIB_FUNCTION("CbRhl+RoYEM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LQuadph);
    LIB_FUNCTION("XrXTtRA7U08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LRecip);
    LIB_FUNCTION("fJmOr59K8QY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LRint);
    LIB_FUNCTION("gobJundphD0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LRteps);
    LIB_FUNCTION("m-Bu5Tzr82A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LSin);
    LIB_FUNCTION("g3dK2qlzwnM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LSincos);
    LIB_FUNCTION("ZWy6IcBqs1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LSinh);
    LIB_FUNCTION("rQ-70s51wrc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LSnan);
    LIB_FUNCTION("mM4OblD9xWM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LTan);
    LIB_FUNCTION("jq4Srfnz9K8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LTgamma);
    LIB_FUNCTION("WSaroeRDE5Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXbig);
    LIB_FUNCTION("QEr-PxGUoic", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_addh);
    LIB_FUNCTION("BuP7bDPGEcQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_addx);
    LIB_FUNCTION("TnublTBYXTI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_getw);
    LIB_FUNCTION("FAreWopdBvE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_invx);
    LIB_FUNCTION("7O-vjsHecbY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_ldexpx);
    LIB_FUNCTION("wlEdSSqaz+E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_movx);
    LIB_FUNCTION("riets0BFHMY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_mulh);
    LIB_FUNCTION("LbLiLA2biaI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_mulx);
    LIB_FUNCTION("hjDoZ6qbkmQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_setn);
    LIB_FUNCTION("kHVXc-AWbMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_setw);
    LIB_FUNCTION("IPjwywTNR8w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_sqrtx);
    LIB_FUNCTION("YCVxOE0lHOI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LXp_subx);
    LIB_FUNCTION("A-cEjaZBDwA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _LZero);
    LIB_FUNCTION("tTDqwhYbUUo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Makeloc);
    LIB_FUNCTION("AnTtuRUE1cE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Makestab);
    LIB_FUNCTION("QalEczzSNqc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Makewct);
    LIB_FUNCTION("RnqlvEmvkdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _malloc_finalize_lv2);
    LIB_FUNCTION("21KFhEQDJ3s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _malloc_fini);
    LIB_FUNCTION("z8GPiQwaAEY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _malloc_init);
    LIB_FUNCTION("20cUk0qX9zo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _malloc_init_lv2);
    LIB_FUNCTION("V94pLruduLg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _malloc_postfork);
    LIB_FUNCTION("aLYyS4Kx6rQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _malloc_prefork);
    LIB_FUNCTION("Sopthb9ztZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _malloc_thread_cleanup);
    LIB_FUNCTION("pCWh67X1PHU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mbcurmax);
    LIB_FUNCTION("wKsLxRq5+fg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mbstate);
    LIB_FUNCTION("sij3OtJpHFc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mbtowc);
    LIB_FUNCTION("-9SIhUr4Iuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mbtowcx);
    LIB_FUNCTION("VYQwFs4CC4Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Mtx_current_owns);
    LIB_FUNCTION("5Lf51jvohTQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mtx_destroy);
    LIB_FUNCTION("YaHc3GS7y7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mtx_init);
    LIB_FUNCTION("tgioGpKtmbE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Mtx_init_with_name);
    LIB_FUNCTION("iS4aWbUonl0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mtx_lock);
    LIB_FUNCTION("hPzYSd5Nasc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Mtx_timedlock);
    LIB_FUNCTION("k6pGNMwJB08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mtx_trylock);
    LIB_FUNCTION("gTuXQwP9rrs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mtx_unlock);
    LIB_FUNCTION("LaPaA6mYA38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mtxdst);
    LIB_FUNCTION("z7STeF6abuU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mtxinit);
    LIB_FUNCTION("pE4Ot3CffW0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mtxlock);
    LIB_FUNCTION("cMwgSSmpE5o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Mtxunlock);
    LIB_FUNCTION("8e2KBTO08Po", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Nan);
    LIB_FUNCTION("KNNNbyRieqQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _new_setup);
    LIB_FUNCTION("Ss3108pBuZY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Nnl);
    LIB_FUNCTION("TMhLRjcQMw8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _PathLocale);
    LIB_FUNCTION("OnHQSrOHTks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _PJP_C_Copyright);
    LIB_FUNCTION("Cqpti4y-D3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _PJP_CPP_Copyright);
    LIB_FUNCTION("1hGmBh83dL8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Plsw);
    LIB_FUNCTION("hrv1BgM68kU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Pmsw);
    LIB_FUNCTION("aINUE2xbhZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Poly);
    LIB_FUNCTION("ECh+p-LRG6Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Pow);
    LIB_FUNCTION("FModQzwn1-4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Printf);
    LIB_FUNCTION("rnxaQ+2hSMI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Putfld);
    LIB_FUNCTION("Fot-m6M2oKE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Putstr);
    LIB_FUNCTION("ijAqq39g4dI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Puttxt);
    LIB_FUNCTION("TQPr-IeNIS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Quad);
    LIB_FUNCTION("VecRKuKdXdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Quadph);
    LIB_FUNCTION("5qtcuXWt5Xc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Randseed);
    LIB_FUNCTION("-u3XfqNumMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _readdir_unlocked);
    LIB_FUNCTION("MxZ4Lc35Rig", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Readloc);
    LIB_FUNCTION("YLEc5sPn1Rg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Recip);
    LIB_FUNCTION("7ZFy2m9rc5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _reclaim_telldir);
    LIB_FUNCTION("77uWF3Z2q90", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Restore_state);
    LIB_FUNCTION("TzxDRcns9e4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Rint);
    LIB_FUNCTION("W8tdMlttt3o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Rteps);
    LIB_FUNCTION("5FoE+V3Aj0c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _rtld_addr_phdr);
    LIB_FUNCTION("R2QKo3hBLkw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _rtld_atfork_post);
    LIB_FUNCTION("i-7v8+UGglc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _rtld_atfork_pre);
    LIB_FUNCTION("dmHEVUqDYGw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _rtld_error);
    LIB_FUNCTION("AdYYKgtPlqg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _rtld_get_stack_prot);
    LIB_FUNCTION("gRw4XrztJ4Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _rtld_thread_init);
    LIB_FUNCTION("0ITXuJOqrSk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Save_state);
    LIB_FUNCTION("s+MeMHbB1Ro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Scanf);
    LIB_FUNCTION("vhETq-NiqJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _SceLibcDebugOut);
    LIB_FUNCTION("1nZ4Xfnyp38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _sceLibcGetMallocParam);
    LIB_FUNCTION("-hOAbTf3Cqc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _SceLibcTelemetoryOut);
    LIB_FUNCTION("Do3zPpsXj1o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _seekdir);
    LIB_FUNCTION("bEk+WGOU90I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Setgloballocale);
    LIB_FUNCTION("KDFy-aPxHVE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Shared_ptr_flag);
    LIB_FUNCTION("cCXjU72Z0Ow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Sin);
    LIB_FUNCTION("j9SGTLclro8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Sincos);
    LIB_FUNCTION("MU25eqxSDTw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Sinh);
    LIB_FUNCTION("Jp6dZm7545A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Skip);
    LIB_FUNCTION("fmHLr9P3dOk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Snan);
    LIB_FUNCTION("H8AprKeZtNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stderr);
    LIB_FUNCTION("1TDo-ImqkJc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stdin);
    LIB_FUNCTION("2sWzhYqFH4E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stdout);
    LIB_FUNCTION("c41UEHVtiEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stod);
    LIB_FUNCTION("QlcJbyd6jxM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stodx);
    LIB_FUNCTION("CpWcnrEZbLA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stof);
    LIB_FUNCTION("wO1-omboFjo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stoflt);
    LIB_FUNCTION("7dlAxeH-htg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stofx);
    LIB_FUNCTION("iNbtyJKM0iQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stold);
    LIB_FUNCTION("BKidCxmLC5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stoldx);
    LIB_FUNCTION("7pNKcscKrf8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stoll);
    LIB_FUNCTION("mOnfZ5aNDQE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stollx);
    LIB_FUNCTION("Ecwid6wJMhY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stolx);
    LIB_FUNCTION("yhbF6MbVuYc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stopfx);
    LIB_FUNCTION("zlfEH8FmyUA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stoul);
    LIB_FUNCTION("q+9E0X3aWpU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stoull);
    LIB_FUNCTION("pSpDCDyxkaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stoullx);
    LIB_FUNCTION("YDnLaav6W6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stoulx);
    LIB_FUNCTION("Ouz5Q8+SUq4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Stoxflt);
    LIB_FUNCTION("v6rXYSx-WGA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Strcollx);
    LIB_FUNCTION("4F11tHMpJa0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Strerror);
    LIB_FUNCTION("CpiD2ZXrhNo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Strxfrmx);
    LIB_FUNCTION("szUft0jERdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Tan);
    LIB_FUNCTION("z-OrNOmb6kk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Tgamma);
    LIB_FUNCTION("JOV1XY47eQA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_abort);
    LIB_FUNCTION("SkRR6WxCTcE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_create);
    LIB_FUNCTION("n2-b3O8qvqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Thrd_current);
    LIB_FUNCTION("L7f7zYwBvZA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_detach);
    LIB_FUNCTION("BnV7WrHdPLU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_equal);
    LIB_FUNCTION("cFsD9R4iY50", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_exit);
    LIB_FUNCTION("np6xXcXEnXE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_id);
    LIB_FUNCTION("YvmY5Jf0VYU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_join);
    LIB_FUNCTION("OCbJ96N1utA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_lt);
    LIB_FUNCTION("jfRI3snge3o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_sleep);
    LIB_FUNCTION("0JidN6q9yGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_start);
    LIB_FUNCTION("gsqXCd6skKs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Thrd_start_with_attr);
    LIB_FUNCTION("ihfGsjLjmOM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Thrd_start_with_name);
    LIB_FUNCTION("ShanbBWU3AA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Thrd_start_with_name_attr);
    LIB_FUNCTION("exNzzCAQuWM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Thrd_yield);
    LIB_FUNCTION("kQelMBYgkK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _thread_autoinit_dummy_decl);
    LIB_FUNCTION("dmUgzUX3nXU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _thread_autoinit_dummy_decl_stub);
    LIB_FUNCTION("PJW+-O4pj6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _thread_init);
    LIB_FUNCTION("1kIpmZX1jw8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _thread_init_stub);
    LIB_FUNCTION("+9ypoH8eJko", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Times);
    LIB_FUNCTION("hbY3mFi8XY0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Costate);
    LIB_FUNCTION("JoeZJ15k5vU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Ctype);
    LIB_FUNCTION("uhYTnarXhZM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Errno);
    LIB_FUNCTION("jr5yQE9WYdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Locale);
    LIB_FUNCTION("7TIcrP513IM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Mbcurmax);
    LIB_FUNCTION("YYG-8VURgXA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Mbstate);
    LIB_FUNCTION("0Hu7rUmhqJM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Times);
    LIB_FUNCTION("hM7qvmxBTx8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Tolotab);
    LIB_FUNCTION("UlJSnyS473g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Touptab);
    LIB_FUNCTION("YUdPel2w8as", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__WCostate);
    LIB_FUNCTION("2d5UH9BnfVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Wcstate);
    LIB_FUNCTION("RYwqCx0hU5c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Wctrans);
    LIB_FUNCTION("KdAc8glk2+8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Tls_setup__Wctype);
    LIB_FUNCTION("J-oDqE1N8R4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Tolotab);
    LIB_FUNCTION("KmfpnwO2lkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Touptab);
    LIB_FUNCTION("DbEnA+MnVIw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Towctrans);
    LIB_FUNCTION("amHyU7v8f-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Tss_create);
    LIB_FUNCTION("g5cG7QtKLTA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Tss_delete);
    LIB_FUNCTION("lOVQnEJEzvY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Tss_get);
    LIB_FUNCTION("ibyFt0bPyTk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Tss_set);
    LIB_FUNCTION("4TTbo2SxxvA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Ttotm);
    LIB_FUNCTION("Csx50UU9b18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Tzoff);
    LIB_FUNCTION("1HYEoANqZ1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Unlock_shared_ptr_spin_lock);
    LIB_FUNCTION("aHUFijEWlxQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Unlock_spin_lock);
    LIB_FUNCTION("0x7rx8TKy2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Unlockfilelock);
    LIB_FUNCTION("9nf8joUTSaQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Unlocksyslock);
    LIB_FUNCTION("s62MgBhosjU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Unwind_Backtrace);
    LIB_FUNCTION("sETNbyWsEHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Unwind_GetIP);
    LIB_FUNCTION("f1zwJ3jAI2k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Unwind_Resume);
    LIB_FUNCTION("xUsJSLsdv9I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Unwind_Resume_or_Rethrow);
    LIB_FUNCTION("xRycekLYXdc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Vacopy);
    LIB_FUNCTION("XQFE8Y58WZM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _warn);
    LIB_FUNCTION("Nd2Y2X6oR18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WCostate);
    LIB_FUNCTION("wmkXFgerLnM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Wcscollx);
    LIB_FUNCTION("RGc3P3UScjY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Wcsftime);
    LIB_FUNCTION("IvP-B8lC89k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Wcstate);
    LIB_FUNCTION("cmIyU0BNYeo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Wcsxfrmx);
    LIB_FUNCTION("oK6C1fys3dU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Wctob);
    LIB_FUNCTION("bSgY14j4Ov4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Wctomb);
    LIB_FUNCTION("stv1S3BKfgw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Wctombx);
    LIB_FUNCTION("DYamMikEv2M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Wctrans);
    LIB_FUNCTION("PlDgAP2AS7M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Wctype);
    LIB_FUNCTION("VbczgfwgScA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WFrprep);
    LIB_FUNCTION("hDuyUWUBrDU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WFwprep);
    LIB_FUNCTION("BYcXjG3Lw-o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WGenld);
    LIB_FUNCTION("Z6CCOW8TZVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WGetfld);
    LIB_FUNCTION("LcHsLn97kcE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WGetfloat);
    LIB_FUNCTION("dWz3HtMMpPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WGetint);
    LIB_FUNCTION("nVS8UHz1bx0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WGetstr);
    LIB_FUNCTION("kUcinoWwBr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WLdtob);
    LIB_FUNCTION("XkT6YSShQcE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WLitob);
    LIB_FUNCTION("kvEP5-KOG1U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WPrintf);
    LIB_FUNCTION("0ISumvb2U5o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WPutfld);
    LIB_FUNCTION("Fh1GjwqvCpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WPutstr);
    LIB_FUNCTION("Kkgg8mWU2WE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WPuttxt);
    LIB_FUNCTION("fzgkSILqRHE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WScanf);
    LIB_FUNCTION("4nRn+exUJAM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStod);
    LIB_FUNCTION("RlewTNtWEcg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStodx);
    LIB_FUNCTION("GUuiOcxL-r0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStof);
    LIB_FUNCTION("FHJlhz0wVts", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStoflt);
    LIB_FUNCTION("JZ9gGlJ22hg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStofx);
    LIB_FUNCTION("w3gRFGRjpZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStold);
    LIB_FUNCTION("waexoHL0Bf4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStoldx);
    LIB_FUNCTION("OmDPJeJXkBM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStoll);
    LIB_FUNCTION("43PYQ2fMT8k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStopfx);
    LIB_FUNCTION("JhVR7D4Ax6Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStoul);
    LIB_FUNCTION("9iCP-hTL5z8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStoull);
    LIB_FUNCTION("DmUIy7m0cyE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _WStoxflt);
    LIB_FUNCTION("QSfaClY45dM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xbig);
    LIB_FUNCTION("ijc7yCXzXsc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_addh);
    LIB_FUNCTION("ycMCyFmWJnU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_addx);
    LIB_FUNCTION("Zb70g9IUs98", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_getw);
    LIB_FUNCTION("f41T4clGlzY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_invx);
    LIB_FUNCTION("c9S0tXDhMBQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_ldexpx);
    LIB_FUNCTION("Zm2LLWgxWu8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_movx);
    LIB_FUNCTION("aOtpC3onyJo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_mulh);
    LIB_FUNCTION("jatbHyxH3ek", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_mulx);
    LIB_FUNCTION("lahbB4B2ugY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_setn);
    LIB_FUNCTION("bIfFaqUwy3I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_setw);
    LIB_FUNCTION("m0uSPrCsVdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_sqrtx);
    LIB_FUNCTION("w178uGYbIJs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xp_subx);
    LIB_FUNCTION("Df1BO64nU-k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Xtime_diff_to_ts);
    LIB_FUNCTION("Cj+Fw5q1tUo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _Xtime_get_ticks);
    LIB_FUNCTION("Zs8Xq-ce3rY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Xtime_to_ts);
    LIB_FUNCTION("MLWl90SFWNE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZdaPv);
    LIB_FUNCTION("FOt55ZNaVJk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZdaPvm);
    LIB_FUNCTION("7lCihI18N9I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdaPvmRKSt9nothrow_t);
    LIB_FUNCTION("Y1RR+IQy6Pg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdaPvmSt11align_val_t);
    LIB_FUNCTION("m-fSo3EbxNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdaPvRKSt9nothrow_t);
    LIB_FUNCTION("Suc8W0QPxjw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZdaPvS_);
    LIB_FUNCTION("v09ZcAhZzSc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdaPvSt11align_val_t);
    LIB_FUNCTION("dH3ucvQhfSY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdaPvSt11align_val_tRKSt9nothrow_t);
    LIB_FUNCTION("z+P+xCnWLBk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZdlPv);
    LIB_FUNCTION("lYDzBVE5mZs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZdlPvm);
    LIB_FUNCTION("7VPIYFpwU2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdlPvmRKSt9nothrow_t);
    LIB_FUNCTION("nwujzxOPXzQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdlPvmSt11align_val_t);
    LIB_FUNCTION("McsGnqV6yRE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdlPvRKSt9nothrow_t);
    LIB_FUNCTION("1vo6qqqa9F4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZdlPvS_);
    LIB_FUNCTION("bZx+FFSlkUM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdlPvSt11align_val_t);
    LIB_FUNCTION("Dt9kllUFXS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZdlPvSt11align_val_tRKSt9nothrow_t);
    LIB_FUNCTION("bPtdppw1+7I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Zero);
    LIB_FUNCTION("Bc4ozvHb4Kg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt10moneypunctIcLb0EE2idE);
    LIB_FUNCTION("yzcKSTTCz1M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt10moneypunctIcLb1EE2idE);
    LIB_FUNCTION("tfmEv+TVGFU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt10moneypunctIwLb0EE2idE);
    LIB_FUNCTION("ksNM8H72JHg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt10moneypunctIwLb1EE2idE);
    LIB_FUNCTION("2G1UznxkcgU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt14_Error_objectsIiE14_System_objectE);
    LIB_FUNCTION("DjLpZIMEkks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt14_Error_objectsIiE15_Generic_objectE);
    LIB_FUNCTION("ieNeByYxFgA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt14_Error_objectsIiE16_Iostream_objectE);
    LIB_FUNCTION("DSnq6xesUo8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt20_Future_error_objectIiE14_Future_objectE);
    LIB_FUNCTION("agAYSUes238", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt7codecvtIcc9_MbstatetE2idE);
    LIB_FUNCTION("gC0DGo+7PVc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt7collateIcE2idE);
    LIB_FUNCTION("jaLGUrwYX84", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt7collateIwE2idE);
    LIB_FUNCTION("8o+oBXdeQPk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("5FD0gWEuuTs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("ZkP0sDpHLLg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("wozVkExRax4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("4ZnE1sEyX3g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt8messagesIcE2idE);
    LIB_FUNCTION("oqYAk3zpC64", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt8messagesIwE2idE);
    LIB_FUNCTION("iHZb2839dBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt8numpunctIcE2idE);
    LIB_FUNCTION("8ArIPXBlkgM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt8numpunctIwE2idE);
    LIB_FUNCTION("z-L6coXk6yo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("TuIEPzIwWcI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("Awj5m1LfcXQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("K+-VjJdCYVc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("HQAa3rCj8ho", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("koazg-62JMk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("HDnBZ+mkyjg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("0ND8MZiuTR8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit);
    LIB_FUNCTION("O2wxIdbMcMQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZGVZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit);
    LIB_FUNCTION("CjzjU2nFUWw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv116__enum_type_infoD0Ev);
    LIB_FUNCTION("upwSZWmYwqE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv116__enum_type_infoD1Ev);
    LIB_FUNCTION("iQiT26+ZGnA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv116__enum_type_infoD2Ev);
    LIB_FUNCTION("R5nRbLQT3oI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv117__array_type_infoD0Ev);
    LIB_FUNCTION("1ZMchlkvNNQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv117__array_type_infoD1Ev);
    LIB_FUNCTION("ckFrsyD2830", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv117__array_type_infoD2Ev);
    LIB_FUNCTION("XAk7W3NcpTY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv117__class_type_infoD0Ev);
    LIB_FUNCTION("goLVqD-eiIY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv117__class_type_infoD1Ev);
    LIB_FUNCTION("xXM1q-ayw2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv117__class_type_infoD2Ev);
    LIB_FUNCTION("GLxD5v2uMHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv117__pbase_type_infoD0Ev);
    LIB_FUNCTION("vIJPARS8imE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv117__pbase_type_infoD1Ev);
    LIB_FUNCTION("krshE4JAE6M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv117__pbase_type_infoD2Ev);
    LIB_FUNCTION("64180GwMVro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv119__pointer_type_infoD0Ev);
    LIB_FUNCTION("bhfgrK+MZdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv119__pointer_type_infoD1Ev);
    LIB_FUNCTION("vCLVhOcdQMY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv119__pointer_type_infoD2Ev);
    LIB_FUNCTION("kVvGL9aF5gg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv120__function_type_infoD0Ev);
    LIB_FUNCTION("dsQ5Xwhl9no", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv120__function_type_infoD1Ev);
    LIB_FUNCTION("NtqD4Q0vUUI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv120__function_type_infoD2Ev);
    LIB_FUNCTION("Fg4w+h9wAMA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv120__si_class_type_infoD0Ev);
    LIB_FUNCTION("6aEkwkEOGRg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv120__si_class_type_infoD1Ev);
    LIB_FUNCTION("bWHwovVFfqc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv120__si_class_type_infoD2Ev);
    LIB_FUNCTION("W5k0jlyBpgM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv121__vmi_class_type_infoD0Ev);
    LIB_FUNCTION("h-a7+0UuK7Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv121__vmi_class_type_infoD1Ev);
    LIB_FUNCTION("yYIymfQGl2E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv121__vmi_class_type_infoD2Ev);
    LIB_FUNCTION("YsZuwZrJZlU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv123__fundamental_type_infoD0Ev);
    LIB_FUNCTION("kzWL2iOsv0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv123__fundamental_type_infoD1Ev);
    LIB_FUNCTION("0jk9oqKd2Gw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv123__fundamental_type_infoD2Ev);
    LIB_FUNCTION("NdeDffcZ-30", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv129__pointer_to_member_type_infoD0Ev);
    LIB_FUNCTION("KaZ3xf5c9Vc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv129__pointer_to_member_type_infoD1Ev);
    LIB_FUNCTION("Re3Lpk8mwEo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN10__cxxabiv129__pointer_to_member_type_infoD2Ev);
    LIB_FUNCTION("yc-vi84-2aE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN6Dinkum7codecvt10_Cvt_checkEmm);
    LIB_FUNCTION("PG-2ZeVVWZE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN6Dinkum7threads10lock_errorD0Ev);
    LIB_FUNCTION("vX+NfOHOI5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN6Dinkum7threads10lock_errorD1Ev);
    LIB_FUNCTION("o27+xO5NBZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN6Dinkum7threads17_Throw_lock_errorEv);
    LIB_FUNCTION("cjmJLdYnT5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN6Dinkum7threads21_Throw_resource_errorEv);
    LIB_FUNCTION("Q+ZnPMGI4M0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN6Dinkum7threads21thread_resource_errorD0Ev);
    LIB_FUNCTION("NOaB7a1PZl8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZN6Dinkum7threads21thread_resource_errorD1Ev);
    LIB_FUNCTION("hdm0YfMa7TQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Znam);
    LIB_FUNCTION("Jh5qUcwiSEk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZnamRKSt9nothrow_t);
    LIB_FUNCTION("kn-rKRB0pfY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZnamSt11align_val_t);
    LIB_FUNCTION("s2eGsgUF9vk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZnamSt11align_val_tRKSt9nothrow_t);
    LIB_FUNCTION("ZRRBeuLmHjc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSbIwSt11char_traitsIwESaIwEE5_XlenEv);
    LIB_FUNCTION("GvYZax3i-Qk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSbIwSt11char_traitsIwESaIwEE5_XranEv);
    LIB_FUNCTION("pDtTdJ2sIz0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSs5_XlenEv);
    LIB_FUNCTION("AzBnOt1DouU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSs5_XranEv);
    LIB_FUNCTION("BbXxNgTW1x4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt10bad_typeid4whatEv);
    LIB_FUNCTION("WMw8eIs0kjM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt10bad_typeid8_DoraiseEv);
    LIB_FUNCTION("++ge3jYlHW8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt11logic_error4whatEv);
    LIB_FUNCTION("YTM5eyFGh2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt11logic_error8_DoraiseEv);
    LIB_FUNCTION("OzMS0BqVUGQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt12bad_weak_ptr4whatEv);
    LIB_FUNCTION("MwAySqTo+-M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt12codecvt_base11do_encodingEv);
    LIB_FUNCTION("FOsY+JAyXow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt12codecvt_base13do_max_lengthEv);
    LIB_FUNCTION("5sfbtNAdt-M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt12future_error4whatEv);
    LIB_FUNCTION("-syPONaWjqw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt12future_error8_DoraiseEv);
    LIB_FUNCTION("uWZBRxLAvEg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt12system_error8_DoraiseEv);
    LIB_FUNCTION("kTlQY47fo88", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt13bad_exception8_DoraiseEv);
    LIB_FUNCTION("2iW5Fv+kFxs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt13runtime_error4whatEv);
    LIB_FUNCTION("GthClwqQAZs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt14error_category10equivalentEiRKSt15error_condition);
    LIB_FUNCTION("9hB8AwIqQfs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt14error_category10equivalentERKSt10error_codei);
    LIB_FUNCTION("8SDojuZyQaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt14error_category23default_error_conditionEi);
    LIB_FUNCTION("XVu4--EWzcM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt17bad_function_call4whatEv);
    LIB_FUNCTION("+5IOQncui3c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt18bad_variant_access4whatEv);
    LIB_FUNCTION("u6UfGT9+HEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt19istreambuf_iteratorIcSt11char_traitsIcEE5equalERKS2_);
    LIB_FUNCTION("jZmLD-ASDto", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt19istreambuf_iteratorIwSt11char_traitsIwEE5equalERKS2_);
    LIB_FUNCTION("Q0VsWTapQ4M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt22_Future_error_category4nameEv);
    LIB_FUNCTION("nWfZplDjbxQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt22_Future_error_category7messageEi);
    LIB_FUNCTION("ww3UUl317Ng", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt22_System_error_category23default_error_conditionEi);
    LIB_FUNCTION("dXy+lFOiaQA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt22_System_error_category4nameEv);
    LIB_FUNCTION("HpAlvhNKb2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt22_System_error_category7messageEi);
    LIB_FUNCTION("xvVR0CBPFAM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt23_Generic_error_category4nameEv);
    LIB_FUNCTION("KZ++filsCL4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt23_Generic_error_category7messageEi);
    LIB_FUNCTION("sb2vivqtLS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt24_Iostream_error_category4nameEv);
    LIB_FUNCTION("n9-NJEULZ-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt24_Iostream_error_category7messageEi);
    LIB_FUNCTION("WXOoCK+kqwY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIcE10do_tolowerEc);
    LIB_FUNCTION("2w+4Mo2DPro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIcE10do_tolowerEPcPKc);
    LIB_FUNCTION("mnq3tbhCs34", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIcE10do_toupperEc);
    LIB_FUNCTION("7glioH0t9HM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIcE10do_toupperEPcPKc);
    LIB_FUNCTION("zwcNQT0Avck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIcE8do_widenEc);
    LIB_FUNCTION("W5OtP+WC800", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIcE8do_widenEPKcS2_Pc);
    LIB_FUNCTION("4SnCJmLL27U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIcE9do_narrowEcc);
    LIB_FUNCTION("-nCVE3kBjjA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIcE9do_narrowEPKcS2_cPc);
    LIB_FUNCTION("pSQz254t3ug", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE10do_scan_isEsPKwS2_);
    LIB_FUNCTION("Ej0X1EwApwM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE10do_tolowerEPwPKw);
    LIB_FUNCTION("ATYj6zra5W0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE10do_tolowerEw);
    LIB_FUNCTION("r1W-NtOi6E8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE10do_toupperEPwPKw);
    LIB_FUNCTION("JsZjB3TnZ8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE10do_toupperEw);
    LIB_FUNCTION("Kbe+LHOer9o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE11do_scan_notEsPKwS2_);
    LIB_FUNCTION("D0lUMKU+ELI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE5do_isEPKwS2_Ps);
    LIB_FUNCTION("rh7L-TliPoc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE5do_isEsw);
    LIB_FUNCTION("h3PbnNnZ-gI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE8do_widenEc);
    LIB_FUNCTION("KM0b6O8show", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE8do_widenEPKcS2_Pw);
    LIB_FUNCTION("Vf68JAD5rbM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE9do_narrowEPKwS2_cPc);
    LIB_FUNCTION("V+c0E+Uqcww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt5ctypeIwE9do_narrowEwc);
    LIB_FUNCTION("aUNISsPboqE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIcE11do_groupingEv);
    LIB_FUNCTION("uUDq10y4Raw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIcE13do_neg_formatEv);
    LIB_FUNCTION("E64hr8yXoXw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIcE13do_pos_formatEv);
    LIB_FUNCTION("VhyjwJugIe8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIcE14do_curr_symbolEv);
    LIB_FUNCTION("C3LC9A6SrVE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIcE14do_frac_digitsEv);
    LIB_FUNCTION("tZj4yquwuhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIcE16do_decimal_pointEv);
    LIB_FUNCTION("Rqu9OmkKY+M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIcE16do_negative_signEv);
    LIB_FUNCTION("ARZszYKvDWg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIcE16do_positive_signEv);
    LIB_FUNCTION("6aFwTNpqTP8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIcE16do_thousands_sepEv);
    LIB_FUNCTION("ckD5sIxo+Co", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIwE11do_groupingEv);
    LIB_FUNCTION("UzmR8lOfyqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIwE13do_neg_formatEv);
    LIB_FUNCTION("zF2GfKzBgqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIwE13do_pos_formatEv);
    LIB_FUNCTION("ypq5jFNRIJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIwE14do_curr_symbolEv);
    LIB_FUNCTION("ij-yZhH9YjY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIwE14do_frac_digitsEv);
    LIB_FUNCTION("v8P1X84ytFY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIwE16do_decimal_pointEv);
    LIB_FUNCTION("zkUC74aJxpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIwE16do_negative_signEv);
    LIB_FUNCTION("PWFePkVdv9w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIwE16do_positive_signEv);
    LIB_FUNCTION("XX+xiPXAN8I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7_MpunctIwE16do_thousands_sepEv);
    LIB_FUNCTION("iCWgjeqMHvg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIcc9_MbstatetE10do_unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("7tIwDZyyKHo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIcc9_MbstatetE16do_always_noconvEv);
    LIB_FUNCTION("TNexGlwiVEQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIcc9_MbstatetE2inERS0_PKcS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("14xKj+SV72o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIcc9_MbstatetE3outERS0_PKcS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("P+q1OLiErP0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIcc9_MbstatetE5do_inERS0_PKcS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("Uc+-Sx0UZ3U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIcc9_MbstatetE6do_outERS0_PKcS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("ikBt0lqkxPc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIcc9_MbstatetE6lengthERS0_PKcS4_m);
    LIB_FUNCTION("N7z+Dnk1cS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIcc9_MbstatetE7unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("Xk7IZcfHDD8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIcc9_MbstatetE9do_lengthERS0_PKcS4_m);
    LIB_FUNCTION("c6Lyc6xOp4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDic9_MbstatetE10do_unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("DDnr3lDwW8I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDic9_MbstatetE11do_encodingEv);
    LIB_FUNCTION("E5NdzqEmWuY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDic9_MbstatetE13do_max_lengthEv);
    LIB_FUNCTION("NQ81EZ7CL6w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDic9_MbstatetE16do_always_noconvEv);
    LIB_FUNCTION("PJ2UDX9Tvwg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDic9_MbstatetE5do_inERS0_PKcS4_RS4_PDiS6_RS6_);
    LIB_FUNCTION("eoW60zcLT8Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDic9_MbstatetE6do_outERS0_PKDiS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("m6rjfL4aMcA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDic9_MbstatetE9do_lengthERS0_PKcS4_m);
    LIB_FUNCTION("RYTHR81Y-Mc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDsc9_MbstatetE10do_unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("Mo6K-pUyNhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDsc9_MbstatetE11do_encodingEv);
    LIB_FUNCTION("BvRS0cGTd6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDsc9_MbstatetE13do_max_lengthEv);
    LIB_FUNCTION("9Vyfb-I-9xw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDsc9_MbstatetE16do_always_noconvEv);
    LIB_FUNCTION("+uPwCGfmJHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDsc9_MbstatetE5do_inERS0_PKcS4_RS4_PDsS6_RS6_);
    LIB_FUNCTION("0FKwlv9iH1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDsc9_MbstatetE6do_outERS0_PKDsS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("lynApfiP6Lw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIDsc9_MbstatetE9do_lengthERS0_PKcS4_m);
    LIB_FUNCTION("oDtGxrzLXMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIwc9_MbstatetE10do_unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("4fPIrH+z+E4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIwc9_MbstatetE11do_encodingEv);
    LIB_FUNCTION("5BQIjX7Y5YU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIwc9_MbstatetE13do_max_lengthEv);
    LIB_FUNCTION("KheIhkaSrlA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIwc9_MbstatetE16do_always_noconvEv);
    LIB_FUNCTION("WAPkmrXx2o8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIwc9_MbstatetE5do_inERS0_PKcS4_RS4_PwS6_RS6_);
    LIB_FUNCTION("ABFE75lbcDc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIwc9_MbstatetE6do_outERS0_PKwS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("G1zcPUEvY7U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7codecvtIwc9_MbstatetE9do_lengthERS0_PKcS4_m);
    LIB_FUNCTION("1eEXfeW6wrI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIcE10do_compareEPKcS2_S2_S2_);
    LIB_FUNCTION("gYlF567r3-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIcE12do_transformEPKcS2_);
    LIB_FUNCTION("6vYXzFD-mrk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIcE4hashEPKcS2_);
    LIB_FUNCTION("Q-8lQCGMj4g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIcE7compareEPKcS2_S2_S2_);
    LIB_FUNCTION("GSAXi4F1SlM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIcE7do_hashEPKcS2_);
    LIB_FUNCTION("XaSxLBnqcWE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIcE9transformEPKcS2_);
    LIB_FUNCTION("roztnFEs5Es", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIwE10do_compareEPKwS2_S2_S2_);
    LIB_FUNCTION("Zxe-nQMgxHM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIwE12do_transformEPKwS2_);
    LIB_FUNCTION("entYnoIu+fc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIwE4hashEPKwS2_);
    LIB_FUNCTION("n-3HFZvDwBw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIwE7compareEPKwS2_S2_S2_);
    LIB_FUNCTION("cWaCDW+Dc9U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIwE7do_hashEPKwS2_);
    LIB_FUNCTION("81uX7PzrtG8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7collateIwE9transformEPKwS2_);
    LIB_FUNCTION("OWO5cpNw3NA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERb);
    LIB_FUNCTION("mAwXCpkWaYc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERd);
    LIB_FUNCTION("wUCRGap1j0U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERe);
    LIB_FUNCTION("6RGkooTERsE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERf);
    LIB_FUNCTION("N1VqUWz2OEI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERj);
    LIB_FUNCTION("I2UzwkwwEPs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERl);
    LIB_FUNCTION("2bfL3yIBi5k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERm);
    LIB_FUNCTION("my9ujasm6-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERPv);
    LIB_FUNCTION("gozsp4urvq8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERt);
    LIB_FUNCTION("4hiQK82QuLc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERx);
    LIB_FUNCTION("eZfFLyWCkvg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERy);
    LIB_FUNCTION("SmtBNDda5qU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERb);
    LIB_FUNCTION("bNQpG-eKogg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERd);
    LIB_FUNCTION("uukWbYS6Bn4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERe);
    LIB_FUNCTION("IntAnFb+tw0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERf);
    LIB_FUNCTION("ywJpNe675zo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERj);
    LIB_FUNCTION("ALEXgLx9fqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERl);
    LIB_FUNCTION("Pq4PkG0x1fk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERm);
    LIB_FUNCTION("VKdXFE7ualw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERPv);
    LIB_FUNCTION("dRu2RLn4SKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERt);
    LIB_FUNCTION("F+AmVDFUyqM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERx);
    LIB_FUNCTION("TtYifKtVkYA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERy);
    LIB_FUNCTION("4+y8-2NsDw0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetffldEPcRS3_S6_RSt8ios_basePi);
    LIB_FUNCTION("G9LB1YD5-xc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6locale);
    LIB_FUNCTION("J-0I2PtiZc4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE9_GetffldxEPcRS3_S6_RSt8ios_basePi);
    LIB_FUNCTION("vW-nnV62ea4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERb);
    LIB_FUNCTION("+hjXHfvy1Mg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERd);
    LIB_FUNCTION("xLZr4GJRMLo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERe);
    LIB_FUNCTION("2mb8FYgER+E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERf);
    LIB_FUNCTION("Y3hBU5FYmhM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERj);
    LIB_FUNCTION("-m2YPwVCwJQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERl);
    LIB_FUNCTION("94ZLp2+AOq0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERm);
    LIB_FUNCTION("zomvAQ5RFdA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERPv);
    LIB_FUNCTION("bZ+lKHGvOr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERt);
    LIB_FUNCTION("cG5hQhjFGog", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERx);
    LIB_FUNCTION("banNSumaAZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERy);
    LIB_FUNCTION("wEU8oFtBXT8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERb);
    LIB_FUNCTION("t39dKpPEuVA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERd);
    LIB_FUNCTION("MCtJ9D7B5Cs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERe);
    LIB_FUNCTION("Gy2iRxp3LGk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERf);
    LIB_FUNCTION("2bUUbbcqHUo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERj);
    LIB_FUNCTION("QossXdwWltI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERl);
    LIB_FUNCTION("ig6SRr1GCU4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERm);
    LIB_FUNCTION("BNZq-mRvDS8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERPv);
    LIB_FUNCTION("kU7PvJJKUng", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERt);
    LIB_FUNCTION("Ou7GV51-ng4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERx);
    LIB_FUNCTION("rYLrGFoqfi4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateERy);
    LIB_FUNCTION("W5VYncHdreo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetffldEPcRS3_S6_RSt8ios_basePi);
    LIB_FUNCTION("GGqIV4cjzzI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6locale);
    LIB_FUNCTION("bZ0oEGQUKO8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE9_GetffldxEPcRS3_S6_RSt8ios_basePi);
    LIB_FUNCTION("nftirmo6hBg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecb);
    LIB_FUNCTION("w9NzCYAjEpQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecd);
    LIB_FUNCTION("VPcTGA-LwSo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basece);
    LIB_FUNCTION("ffnhh0HcxJ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecl);
    LIB_FUNCTION("uODuM76vS4U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecm);
    LIB_FUNCTION("8NVUcufbklM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecPKv);
    LIB_FUNCTION("NJtKruu9qOs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecx);
    LIB_FUNCTION("dep6W2Ix35s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecy);
    LIB_FUNCTION("k8zgjeBmpVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE4_PutES3_PKcm);
    LIB_FUNCTION("tCihLs4UJxQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE4_RepES3_cm);
    LIB_FUNCTION("w11G58-u4p8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_FfmtEPccNSt5_IosbIiE9_FmtflagsE);
    LIB_FUNCTION("ll99KkqO6ig", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_FputES3_RSt8ios_basecPKcm);
    LIB_FUNCTION("mNk6FfI8T7I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_FputES3_RSt8ios_basecPKcmmmm);
    LIB_FUNCTION("xlgA01CQtBo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_IfmtEPcPKcNSt5_IosbIiE9_FmtflagsE);
    LIB_FUNCTION("jykT-VWQVBY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_IputES3_RSt8ios_basecPcm);
    LIB_FUNCTION("ke36E2bqNmI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecb);
    LIB_FUNCTION("F+cp2B3cWNU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecd);
    LIB_FUNCTION("rLiFc4+HyHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basece);
    LIB_FUNCTION("I3+xmBWGPGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecl);
    LIB_FUNCTION("nlAk46weq1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecm);
    LIB_FUNCTION("0xgFRKf0Lc4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecPKv);
    LIB_FUNCTION("H2KGT3vA7yQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecx);
    LIB_FUNCTION("Vbeoft607aI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecy);
    LIB_FUNCTION("mY9FWooxqJY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewb);
    LIB_FUNCTION("V7aIsVIsIIA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewd);
    LIB_FUNCTION("vCIFGeI6adI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewe);
    LIB_FUNCTION("USLhWp7sZoU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewl);
    LIB_FUNCTION("qtpzdwMMCPc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewm);
    LIB_FUNCTION("xfOSCbCiY44", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewPKv);
    LIB_FUNCTION("ryykbHJ04Cw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewx);
    LIB_FUNCTION("lmb3oBpMNPU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewy);
    LIB_FUNCTION("kRGVhisjgMg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE4_PutES3_PKwm);
    LIB_FUNCTION("-b+Avqa2v9k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE4_RepES3_wm);
    LIB_FUNCTION("T07KcAOlIeU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_FfmtEPccNSt5_IosbIiE9_FmtflagsE);
    LIB_FUNCTION("IdV-tXejEGQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_FputES3_RSt8ios_basewPKcm);
    LIB_FUNCTION("B6JXVOMDdlw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_FputES3_RSt8ios_basewPKcmmmm);
    LIB_FUNCTION("WheFSRlZ9JA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_IfmtEPcPKcNSt5_IosbIiE9_FmtflagsE);
    LIB_FUNCTION("4pQ3B1BTMgo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_IputES3_RSt8ios_basewPcm);
    LIB_FUNCTION("1C2-2WB9NN4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewb);
    LIB_FUNCTION("sX3o6Zmihw0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewd);
    LIB_FUNCTION("6OYWLisfrB8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewe);
    LIB_FUNCTION("VpwhOe58wsM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewl);
    LIB_FUNCTION("jHo78LGEtmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewm);
    LIB_FUNCTION("BDteGj1gqBY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewPKv);
    LIB_FUNCTION("9SSHrlIamto", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewx);
    LIB_FUNCTION("uX0nKsUo8gc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewy);
    LIB_FUNCTION("6CPwoi-cFZM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8bad_cast4whatEv);
    LIB_FUNCTION("NEemVJeMwd0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8bad_cast8_DoraiseEv);
    LIB_FUNCTION("F27xQUBtNdU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8ios_base7failure8_DoraiseEv);
    LIB_FUNCTION("XxsPrrWJ52I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIcE3getEiiiRKSs);
    LIB_FUNCTION("U2t+WvMQj8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIcE4openERKSsRKSt6locale);
    LIB_FUNCTION("EeBQ7253LkE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIcE5closeEi);
    LIB_FUNCTION("vbgCuYKySLI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIcE6do_getEiiiRKSs);
    LIB_FUNCTION("HeBwePMtuFs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIcE7do_openERKSsRKSt6locale);
    LIB_FUNCTION("rRmMX83UL1E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIcE8do_closeEi);
    LIB_FUNCTION("Ea+awuQ5Bm8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIwE3getEiiiRKSbIwSt11char_traitsIwESaIwEE);
    LIB_FUNCTION("TPq0HfoACeI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIwE4openERKSsRKSt6locale);
    LIB_FUNCTION("GGoH7e6SZSY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIwE5closeEi);
    LIB_FUNCTION("UM6rGQxnEMg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIwE6do_getEiiiRKSbIwSt11char_traitsIwESaIwEE);
    LIB_FUNCTION("zSehLdHI0FA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIwE7do_openERKSsRKSt6locale);
    LIB_FUNCTION("AjkxQBlsOOY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8messagesIwE8do_closeEi);
    LIB_FUNCTION("cnNz2ftNwEU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE11do_groupingEv);
    LIB_FUNCTION("nRf0VQ++OEw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE11do_truenameEv);
    LIB_FUNCTION("ozLi0i4r6ds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE12do_falsenameEv);
    LIB_FUNCTION("klWxQ2nKAHY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE13decimal_pointEv);
    LIB_FUNCTION("QGSIlqfIU2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE13thousands_sepEv);
    LIB_FUNCTION("JXzQGOtumdM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE16do_decimal_pointEv);
    LIB_FUNCTION("zv1EMhI7R1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE16do_thousands_sepEv);
    LIB_FUNCTION("JWplGh2O0Rs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE8groupingEv);
    LIB_FUNCTION("fXUuZEw7C24", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE8truenameEv);
    LIB_FUNCTION("3+VwUA8-QPI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIcE9falsenameEv);
    LIB_FUNCTION("2BmJdX269kI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE11do_groupingEv);
    LIB_FUNCTION("nvSsAW7tcX8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE11do_truenameEv);
    LIB_FUNCTION("-amctzWbEtw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE12do_falsenameEv);
    LIB_FUNCTION("leSFwTZZuE4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE13decimal_pointEv);
    LIB_FUNCTION("2Olt9gqOauQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE13thousands_sepEv);
    LIB_FUNCTION("mzRlAVX65hQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE16do_decimal_pointEv);
    LIB_FUNCTION("Utj8Sh5L0jE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE16do_thousands_sepEv);
    LIB_FUNCTION("VsJCpXqMPJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE8groupingEv);
    LIB_FUNCTION("3M20pLo9Gdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE8truenameEv);
    LIB_FUNCTION("LDbKkgI-TZg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8numpunctIwE9falsenameEv);
    LIB_FUNCTION("ShlQcYrzRF8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE10date_orderEv);
    LIB_FUNCTION("T85u2sPrKOo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE11do_get_dateES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("73GV+sRHbeY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE11do_get_timeES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("dSfKN47p6ac", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE11do_get_yearES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("KwJ5V3D0v3A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE11get_weekdayES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("8PIh8BFpNYQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE13do_date_orderEv);
    LIB_FUNCTION("vvA7HtdtWnY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE13get_monthnameES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("xzYpD5d24aA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE14do_get_weekdayES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("ZuCHPDq-dPw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE16do_get_monthnameES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("+RuThw5axA4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmcc);
    LIB_FUNCTION("S5WbPO54nD0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmPKcSE_);
    LIB_FUNCTION("Vw03kdKZUN0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmcc);
    LIB_FUNCTION("E7UermPZVcw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetfmtES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmPKc);
    LIB_FUNCTION("8raXTYQ11cg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetintERS3_S5_iiRiRKSt5ctypeIcE);
    LIB_FUNCTION("OY5mqEBxP+8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8get_dateES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("rrqNi95bhMs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8get_timeES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("5L5Aft+9nZU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8get_yearES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("nc6OsiDx630", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE10date_orderEv);
    LIB_FUNCTION("SYCwZXKZQ08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE11do_get_dateES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("2pJJ0dl-aPQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE11do_get_timeES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("cRSJysDpVl4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE11do_get_yearES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("A0PftWMfrhk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE11get_weekdayES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("dP14OHWe4nI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE13do_date_orderEv);
    LIB_FUNCTION("xy0MR+OOZI8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE13get_monthnameES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("hGlkh5YpcKw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE14do_get_weekdayES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("R1ITHuTUMEI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE16do_get_monthnameES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("64pqofAwJEg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmcc);
    LIB_FUNCTION("B8c4P1vCixQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmPKwSE_);
    LIB_FUNCTION("0MzJAexrlr4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmcc);
    LIB_FUNCTION("r8003V6UwZg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetfmtES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tmPKc);
    LIB_FUNCTION("lhJWkEh-HXM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetintERS3_S5_iiRiRKSt5ctypeIwE);
    LIB_FUNCTION("kwp-0uidHpw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8get_dateES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("9TfGnN6xq-U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8get_timeES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("Krt-A7EnHHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8get_yearES3_S3_RSt8ios_baseRNSt5_IosbIiE8_IostateEP2tm);
    LIB_FUNCTION("qkuA-unH7PU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecPK2tmcc);
    LIB_FUNCTION("j9LU8GsuEGw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_RSt8ios_basecPK2tmPKcSB_);
    LIB_FUNCTION("+i81FtUCarA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_RSt8ios_basecPK2tmcc);
    LIB_FUNCTION("Nt6eyVKm+Z4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewPK2tmcc);
    LIB_FUNCTION("Sc0lXhQG5Ko", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_RSt8ios_basewPK2tmPKwSB_);
    LIB_FUNCTION("Fr7j8dMsy4s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_RSt8ios_basewPK2tmcc);
    LIB_FUNCTION("xvRvFtnUk3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9bad_alloc4whatEv);
    LIB_FUNCTION("pS-t9AJblSM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9bad_alloc8_DoraiseEv);
    LIB_FUNCTION("apPZ6HKZWaQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9exception4whatEv);
    LIB_FUNCTION("DuW5ZqZv-70", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9exception6_RaiseEv);
    LIB_FUNCTION("tyHd3P7oDrU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9exception8_DoraiseEv);
    LIB_FUNCTION("G84okRnyJJg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERe);
    LIB_FUNCTION("2fxdcyt5tGs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE3getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERSs);
    LIB_FUNCTION("IRVqdGwSNXE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERe);
    LIB_FUNCTION("D2njLPpEt1E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE6do_getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERSs);
    LIB_FUNCTION("CLT04GjI7UE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetmfldERS3_S5_bRSt8ios_basePc);
    LIB_FUNCTION("cx-1THpef1A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERe);
    LIB_FUNCTION("wWIsjOqfcSc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE3getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERSbIwS2_SaIwEE);
    LIB_FUNCTION("zzubCm+nDzc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERe);
    LIB_FUNCTION("DhXTD5eM7LQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE6do_getES3_S3_bRSt8ios_baseRNSt5_IosbIiE8_IostateERSbIwS2_SaIwEE);
    LIB_FUNCTION("RalOJcOXJJo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetmfldERS3_S5_bRSt8ios_basePw);
    LIB_FUNCTION("65cvm2NDLmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_bRSt8ios_basece);
    LIB_FUNCTION("DR029KeWsHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE3putES3_bRSt8ios_basecRKSs);
    LIB_FUNCTION("iXVrhA51z0M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_bRSt8ios_basece);
    LIB_FUNCTION("OR-4zyIi2aE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_bRSt8ios_basecRKSs);
    LIB_FUNCTION("d57FDzON1h0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE8_PutmfldES3_bRSt8ios_basecbSsc);
    LIB_FUNCTION("fsF-tGtGsD4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_bRSt8ios_basewe);
    LIB_FUNCTION("JruBeQgsAaU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE3putES3_bRSt8ios_basewRKSbIwS2_SaIwEE);
    LIB_FUNCTION("wVY5DpvU6PU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_bRSt8ios_basewe);
    LIB_FUNCTION("GDiCYtaiUyM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_bRSt8ios_basewRKSbIwS2_SaIwEE);
    LIB_FUNCTION("r-JSsJQFUsY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE8_PutmfldES3_bRSt8ios_basewbSbIwS2_SaIwEEw);
    LIB_FUNCTION("Ti86LmOKvr0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSbIwSt11char_traitsIwESaIwEE5_CopyEmm);
    LIB_FUNCTION("TgEb5a+nOnk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSbIwSt11char_traitsIwESaIwEE5eraseEmm);
    LIB_FUNCTION("nF8-CM+tro4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSbIwSt11char_traitsIwESaIwEE6appendEmw);
    LIB_FUNCTION("hSUcSStZEHM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSbIwSt11char_traitsIwESaIwEE6appendERKS2_mm);
    LIB_FUNCTION("8oO55jndPRg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSbIwSt11char_traitsIwESaIwEE6assignEmw);
    LIB_FUNCTION("IJmeA5ayVJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSbIwSt11char_traitsIwESaIwEE6assignEPKwm);
    LIB_FUNCTION("piJabTDQRVs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSbIwSt11char_traitsIwESaIwEE6assignERKS2_mm);
    LIB_FUNCTION("w2GyuoXCnkw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSbIwSt11char_traitsIwESaIwEE6insertEmmw);
    LIB_FUNCTION("6ZDv6ZusiFg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZNSiD0Ev);
    LIB_FUNCTION("tJU-ttrsXsk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZNSiD1Ev);
    LIB_FUNCTION("gVTWlvyBSIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSo6sentryC2ERSo);
    LIB_FUNCTION("nk+0yTWvoRE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSo6sentryD2Ev);
    LIB_FUNCTION("lTTrDj5OIwQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZNSoD0Ev);
    LIB_FUNCTION("HpCeP12cuNY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZNSoD1Ev);
    LIB_FUNCTION("9HILqEoh24E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSs5_CopyEmm);
    LIB_FUNCTION("0Ir3jiT4V6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSs5eraseEmm);
    LIB_FUNCTION("QqBWUNEfIAo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSs6appendEmc);
    LIB_FUNCTION("qiR-4jx1abE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSs6appendERKSsmm);
    LIB_FUNCTION("ikjnoeemspQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSs6assignEmc);
    LIB_FUNCTION("xSxPHmpcNzY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSs6assignEPKcm);
    LIB_FUNCTION("pGxNI4JKfmY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSs6assignERKSsmm);
    LIB_FUNCTION("KDgQWX1eDeo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSs6insertEmmc);
    LIB_FUNCTION("MHA0XR2YHoQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10bad_typeidD0Ev);
    LIB_FUNCTION("vzh0qoLIEuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10bad_typeidD1Ev);
    LIB_FUNCTION("tkZ7jVV6wJ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10bad_typeidD2Ev);
    LIB_FUNCTION("xGbaQPsHCFI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem10_Close_dirEPv);
    LIB_FUNCTION("PbCV7juCZVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem10_Copy_fileEPKcS1_);
    LIB_FUNCTION("SQ02ZA5E-UE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem10_File_sizeEPKc);
    LIB_FUNCTION("XD9FmX1mavU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem11_EquivalentEPKcS1_);
    LIB_FUNCTION("YDQxE4cIwa4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem11_Remove_dirEPKc);
    LIB_FUNCTION("8VKAqiw7lC0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem12_Current_getERA260_c);
    LIB_FUNCTION("Yl10kSufa5k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem12_Current_setEPKc);
    LIB_FUNCTION("HCB1auZdcmo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem16_Last_write_timeEPKc);
    LIB_FUNCTION("Wut42WAe7Rw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem18_Xfilesystem_errorEPKcRKNS_4pathES4_St10error_code);
    LIB_FUNCTION("C6-7Mo5WbwU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem18_Xfilesystem_errorEPKcRKNS_4pathESt10error_code);
    LIB_FUNCTION("B0CeIhQty7Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem18_Xfilesystem_errorEPKcSt10error_code);
    LIB_FUNCTION("VSk+sij2mwg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem20_Set_last_write_timeEPKcl);
    LIB_FUNCTION("EBwahsMLokw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem5_StatEPKcPNS_5permsE);
    LIB_FUNCTION("XyKw6Hs1P9Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem6_ChmodEPKcNS_5permsE);
    LIB_FUNCTION("o1qlZJqrvmI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem6_LstatEPKcPNS_5permsE);
    LIB_FUNCTION("srwl1hhFoUI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem7_RenameEPKcS1_);
    LIB_FUNCTION("O4mPool-pow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem7_ResizeEPKcm);
    LIB_FUNCTION("Iok1WdvAROg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem7_UnlinkEPKc);
    LIB_FUNCTION("SdKk439pgjg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem8_StatvfsEPKcRNS_10space_infoE);
    LIB_FUNCTION("x7pQExTeqBY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem9_Make_dirEPKcS1_);
    LIB_FUNCTION("8iuHpl+kg8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem9_Open_dirERA260_cPKcRiRNS_9file_typeE);
    LIB_FUNCTION("w5CGykBBU5M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10filesystem9_Read_dirERA260_cPvRNS_9file_typeE);
    LIB_FUNCTION("eF26YAKQWKA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EE2idE);
    LIB_FUNCTION("UbuTnKIXyCk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EE4intlE);
    LIB_FUNCTION("mU88GYCirhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("tYBLm0BoQdQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EEC1Em);
    LIB_FUNCTION("5afBJmEfUQI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EEC1EPKcm);
    LIB_FUNCTION("wrR3T5i7gpY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("5DFeXjP+Plg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EEC2Em);
    LIB_FUNCTION("aNfpdhcsMWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EEC2EPKcm);
    LIB_FUNCTION("uQFv8aNF8Jc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("sS5fF+fht2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EED0Ev);
    LIB_FUNCTION("3cW6MrkCKt0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EED1Ev);
    LIB_FUNCTION("mbGmSOLXgN0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb0EED2Ev);
    LIB_FUNCTION("PgiTG7nVxXE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EE2idE);
    LIB_FUNCTION("XhdnPX5bosc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EE4intlE);
    LIB_FUNCTION("BuxsERsopss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("nbTAoMwiO38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EEC1Em);
    LIB_FUNCTION("9S960jA8tB0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EEC1EPKcm);
    LIB_FUNCTION("TRn3cMU4mjY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("kPELiw9L-gw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EEC2Em);
    LIB_FUNCTION("RxJnJ-HoySc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EEC2EPKcm);
    LIB_FUNCTION("7e3DrnZea-Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("tcdvTUlPnL0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EED0Ev);
    LIB_FUNCTION("wT+HL7oqjYc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EED1Ev);
    LIB_FUNCTION("F7CUCpiasec", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIcLb1EED2Ev);
    LIB_FUNCTION("mhoxSElvH0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EE2idE);
    LIB_FUNCTION("D0gqPsqeZac", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EE4intlE);
    LIB_FUNCTION("0OjBJZd9VeM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("McUBYCqjLMg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EEC1Em);
    LIB_FUNCTION("jna5sqISK4s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EEC1EPKcm);
    LIB_FUNCTION("dHs7ndrQBiI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("bBGvmspg3Xs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EEC2Em);
    LIB_FUNCTION("5bQqdR3hTZw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EEC2EPKcm);
    LIB_FUNCTION("1kvQkOSaaVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("95MaQlRbfC8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EED0Ev);
    LIB_FUNCTION("ki5SQPsB4m4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EED1Ev);
    LIB_FUNCTION("6F1JfiING18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb0EED2Ev);
    LIB_FUNCTION("XUs40umcJLQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EE2idE);
    LIB_FUNCTION("8vEBRx0O1fc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EE4intlE);
    LIB_FUNCTION("HmcMLz3cPkM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("X69UlAXF-Oc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EEC1Em);
    LIB_FUNCTION("pyBabUesXN4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EEC1EPKcm);
    LIB_FUNCTION("uROsAczW6OU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("sTaUDDnGOpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EEC2Em);
    LIB_FUNCTION("GS1AvxBwVgY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EEC2EPKcm);
    LIB_FUNCTION("H0a2QXvgHOk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("fWuQSVGOivA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EED0Ev);
    LIB_FUNCTION("OM0FnA7Tldk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EED1Ev);
    LIB_FUNCTION("uVOxy7dQTFc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt10moneypunctIwLb1EED2Ev);
    LIB_FUNCTION("fn1i72X18Gs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt11logic_errorD0Ev);
    LIB_FUNCTION("i726T0BHbOU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt11logic_errorD1Ev);
    LIB_FUNCTION("wgDImKoGKCM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt11logic_errorD2Ev);
    LIB_FUNCTION("efXnxYFN5oE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt11range_errorD0Ev);
    LIB_FUNCTION("NnNaWa16OvE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt11range_errorD1Ev);
    LIB_FUNCTION("XgmUR6WSeXg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt11range_errorD2Ev);
    LIB_FUNCTION("ASUJmlcHSLo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt11regex_errorD0Ev);
    LIB_FUNCTION("gDsvnPIkLIE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt11regex_errorD1Ev);
    LIB_FUNCTION("X2wfcFYusTk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt11regex_errorD2Ev);
    LIB_FUNCTION("JyAoulEqA1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12bad_weak_ptrD0Ev);
    LIB_FUNCTION("jAO1IJKMhE4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12bad_weak_ptrD1Ev);
    LIB_FUNCTION("2R2j1QezUGM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12bad_weak_ptrD2Ev);
    LIB_FUNCTION("q89N9L8q8FU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12domain_errorD0Ev);
    LIB_FUNCTION("7P1Wm-5KgAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12domain_errorD1Ev);
    LIB_FUNCTION("AsShnG3DulM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12domain_errorD2Ev);
    LIB_FUNCTION("rNYLEsL7M0k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12future_errorD0Ev);
    LIB_FUNCTION("fuyXHeERajE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12future_errorD1Ev);
    LIB_FUNCTION("XFh0C66aEms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12future_errorD2Ev);
    LIB_FUNCTION("QS7CASjt4FU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12length_errorD0Ev);
    LIB_FUNCTION("n3y8Rn9hXJo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12length_errorD1Ev);
    LIB_FUNCTION("NjJfVHJL2Gg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12length_errorD2Ev);
    LIB_FUNCTION("TqvziWHetnw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12out_of_rangeD0Ev);
    LIB_FUNCTION("Kcb+MNSzZcc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12out_of_rangeD1Ev);
    LIB_FUNCTION("cCXMypoz4Vs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12out_of_rangeD2Ev);
    LIB_FUNCTION("+CnX+ZDO8qg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders2_1E);
    LIB_FUNCTION("GHsPYRKjheE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders2_2E);
    LIB_FUNCTION("X1C-YhubpGY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders2_3E);
    LIB_FUNCTION("fjnxuk9ucsE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders2_4E);
    LIB_FUNCTION("jxlpClEsfJQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders2_5E);
    LIB_FUNCTION("-cgB1bQG6jo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders2_6E);
    LIB_FUNCTION("Vj+KUu5khTE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders2_7E);
    LIB_FUNCTION("9f-LMAJYGCY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders2_8E);
    LIB_FUNCTION("RlB3+37KJaE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders2_9E);
    LIB_FUNCTION("b8ySy0pHgSQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_10E);
    LIB_FUNCTION("or0CNRlAEeE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_11E);
    LIB_FUNCTION("BO1r8DPhmyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_12E);
    LIB_FUNCTION("eeeT3NKKQZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_13E);
    LIB_FUNCTION("s0V1HJcZWEs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_14E);
    LIB_FUNCTION("94OiPulKcao", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_15E);
    LIB_FUNCTION("XOEdRCackI4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_16E);
    LIB_FUNCTION("pP76ElRLm78", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_17E);
    LIB_FUNCTION("WVB9rXLAUFs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_18E);
    LIB_FUNCTION("BE7U+QsixQA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_19E);
    LIB_FUNCTION("dFhgrqyzqhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12placeholders3_20E);
    LIB_FUNCTION("oly3wSwEJ2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12system_errorC2ESt10error_codePKc);
    LIB_FUNCTION("cyy-9ntjWT8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12system_errorD0Ev);
    LIB_FUNCTION("3qWXO9GTUYU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12system_errorD1Ev);
    LIB_FUNCTION("it6DDrqKGvo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt12system_errorD2Ev);
    LIB_FUNCTION("Ntg7gSs99PY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13_Num_int_base10is_boundedE);
    LIB_FUNCTION("90T0XESrYzU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13_Num_int_base10is_integerE);
    LIB_FUNCTION("WFTOZxDfpbQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13_Num_int_base14is_specializedE);
    LIB_FUNCTION("ongs2C6YZgA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13_Num_int_base5radixE);
    LIB_FUNCTION("VET8UnnaQKo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13_Num_int_base8is_exactE);
    LIB_FUNCTION("rZ5sEWyLqa4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13_Num_int_base9is_moduloE);
    LIB_FUNCTION("diSRws0Ppxg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13_Regex_traitsIcE6_NamesE);
    LIB_FUNCTION("xsRN6gUx-DE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13_Regex_traitsIwE6_NamesE);
    LIB_FUNCTION("lX9M5u0e48k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13bad_exceptionD0Ev);
    LIB_FUNCTION("t6egllDqQ2M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13bad_exceptionD1Ev);
    LIB_FUNCTION("iWNC2tkDgxw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13bad_exceptionD2Ev);
    LIB_FUNCTION("VNaqectsZNs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE4syncEv);
    LIB_FUNCTION("9biiDDejX3Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE5_LockEv);
    LIB_FUNCTION("qM0gUepGWT0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE5imbueERKSt6locale);
    LIB_FUNCTION("jfr41GGp2jA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE5uflowEv);
    LIB_FUNCTION("SYFNsz9K2rs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE6setbufEPci);
    LIB_FUNCTION("yofHspnD9us", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE7_UnlockEv);
    LIB_FUNCTION("7oio2Gs1GNk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE7seekoffElNSt5_IosbIiE8_SeekdirENS4_9_OpenmodeE);
    LIB_FUNCTION("rsS5cBMihAM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE7seekposESt4fposI9_MbstatetENSt5_IosbIiE9_OpenmodeE);
    LIB_FUNCTION("oYMRgkQHoJM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE8overflowEi);
    LIB_FUNCTION("JTwt9OTgk1k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE9_EndwriteEv);
    LIB_FUNCTION("jerxcj2Xnbg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE9pbackfailEi);
    LIB_FUNCTION("Nl6si1CfINw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEE9underflowEv);
    LIB_FUNCTION("MYCRRmc7cDA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEED0Ev);
    LIB_FUNCTION("Yc2gZRtDeNQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEED1Ev);
    LIB_FUNCTION("gOxGOQmSVU0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIcSt11char_traitsIcEED2Ev);
    LIB_FUNCTION("+WvmZi3216M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE4syncEv);
    LIB_FUNCTION("GYTma8zq0NU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE5_LockEv);
    LIB_FUNCTION("kmzNbhlkddA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE5imbueERKSt6locale);
    LIB_FUNCTION("VrXGNMTgNdE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE5uflowEv);
    LIB_FUNCTION("wAcnCK2HCeI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE6setbufEPwi);
    LIB_FUNCTION("ryl2DYMxlZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE7_UnlockEv);
    LIB_FUNCTION("g7gjCDEedJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE7seekoffElNSt5_IosbIiE8_SeekdirENS4_9_OpenmodeE);
    LIB_FUNCTION("10VcrHqHAlw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE7seekposESt4fposI9_MbstatetENSt5_IosbIiE9_OpenmodeE);
    LIB_FUNCTION("PjH5dZGfQHQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE8overflowEi);
    LIB_FUNCTION("cV6KpJiF0Ck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE9_EndwriteEv);
    LIB_FUNCTION("NeiFvKblpZM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE9pbackfailEi);
    LIB_FUNCTION("hXsvfky362s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEE9underflowEv);
    LIB_FUNCTION("JJ-mkOhdook", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEED0Ev);
    LIB_FUNCTION("XcuCO1YXaRs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEED1Ev);
    LIB_FUNCTION("aC9OWBGjvxA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_filebufIwSt11char_traitsIwEED2Ev);
    LIB_FUNCTION("StJaKYTRdUE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_istreamIwSt11char_traitsIwEED0Ev);
    LIB_FUNCTION("RP7ijkGGx50", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_istreamIwSt11char_traitsIwEED1Ev);
    LIB_FUNCTION("4GbIwW5u5us", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_ostreamIwSt11char_traitsIwEE6sentryC2ERS2_);
    LIB_FUNCTION("MB1VCygerRU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_ostreamIwSt11char_traitsIwEE6sentryD2Ev);
    LIB_FUNCTION("7VRfkz22vPk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_ostreamIwSt11char_traitsIwEED0Ev);
    LIB_FUNCTION("EYZJsnX58DE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13basic_ostreamIwSt11char_traitsIwEED1Ev);
    LIB_FUNCTION("94dk1V7XfYw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13runtime_errorD0Ev);
    LIB_FUNCTION("uBlwRfRb-CM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13runtime_errorD1Ev);
    LIB_FUNCTION("oe9tS0VztYk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt13runtime_errorD2Ev);
    LIB_FUNCTION("3CtP20nk8fs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Error_objectsIiE14_System_objectE);
    LIB_FUNCTION("fMfCVl0JvfE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Error_objectsIiE15_Generic_objectE);
    LIB_FUNCTION("D5m73fSIxAU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Error_objectsIiE16_Iostream_objectE);
    LIB_FUNCTION("y8PXwxTZ9Hc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base10has_denormE);
    LIB_FUNCTION("G4Pw4hv6NKc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base10is_boundedE);
    LIB_FUNCTION("Zwn1Rlbirio", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base10is_integerE);
    LIB_FUNCTION("M+F+0jd4+Y0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base11round_styleE);
    LIB_FUNCTION("f06wGEmo5Pk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base12has_infinityE);
    LIB_FUNCTION("xd7O9oMO+nI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base13has_quiet_NaNE);
    LIB_FUNCTION("8hyOiMUD36c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base14is_specializedE);
    LIB_FUNCTION("F+ehGYUe36Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base15has_denorm_lossE);
    LIB_FUNCTION("0JlZYApT0UM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base15tinyness_beforeE);
    LIB_FUNCTION("ec8jeC2LMOc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base17has_signaling_NaNE);
    LIB_FUNCTION("7tACjdACOBM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base5radixE);
    LIB_FUNCTION("7gc-QliZnMc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base5trapsE);
    LIB_FUNCTION("4PL4SkJXTos", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base8is_exactE);
    LIB_FUNCTION("tsiBm2NZQfo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base9is_iec559E);
    LIB_FUNCTION("c27lOSHxPA4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base9is_moduloE);
    LIB_FUNCTION("LV2FB+f1MJE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14_Num_ldbl_base9is_signedE);
    LIB_FUNCTION("g8Jw7V6mn8k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14error_categoryD2Ev);
    LIB_FUNCTION("KQTHP-ij0yo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIaE6digitsE);
    LIB_FUNCTION("btueF8F0fQE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIaE8digits10E);
    LIB_FUNCTION("iBrS+wbpuT0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIaE9is_signedE);
    LIB_FUNCTION("x1vTXM-GLCE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIbE6digitsE);
    LIB_FUNCTION("lnOqjnXNTwQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIbE8digits10E);
    LIB_FUNCTION("qOkciFIHghY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIbE9is_moduloE);
    LIB_FUNCTION("0mi6NtGz04Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIbE9is_signedE);
    LIB_FUNCTION("nlxVZWbqzsU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIcE6digitsE);
    LIB_FUNCTION("VVK0w0uxDLE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIcE8digits10E);
    LIB_FUNCTION("M+AMxjxwWlA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIcE9is_signedE);
    LIB_FUNCTION("hqVKCQr0vU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIdE12max_digits10E);
    LIB_FUNCTION("fjI2ddUGZZs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIdE12max_exponentE);
    LIB_FUNCTION("AwdlDnuQ6c0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIdE12min_exponentE);
    LIB_FUNCTION("VmOyIzWFNKs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIdE14max_exponent10E);
    LIB_FUNCTION("odyn6PGg5LY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIdE14min_exponent10E);
    LIB_FUNCTION("xQtNieUQLVg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIdE6digitsE);
    LIB_FUNCTION("EXW20cJ3oNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIdE8digits10E);
    LIB_FUNCTION("Zhtj6WalERg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIDiE6digitsE);
    LIB_FUNCTION("r1k-y+1yDcQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIDiE8digits10E);
    LIB_FUNCTION("TEMThaOLu+c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIDiE9is_signedE);
    LIB_FUNCTION("EL+4ceAj+UU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIDsE6digitsE);
    LIB_FUNCTION("vEdl5Er9THU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIDsE8digits10E);
    LIB_FUNCTION("ZaOkYNQyQ6g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIDsE9is_signedE);
    LIB_FUNCTION("u16WKNmQUNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIeE12max_digits10E);
    LIB_FUNCTION("bzmM0dI80jM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIeE12max_exponentE);
    LIB_FUNCTION("ERYMucecNws", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIeE12min_exponentE);
    LIB_FUNCTION("tUo2aRfWs5I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIeE14max_exponent10E);
    LIB_FUNCTION("3+5qZWL6APo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIeE14min_exponent10E);
    LIB_FUNCTION("NLHWcHpvMss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIeE6digitsE);
    LIB_FUNCTION("JYZigPvvB6c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIeE8digits10E);
    LIB_FUNCTION("MFqdrWyu9Ls", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIfE12max_digits10E);
    LIB_FUNCTION("L29QQz-6+X8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIfE12max_exponentE);
    LIB_FUNCTION("SPlcBQ4pIZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIfE12min_exponentE);
    LIB_FUNCTION("R8xUpEJwAA8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIfE14max_exponent10E);
    LIB_FUNCTION("n+NFkoa0VD0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIfE14min_exponent10E);
    LIB_FUNCTION("W6qgdoww-3k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIfE6digitsE);
    LIB_FUNCTION("J7d2Fq6Mb0k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIfE8digits10E);
    LIB_FUNCTION("T1YYqsPgrn0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIhE6digitsE);
    LIB_FUNCTION("uTiJLq4hayE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIhE8digits10E);
    LIB_FUNCTION("o0WexTj82pU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIhE9is_signedE);
    LIB_FUNCTION("ZvahxWPLKm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIiE6digitsE);
    LIB_FUNCTION("aQjlTguvFMw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIiE8digits10E);
    LIB_FUNCTION("GST3YemNZD8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIiE9is_signedE);
    LIB_FUNCTION("-jpk31lZR6E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIjE6digitsE);
    LIB_FUNCTION("csNIBfF6cyI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIjE8digits10E);
    LIB_FUNCTION("P9XP5U7AfXs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIjE9is_signedE);
    LIB_FUNCTION("31lJOpD3GGk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIlE6digitsE);
    LIB_FUNCTION("4MdGVqrsl7s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIlE8digits10E);
    LIB_FUNCTION("4llda2Y+Q+4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIlE9is_signedE);
    LIB_FUNCTION("7AaHj1O8-gI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsImE6digitsE);
    LIB_FUNCTION("h9RyP3R30HI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsImE8digits10E);
    LIB_FUNCTION("FXrK1DiAosQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsImE9is_signedE);
    LIB_FUNCTION("QO6Q+6WPgy0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIsE6digitsE);
    LIB_FUNCTION("kW5K7R4rXy8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIsE8digits10E);
    LIB_FUNCTION("L0nMzhz-axs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIsE9is_signedE);
    LIB_FUNCTION("4r9P8foa6QQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsItE6digitsE);
    LIB_FUNCTION("OQorbmM+NbA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsItE8digits10E);
    LIB_FUNCTION("vyqQpWI+O48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsItE9is_signedE);
    LIB_FUNCTION("Tlfgn9TIWkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIwE6digitsE);
    LIB_FUNCTION("mdcx6KcRIkE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIwE8digits10E);
    LIB_FUNCTION("YVacrIa4L0c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIwE9is_signedE);
    LIB_FUNCTION("LN2bC6QtGQE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIxE6digitsE);
    LIB_FUNCTION("OwcpepSk5lg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIxE8digits10E);
    LIB_FUNCTION("mmrSzkWDrgA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIxE9is_signedE);
    LIB_FUNCTION("v7XHt2HwUVI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIyE6digitsE);
    LIB_FUNCTION("Eubj+4g8dWA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIyE8digits10E);
    LIB_FUNCTION("F2uQDOc7fMo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14numeric_limitsIyE9is_signedE);
    LIB_FUNCTION("y1dYQsc67ys", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14overflow_errorD0Ev);
    LIB_FUNCTION("XilOsTdCZuM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14overflow_errorD1Ev);
    LIB_FUNCTION("OypvNf3Uq3c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt14overflow_errorD2Ev);
    LIB_FUNCTION("q-WOrJNOlhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base10has_denormE);
    LIB_FUNCTION("XbD-A2MEsS4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base10is_boundedE);
    LIB_FUNCTION("mxv24Oqmp0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base10is_integerE);
    LIB_FUNCTION("9AcX4Qk47+o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base11round_styleE);
    LIB_FUNCTION("MIKN--3fORE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base12has_infinityE);
    LIB_FUNCTION("nxdioQgDF2E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base13has_quiet_NaNE);
    LIB_FUNCTION("N03wZLr2RrE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base14is_specializedE);
    LIB_FUNCTION("rhJg5tjs83Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base15has_denorm_lossE);
    LIB_FUNCTION("EzuahjKzeGQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base15tinyness_beforeE);
    LIB_FUNCTION("uMMG8cuJNr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base17has_signaling_NaNE);
    LIB_FUNCTION("1KngsM7trps", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base5radixE);
    LIB_FUNCTION("mMPu4-jx9oI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base5trapsE);
    LIB_FUNCTION("J5QA0ZeLmhs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base8is_exactE);
    LIB_FUNCTION("JwPU+6+T20M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base9is_iec559E);
    LIB_FUNCTION("HU3yzCPz3GQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base9is_moduloE);
    LIB_FUNCTION("S7kkgAPGxLQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15_Num_float_base9is_signedE);
    LIB_FUNCTION("VpwymQiS4ck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15basic_streambufIcSt11char_traitsIcEE6xsgetnEPci);
    LIB_FUNCTION("sXaxl1QGorg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15basic_streambufIcSt11char_traitsIcEE6xsputnEPKci);
    LIB_FUNCTION("IAEl1Ta7yVU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15basic_streambufIcSt11char_traitsIcEE9showmanycEv);
    LIB_FUNCTION("lZVehk7yFok", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15basic_streambufIwSt11char_traitsIwEE6xsgetnEPwi);
    LIB_FUNCTION("041c37QfoUc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15basic_streambufIwSt11char_traitsIwEE6xsputnEPKwi);
    LIB_FUNCTION("olsoiZsezkk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15basic_streambufIwSt11char_traitsIwEE9showmanycEv);
    LIB_FUNCTION("iHILAmwYRGY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15underflow_errorD0Ev);
    LIB_FUNCTION("ywv2X-q-9is", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15underflow_errorD1Ev);
    LIB_FUNCTION("xiqd+QkuYXc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt15underflow_errorD2Ev);
    LIB_FUNCTION("1GhiIeIpkms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt16invalid_argumentD0Ev);
    LIB_FUNCTION("oQDS9nX05Qg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt16invalid_argumentD1Ev);
    LIB_FUNCTION("ddr7Ie4u5Nw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt16invalid_argumentD2Ev);
    LIB_FUNCTION("za50kXyi3SA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt16nested_exceptionD0Ev);
    LIB_FUNCTION("+qKS53qzWdA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt16nested_exceptionD1Ev);
    LIB_FUNCTION("8R00hgzXQDY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt16nested_exceptionD2Ev);
    LIB_FUNCTION("q9rMtHuXvZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt17bad_function_callD0Ev);
    LIB_FUNCTION("YEDrb1pSx2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt17bad_function_callD1Ev);
    LIB_FUNCTION("NqMgmxSA1rc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt17bad_function_callD2Ev);
    LIB_FUNCTION("8DNJW5tX-A8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt18bad_variant_accessD0Ev);
    LIB_FUNCTION("U3b5A2LEiTc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt18bad_variant_accessD1Ev);
    LIB_FUNCTION("QUeUgxy7PTA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt20_Future_error_objectIiE14_Future_objectE);
    LIB_FUNCTION("-UKRka-33sM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt20bad_array_new_lengthD0Ev);
    LIB_FUNCTION("XO3N4SBvCy0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt20bad_array_new_lengthD1Ev);
    LIB_FUNCTION("15lB7flw-9w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt20bad_array_new_lengthD2Ev);
    LIB_FUNCTION("WDKzMM-uuLE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt22_Future_error_categoryD0Ev);
    LIB_FUNCTION("xsXQD5ybREw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt22_Future_error_categoryD1Ev);
    LIB_FUNCTION("Dc4ZMWmPMl8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt22_System_error_categoryD0Ev);
    LIB_FUNCTION("hVQgfGhJz3U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt22_System_error_categoryD1Ev);
    LIB_FUNCTION("YBrp9BlADaA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt23_Generic_error_categoryD0Ev);
    LIB_FUNCTION("MAalgQhejPc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt23_Generic_error_categoryD1Ev);
    LIB_FUNCTION("jVwxMhFRw8Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt24_Iostream_error_categoryD0Ev);
    LIB_FUNCTION("27Z-Cx1jbkU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt24_Iostream_error_categoryD1Ev);
    LIB_FUNCTION("9G32u5RRYxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt3pmr19new_delete_resourceEv);
    LIB_FUNCTION("d2u38zs4Pe8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt3pmr20get_default_resourceEv);
    LIB_FUNCTION("eWMGI7B7Lyc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt3pmr20null_memory_resourceEv);
    LIB_FUNCTION("TKYsv0jdvRw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt3pmr20set_default_resourceEPNS_15memory_resourceE);
    LIB_FUNCTION("H7-7Z3ixv-w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt4_Pad7_LaunchEPKcPP12pthread_attrPP7pthread);
    LIB_FUNCTION("PBbZjsL6nfc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt4_Pad7_LaunchEPKcPP7pthread);
    LIB_FUNCTION("fLBZMOQh-3Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt4_Pad7_LaunchEPP12pthread_attrPP7pthread);
    LIB_FUNCTION("xZqiZvmcp9k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt4_Pad7_LaunchEPP7pthread);
    LIB_FUNCTION("a-z7wxuYO2E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt4_Pad8_ReleaseEv);
    LIB_FUNCTION("uhnb6dnXOnc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt4_PadC2EPKc);
    LIB_FUNCTION("dGYo9mE8K2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt4_PadC2Ev);
    LIB_FUNCTION("XyJPhPqpzMw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt4_PadD1Ev);
    LIB_FUNCTION("gjLRZgfb3i0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt4_PadD2Ev);
    LIB_FUNCTION("rX58aCQCMS4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt5ctypeIcE10table_sizeE);
    LIB_FUNCTION("Cv+zC4EjGMA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt5ctypeIcE2idE);
    LIB_FUNCTION("p8-44cVeO84", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt5ctypeIcED0Ev);
    LIB_FUNCTION("tPsGA6EzNKA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt5ctypeIcED1Ev);
    LIB_FUNCTION("VmqsS6auJzo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt5ctypeIwE2idE);
    LIB_FUNCTION("zOPA5qnbW2U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt5ctypeIwED0Ev);
    LIB_FUNCTION("P0383AW3Y9A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt5ctypeIwED1Ev);
    LIB_FUNCTION("U54NBtdj6UY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_Mutex5_LockEv);
    LIB_FUNCTION("7OCTkL2oWyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_Mutex7_UnlockEv);
    LIB_FUNCTION("2KNnG2Z9zJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_MutexC1Ev);
    LIB_FUNCTION("log6zy2C9iQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_MutexC2Ev);
    LIB_FUNCTION("djHbPE+TFIo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_MutexD1Ev);
    LIB_FUNCTION("j7e7EQBD6ZA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_MutexD2Ev);
    LIB_FUNCTION("0WY1SH7eoIs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_Winit9_Init_cntE);
    LIB_FUNCTION("-Bl9-SZ2noc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_WinitC1Ev);
    LIB_FUNCTION("57mMrw0l-40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_WinitC2Ev);
    LIB_FUNCTION("Uw3OTZFPNt4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_WinitD1Ev);
    LIB_FUNCTION("2yOarodWACE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6_WinitD2Ev);
    LIB_FUNCTION("z83caOn94fM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6chrono12steady_clock12is_monotonicE);
    LIB_FUNCTION("vHy+a4gLBfs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6chrono12steady_clock9is_steadyE);
    LIB_FUNCTION("jCX3CPIVB8I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6chrono12system_clock12is_monotonicE);
    LIB_FUNCTION("88EyUEoBX-E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6chrono12system_clock9is_steadyE);
    LIB_FUNCTION("hEQ2Yi4PJXA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale16_GetgloballocaleEv);
    LIB_FUNCTION("1TaQLyPDJEY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale16_SetgloballocaleEPv);
    LIB_FUNCTION("H4fcpQOpc08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale2id7_Id_cntE);
    LIB_FUNCTION("9rMML086SEE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale5_InitEv);
    LIB_FUNCTION("K-5mtupQZ4g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale5emptyEv);
    LIB_FUNCTION("AgxEl+HeWRQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale5facet7_DecrefEv);
    LIB_FUNCTION("-EgSegeAKl4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale5facet7_IncrefEv);
    LIB_FUNCTION("QW2jL1J5rwY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale5facet9_RegisterEv);
    LIB_FUNCTION("ptwhA0BQVeE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale6globalERKS_);
    LIB_FUNCTION("uuga3RipCKQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_Locimp7_AddfacEPNS_5facetEm);
    LIB_FUNCTION("9FF+T5Xks9E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_Locimp8_ClocptrE);
    LIB_FUNCTION("5r801ZWiJJI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_Locimp8_MakelocERKSt8_LocinfoiPS0_PKS_);
    LIB_FUNCTION("BcbHFSrcg3Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_Locimp9_MakewlocERKSt8_LocinfoiPS0_PKS_);
    LIB_FUNCTION("fkFGlPdquqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_Locimp9_MakexlocERKSt8_LocinfoiPS0_PKS_);
    LIB_FUNCTION("6b3KIjPD33k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_LocimpC1Eb);
    LIB_FUNCTION("WViwxtEKxHk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_LocimpC1ERKS0_);
    LIB_FUNCTION("zrmR88ClfOs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_LocimpC2Eb);
    LIB_FUNCTION("dsJKehuajH4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_LocimpC2ERKS0_);
    LIB_FUNCTION("bleKr8lOLr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_LocimpD0Ev);
    LIB_FUNCTION("aD-iqbVlHmQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_LocimpD1Ev);
    LIB_FUNCTION("So6gSmJMYDs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7_LocimpD2Ev);
    LIB_FUNCTION("Uq5K8tl8I9U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6locale7classicEv);
    LIB_FUNCTION("pMWnITHysPc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6localeD1Ev);
    LIB_FUNCTION("CHrhwd8QSBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt6thread20hardware_concurrencyEv);
    LIB_FUNCTION("m7qAgircaZY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIcE5_InitERKSt8_Locinfob);
    LIB_FUNCTION("zWSNYg14Uag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIcEC2Emb);
    LIB_FUNCTION("0il9qdo6fhs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIcEC2EPKcmbb);
    LIB_FUNCTION("Lzj4ws7DlhQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIcED0Ev);
    LIB_FUNCTION("0AeC+qCELEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIcED1Ev);
    LIB_FUNCTION("iCoD0EOIbTM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIwE5_InitERKSt8_Locinfob);
    LIB_FUNCTION("Pr1yLzUe230", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIwEC2Emb);
    LIB_FUNCTION("TDhjx3nyaoU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIwEC2EPKcmbb);
    LIB_FUNCTION("8UeuxGKjQr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIwED0Ev);
    LIB_FUNCTION("0TADyPWrobI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7_MpunctIwED1Ev);
    LIB_FUNCTION("eVFYZnYNDo0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetE2idE);
    LIB_FUNCTION("iZCHNahj++4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("zyhiiLKndO8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetE7_GetcatEPPKNSt6locale5facetEPKS2_);
    LIB_FUNCTION("XhwSbwsBdx0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetEC1Em);
    LIB_FUNCTION("3YCLxZqgIdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetEC1ERKSt8_Locinfom);
    LIB_FUNCTION("e5Hwcntvd8c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetEC2Em);
    LIB_FUNCTION("4qHwSTPt-t8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetEC2ERKSt8_Locinfom);
    LIB_FUNCTION("2TYdayAO39E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetED0Ev);
    LIB_FUNCTION("djNkrJKTb6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetED1Ev);
    LIB_FUNCTION("to7GggwECZU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIcc9_MbstatetED2Ev);
    LIB_FUNCTION("u2MAta5SS84", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIDic9_MbstatetE2idE);
    LIB_FUNCTION("vwMx2NhWdLw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIDic9_MbstatetED0Ev);
    LIB_FUNCTION("TuhGCIxgLvA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIDic9_MbstatetED1Ev);
    LIB_FUNCTION("xM5re58mxj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIDsc9_MbstatetE2idE);
    LIB_FUNCTION("zYHryd8vd0w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIDsc9_MbstatetED0Ev);
    LIB_FUNCTION("Oeo9tUbzW7s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIDsc9_MbstatetED1Ev);
    LIB_FUNCTION("FjZCPmK0SbA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIwc9_MbstatetE2idE);
    LIB_FUNCTION("9BI3oYkCTCU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIwc9_MbstatetED0Ev);
    LIB_FUNCTION("0fkFA3za2N8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7codecvtIwc9_MbstatetED1Ev);
    LIB_FUNCTION("7brRfHVVAlI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcE2idE);
    LIB_FUNCTION("CKlZ-H-D1CE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("BSVJqITGCyI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("Oo1r8jKGZQ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcEC1Em);
    LIB_FUNCTION("splBMMcF3yk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcEC1EPKcm);
    LIB_FUNCTION("raLgIUi3xmk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcEC1ERKSt8_Locinfom);
    LIB_FUNCTION("tkqNipin1EI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcEC2Em);
    LIB_FUNCTION("VClCrMDyydE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcEC2EPKcm);
    LIB_FUNCTION("L71JAnoQees", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcEC2ERKSt8_Locinfom);
    LIB_FUNCTION("Lt4407UMs2o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcED0Ev);
    LIB_FUNCTION("8pXCeme0FC4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcED1Ev);
    LIB_FUNCTION("dP5zwQ2Yc8g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIcED2Ev);
    LIB_FUNCTION("irGo1yaJ-vM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwE2idE);
    LIB_FUNCTION("LxKs-IGDsFU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("2wz4rthdiy8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("d-MOtyu8GAk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwEC1Em);
    LIB_FUNCTION("fjHAU8OSaW8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwEC1EPKcm);
    LIB_FUNCTION("wggIIjWSt-E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwEC1ERKSt8_Locinfom);
    LIB_FUNCTION("HQbgeUdQyyw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwEC2Em);
    LIB_FUNCTION("PSAw7g1DD24", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwEC2EPKcm);
    LIB_FUNCTION("2PoQu-K2qXk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwEC2ERKSt8_Locinfom);
    LIB_FUNCTION("ig4VDIRc21Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwED0Ev);
    LIB_FUNCTION("ZO3a6HfALTQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwED1Ev);
    LIB_FUNCTION("84wIPnwBGiU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7collateIwED2Ev);
    LIB_FUNCTION("-mLzBSk-VGs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("cXL+LN0lSwc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("p1WMhxL4Wds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("uXj-oWD2334", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1Em);
    LIB_FUNCTION("iTODM3uXS2s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("RNmYVYlZvv8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2Em);
    LIB_FUNCTION("yAobGI2Whrg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("-1G1iE3KyGI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED0Ev);
    LIB_FUNCTION("kAay0hfgDJs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED1Ev);
    LIB_FUNCTION("6S8jzWWGcWo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED2Ev);
    LIB_FUNCTION("NFAhHKyMnCg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("4MHgRGOKOXY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("zTX7LL+w12Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("18rLbEV-NQs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1Em);
    LIB_FUNCTION("0UPU3kvxWb0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("-+RKa3As0gE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2Em);
    LIB_FUNCTION("e3shgCIZxRc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("aHDdLa7jA1U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED0Ev);
    LIB_FUNCTION("Zbaaq-d70ms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED1Ev);
    LIB_FUNCTION("bwVJf3kat9c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED2Ev);
    LIB_FUNCTION("E14mW8pVpoE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("BbJ4naWZeRw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("hNAh1l09UpA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("LAEVU8cBSh4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1Em);
    LIB_FUNCTION("Hg1im-rUeHc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("1gYJIrfHxkQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2Em);
    LIB_FUNCTION("Mniutm2JL2M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("aOK5ucXO-5g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED0Ev);
    LIB_FUNCTION("WoCt9o2SYHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED1Ev);
    LIB_FUNCTION("U4JP-R+-70c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED2Ev);
    LIB_FUNCTION("kImHEIWZ58Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("V2FICbvPa+s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("6iDi6e2e4x8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("xdHqQoggdfo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1Em);
    LIB_FUNCTION("Ky+C-qbKcX0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("f1ZGLUnQGgo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2Em);
    LIB_FUNCTION("0Pd-K5jGcgM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("jyXTVnmlJD4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED0Ev);
    LIB_FUNCTION("WiUy3dEtCOQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED1Ev);
    LIB_FUNCTION("6hV3y21d59k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED2Ev);
    LIB_FUNCTION("WkAsdy5CUAo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8_Locinfo8_AddcatsEiPKc);
    LIB_FUNCTION("L1Ze94yof2I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8_LocinfoC1EiPKc);
    LIB_FUNCTION("hqi8yMOCmG0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8_LocinfoC1EPKc);
    LIB_FUNCTION("2aSk2ruCP0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8_LocinfoC1ERKSs);
    LIB_FUNCTION("i180MNC9p4c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8_LocinfoC2EiPKc);
    LIB_FUNCTION("pN02FS5SPgg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8_LocinfoC2EPKc);
    LIB_FUNCTION("ReK9U6EUWuU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8_LocinfoC2ERKSs);
    LIB_FUNCTION("p6LrHjIQMdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8_LocinfoD1Ev);
    LIB_FUNCTION("YXVCU6PdgZk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8_LocinfoD2Ev);
    LIB_FUNCTION("2MK5Lr9pgQc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8bad_castD0Ev);
    LIB_FUNCTION("47RvLSo2HN8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8bad_castD1Ev);
    LIB_FUNCTION("rF07weLXJu8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8bad_castD2Ev);
    LIB_FUNCTION("QZb07KKwTU0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base4Init9_Init_cntE);
    LIB_FUNCTION("sqWytnhYdEg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base4InitC1Ev);
    LIB_FUNCTION("bTQcNwRc8hE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base4InitC2Ev);
    LIB_FUNCTION("kxXCvcat1cM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base4InitD1Ev);
    LIB_FUNCTION("bxLH5WHgMBY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base4InitD2Ev);
    LIB_FUNCTION("8tL6yJaX1Ro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base5_SyncE);
    LIB_FUNCTION("QXJCcrXoqpU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base5clearENSt5_IosbIiE8_IostateEb);
    LIB_FUNCTION("4EkPKYzOjPc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base6_IndexE);
    LIB_FUNCTION("LTov9gMEqCU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base7_AddstdEPS_);
    LIB_FUNCTION("x7vtyar1sEY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base7failureD0Ev);
    LIB_FUNCTION("N2f485TmJms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base7failureD1Ev);
    LIB_FUNCTION("fjG5plxblj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_base7failureD2Ev);
    LIB_FUNCTION("I5jcbATyIWo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_baseD0Ev);
    LIB_FUNCTION("X9D8WWSG3As", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_baseD1Ev);
    LIB_FUNCTION("P8F2oavZXtY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8ios_baseD2Ev);
    LIB_FUNCTION("lA+PfiZ-S5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcE2idE);
    LIB_FUNCTION("eLB2+1+mVvg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("96Ev+CE1luE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("6gCBQs1mIi4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcEC1Em);
    LIB_FUNCTION("W0w8TGzAu0Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcEC1EPKcm);
    LIB_FUNCTION("SD403oMc1pQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcEC1ERKSt8_Locinfom);
    LIB_FUNCTION("6DBUo0dty1k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcEC2Em);
    LIB_FUNCTION("qF3mHeMAHVk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcEC2EPKcm);
    LIB_FUNCTION("969Euioo12Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcEC2ERKSt8_Locinfom);
    LIB_FUNCTION("jy9urODH0Wo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcED0Ev);
    LIB_FUNCTION("34mi8lteNTs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcED1Ev);
    LIB_FUNCTION("yDdbQr1oLOc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIcED2Ev);
    LIB_FUNCTION("n1Y6pGR-8AU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwE2idE);
    LIB_FUNCTION("Zz-RfDtowlo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("XghI4vmw8mU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("n4+3hznhkU4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwEC1Em);
    LIB_FUNCTION("4Srtnk+NpC4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwEC1EPKcm);
    LIB_FUNCTION("RrTMGyPhYU4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwEC1ERKSt8_Locinfom);
    LIB_FUNCTION("x8PFBjJhH7E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwEC2Em);
    LIB_FUNCTION("DlDsyX+XsoA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwEC2EPKcm);
    LIB_FUNCTION("DDQjbwNC31E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwEC2ERKSt8_Locinfom);
    LIB_FUNCTION("gMwkpZNI9Us", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwED0Ev);
    LIB_FUNCTION("6sAaleB7Zgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwED1Ev);
    LIB_FUNCTION("I-e7Dxo087A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8messagesIwED2Ev);
    LIB_FUNCTION("9iXtwvGVFRI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcE2idE);
    LIB_FUNCTION("1LvbNeZZJ-o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcE5_InitERKSt8_Locinfob);
    LIB_FUNCTION("fFnht9SPed8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcE5_TidyEv);
    LIB_FUNCTION("zCB24JBovnQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("TEtyeXjcZ0w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcEC1Em);
    LIB_FUNCTION("WK24j1F3rCU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcEC1EPKcmb);
    LIB_FUNCTION("CDm+TUClE7E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("1eVdDzPtzD4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcEC2Em);
    LIB_FUNCTION("yIn4l8OO1zA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcEC2EPKcmb);
    LIB_FUNCTION("Cb1hI+w9nyU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("Lf6h5krl2fA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcED0Ev);
    LIB_FUNCTION("qEob3o53s2M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcED1Ev);
    LIB_FUNCTION("xFva4yxsVW8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIcED2Ev);
    LIB_FUNCTION("XZNi3XtbWQ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwE2idE);
    LIB_FUNCTION("uiRALKOdAZk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwE5_InitERKSt8_Locinfob);
    LIB_FUNCTION("2YCDWkuFEy8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwE5_TidyEv);
    LIB_FUNCTION("SdXFaufpLIs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("XOgjMgZ3fjo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwEC1Em);
    LIB_FUNCTION("H+T2VJ91dds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwEC1EPKcmb);
    LIB_FUNCTION("s1EM2NdPf0Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("ElKI+ReiehU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwEC2Em);
    LIB_FUNCTION("m4kEqv7eGVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwEC2EPKcmb);
    LIB_FUNCTION("MQJQCxbLfM0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("VHBnRBxBg5E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwED0Ev);
    LIB_FUNCTION("lzK3uL1rWJY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwED1Ev);
    LIB_FUNCTION("XDm4jTtoEbo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8numpunctIwED2Ev);
    LIB_FUNCTION("a54t8+k7KpY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("+yrOX7MgVlk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE5_TidyEv);
    LIB_FUNCTION("eMnBe5mZFLw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("13xzrgS8N4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1Em);
    LIB_FUNCTION("9pPbNXw5N9E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1EPKcm);
    LIB_FUNCTION("iO5AOflrTaA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("dU8Q2yzFNQg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2Em);
    LIB_FUNCTION("M0n7l76UVyE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2EPKcm);
    LIB_FUNCTION("l7OtvplI42U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("mmq9OwwYx74", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED0Ev);
    LIB_FUNCTION("Cp9ksNOeun8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED1Ev);
    LIB_FUNCTION("dOKh96qQFd0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED2Ev);
    LIB_FUNCTION("Q17eavfOw2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("ImblNB7fVVU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE5_TidyEv);
    LIB_FUNCTION("e5jQyuEE+9U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("J2xO4cttypo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1Em);
    LIB_FUNCTION("gOzIhGUAkME", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1EPKcm);
    LIB_FUNCTION("y0hzUSFrkng", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("p-SW25yE-Q8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2Em);
    LIB_FUNCTION("XBmd6G-HoYI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2EPKcm);
    LIB_FUNCTION("bU3S1OS1sc0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("8H3yBUytv-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED0Ev);
    LIB_FUNCTION("QTgRx1NTp6o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED1Ev);
    LIB_FUNCTION("Zqc++JB04Qs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED2Ev);
    LIB_FUNCTION("BamOsNbUcn4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("QdPT7uDTlo0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("ec0YLGHS8cw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("6htjEH2Gi-w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1Em);
    LIB_FUNCTION("u5yK3bGG1+w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("6NH0xVj6p7M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2Em);
    LIB_FUNCTION("IuFImJ5+kTk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("WQQlL0n2SpU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED0Ev);
    LIB_FUNCTION("h+c9OSfCAEg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED1Ev);
    LIB_FUNCTION("vu0B+BGlol4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED2Ev);
    LIB_FUNCTION("JFiji2DpvXQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("ZolDcuDSD0Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("bF2uVCqVhBY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("X3DrtC2AZCI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1Em);
    LIB_FUNCTION("oi3kpQPqpMY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("lOF5jrFNZUA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2Em);
    LIB_FUNCTION("b1LciG4lUUk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("6yplvTHbxpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED0Ev);
    LIB_FUNCTION("CiD6-BPDZrA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED1Ev);
    LIB_FUNCTION("8PJ9qmklj2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED2Ev);
    LIB_FUNCTION("cDHRgSXYdqA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base10has_denormE);
    LIB_FUNCTION("v9HHsaa42qE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base10is_boundedE);
    LIB_FUNCTION("EgSIYe3IYso", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base10is_integerE);
    LIB_FUNCTION("XXiGcYa5wtg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base11round_styleE);
    LIB_FUNCTION("98w+P+GuFMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base12has_infinityE);
    LIB_FUNCTION("qeA5qUg9xBk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base12max_digits10E);
    LIB_FUNCTION("E4gWXl6V2J0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base12max_exponentE);
    LIB_FUNCTION("KqdclsYd24w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base12min_exponentE);
    LIB_FUNCTION("gF5aGNmzWSg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base13has_quiet_NaNE);
    LIB_FUNCTION("RCWKbkEaDAU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base14is_specializedE);
    LIB_FUNCTION("Dl4hxL59YF4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base14max_exponent10E);
    LIB_FUNCTION("zBHGQsN5Yfw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base14min_exponent10E);
    LIB_FUNCTION("96Bg8h09w+o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base15has_denorm_lossE);
    LIB_FUNCTION("U0FdtOUjUPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base15tinyness_beforeE);
    LIB_FUNCTION("fSdpGoYfYs8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base17has_signaling_NaNE);
    LIB_FUNCTION("Xb9FhMysEHo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base5radixE);
    LIB_FUNCTION("suaBxzlL0p0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base5trapsE);
    LIB_FUNCTION("ejBz8a8TCWU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base6digitsE);
    LIB_FUNCTION("M-KRaPj9tQQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base8digits10E);
    LIB_FUNCTION("bUQLE6uEu10", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base8is_exactE);
    LIB_FUNCTION("0ZdjsAWtbG8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base9is_iec559E);
    LIB_FUNCTION("qO4MLGs1o58", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base9is_moduloE);
    LIB_FUNCTION("5DzttCF356U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9_Num_base9is_signedE);
    LIB_FUNCTION("qb6A7pSgAeY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9bad_allocD0Ev);
    LIB_FUNCTION("khbdMADH4cQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9bad_allocD1Ev);
    LIB_FUNCTION("WiH8rbVv5s4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9bad_allocD2Ev);
    LIB_FUNCTION("UQPicLg8Sx8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9basic_iosIcSt11char_traitsIcEE4initEPSt15basic_streambufIcS1_Eb);
    LIB_FUNCTION("uqLGWOX7-YE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9basic_iosIwSt11char_traitsIwEE4initEPSt15basic_streambufIwS1_Eb);
    LIB_FUNCTION("Bin7e2UR+a0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9exception18_Set_raise_handlerEPFvRKS_E);
    LIB_FUNCTION("+Nc8JGdVLQs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9exceptionD0Ev);
    LIB_FUNCTION("BgZcGDh7o9g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9exceptionD1Ev);
    LIB_FUNCTION("MOBxtefPZUg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9exceptionD2Ev);
    LIB_FUNCTION("TsGewdW9Rto", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("6zg3ziZ4Qis", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("MSSvHmcbs3g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("YGPopdkhujM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1Em);
    LIB_FUNCTION("7NQGsY7VY3c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("f+1EaDVL5C4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2Em);
    LIB_FUNCTION("iWtXRduTjHA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("b9QSruV4nnc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED0Ev);
    LIB_FUNCTION("zkCx9c2QKBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED1Ev);
    LIB_FUNCTION("CClObiVHzDY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEED2Ev);
    LIB_FUNCTION("dplyQ6+xatg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("JOj6qfc4VLs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("DTH1zTBrOO8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("bY9Y0J3GGbA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1Em);
    LIB_FUNCTION("ej+44l1PjjY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("x5yAFCJRz8I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2Em);
    LIB_FUNCTION("m2lChTx-9tM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("RB3ratfpZDc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED0Ev);
    LIB_FUNCTION("a6yvHMSqsV0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED1Ev);
    LIB_FUNCTION("7ZmeGHyM6ew", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEED2Ev);
    LIB_FUNCTION("hf2Ljaf19Fs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE2idE);
    LIB_FUNCTION("66AuqgLnsQE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE4_PutES3_St22_String_const_iteratorISt11_String_valISt13_Simple_typesIcEEEm);
    LIB_FUNCTION("1dY2KJfkgMM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE4_RepES3_cm);
    LIB_FUNCTION("riBxNiKLvI0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("w9fCz0pbHdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("Qi5fpNt5+T0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1Em);
    LIB_FUNCTION("mdYczJb+bb0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("XqbpfYmAZB4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2Em);
    LIB_FUNCTION("b2na0Dzd5j8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("s2zG12AYKTg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED0Ev);
    LIB_FUNCTION("AnE9WWbyWkM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED1Ev);
    LIB_FUNCTION("MuACiCSA8-s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEED2Ev);
    LIB_FUNCTION("pzfFqaTMsFc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE2idE);
    LIB_FUNCTION("-hrHhi-UFxs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE4_PutES3_St22_String_const_iteratorISt11_String_valISt13_Simple_typesIwEEEm);
    LIB_FUNCTION("6QU40olMkOM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE4_RepES3_wm);
    LIB_FUNCTION("kJmdxo4uM+8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("0sHarDG9BY4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE7_GetcatEPPKNSt6locale5facetEPKS5_);
    LIB_FUNCTION("rme+Po9yI5M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1Em);
    LIB_FUNCTION("RV6sGVpYa-o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC1ERKSt8_Locinfom);
    LIB_FUNCTION("jIvWFH24Bjw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2Em);
    LIB_FUNCTION("aTjYlKCxPGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom);
    LIB_FUNCTION("qkl3Siab04M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED0Ev);
    LIB_FUNCTION("hnGhTkIDFHg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED1Ev);
    LIB_FUNCTION("4+oswXtp7PQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEED2Ev);
    LIB_FUNCTION("N5nZ3wQw+Vc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9type_infoD0Ev);
    LIB_FUNCTION("LLssoYjMOow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9type_infoD1Ev);
    LIB_FUNCTION("zb3436295XU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZNSt9type_infoD2Ev);
    LIB_FUNCTION("fJnpuVVBbKk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _Znwm);
    LIB_FUNCTION("ryUxD-60bKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZnwmRKSt9nothrow_t);
    LIB_FUNCTION("3yxLpdKD0RA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZnwmSt11align_val_t);
    LIB_FUNCTION("iQXBbJbfT5k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZnwmSt11align_val_tRKSt9nothrow_t);
    LIB_FUNCTION("bsohl1ZrRXE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt10_GetloctxtIcSt19istreambuf_iteratorIcSt11char_traitsIcEEEiRT0_S5_mPKT_);
    LIB_FUNCTION("FX+eS2YsEtY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt10_GetloctxtIcSt19istreambuf_iteratorIwSt11char_traitsIwEEEiRT0_S5_mPKT_);
    LIB_FUNCTION("i4J5FvRPG-w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt10_GetloctxtIwSt19istreambuf_iteratorIwSt11char_traitsIwEEEiRT0_S5_mPKT_);
    LIB_FUNCTION("VrWUXy1gqn8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt10_Rng_abortPKc);
    LIB_FUNCTION("Zmeuhg40yNI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt10adopt_lock);
    LIB_FUNCTION("mhR3IufA7fE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt10defer_lock);
    LIB_FUNCTION("lKfKm6INOpQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt10unexpectedv);
    LIB_FUNCTION("eT2UsmTewbU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt11_Xbad_allocv);
    LIB_FUNCTION("L7Vnk06zC2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt11setiosflagsNSt5_IosbIiE9_FmtflagsE);
    LIB_FUNCTION("kFYQ4d6jVls", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt11try_to_lock);
    LIB_FUNCTION("1h8hFQghR7w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt12setprecisioni);
    LIB_FUNCTION("ybn35k-I+B0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt13_Cl_charnames);
    LIB_FUNCTION("DiGVep5yB5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt13_Execute_onceRSt9once_flagPFiPvS1_PS1_ES1_);
    LIB_FUNCTION("PDX01ziUz+4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt13_Syserror_mapi);
    LIB_FUNCTION("UWyL6KoR96U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt13_Xregex_errorNSt15regex_constants10error_typeE);
    LIB_FUNCTION("Zb+hMspRR-o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt13get_terminatev);
    LIB_FUNCTION("qMXslRdZVj4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt13resetiosflagsNSt5_IosbIiE9_FmtflagsE);
    LIB_FUNCTION("NG1phipELJE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt13set_terminatePFvvE);
    LIB_FUNCTION("u2tMGOLaqnE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14_Atomic_assertPKcS0_);
    LIB_FUNCTION("T+zVxpVaaTo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14_Cl_wcharnames);
    LIB_FUNCTION("Zn44KZgJtWY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14_Debug_messagePKcS0_j);
    LIB_FUNCTION("u0yN6tzBors", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14_Raise_handler);
    LIB_FUNCTION("Nmtr628eA3A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14_Random_devicev);
    LIB_FUNCTION("bRujIheWlB0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14_Throw_C_errori);
    LIB_FUNCTION("tQIo+GIPklo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14_Xlength_errorPKc);
    LIB_FUNCTION("ozMAr28BwSY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14_Xout_of_rangePKc);
    LIB_FUNCTION("zPWCqkg7V+o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14get_unexpectedv);
    LIB_FUNCTION("Eva2i4D5J6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt14set_unexpectedPFvvE);
    LIB_FUNCTION("zugltxeIXM0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt15_sceLibcLocinfoPKc);
    LIB_FUNCTION("y7aMFEkj4PE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt15_Xruntime_errorPKc);
    LIB_FUNCTION("vI85k3GQcz8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt15future_categoryv);
    LIB_FUNCTION("CkY0AVa-frg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt15get_new_handlerv);
    LIB_FUNCTION("RqeErO3cFHU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt15set_new_handlerPFvvE);
    LIB_FUNCTION("aotaAaQK6yc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt15system_categoryv);
    LIB_FUNCTION("W0j6vCxh9Pc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt16_Throw_Cpp_errori);
    LIB_FUNCTION("saSUnPWLq9E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt16_Xoverflow_errorPKc);
    LIB_FUNCTION("YxwfcCH5Q0I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt16generic_categoryv);
    LIB_FUNCTION("0r8rbw2SFqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt17_Future_error_mapi);
    LIB_FUNCTION("V23qt24VPVs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt17iostream_categoryv);
    LIB_FUNCTION("VoUwme28y4w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt18_String_cpp_unused);
    LIB_FUNCTION("NU-T4QowTNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt18_Xinvalid_argumentPKc);
    LIB_FUNCTION("Q1BL70XVV0o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt18uncaught_exceptionv);
    LIB_FUNCTION("PjwbqtUehPU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt19_Throw_future_errorRKSt10error_code);
    LIB_FUNCTION("MELi-cKqWq0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt19_Xbad_function_callv);
    LIB_FUNCTION("Qoo175Ig+-k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt21_sceLibcClassicLocale);
    LIB_FUNCTION("cPNeOAYgB0A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt22_Get_future_error_whati);
    LIB_FUNCTION("mDIHE-aaRaI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt22_Random_device_entropyv);
    LIB_FUNCTION("DNbsDRZ-ntI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt25_Rethrow_future_exceptionSt13exception_ptr);
    LIB_FUNCTION("2WVBaSdGIds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt3cin);
    LIB_FUNCTION("wiR+rIcbnlc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt4_Fpz);
    LIB_FUNCTION("TVfbf1sXt0A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt4cerr);
    LIB_FUNCTION("jSquWN7i7lc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt4clog);
    LIB_FUNCTION("5PfqUBaQf4g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt4cout);
    LIB_FUNCTION("vU9svJtEnWc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt4setwi);
    LIB_FUNCTION("2bASh0rEeXI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt4wcin);
    LIB_FUNCTION("CvJ3HUPlMIY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt5wcerr);
    LIB_FUNCTION("awr1A2VAVZQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt5wclog);
    LIB_FUNCTION("d-YRIvO0jXI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt5wcout);
    LIB_FUNCTION("pDFe-IgbTPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt6_ThrowRKSt9exception);
    LIB_FUNCTION("kr5ph+wVAtU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt6ignore);
    LIB_FUNCTION("FQ9NFbBHb5Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt7_BADOFF);
    LIB_FUNCTION("vYWK2Pz8vGE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt7_FiopenPKcNSt5_IosbIiE9_OpenmodeEi);
    LIB_FUNCTION("aqyjhIx7jaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt7_FiopenPKwNSt5_IosbIiE9_OpenmodeEi);
    LIB_FUNCTION("QfPuSqhub7o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt7_MP_AddPyy);
    LIB_FUNCTION("lrQSsTRMMr4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt7_MP_GetPy);
    LIB_FUNCTION("Gav1Xt1Ce+c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt7_MP_MulPyyy);
    LIB_FUNCTION("Ozk+Z6QnlTY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt7_MP_RemPyy);
    LIB_FUNCTION("NLwJ3q+64bY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZSt7nothrow);
    LIB_FUNCTION("4rrOHCHAV1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt7setbasei);
    LIB_FUNCTION("yYk819F9TyU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt8_XLgammad);
    LIB_FUNCTION("bl0DPP6kFBk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt8_XLgammae);
    LIB_FUNCTION("DWMcG8yogkY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt8_XLgammaf);
    LIB_FUNCTION("X1DNtCe22Ks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt9_LStrcollIcEiPKT_S2_S2_S2_PKSt8_Collvec);
    LIB_FUNCTION("m6uU37-b27s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt9_LStrcollIwEiPKT_S2_S2_S2_PKSt8_Collvec);
    LIB_FUNCTION("V62E2Q8bJVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt9_LStrxfrmIcEmPT_S1_PKS0_S3_PKSt8_Collvec);
    LIB_FUNCTION("BloPUt1HCH0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt9_LStrxfrmIwEmPT_S1_PKS0_S3_PKSt8_Collvec);
    LIB_FUNCTION("qYhnoevd9bI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZSt9terminatev);
    LIB_FUNCTION("XO9ihAZCBcY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIa);
    LIB_FUNCTION("nEuTkSQAQFw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIb);
    LIB_FUNCTION("smeljzleGRQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIc);
    LIB_FUNCTION("iZrCfFRsE3Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTId);
    LIB_FUNCTION("ltRLAWAeSaM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIDh);
    LIB_FUNCTION("7TW4UgJjwJ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIDi);
    LIB_FUNCTION("SK0Syya+scs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIDn);
    LIB_FUNCTION("rkWOabkkpVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIDs);
    LIB_FUNCTION("NlgA2fMtxl4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIe);
    LIB_FUNCTION("c5-Jw-LTekM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIf);
    LIB_FUNCTION("g-fUPD4HznU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIh);
    LIB_FUNCTION("St4apgcBNfo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIi);
    LIB_FUNCTION("MpiTv3MErEQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIj);
    LIB_FUNCTION("b5JSEuAHuDo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIl);
    LIB_FUNCTION("DoGS21ugIfI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIm);
    LIB_FUNCTION("2EEDQ6uHY2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIn);
    LIB_FUNCTION("h1Eewgzowes", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv116__enum_type_infoE);
    LIB_FUNCTION("eD+mC6biMFI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv117__array_type_infoE);
    LIB_FUNCTION("EeOtHxoUkvM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv117__class_type_infoE);
    LIB_FUNCTION("dSBshTZ8JcA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv117__pbase_type_infoE);
    LIB_FUNCTION("YglrcQaNfds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv119__pointer_type_infoE);
    LIB_FUNCTION("DZhZwYkJDCE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv120__function_type_infoE);
    LIB_FUNCTION("N2VV+vnEYlw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv120__si_class_type_infoE);
    LIB_FUNCTION("gjLRFhKCMNE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv121__vmi_class_type_infoE);
    LIB_FUNCTION("dHw0YAjyIV4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv123__fundamental_type_infoE);
    LIB_FUNCTION("7tTpzMt-PzY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN10__cxxabiv129__pointer_to_member_type_infoE);
    LIB_FUNCTION("yZmHOKICuxg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN6Dinkum7threads10lock_errorE);
    LIB_FUNCTION("qcaIknDQLwE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTIN6Dinkum7threads21thread_resource_errorE);
    LIB_FUNCTION("sJUU2ZW-yxU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTINSt6locale5facetE);
    LIB_FUNCTION("8Wc+t3BCF-k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTINSt6locale7_LocimpE);
    LIB_FUNCTION("sBCTjFk7Gi4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTINSt8ios_base7failureE);
    LIB_FUNCTION("Sn3TCBWJ5wo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIo);
    LIB_FUNCTION("Jk+LgZzCsi8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPa);
    LIB_FUNCTION("+qso2nVwQzg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPb);
    LIB_FUNCTION("M1jmeNsWVK8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPc);
    LIB_FUNCTION("3o0PDVnn1qA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPd);
    LIB_FUNCTION("7OO0uCJWILQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPDh);
    LIB_FUNCTION("DOBCPW6DL3w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPDi);
    LIB_FUNCTION("QvWOlLyuQ2o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPDn);
    LIB_FUNCTION("OkYxbdkrv64", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPDs);
    LIB_FUNCTION("96xdSFbiR7Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPe);
    LIB_FUNCTION("01FSgNK1wwA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPf);
    LIB_FUNCTION("ota-3+co4Jk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPh);
    LIB_FUNCTION("YstfcFbhwvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPi);
    LIB_FUNCTION("DQ9mChn0nnE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPj);
    LIB_FUNCTION("Ml1z3dYEVPM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKa);
    LIB_FUNCTION("WV94zKqwgxY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKb);
    LIB_FUNCTION("I4y33AOamns", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKc);
    LIB_FUNCTION("0G36SAiYUhQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKd);
    LIB_FUNCTION("NVCBWomXpcw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKDh);
    LIB_FUNCTION("50aDlGVFt5I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKDi);
    LIB_FUNCTION("liR+QkhejDk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKDn);
    LIB_FUNCTION("kzfj-YSkW7w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKDs);
    LIB_FUNCTION("7uX6IsXWwak", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKe);
    LIB_FUNCTION("2PXZUKjolAA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKf);
    LIB_FUNCTION("RKvygdQzGaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKh);
    LIB_FUNCTION("sVUkO0TTpM8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKi);
    LIB_FUNCTION("4zhc1xNSIno", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKj);
    LIB_FUNCTION("Gr+ih5ipgNk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKl);
    LIB_FUNCTION("0cLFYdr1AGc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKm);
    LIB_FUNCTION("0Xxtiar8Ceg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKn);
    LIB_FUNCTION("hsttk-IbL1o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKo);
    LIB_FUNCTION("zqOGToT2dH8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKs);
    LIB_FUNCTION("WY7615THqKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKt);
    LIB_FUNCTION("0g+zCGZ7dgQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKv);
    LIB_FUNCTION("jfqTdKTGbBI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKw);
    LIB_FUNCTION("sOz2j1Lxl48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKx);
    LIB_FUNCTION("qTgw+f54K34", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPKy);
    LIB_FUNCTION("1+5ojo5J2xU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPl);
    LIB_FUNCTION("SPiW3NTO8I0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPm);
    LIB_FUNCTION("zUwmtNuJABI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPn);
    LIB_FUNCTION("A9PfIjQCOUw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPo);
    LIB_FUNCTION("nqpARwWZmjI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPs);
    LIB_FUNCTION("KUW22XiVxvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPt);
    LIB_FUNCTION("OJPn-YR1bow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPv);
    LIB_FUNCTION("7gj0BXUP3dc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPw);
    LIB_FUNCTION("9opd1ucwDqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPx);
    LIB_FUNCTION("a9KMkfXXUsE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIPy);
    LIB_FUNCTION("j97CjKJNtQI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIs);
    LIB_FUNCTION("U1CBVMD42HA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTISi);
    LIB_FUNCTION("iLSavTYoxx0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTISo);
    LIB_FUNCTION("H0aqk25W6BI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt10bad_typeid);
    LIB_FUNCTION("2GWRrgT8o20", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt10ctype_base);
    LIB_FUNCTION("IBtzswgYU3A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt10money_base);
    LIB_FUNCTION("2e96MkSXo3U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt10moneypunctIcLb0EE);
    LIB_FUNCTION("Ks2FIQJ2eDc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt10moneypunctIcLb1EE);
    LIB_FUNCTION("EnMjfRlO5f0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt10moneypunctIwLb0EE);
    LIB_FUNCTION("gBZnTFMk6N0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt10moneypunctIwLb1EE);
    LIB_FUNCTION("n7iD5r9+4Eo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt11_Facet_base);
    LIB_FUNCTION("x8LHSvl5N6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt11logic_error);
    LIB_FUNCTION("C0IYaaVSC1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt11range_error);
    LIB_FUNCTION("9-TRy4p-YTM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt11regex_error);
    LIB_FUNCTION("XtP9KKwyK9Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt12bad_weak_ptr);
    LIB_FUNCTION("dDIjj8NBxNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt12codecvt_base);
    LIB_FUNCTION("5BIbzIuDxTQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt12domain_error);
    LIB_FUNCTION("DCY9coLQcVI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt12future_error);
    LIB_FUNCTION("cxqzgvGm1GI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt12length_error);
    LIB_FUNCTION("dKjhNUf9FBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt12out_of_range);
    LIB_FUNCTION("eDciML+moZs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt12system_error);
    LIB_FUNCTION("Z7NWh8jD+Nw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt13bad_exception);
    LIB_FUNCTION("STNAj1oxtpk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt13basic_filebufIcSt11char_traitsIcEE);
    LIB_FUNCTION("37CMzzbbHn8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt13basic_filebufIwSt11char_traitsIwEE);
    LIB_FUNCTION("iXChH4Elf7M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt13basic_istreamIwSt11char_traitsIwEE);
    LIB_FUNCTION("Lc-l1GQi7tg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt13basic_ostreamIwSt11char_traitsIwEE);
    LIB_FUNCTION("WbBz4Oam3wM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt13messages_base);
    LIB_FUNCTION("bLPn1gfqSW8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt13runtime_error);
    LIB_FUNCTION("cbvW20xPgyc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt14error_category);
    LIB_FUNCTION("lt0mLhNwjs0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt14overflow_error);
    LIB_FUNCTION("ymXfiwv59Z0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt15basic_streambufIcSt11char_traitsIcEE);
    LIB_FUNCTION("muIOyDB+DP8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt15basic_streambufIwSt11char_traitsIwEE);
    LIB_FUNCTION("oNRAB0Zs2+0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt15underflow_error);
    LIB_FUNCTION("XZzWt0ygWdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt16invalid_argument);
    LIB_FUNCTION("FtPFMdiURuM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt16nested_exception);
    LIB_FUNCTION("c33GAGjd7Is", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt17bad_function_call);
    LIB_FUNCTION("8rd5FvOFk+w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt18bad_variant_access);
    LIB_FUNCTION("lbLEAN+Y9iI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt20bad_array_new_length);
    LIB_FUNCTION("3aZN32UTqqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt22_Future_error_category);
    LIB_FUNCTION("QLqM1r9nPow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt22_System_error_category);
    LIB_FUNCTION("+Le0VsFb9mE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt23_Generic_error_category);
    LIB_FUNCTION("FCuvlxsgg0w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt24_Iostream_error_category);
    LIB_FUNCTION("QQsnQ2bWkdM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTISt4_Pad);
    LIB_FUNCTION("sIvK5xl5pzw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt5_IosbIiE);
    LIB_FUNCTION("gZvNGjQsmf8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt5ctypeIcE);
    LIB_FUNCTION("Fj7VTFzlI3k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt5ctypeIwE);
    LIB_FUNCTION("weALTw0uesc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7_MpunctIcE);
    LIB_FUNCTION("DaYYQBc+SY8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7_MpunctIwE);
    LIB_FUNCTION("Cs3DBACRSY8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7codecvtIcc9_MbstatetE);
    LIB_FUNCTION("+TtUFzALoDc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7codecvtIDic9_MbstatetE);
    LIB_FUNCTION("v1WebHtIa24", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7codecvtIDsc9_MbstatetE);
    LIB_FUNCTION("hbU5HOTy1HM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7codecvtIwc9_MbstatetE);
    LIB_FUNCTION("fvgYbBEhXnc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7collateIcE);
    LIB_FUNCTION("pphEhnnuXKA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7collateIwE);
    LIB_FUNCTION("6ddOFPDvuCo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("dO7-MxIPfsw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("RYlvfQvnOzo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("C3sAx2aJy3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("qOD-ksTkE08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8bad_cast);
    LIB_FUNCTION("BJCgW9-OxLA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8ios_base);
    LIB_FUNCTION("UFsKD1fd1-w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8messagesIcE);
    LIB_FUNCTION("007PjrBCaUM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8messagesIwE);
    LIB_FUNCTION("ddLNBT9ks2I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8numpunctIcE);
    LIB_FUNCTION("A2TTRMAe6Sw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8numpunctIwE);
    LIB_FUNCTION("C4j57iQD4I8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("oYliMCqNYQg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("33t+tvosxCI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("h9C+J68WriE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("DwH3gdbYfZo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9bad_alloc);
    LIB_FUNCTION("7f4Nl2VS0gw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9basic_iosIcSt11char_traitsIcEE);
    LIB_FUNCTION("RjWhdj0ztTs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9basic_iosIwSt11char_traitsIwEE);
    LIB_FUNCTION("n2kx+OmFUis", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9exception);
    LIB_FUNCTION("oNAnn5cOxfs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("QNAIWEkBocY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("hBeW7FhedsY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("7DzM2fl46gU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("CVcmmf8VL40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9time_base);
    LIB_FUNCTION("xX6s+z0q6oo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTISt9type_info);
    LIB_FUNCTION("Qd6zUdRhrhs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIt);
    LIB_FUNCTION("JrUnjJ-PCTg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIv);
    LIB_FUNCTION("qUxH+Damft4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIw);
    LIB_FUNCTION("8Ijx3Srynh0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIx);
    LIB_FUNCTION("KBBVmt8Td7c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTIy);
    LIB_FUNCTION("iOLTktXe6a0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSa);
    LIB_FUNCTION("M86y4bmx+WA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSb);
    LIB_FUNCTION("zGpCWBtVC0A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSc);
    LIB_FUNCTION("pMQUQSfX6ZE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSd);
    LIB_FUNCTION("DghzFjzLqaE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSDi);
    LIB_FUNCTION("FUvnVyCDhjg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSDn);
    LIB_FUNCTION("Z7+siBC690w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSDs);
    LIB_FUNCTION("KNgcEteI72I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSe);
    LIB_FUNCTION("aFMVMBzO5jk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSf);
    LIB_FUNCTION("BNC7IeJelZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSh);
    LIB_FUNCTION("papHVcWkO5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSi);
    LIB_FUNCTION("1nylaCTiH08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSj);
    LIB_FUNCTION("k9kErpz2Sv8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSl);
    LIB_FUNCTION("OzMC6yz6Row", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSm);
    LIB_FUNCTION("au+YxKwehQM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv116__enum_type_infoE);
    LIB_FUNCTION("6BYT26CFh58", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv117__array_type_infoE);
    LIB_FUNCTION("8Vs1AjNm2mE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv117__class_type_infoE);
    LIB_FUNCTION("bPUMNZBqRqQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv117__pbase_type_infoE);
    LIB_FUNCTION("UVft3+rc06o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv119__pointer_type_infoE);
    LIB_FUNCTION("4ZXlZy7iRWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv120__function_type_infoE);
    LIB_FUNCTION("AQlqO860Ztc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv120__si_class_type_infoE);
    LIB_FUNCTION("I1Ru2fZJDoE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv121__vmi_class_type_infoE);
    LIB_FUNCTION("6WYrZgAyjuE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv123__fundamental_type_infoE);
    LIB_FUNCTION("K+w0ofCSsAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN10__cxxabiv129__pointer_to_member_type_infoE);
    LIB_FUNCTION("y-bbIiLALd4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN6Dinkum7threads10lock_errorE);
    LIB_FUNCTION("hmHH6DsLWgA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSN6Dinkum7threads21thread_resource_errorE);
    LIB_FUNCTION("RVDooP5gZ4s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSNSt6locale5facetE);
    LIB_FUNCTION("JjTc4SCuILE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSNSt6locale7_LocimpE);
    LIB_FUNCTION("C-3N+mEQli4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSNSt8ios_base7failureE);
    LIB_FUNCTION("p07Yvdjjoo4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPa);
    LIB_FUNCTION("ickyvjtMLm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPb);
    LIB_FUNCTION("jJtoPFrxG-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPc);
    LIB_FUNCTION("dIxG0L1esAI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPd);
    LIB_FUNCTION("TSMc8vgtvHI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPDi);
    LIB_FUNCTION("cj+ge8YLU7s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPDn);
    LIB_FUNCTION("mQCm5NmJORg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPDs);
    LIB_FUNCTION("N84qS6rJuGI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPe);
    LIB_FUNCTION("DN0xDLRXD2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPf);
    LIB_FUNCTION("HHVZLHmCfI4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPh);
    LIB_FUNCTION("g8phA3duRm8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPi);
    LIB_FUNCTION("bEbjy6yceWo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPj);
    LIB_FUNCTION("dSifrMdPVQ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKa);
    LIB_FUNCTION("qSiIrmgy1D8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKb);
    LIB_FUNCTION("wm9QKozFM+s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKc);
    LIB_FUNCTION("-7c7thUsi1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKd);
    LIB_FUNCTION("lFKA8eMU5PA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKDi);
    LIB_FUNCTION("2veyNsXFZuw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKDn);
    LIB_FUNCTION("qQ4c52GZlYw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKDs);
    LIB_FUNCTION("8Ce6O0B-KpA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKe);
    LIB_FUNCTION("emnRy3TNxFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKf);
    LIB_FUNCTION("thDTXTikSmc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKh);
    LIB_FUNCTION("3Fd+8Pk6fgE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKi);
    LIB_FUNCTION("6azovDgjxt0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKj);
    LIB_FUNCTION("QdPk9cbJrOY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKl);
    LIB_FUNCTION("ER8-AFoFDfM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKm);
    LIB_FUNCTION("5rD2lCo4688", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKs);
    LIB_FUNCTION("iWMhoHS8gqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKt);
    LIB_FUNCTION("3op2--wf660", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKv);
    LIB_FUNCTION("h64u7Gu3-TM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKw);
    LIB_FUNCTION("3THnS7v0D+M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKx);
    LIB_FUNCTION("h+xQETZ+6Yo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPKy);
    LIB_FUNCTION("6cfcRTPD2zU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPl);
    LIB_FUNCTION("OXkzGA9WqVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPm);
    LIB_FUNCTION("6XcQYYO2YMY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPs);
    LIB_FUNCTION("8OSy0MMQ7eI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPt);
    LIB_FUNCTION("s1b2SRBzSAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPv);
    LIB_FUNCTION("4r--aFJyPpI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPw);
    LIB_FUNCTION("pc4-Lqosxgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPx);
    LIB_FUNCTION("VJL9W+nOv1U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSPy);
    LIB_FUNCTION("VenLJSDuDXY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSs);
    LIB_FUNCTION("46haDPRVtPo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSSi);
    LIB_FUNCTION("RgJjmluR+QA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSSo);
    LIB_FUNCTION("ALcclvT4W3Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt10bad_typeid);
    LIB_FUNCTION("idsapmYZ49w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt10ctype_base);
    LIB_FUNCTION("VxObo0uiafo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt10money_base);
    LIB_FUNCTION("h+iBEkE50Zs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt10moneypunctIcLb0EE);
    LIB_FUNCTION("o4DiZqXId90", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt10moneypunctIcLb1EE);
    LIB_FUNCTION("MxGclWMtl4g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt10moneypunctIwLb0EE);
    LIB_FUNCTION("J+hjiBreZr4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt10moneypunctIwLb1EE);
    LIB_FUNCTION("FAah-AY8+vY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt11_Facet_base);
    LIB_FUNCTION("VNHXByo1yuY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt11logic_error);
    LIB_FUNCTION("msxwgUAPy-Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt11range_error);
    LIB_FUNCTION("UG6HJeH5GNI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt11regex_error);
    LIB_FUNCTION("P7l9+yBL5VU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt12bad_weak_ptr);
    LIB_FUNCTION("NXKsxT-x76M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt12codecvt_base);
    LIB_FUNCTION("2ud1bFeR0h8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt12domain_error);
    LIB_FUNCTION("YeBP0Rja7vc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt12future_error);
    LIB_FUNCTION("zEhcQGEiPik", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt12length_error);
    LIB_FUNCTION("eNW5jsFxS6k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt12out_of_range);
    LIB_FUNCTION("XRxuwvN++2w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt12system_error);
    LIB_FUNCTION("G8z7rz17xYM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt13bad_exception);
    LIB_FUNCTION("WYWf+rJuDVU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt13basic_filebufIcSt11char_traitsIcEE);
    LIB_FUNCTION("coVkgLzNtaw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt13basic_filebufIwSt11char_traitsIwEE);
    LIB_FUNCTION("0Ys3rv0tw7Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt13basic_istreamIwSt11char_traitsIwEE);
    LIB_FUNCTION("R72NCZqMX58", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt13basic_ostreamIwSt11char_traitsIwEE);
    LIB_FUNCTION("N0EHkukBz6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt13messages_base);
    LIB_FUNCTION("CX3WC8qekJE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt13runtime_error);
    LIB_FUNCTION("u5zp3yXW5wA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt14error_category);
    LIB_FUNCTION("iy1lPjADRUs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt14overflow_error);
    LIB_FUNCTION("IfWUkB7Snkc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt15basic_streambufIcSt11char_traitsIcEE);
    LIB_FUNCTION("qiloU7D8MBM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt15basic_streambufIwSt11char_traitsIwEE);
    LIB_FUNCTION("Uea1kfRJ7Oc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt15underflow_error);
    LIB_FUNCTION("KJutwrVUFUo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt16invalid_argument);
    LIB_FUNCTION("S8kp05fMCqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt16nested_exception);
    LIB_FUNCTION("ql6hz7ZOZTs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt17bad_function_call);
    LIB_FUNCTION("ObdBkrZylOg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt18bad_variant_access);
    LIB_FUNCTION("hBvqSQD5yNk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt20bad_array_new_length);
    LIB_FUNCTION("ouo2obDE6yU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt22_Future_error_category);
    LIB_FUNCTION("iwIUndpU5ZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt22_System_error_category);
    LIB_FUNCTION("88Fre+wfuT0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt23_Generic_error_category);
    LIB_FUNCTION("ItmiNlkXVkQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt24_Iostream_error_category);
    LIB_FUNCTION("qR6GVq1IplU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSSt4_Pad);
    LIB_FUNCTION("uO6YxonQkJI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt5_IosbIiE);
    LIB_FUNCTION("jUQ+FlOMEHk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt5ctypeIcE);
    LIB_FUNCTION("1jUJO+TZm5k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt5ctypeIwE);
    LIB_FUNCTION("LfMY9H6d5mI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7_MpunctIcE);
    LIB_FUNCTION("mh9Jro0tcjg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7_MpunctIwE);
    LIB_FUNCTION("rf0BfDQG1KU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7codecvtIcc9_MbstatetE);
    LIB_FUNCTION("Tt3ZSp9XD4E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7codecvtIDic9_MbstatetE);
    LIB_FUNCTION("9XL3Tlgx6lc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7codecvtIDsc9_MbstatetE);
    LIB_FUNCTION("YrYO5bTIPqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7codecvtIwc9_MbstatetE);
    LIB_FUNCTION("wElyE0OmoRw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7collateIcE);
    LIB_FUNCTION("BdfPxmlM9bs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7collateIwE);
    LIB_FUNCTION("5DdJdPeXCHE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("XYqrcE4cVMM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("B9rn6eKNPJg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("KY+yxjxFBSY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("--fMWwCvo+c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8bad_cast);
    LIB_FUNCTION("Nr+GiZ0tGAk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8ios_base);
    LIB_FUNCTION("iUhx-JN27uI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8messagesIcE);
    LIB_FUNCTION("5ViZYJRew6g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8messagesIwE);
    LIB_FUNCTION("2ZqL1jnL8so", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8numpunctIcE);
    LIB_FUNCTION("xuEUMolGMwU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8numpunctIwE);
    LIB_FUNCTION("-l+ODHZ96LI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("bn3sb2SwGk8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("OI989Lb3WK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("gqwPsSmdh+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("22g2xONdXV4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9bad_alloc);
    LIB_FUNCTION("TuKJRIKcceA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9basic_iosIcSt11char_traitsIcEE);
    LIB_FUNCTION("wYWYC8xNFOI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9basic_iosIwSt11char_traitsIwEE);
    LIB_FUNCTION("H61hE9pLBmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9exception);
    LIB_FUNCTION("K8CzKJ7h1-8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("Q3YIaCcEeOM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("wRyKNdtYYEY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("0x4NT++LU9s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("5jX3QET-Jhw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9time_base);
    LIB_FUNCTION("WG7lrmFxyKY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTSSt9type_info);
    LIB_FUNCTION("f5zmgYKSpIY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSt);
    LIB_FUNCTION("mI0SR5s7kxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSv);
    LIB_FUNCTION("UXS8VgAnIP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSw);
    LIB_FUNCTION("N8KLCZc3Y1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSx);
    LIB_FUNCTION("kfuINXyHayQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTSy);
    LIB_FUNCTION("0bGGr4zLE3w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTv0_n24_NSiD0Ev);
    LIB_FUNCTION("+Uuj++A+I14", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTv0_n24_NSiD1Ev);
    LIB_FUNCTION("QJJ-4Dgm8YQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTv0_n24_NSoD0Ev);
    LIB_FUNCTION("kvqg376KsJo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTv0_n24_NSoD1Ev);
    LIB_FUNCTION("izmoTISVoF8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTv0_n24_NSt13basic_istreamIwSt11char_traitsIwEED0Ev);
    LIB_FUNCTION("q05IXuNA2NE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTv0_n24_NSt13basic_istreamIwSt11char_traitsIwEED1Ev);
    LIB_FUNCTION("0j1jspKbuFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTv0_n24_NSt13basic_ostreamIwSt11char_traitsIwEED0Ev);
    LIB_FUNCTION("HSkPyRyFFHQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTv0_n24_NSt13basic_ostreamIwSt11char_traitsIwEED1Ev);
    LIB_FUNCTION("fjni7nkqJ4M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv116__enum_type_infoE);
    LIB_FUNCTION("aMQhMoYipk4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv117__array_type_infoE);
    LIB_FUNCTION("byV+FWlAnB4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv117__class_type_infoE);
    LIB_FUNCTION("7EirbE7st4E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv117__pbase_type_infoE);
    LIB_FUNCTION("aeHxLWwq0gQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv119__pointer_type_infoE);
    LIB_FUNCTION("CSEjkTYt5dw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv120__function_type_infoE);
    LIB_FUNCTION("pZ9WXcClPO8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv120__si_class_type_infoE);
    LIB_FUNCTION("9ByRMdo7ywg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv121__vmi_class_type_infoE);
    LIB_FUNCTION("G4XM-SS1wxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv123__fundamental_type_infoE);
    LIB_FUNCTION("2H51caHZU0Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN10__cxxabiv129__pointer_to_member_type_infoE);
    LIB_FUNCTION("WJU9B1OjRbA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN6Dinkum7threads10lock_errorE);
    LIB_FUNCTION("ouXHPXjKUL4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVN6Dinkum7threads21thread_resource_errorE);
    LIB_FUNCTION("QGkJzBs3WmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVNSt6locale7_LocimpE);
    LIB_FUNCTION("yLE5H3058Ao", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVNSt8ios_base7failureE);
    LIB_FUNCTION("+8jItptyeQQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTVSi);
    LIB_FUNCTION("qjyK90UVVCM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTVSo);
    LIB_FUNCTION("jRLwj8TLcQY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt10bad_typeid);
    LIB_FUNCTION("XbFyGCk3G2s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt10moneypunctIcLb0EE);
    LIB_FUNCTION("MfyPz2J5E0I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt10moneypunctIcLb1EE);
    LIB_FUNCTION("RfpPDUaxVJM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt10moneypunctIwLb0EE);
    LIB_FUNCTION("APrAh-3-ICg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt10moneypunctIwLb1EE);
    LIB_FUNCTION("udTM6Nxx-Ng", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt11logic_error);
    LIB_FUNCTION("RbzWN8X21hY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt11range_error);
    LIB_FUNCTION("c-EfVOIbo8M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt11regex_error);
    LIB_FUNCTION("apHKv46QaCw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt12bad_weak_ptr);
    LIB_FUNCTION("oAidKrxuUv0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt12domain_error);
    LIB_FUNCTION("6-LMlTS1nno", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt12future_error);
    LIB_FUNCTION("cqvea9uWpvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt12length_error);
    LIB_FUNCTION("n+aUKkC-3sI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt12out_of_range);
    LIB_FUNCTION("Bq8m04PN1zw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt12system_error);
    LIB_FUNCTION("Gvp-ypl9t5E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt13bad_exception);
    LIB_FUNCTION("rSADYzp-RTU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt13basic_filebufIcSt11char_traitsIcEE);
    LIB_FUNCTION("Tx5Y+BQJrzs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt13basic_filebufIwSt11char_traitsIwEE);
    LIB_FUNCTION("DBO-xlHHEn8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt13basic_istreamIwSt11char_traitsIwEE);
    LIB_FUNCTION("BMuRmwMy6eE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt13basic_ostreamIwSt11char_traitsIwEE);
    LIB_FUNCTION("-L+-8F0+gBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt13runtime_error);
    LIB_FUNCTION("lF66NEAqanc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt14error_category);
    LIB_FUNCTION("Azw9C8cy7FY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt14overflow_error);
    LIB_FUNCTION("ZrFcJ-Ab0vw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt15underflow_error);
    LIB_FUNCTION("keXoyW-rV-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt16invalid_argument);
    LIB_FUNCTION("j6qwOi2Nb7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt16nested_exception);
    LIB_FUNCTION("wuOrunkpIrU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt17bad_function_call);
    LIB_FUNCTION("AZGKZIVok6U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt18bad_variant_access);
    LIB_FUNCTION("Z+vcX3rnECg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt20bad_array_new_length);
    LIB_FUNCTION("YHfG3-K23CY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt22_Future_error_category);
    LIB_FUNCTION("qTwVlzGoViY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt22_System_error_category);
    LIB_FUNCTION("UuVHsmfVOHU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt23_Generic_error_category);
    LIB_FUNCTION("lJPCM50sdTc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt24_Iostream_error_category);
    LIB_FUNCTION("CRoMIoZkYhU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, _ZTVSt4_Pad);
    LIB_FUNCTION("GKWcAz6-G7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt5ctypeIcE);
    LIB_FUNCTION("qdHsu+gIxRo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt5ctypeIwE);
    LIB_FUNCTION("6gAhNHCNHxY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7_MpunctIcE);
    LIB_FUNCTION("+hlZqs-XpUM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7_MpunctIwE);
    LIB_FUNCTION("aK1Ymf-NhAs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7codecvtIcc9_MbstatetE);
    LIB_FUNCTION("9H2BStEAAMg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7codecvtIDic9_MbstatetE);
    LIB_FUNCTION("jlNI3SSF41o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7codecvtIDsc9_MbstatetE);
    LIB_FUNCTION("H-TDszhsYuY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7codecvtIwc9_MbstatetE);
    LIB_FUNCTION("ruZtIwbCFjk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7collateIcE);
    LIB_FUNCTION("rZwUkaQ02J4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7collateIwE);
    LIB_FUNCTION("KfcTPbeaOqg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("Y9C9GeKyZ3A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("1kZFcktOm+s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7num_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("59oywaaZbJk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("tVHE+C8vGXk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8bad_cast);
    LIB_FUNCTION("AJsqpbcCiwY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8ios_base);
    LIB_FUNCTION("FnEnECMJGag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8messagesIcE);
    LIB_FUNCTION("2FezsYwelgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8messagesIwE);
    LIB_FUNCTION("lJnP-cn0cvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8numpunctIcE);
    LIB_FUNCTION("Gtsl8PUl40U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8numpunctIwE);
    LIB_FUNCTION("o4kt51-uO48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("sEca1nUOueA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8time_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("OwfBD-2nhJQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8time_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("5KOPB+1eEfs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt8time_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("EMNG6cHitlQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt9bad_alloc);
    LIB_FUNCTION("dCzeFfg9WWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt9exception);
    LIB_FUNCTION("3n3wCJGFP7o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("CUkG1cK2T+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("zdCex1HjCCM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE);
    LIB_FUNCTION("ogi5ZolMUs4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE);
    LIB_FUNCTION("749AEdSd4Go", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZTVSt9type_info);
    LIB_FUNCTION("4-Fllbzfh2k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetffldEPcRS3_S6_RSt8ios_basePiE4_Src);
    LIB_FUNCTION("NQW6QjEPUak", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6localeE4_Src);
    LIB_FUNCTION("3P+CcdakSi0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE9_GetffldxEPcRS3_S6_RSt8ios_basePiE4_Src);
    LIB_FUNCTION("o-gc5R8f50M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetffldEPcRS3_S6_RSt8ios_basePiE4_Src);
    LIB_FUNCTION("3kjXzznHyCg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetifldEPcRS3_S6_NSt5_IosbIiE9_FmtflagsERKSt6localeE4_Src);
    LIB_FUNCTION("DKkwPpi+uWc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt7num_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE9_GetffldxEPcRS3_S6_RSt8ios_basePiE4_Src);
    LIB_FUNCTION("mZW-My-zemM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt9money_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE8_GetmfldERS3_S5_bRSt8ios_basePcE4_Src);
    LIB_FUNCTION("HCzNCcPxu+w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt9money_getIwSt19istreambuf_iteratorIwSt11char_traitsIwEEE8_GetmfldERS3_S5_bRSt8ios_basePwE4_Src);
    LIB_FUNCTION("sHagUsvHBnk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt9money_putIcSt19ostreambuf_iteratorIcSt11char_traitsIcEEE6do_putES3_bRSt8ios_basecRKSsE4_Src);
    LIB_FUNCTION("A5EX+eJmQI8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNKSt9money_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEE6do_putES3_bRSt8ios_basewRKSbIwS2_SaIwEEE4_Src);
    LIB_FUNCTION("jfq92K8E1Vc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit);
    LIB_FUNCTION("AoZRvn-vaq4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 _ZZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit);
    LIB_FUNCTION("L1SBTkC+Cvw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, abort);
    LIB_FUNCTION("SmYrO79NzeI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 abort_handler_s);
    LIB_FUNCTION("Ye20uNnlglA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, abs);
    LIB_FUNCTION("JBcgYuW8lPU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, acos);
    LIB_FUNCTION("QI-x0SL8jhw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, acosf);
    LIB_FUNCTION("Fk7-KFKZi-8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, acosh);
    LIB_FUNCTION("XJp2C-b0tRU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, acoshf);
    LIB_FUNCTION("u14Y1HFh0uY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, acoshl);
    LIB_FUNCTION("iH4YAIRcecA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, acosl);
    LIB_FUNCTION("DQXJraCc1rA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, alarm);
    LIB_FUNCTION("2Btkg8k24Zg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 aligned_alloc);
    LIB_FUNCTION("jT3xiGpA3B4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, asctime);
    LIB_FUNCTION("qPe7-h5Jnuc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, asctime_s);
    LIB_FUNCTION("7Ly52zaL44Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, asin);
    LIB_FUNCTION("GZWjF-YIFFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, asinf);
    LIB_FUNCTION("2eQpqTjJ5Y4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, asinh);
    LIB_FUNCTION("yPPtp1RMihw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, asinhf);
    LIB_FUNCTION("iCl-Z-g-uuA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, asinhl);
    LIB_FUNCTION("Nx-F5v0-qU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, asinl);
    LIB_FUNCTION("cOYia2dE0Ik", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, asprintf);
    LIB_FUNCTION("HC8vbJStYVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 at_quick_exit);
    LIB_FUNCTION("OXmauLdQ8kY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atan);
    LIB_FUNCTION("HUbZmOnT-Dg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atan2);
    LIB_FUNCTION("EH-x713A99c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atan2f);
    LIB_FUNCTION("9VeY8wiqf8M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atan2l);
    LIB_FUNCTION("weDug8QD-lE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atanf);
    LIB_FUNCTION("YjbpxXpi6Zk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atanh);
    LIB_FUNCTION("cPGyc5FGjy0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atanhf);
    LIB_FUNCTION("a3BNqojL4LM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atanhl);
    LIB_FUNCTION("KvOHPTz595Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atanl);
    LIB_FUNCTION("8G2LB+A3rzg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atexit);
    LIB_FUNCTION("SRI6S9B+-a4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atof);
    LIB_FUNCTION("fPxypibz2MY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atoi);
    LIB_FUNCTION("+my9jdHCMIQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atol);
    LIB_FUNCTION("fLcU5G6Qrjs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, atoll);
    LIB_FUNCTION("rg5JEBlKCuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, basename);
    LIB_FUNCTION("vsK6LzRtRLI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, basename_r);
    LIB_FUNCTION("5TjaJwkLWxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, bcmp);
    LIB_FUNCTION("RMo7j0iTPfA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, bcopy);
    LIB_FUNCTION("NesIgTmfF0Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, bsearch);
    LIB_FUNCTION("hzX87C+zDAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, bsearch_s);
    LIB_FUNCTION("LEvm25Sxi7I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, btowc);
    LIB_FUNCTION("9oiX1kyeedA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, bzero);
    LIB_FUNCTION("EOLQfNZ9HpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, c16rtomb);
    LIB_FUNCTION("MzsycG6RYto", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, c32rtomb);
    LIB_FUNCTION("2X5agFjKxMc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, calloc);
    LIB_FUNCTION("5ZkEP3Rq7As", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, cbrt);
    LIB_FUNCTION("GlelR9EEeck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, cbrtf);
    LIB_FUNCTION("lO01m-JcDqM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, cbrtl);
    LIB_FUNCTION("gacfOmO8hNs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ceil);
    LIB_FUNCTION("GAUuLKGhsCw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ceilf);
    LIB_FUNCTION("aJKn6X+40Z8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ceill);
    LIB_FUNCTION("St9nbxSoezk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, clearerr);
    LIB_FUNCTION("cYNk9M+7YkY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 clearerr_unlocked);
    LIB_FUNCTION("QZP6I9ZZxpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, clock);
    LIB_FUNCTION("n8onIBR4Qdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, clock_1700);
    LIB_FUNCTION("XepdqehVYe4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, closedir);
    LIB_FUNCTION("BEFy1ZFv8Fw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, copysign);
    LIB_FUNCTION("x-04iOzl1xs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, copysignf);
    LIB_FUNCTION("j84nSG4V0B0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, copysignl);
    LIB_FUNCTION("2WE3BTYVwKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, cos);
    LIB_FUNCTION("-P6FNMzk2Kc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, cosf);
    LIB_FUNCTION("m7iLTaO9RMs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, cosh);
    LIB_FUNCTION("RCQAffkEh9A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, coshf);
    LIB_FUNCTION("XK2R46yx0jc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, coshl);
    LIB_FUNCTION("x8dc5Y8zFgc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, cosl);
    LIB_FUNCTION("0uAUs3hYuG4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ctime);
    LIB_FUNCTION("2UFh+YKfuzk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ctime_s);
    LIB_FUNCTION("Wn6I3wVATUE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, daemon);
    LIB_FUNCTION("tOicWgmk4ZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, daylight);
    LIB_FUNCTION("fqLrWSWcGHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, devname);
    LIB_FUNCTION("BIALMFTZ75I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, devname_r);
    LIB_FUNCTION("-VVn74ZyhEs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, difftime);
    LIB_FUNCTION("E4wZaG1zSFc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, dirname);
    LIB_FUNCTION("2gbcltk3swE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, div);
    LIB_FUNCTION("WIg11rA+MRY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, drand48);
    LIB_FUNCTION("5OpjqFs8yv8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, drem);
    LIB_FUNCTION("Gt5RT417EGA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, dremf);
    LIB_FUNCTION("Fncgcl1tnXg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, erand48);
    LIB_FUNCTION("oXgaqGVnW5o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, erf);
    LIB_FUNCTION("arIKLlen2sg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, erfc);
    LIB_FUNCTION("IvF98yl5u4s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, erfcf);
    LIB_FUNCTION("f2YbMj0gBf8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, erfcl);
    LIB_FUNCTION("RePA3bDBJqo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, erff);
    LIB_FUNCTION("fNH4tsl7rB8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, erfl);
    LIB_FUNCTION("aeeMZ0XrNsY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, err);
    LIB_FUNCTION("9aODPZAKOmA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, err_set_exit);
    LIB_FUNCTION("FihG2CudUNs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, err_set_file);
    LIB_FUNCTION("L-jLYJFP9Mc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, errc);
    LIB_FUNCTION("t8sFCgJAClE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, errx);
    LIB_FUNCTION("uMei1W9uyNo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, exit);
    LIB_FUNCTION("NVadfnzQhHQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, exp);
    LIB_FUNCTION("dnaeGXbjP6E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, exp2);
    LIB_FUNCTION("wuAQt-j+p4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, exp2f);
    LIB_FUNCTION("9O1Xdko-wSo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, exp2l);
    LIB_FUNCTION("8zsu04XNsZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, expf);
    LIB_FUNCTION("qMp2fTDCyMo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, expl);
    LIB_FUNCTION("gqKfOiJaCOo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, expm1);
    LIB_FUNCTION("3EgxfDRefdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, expm1f);
    LIB_FUNCTION("jVS263HH1b0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, expm1l);
    LIB_FUNCTION("388LcMWHRCA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fabs);
    LIB_FUNCTION("fmT2cjPoWBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fabsf);
    LIB_FUNCTION("w-AryX51ObA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fabsl);
    LIB_FUNCTION("uodLYyUip20", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fclose);
    LIB_FUNCTION("cBSM-YB7JVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fcloseall);
    LIB_FUNCTION("Zs4p6RemDxM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fdim);
    LIB_FUNCTION("yb9iUBPkSS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fdimf);
    LIB_FUNCTION("IMt+UO5YoQI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fdiml);
    LIB_FUNCTION("qdlHjTa9hQ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fdopen);
    LIB_FUNCTION("j+XjoRSIvwU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fdopendir);
    LIB_FUNCTION("cqt8emEH3kQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 feclearexcept);
    LIB_FUNCTION("y4WlO8qzHqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 fedisableexcept);
    LIB_FUNCTION("utLW7uXm3Ss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 feenableexcept);
    LIB_FUNCTION("psx0YiAKm7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fegetenv);
    LIB_FUNCTION("VtRkfsD292M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fegetexcept);
    LIB_FUNCTION("myQDQapYJdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 fegetexceptflag);
    LIB_FUNCTION("AeZTCCm1Qqc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fegetround);
    LIB_FUNCTION("P38JvXuK-uE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 fegettrapenable);
    LIB_FUNCTION("1kduKXMqx7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, feholdexcept);
    LIB_FUNCTION("LxcEU+ICu8U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, feof);
    LIB_FUNCTION("NuydofHcR1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 feof_unlocked);
    LIB_FUNCTION("NIfFNcyeCTo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 feraiseexcept);
    LIB_FUNCTION("AHxyhN96dy4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ferror);
    LIB_FUNCTION("yxbGzBQC5xA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 ferror_unlocked);
    LIB_FUNCTION("Q-bLp+b-RVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fesetenv);
    LIB_FUNCTION("FuxaUZsWTok", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 fesetexceptflag);
    LIB_FUNCTION("hAJZ7-FBpEQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fesetround);
    LIB_FUNCTION("u5a7Ofymqlg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 fesettrapenable);
    LIB_FUNCTION("pVjisbvtQKU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fetestexcept);
    LIB_FUNCTION("YXQ4gXamCrY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, feupdateenv);
    LIB_FUNCTION("MUjC4lbHrK4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fflush);
    LIB_FUNCTION("AEuF3F2f8TA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fgetc);
    LIB_FUNCTION("KKgUiHSYGRE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fgetln);
    LIB_FUNCTION("SHlt7EhOtqA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fgetpos);
    LIB_FUNCTION("KdP-nULpuGw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fgets);
    LIB_FUNCTION("bzbQ5zQ2Y3g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fgetwc);
    LIB_FUNCTION("F81hKe2k2tg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fgetws);
    LIB_FUNCTION("Fm-dmyywH9Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fileno);
    LIB_FUNCTION("kxm0z4T5mMI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 fileno_unlocked);
    LIB_FUNCTION("TJFQFm+W3wg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, finite);
    LIB_FUNCTION("2nqzJ87zsB8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, finitef);
    LIB_FUNCTION("hGljHZEfF0U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, flockfile);
    LIB_FUNCTION("mpcTgMzhUY8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, floor);
    LIB_FUNCTION("mKhVDmYciWA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, floorf);
    LIB_FUNCTION("06QaR1Cpn-k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, floorl);
    LIB_FUNCTION("G3qjOUu7KnM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, flsl);
    LIB_FUNCTION("YKbL5KR6RDI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fma);
    LIB_FUNCTION("RpTR+VY15ss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fmaf);
    LIB_FUNCTION("uMeLdbwheag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fmal);
    LIB_FUNCTION("fiOgmWkP+Xc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fmax);
    LIB_FUNCTION("Lyx2DzUL7Lc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fmaxf);
    LIB_FUNCTION("0H5TVprQSkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fmaxl);
    LIB_FUNCTION("iU0z6SdUNbI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fmin);
    LIB_FUNCTION("uVRcM2yFdP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fminf);
    LIB_FUNCTION("DQ7K6s8euWY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fminl);
    LIB_FUNCTION("pKwslsMUmSk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fmod);
    LIB_FUNCTION("88Vv-AzHVj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fmodf);
    LIB_FUNCTION("A1R5T0xOyn8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fmodl);
    LIB_FUNCTION("xeYO4u7uyJ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fopen);
    LIB_FUNCTION("NL836gOLANs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fopen_s);
    LIB_FUNCTION("fffwELXNVFA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fprintf);
    LIB_FUNCTION("-e-F9HjUFp8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fprintf_s);
    LIB_FUNCTION("y1Ch2nXs4IQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fpurge);
    LIB_FUNCTION("aZK8lNei-Qw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fputc);
    LIB_FUNCTION("QrZZdJ8XsX0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fputs);
    LIB_FUNCTION("1QJWxoB6pCo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fputwc);
    LIB_FUNCTION("-7nRJFXMxnM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fputws);
    LIB_FUNCTION("lbB+UlZqVG0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fread);
    LIB_FUNCTION("tIhsqj0qsFE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, free);
    LIB_FUNCTION("N2OjwJJGjeQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, freeifaddrs);
    LIB_FUNCTION("gkWgn0p1AfU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, freopen);
    LIB_FUNCTION("NdvAi34vV3g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, freopen_s);
    LIB_FUNCTION("kA-TdiOCsaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, frexp);
    LIB_FUNCTION("aaDMGGkXFxo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, frexpf);
    LIB_FUNCTION("YZk9sHO0yNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, frexpl);
    LIB_FUNCTION("npLpPTaSuHg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fscanf);
    LIB_FUNCTION("vj2WUi2LrfE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fscanf_s);
    LIB_FUNCTION("rQFVBXp-Cxg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fseek);
    LIB_FUNCTION("pkYiKw09PRA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fseeko);
    LIB_FUNCTION("7PkSz+qnTto", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fsetpos);
    LIB_FUNCTION("6IM2up2+a-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fstatvfs);
    LIB_FUNCTION("Qazy8LmXTvw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ftell);
    LIB_FUNCTION("5qP1iVQkdck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ftello);
    LIB_FUNCTION("h05OHOMZNMw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ftrylockfile);
    LIB_FUNCTION("vAc9y8UQ31o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, funlockfile);
    LIB_FUNCTION("w6Aq68dFoP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fwide);
    LIB_FUNCTION("ZRAcn3dPVmA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fwprintf);
    LIB_FUNCTION("9kOFELic7Pk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fwprintf_s);
    LIB_FUNCTION("MpxhMh8QFro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fwrite);
    LIB_FUNCTION("a6CYO8YOzfw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fwscanf);
    LIB_FUNCTION("Bo5wtXSj4kc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, fwscanf_s);
    LIB_FUNCTION("BD-xV2fLe2M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, gamma);
    LIB_FUNCTION("q+AdV-EHiKc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, gamma_r);
    LIB_FUNCTION("sZ93QMbGRJY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, gammaf);
    LIB_FUNCTION("E3RYvWbYLgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, gammaf_r);
    LIB_FUNCTION("8Q60JLJ6Rv4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getc);
    LIB_FUNCTION("5tM252Rs2fc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 getc_unlocked);
    LIB_FUNCTION("L3XZiuKqZUM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getchar);
    LIB_FUNCTION("H0pVDvSuAVQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 getchar_unlocked);
    LIB_FUNCTION("DYivN1nO-JQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getcwd);
    LIB_FUNCTION("smbQukfxYJM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getenv);
    LIB_FUNCTION("-nvxBWa0iDs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, gethostname);
    LIB_FUNCTION("j-gWL6wDros", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getifaddrs);
    LIB_FUNCTION("VUtibKJCt1o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getopt);
    LIB_FUNCTION("8VVXJxB5nlk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getopt_long);
    LIB_FUNCTION("oths6jEyBqo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 getopt_long_only);
    LIB_FUNCTION("7Psx1DlAyE4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getprogname);
    LIB_FUNCTION("Ok+SYcoL19Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, gets);
    LIB_FUNCTION("lb+HLLZkbbw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, gets_s);
    LIB_FUNCTION("AoLA2MRWJvc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getw);
    LIB_FUNCTION("CosTELN5ETk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getwc);
    LIB_FUNCTION("n2mWDsholo8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, getwchar);
    LIB_FUNCTION("1mecP7RgI2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, gmtime);
    LIB_FUNCTION("5bBacGLyLOs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, gmtime_s);
    LIB_FUNCTION("YFoOw5GkkK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, hypot);
    LIB_FUNCTION("2HzgScoQq9o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, hypot3);
    LIB_FUNCTION("xlRcc7Rcqgo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, hypot3f);
    LIB_FUNCTION("aDmly36AAgI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, hypot3l);
    LIB_FUNCTION("iz2shAGFIxc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, hypotf);
    LIB_FUNCTION("jJC7x18ge8k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, hypotl);
    LIB_FUNCTION("ODGONXcSmz4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 ignore_handler_s);
    LIB_FUNCTION("h6pVBKjcLiU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ilogb);
    LIB_FUNCTION("0dQrYWd7g94", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ilogbf);
    LIB_FUNCTION("wXs12eD3uvA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ilogbl);
    LIB_FUNCTION("UgZ7Rhk60cQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, imaxabs);
    LIB_FUNCTION("V0X-mrfdM9E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, imaxdiv);
    LIB_FUNCTION("t3RFHn0bTPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, index);
    LIB_FUNCTION("sBBuXmJ5Kjk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, inet_addr);
    LIB_FUNCTION("ISTLytNGT0c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, inet_aton);
    LIB_FUNCTION("7iTp7O6VOXQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, inet_ntoa);
    LIB_FUNCTION("i3E1Ywn4t+8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, inet_ntoa_r);
    LIB_FUNCTION("IIUY-5hk-4k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, initstate);
    LIB_FUNCTION("4uJJNi+C9wk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isalnum);
    LIB_FUNCTION("+xU0WKT8mDc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isalpha);
    LIB_FUNCTION("lhnrCOBiTGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isblank);
    LIB_FUNCTION("akpGErA1zdg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iscntrl);
    LIB_FUNCTION("JWBr5N8zyNE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isdigit);
    LIB_FUNCTION("rrgxakQtvc0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isgraph);
    LIB_FUNCTION("2q5PPh7HsKE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isinf);
    LIB_FUNCTION("KqYTqtSfGos", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, islower);
    LIB_FUNCTION("20qj+7O69XY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isnan);
    LIB_FUNCTION("3pF7bUSIH8o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isnanf);
    LIB_FUNCTION("eGkOpTojJl4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isprint);
    LIB_FUNCTION("I6Z-684E2C4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ispunct);
    LIB_FUNCTION("wazw2x2m3DQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isspace);
    LIB_FUNCTION("GcFKlTJEMkI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isupper);
    LIB_FUNCTION("wDmL2EH0CBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswalnum);
    LIB_FUNCTION("D-qDARDb1aM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswalpha);
    LIB_FUNCTION("p6DbM0OAHNo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswblank);
    LIB_FUNCTION("6A+1YZ79qFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswcntrl);
    LIB_FUNCTION("45E7omS0vvc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswctype);
    LIB_FUNCTION("n0kT+8Eeizs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswdigit);
    LIB_FUNCTION("wjG0GyCyaP0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswgraph);
    LIB_FUNCTION("Ok8KPy3nFls", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswlower);
    LIB_FUNCTION("U7IhU4VEB-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswprint);
    LIB_FUNCTION("AEPvEZkaLsU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswpunct);
    LIB_FUNCTION("vqtytrxgLMs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswspace);
    LIB_FUNCTION("1QcrrL9UDRQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswupper);
    LIB_FUNCTION("cjmSjRlnMAs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, iswxdigit);
    LIB_FUNCTION("srzSVSbKn7M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, isxdigit);
    LIB_FUNCTION("tcN0ngcXegg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, j0);
    LIB_FUNCTION("RmE3aE8WHuY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, j0f);
    LIB_FUNCTION("BNbWdC9Jg+4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, j1);
    LIB_FUNCTION("uVXcivvVHzU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, j1f);
    LIB_FUNCTION("QdE7Arjzxos", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, jn);
    LIB_FUNCTION("M5KJmq-gKM8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, jnf);
    LIB_FUNCTION("M7KmRg9CERk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, jrand48);
    LIB_FUNCTION("xzZiQgReRGE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, labs);
    LIB_FUNCTION("wTjDJ6In3Cg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lcong48);
    LIB_FUNCTION("JrwFIMzKNr0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ldexp);
    LIB_FUNCTION("kn0yiYeExgA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ldexpf);
    LIB_FUNCTION("aX8H2+BBlWE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ldexpl);
    LIB_FUNCTION("gfP0im5Z3g0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ldiv);
    LIB_FUNCTION("o-kMHRBvkbQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lgamma);
    LIB_FUNCTION("EjL+gY1G2lk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lgamma_r);
    LIB_FUNCTION("i-ifjh3SLBU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lgammaf);
    LIB_FUNCTION("RlGUiqyKf9I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lgammaf_r);
    LIB_FUNCTION("lPYpsOb9s-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lgammal);
    LIB_FUNCTION("rHRr+131ATY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, llabs);
    LIB_FUNCTION("iVhJZvAO2aQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lldiv);
    LIB_FUNCTION("-431A-YBAks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, llrint);
    LIB_FUNCTION("KPsQA0pis8o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, llrintf);
    LIB_FUNCTION("6bRANWNYID0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, llrintl);
    LIB_FUNCTION("w-BvXF4O6xo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, llround);
    LIB_FUNCTION("eQhBFnTOp40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, llroundf);
    LIB_FUNCTION("wRs5S54zjm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, llroundl);
    LIB_FUNCTION("0hlfW1O4Aa4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, localeconv);
    LIB_FUNCTION("efhK-YSUYYQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, localtime);
    LIB_FUNCTION("fiiNDnNBKVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, localtime_s);
    LIB_FUNCTION("rtV7-jWC6Yg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log);
    LIB_FUNCTION("WuMbPBKN1TU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log10);
    LIB_FUNCTION("lhpd6Wk6ccs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log10f);
    LIB_FUNCTION("CT4aR0tBgkQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log10l);
    LIB_FUNCTION("VfsML+n9cDM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log1p);
    LIB_FUNCTION("MFe91s8apQk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log1pf);
    LIB_FUNCTION("77qd0ksTwdI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log1pl);
    LIB_FUNCTION("Y5DhuDKGlnQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log2);
    LIB_FUNCTION("hsi9drzHR2k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log2f);
    LIB_FUNCTION("CfOrGjBj-RY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, log2l);
    LIB_FUNCTION("owKuegZU4ew", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, logb);
    LIB_FUNCTION("RWqyr1OKuw4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, logbf);
    LIB_FUNCTION("txJTOe0Db6M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, logbl);
    LIB_FUNCTION("RQXLbdT2lc4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, logf);
    LIB_FUNCTION("EiHf-aLDImI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, logl);
    LIB_FUNCTION("lKEN2IebgJ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, longjmp);
    LIB_FUNCTION("5IpoNfxu84U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lrand48);
    LIB_FUNCTION("VOKOgR7L-2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lrint);
    LIB_FUNCTION("rcVv5ivMhY0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lrintf);
    LIB_FUNCTION("jp2e+RSrcow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lrintl);
    LIB_FUNCTION("J3XuGS-cC0Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lround);
    LIB_FUNCTION("C6gWCWJKM+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lroundf);
    LIB_FUNCTION("4ITASgL50uc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, lroundl);
    LIB_FUNCTION("GipcbdDM5cI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, makecontext);
    LIB_FUNCTION("gQX+4GDQjpM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, malloc);
    LIB_FUNCTION("ECOPpUQEch0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 malloc_check_memory_bounds);
    LIB_FUNCTION("J6FoFNydpFI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 malloc_finalize);
    LIB_FUNCTION("SlG1FN-y0N0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 malloc_get_footer_value);
    LIB_FUNCTION("Nmezc1Lh7TQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 malloc_get_malloc_state);
    LIB_FUNCTION("owT6zLJxrTs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 malloc_initialize);
    LIB_FUNCTION("0F08WOP8G3s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 malloc_report_memory_blocks);
    LIB_FUNCTION("CC-BLMBu9-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, malloc_stats);
    LIB_FUNCTION("KuOuD58hqn4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 malloc_stats_fast);
    LIB_FUNCTION("NDcSfcYZRC8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 malloc_usable_size);
    LIB_FUNCTION("hew0fReI2H0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mblen);
    LIB_FUNCTION("j6OnScWpu7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbrlen);
    LIB_FUNCTION("ogPDBoLmCcA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbrtoc16);
    LIB_FUNCTION("TEd4egxRmdE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbrtoc32);
    LIB_FUNCTION("qVHpv0PxouI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbrtowc);
    LIB_FUNCTION("UbnVmck+o10", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbsinit);
    LIB_FUNCTION("8hygs6D9KBY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbsrtowcs);
    LIB_FUNCTION("1NFvAuzw8dA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbsrtowcs_s);
    LIB_FUNCTION("VUzjXknPPBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbstowcs);
    LIB_FUNCTION("tdcAqgCS+uI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbstowcs_s);
    LIB_FUNCTION("6eU9xX9oEdQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mbtowc);
    LIB_FUNCTION("Ujf3KzMvRmI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memalign);
    LIB_FUNCTION("8u8lPzUEq+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memchr);
    LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memcmp);
    LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memcpy);
    LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memcpy_s);
    LIB_FUNCTION("+P6FRGH4LfA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memmove);
    LIB_FUNCTION("B59+zQQCcbU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memmove_s);
    LIB_FUNCTION("5G2ONUzRgjY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memrchr);
    LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memset);
    LIB_FUNCTION("h8GwqPFbu6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, memset_s);
    LIB_FUNCTION("HWEOv0+n7cU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mergesort);
    LIB_FUNCTION("n7AepwR0s34", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mktime);
    LIB_FUNCTION("0WMHDb5Dt94", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, modf);
    LIB_FUNCTION("3+UPM-9E6xY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, modff);
    LIB_FUNCTION("tG8pGyxdLEs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, modfl);
    LIB_FUNCTION("k-l0Jth-Go8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, mrand48);
    LIB_FUNCTION("zck+6bVj5pA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nan);
    LIB_FUNCTION("DZU+K1wozGI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nanf);
    LIB_FUNCTION("ZUvemFIkkhQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nanl);
    LIB_FUNCTION("cJLTwtKGXJk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nearbyint);
    LIB_FUNCTION("c+4r-T-tEIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nearbyintf);
    LIB_FUNCTION("6n23e0gIJ9s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nearbyintl);
    LIB_FUNCTION("ZT4ODD2Ts9o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 Need_sceLibcInternal);
    LIB_FUNCTION("h+J60RRlfnk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nextafter);
    LIB_FUNCTION("3m2ro+Di+Ck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nextafterf);
    LIB_FUNCTION("R0-hvihVoy0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nextafterl);
    LIB_FUNCTION("-Q6FYBO4sn0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nexttoward);
    LIB_FUNCTION("QaTrhMKUT18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nexttowardf);
    LIB_FUNCTION("ryyn6-WJm6U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nexttowardl);
    LIB_FUNCTION("3wcYIMz8LUo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, nrand48);
    LIB_FUNCTION("ay3uROQAc5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, opendir);
    LIB_FUNCTION("zG0BNJOZdm4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, optarg);
    LIB_FUNCTION("yaFXXViLWPw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, opterr);
    LIB_FUNCTION("zCnSJWp-Qj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, optind);
    LIB_FUNCTION("FwzVaZ8Vnus", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, optopt);
    LIB_FUNCTION("CZNm+oNmB-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, optreset);
    LIB_FUNCTION("EMutwaQ34Jo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, perror);
    LIB_FUNCTION("cVSk9y8URbc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_memalign);
    LIB_FUNCTION("3Nr9caNHhyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, posix_spawn);
    LIB_FUNCTION("Heh4KJwyoX8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawn_file_actions_addclose);
    LIB_FUNCTION("LG6O0oW9bQU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawn_file_actions_adddup2);
    LIB_FUNCTION("Sj7y+JO5PcM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawn_file_actions_addopen);
    LIB_FUNCTION("Ud8CbISKRGM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawn_file_actions_destroy);
    LIB_FUNCTION("p--TkNVsXjA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawn_file_actions_init);
    LIB_FUNCTION("Hq9-2AMG+ks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_destroy);
    LIB_FUNCTION("7BGUDQDJu-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_getflags);
    LIB_FUNCTION("Q-GfRQNi66I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_getpgroup);
    LIB_FUNCTION("jbgqYhmVEGY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_getschedparam);
    LIB_FUNCTION("KUYSaO1qv0Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_getschedpolicy);
    LIB_FUNCTION("7pASQ1hhH00", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_getsigdefault);
    LIB_FUNCTION("wvqDod5pVZg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_getsigmask);
    LIB_FUNCTION("44hlATrd47U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_init);
    LIB_FUNCTION("UV4m0bznVtU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_setflags);
    LIB_FUNCTION("aPDKI3J8PqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_setpgroup);
    LIB_FUNCTION("SFlW4kqPgU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_setschedparam);
    LIB_FUNCTION("fBne7gcou0s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_setschedpolicy);
    LIB_FUNCTION("Ani6e+T-y6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_setsigdefault);
    LIB_FUNCTION("wCavZQ+m1PA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 posix_spawnattr_setsigmask);
    LIB_FUNCTION("IUfBO5UIZNc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, posix_spawnp);
    LIB_FUNCTION("9LCjpWyQ5Zc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, pow);
    LIB_FUNCTION("1D0H2KNjshE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, powf);
    LIB_FUNCTION("95V3PF0kUEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, powl);
    LIB_FUNCTION("hcuQgD53UxM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, printf);
    LIB_FUNCTION("w1NxRBQqfmQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, printf_s);
    LIB_FUNCTION("tjuEJo1obls", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, psignal);
    LIB_FUNCTION("tLB5+4TEOK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, putc);
    LIB_FUNCTION("H-QeERgWuTM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 putc_unlocked);
    LIB_FUNCTION("m5wN+SwZOR4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, putchar);
    LIB_FUNCTION("v95AEAzqm+0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 putchar_unlocked);
    LIB_FUNCTION("t1ytXodWUH0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, putenv);
    LIB_FUNCTION("YQ0navp+YIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, puts);
    LIB_FUNCTION("DwcWtj3tSPA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, putw);
    LIB_FUNCTION("UZJnC81pUCw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, putwc);
    LIB_FUNCTION("aW9KhGC4cOo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, putwchar);
    LIB_FUNCTION("AEJdIVZTEmo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, qsort);
    LIB_FUNCTION("G7yOZJObV+4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, qsort_s);
    LIB_FUNCTION("qdGFBoLVNKI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, quick_exit);
    LIB_FUNCTION("cpCOXWMgha0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, rand);
    LIB_FUNCTION("dW3xsu3EgFI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, rand_r);
    LIB_FUNCTION("w1o05aHJT4c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, random);
    LIB_FUNCTION("lybyyKtP54c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, readdir);
    LIB_FUNCTION("J0kng1yac3M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, readdir_r);
    LIB_FUNCTION("Y7aJ1uydPMo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, realloc);
    LIB_FUNCTION("OGybVuPAhAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, reallocalign);
    LIB_FUNCTION("YMZO9ChZb0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, reallocf);
    LIB_FUNCTION("vhtcIgZG-Lk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, realpath);
    LIB_FUNCTION("pv2etu4pocs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, remainder);
    LIB_FUNCTION("eS+MVq+Lltw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, remainderf);
    LIB_FUNCTION("MvdnffYU3jg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, remainderl);
    LIB_FUNCTION("MZO7FXyAPU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, remove);
    LIB_FUNCTION("XI0YDgH8x1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, remquo);
    LIB_FUNCTION("AqpZU2Njrmk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, remquof);
    LIB_FUNCTION("Fwow0yyW0nI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, remquol);
    LIB_FUNCTION("3QIPIh-GDjw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, rewind);
    LIB_FUNCTION("kCKHi6JYtmM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, rewinddir);
    LIB_FUNCTION("CWiqHSTO5hk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, rindex);
    LIB_FUNCTION("LxGIYYKwKYc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, rint);
    LIB_FUNCTION("q5WzucyVSkM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, rintf);
    LIB_FUNCTION("Yy5yMiZHBIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, rintl);
    LIB_FUNCTION("nlaojL9hDtA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, round);
    LIB_FUNCTION("DDHG1a6+3q0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, roundf);
    LIB_FUNCTION("8F1ctQaP0uk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, roundl);
    LIB_FUNCTION("HI4N2S6ZWpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scalb);
    LIB_FUNCTION("rjak2Xm+4mE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scalbf);
    LIB_FUNCTION("7Jp3g-qTgZw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scalbln);
    LIB_FUNCTION("S6LHwvK4h8c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scalblnf);
    LIB_FUNCTION("NFxDIuqfmgw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scalblnl);
    LIB_FUNCTION("KGKBeVcqJjc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scalbn);
    LIB_FUNCTION("9fs1btfLoUs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scalbnf);
    LIB_FUNCTION("l3fh3QW0Tss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scalbnl);
    LIB_FUNCTION("7XEv6NnznWw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scanf);
    LIB_FUNCTION("-B76wP6IeVA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, scanf_s);
    LIB_FUNCTION("aqqpmI7-1j0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcDebugOut);
    LIB_FUNCTION("Sj3fKG7MwMk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapGetAddressRanges);
    LIB_FUNCTION("HFtbbWvBO+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapMutexCalloc);
    LIB_FUNCTION("jJKMkpqQr7I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapMutexFree);
    LIB_FUNCTION("4iOzclpv1M0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapSetAddressRangeCallback);
    LIB_FUNCTION("M6qiY0nhk54", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapSetTraceMarker);
    LIB_FUNCTION("RlhJVAYLSqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcHeapUnsetTraceMarker);
    LIB_FUNCTION("YrL-1y6vfyo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcInternalMemoryGetWakeAddr);
    LIB_FUNCTION("h8jq9ee4h5c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcInternalMemoryMutexEnable);
    LIB_FUNCTION("LXqt47GvaRA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcInternalSetMallocCallback);
    LIB_FUNCTION("ljkqMcC4-mk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceAlignedAlloc);
    LIB_FUNCTION("LYo3GhIlB38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceCalloc);
    LIB_FUNCTION("-hn1tcVHq5Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceCreate);
    LIB_FUNCTION("W6SiVSiCDtI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceDestroy);
    LIB_FUNCTION("Vla-Z+eXlxo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceFree);
    LIB_FUNCTION("raRgiuQfvWk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceGetAddressRanges);
    LIB_FUNCTION("pzUa7KEoydw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceIsHeapEmpty);
    LIB_FUNCTION("OJjm-QOIHlI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMalloc);
    LIB_FUNCTION("mfHdJTIvhuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMallocStats);
    LIB_FUNCTION("k04jLXu3+Ic", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMallocStatsFast);
    LIB_FUNCTION("fEoW6BJsPt4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMallocUsableSize);
    LIB_FUNCTION("iF1iQHzxBJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceMemalign);
    LIB_FUNCTION("qWESlyXMI3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspacePosixMemalign);
    LIB_FUNCTION("gigoVHZvVPE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceRealloc);
    LIB_FUNCTION("p6lrRW8-MLY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceReallocalign);
    LIB_FUNCTION("+CbwGRMnlfU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcMspaceSetMallocCallback);
    LIB_FUNCTION("HmgKoOWpUc8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sceLibcOnce);
    LIB_FUNCTION("-lZdT34nAAE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceCalloc);
    LIB_FUNCTION("Pcq7UoYAcFE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceCheckMemoryBounds);
    LIB_FUNCTION("6hdfGRKHefs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceCreate);
    LIB_FUNCTION("qB5nGjWa-bk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceDestroy);
    LIB_FUNCTION("9mMuuhXMwqQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceFree);
    LIB_FUNCTION("kv4kgdjswN0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceGetFooterValue);
    LIB_FUNCTION("htdTOnMxDbQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceIsHeapEmpty);
    LIB_FUNCTION("QuZzFJD5Hrw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMalloc);
    LIB_FUNCTION("mO8NB8whKy8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMallocStats);
    LIB_FUNCTION("OmG3YPCBLJs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMallocStatsFast);
    LIB_FUNCTION("6JcY5RDA4jY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMallocUsableSize);
    LIB_FUNCTION("PKJcFUfhKtw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceMemalign);
    LIB_FUNCTION("7hOUKGcT6jM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspacePosixMemalign);
    LIB_FUNCTION("u32UXVridxQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceRealloc);
    LIB_FUNCTION("4SvlEtd0j40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceReallocalign);
    LIB_FUNCTION("0FnzR6qum90", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceReportMemoryBlocks);
    LIB_FUNCTION("AUYdq63RG3U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 sceLibcPafMspaceTrim);
    LIB_FUNCTION("2g5wco7AAHg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, seed48);
    LIB_FUNCTION("7WoI+lVawlc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, seekdir);
    LIB_FUNCTION("ENLfKJEZTjE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 set_constraint_handler_s);
    LIB_FUNCTION("vZMcAfsA31I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, setbuf);
    LIB_FUNCTION("M4YYbSFfJ8g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, setenv);
    LIB_FUNCTION("gNQ1V2vfXDE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, setjmp);
    LIB_FUNCTION("PtsB1Q9wsFA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, setlocale);
    LIB_FUNCTION("woQta4WRpk0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, setstate);
    LIB_FUNCTION("QMFyLoqNxIg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, setvbuf);
    LIB_FUNCTION("Bm3k7JQMN5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sigblock);
    LIB_FUNCTION("TsrS8nGDQok", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, siginterrupt);
    LIB_FUNCTION("SQGxZCv3aYk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 signalcontext);
    LIB_FUNCTION("5gOOC0kzW0c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, signgam);
    LIB_FUNCTION("Az3tTyAy380", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, significand);
    LIB_FUNCTION("L2YaHYQdmHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, significandf);
    LIB_FUNCTION("cJvOg1KV8uc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sigsetmask);
    LIB_FUNCTION("yhxKO9LYc8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sigvec);
    LIB_FUNCTION("H8ya2H00jbI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sin);
    LIB_FUNCTION("jMB7EFyu30Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sincos);
    LIB_FUNCTION("pztV4AF18iI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sincosf);
    LIB_FUNCTION("Q4rRL34CEeE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sinf);
    LIB_FUNCTION("ZjtRqSMJwdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sinh);
    LIB_FUNCTION("1t1-JoZ0sZQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sinhf);
    LIB_FUNCTION("lYdqBvDgeHU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sinhl);
    LIB_FUNCTION("vxgqrJxDPHo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sinl);
    LIB_FUNCTION("eLdDw6l0-bU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, snprintf);
    LIB_FUNCTION("3BytPOQgVKc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, snprintf_s);
    LIB_FUNCTION("jbj2wBoiCyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, snwprintf_s);
    LIB_FUNCTION("tcVi5SivF7Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sprintf);
    LIB_FUNCTION("xEszJVGpybs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sprintf_s);
    LIB_FUNCTION("MXRNWnosNlM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sqrt);
    LIB_FUNCTION("Q+xU11-h0xQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sqrtf);
    LIB_FUNCTION("RIkUZRadZgc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sqrtl);
    LIB_FUNCTION("VPbJwTCgME0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, srand);
    LIB_FUNCTION("+KSnjvZ0NMc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, srand48);
    LIB_FUNCTION("sPC7XE6hfFY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, srandom);
    LIB_FUNCTION("a2MOZf++Wjg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, srandomdev);
    LIB_FUNCTION("1Pk0qZQGeWo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sscanf);
    LIB_FUNCTION("24m4Z4bUaoY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sscanf_s);
    LIB_FUNCTION("ayTeobcoGj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, statvfs);
    LIB_FUNCTION("+CUrIMpVuaM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, stderr);
    LIB_FUNCTION("omQZ36ESr98", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, stdin);
    LIB_FUNCTION("3eGXiXpFLt0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, stdout);
    LIB_FUNCTION("ZSnX-xZBGCg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, stpcpy);
    LIB_FUNCTION("AV6ipCNa4Rw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strcasecmp);
    LIB_FUNCTION("Ls4tzzhimqQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strcat);
    LIB_FUNCTION("K+gcnFFJKVc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strcat_s);
    LIB_FUNCTION("ob5xAW4ln-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strchr);
    LIB_FUNCTION("Ovb2dSJOAuE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strcmp);
    LIB_FUNCTION("gjbmYpP-XJQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strcoll);
    LIB_FUNCTION("kiZSXIWd9vg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strcpy);
    LIB_FUNCTION("5Xa2ACNECdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strcpy_s);
    LIB_FUNCTION("q0F6yS-rCms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strcspn);
    LIB_FUNCTION("g7zzzLDYGw0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strdup);
    LIB_FUNCTION("RIa6GnWp+iU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strerror);
    LIB_FUNCTION("RBcs3uut1TA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strerror_r);
    LIB_FUNCTION("o+ok6Y+DtgY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strerror_s);
    LIB_FUNCTION("-g26XITGVgE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 strerrorlen_s);
    LIB_FUNCTION("Av3zjWi64Kw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strftime);
    LIB_FUNCTION("ByfjUZsWiyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strlcat);
    LIB_FUNCTION("SfQIZcqvvms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strlcpy);
    LIB_FUNCTION("j4ViWNHEgww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strlen);
    LIB_FUNCTION("pXvbDfchu6k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strncasecmp);
    LIB_FUNCTION("kHg45qPC6f0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strncat);
    LIB_FUNCTION("NC4MSB+BRQg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strncat_s);
    LIB_FUNCTION("aesyjrHVWy4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strncmp);
    LIB_FUNCTION("6sJWiWSRuqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strncpy);
    LIB_FUNCTION("YNzNkJzYqEg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strncpy_s);
    LIB_FUNCTION("XGnuIBmEmyk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strndup);
    LIB_FUNCTION("5jNubw4vlAA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strnlen);
    LIB_FUNCTION("DQbtGaBKlaw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strnlen_s);
    LIB_FUNCTION("Xnrfb2-WhVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strnstr);
    LIB_FUNCTION("kDZvoVssCgQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strpbrk);
    LIB_FUNCTION("9yDWMxEFdJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strrchr);
    LIB_FUNCTION("cJWGxiQPmDQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strsep);
    LIB_FUNCTION("-kU6bB4M-+k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strspn);
    LIB_FUNCTION("viiwFMaNamA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strstr);
    LIB_FUNCTION("2vDqwBlpF-o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtod);
    LIB_FUNCTION("xENtRue8dpI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtof);
    LIB_FUNCTION("q5MWYCDfu3c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtoimax);
    LIB_FUNCTION("oVkZ8W8-Q8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtok);
    LIB_FUNCTION("enqPGLfmVNU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtok_r);
    LIB_FUNCTION("-vXEQdRADLI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtok_s);
    LIB_FUNCTION("mXlxhmLNMPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtol);
    LIB_FUNCTION("nW9JRkciRk4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtold);
    LIB_FUNCTION("VOBg+iNwB-4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtoll);
    LIB_FUNCTION("QxmSHBCuKTk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtoul);
    LIB_FUNCTION("5OqszGpy7Mg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtoull);
    LIB_FUNCTION("QNyUWGXmXNc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtoumax);
    LIB_FUNCTION("g-McpZfseZo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strtouq);
    LIB_FUNCTION("zogPrkd46DY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, strxfrm);
    LIB_FUNCTION("nJz16JE1txM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, swprintf);
    LIB_FUNCTION("Im55VJ-Bekc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, swprintf_s);
    LIB_FUNCTION("HNnWdT43ues", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, swscanf);
    LIB_FUNCTION("tQNolUV1q5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, swscanf_s);
    LIB_FUNCTION("ZD+Dp+-LsGg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sys_nsig);
    LIB_FUNCTION("yCdGspbNHZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sys_siglist);
    LIB_FUNCTION("Y16fu+FC+3Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, sys_signame);
    LIB_FUNCTION("UNS2V4S097M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, syslog);
    LIB_FUNCTION("T7uyNqP7vQA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tan);
    LIB_FUNCTION("ZE6RNL+eLbk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tanf);
    LIB_FUNCTION("JM4EBvWT9rc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tanh);
    LIB_FUNCTION("SAd0Z3wKwLA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tanhf);
    LIB_FUNCTION("JCmHsYVc2eo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tanhl);
    LIB_FUNCTION("QL+3q43NfEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tanl);
    LIB_FUNCTION("RZA5RZawY04", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, telldir);
    LIB_FUNCTION("b7J3q7-UABY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tgamma);
    LIB_FUNCTION("B2ZbqV9geCM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tgammaf);
    LIB_FUNCTION("FU03r76UxaU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tgammal);
    LIB_FUNCTION("wLlFkwG9UcQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, time);
    LIB_FUNCTION("-Oet9AHzwtU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, timezone);
    LIB_FUNCTION("PqF+kHW-2WQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tolower);
    LIB_FUNCTION("TYE4irxSmko", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, toupper);
    LIB_FUNCTION("BEKIcbCV-MU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, towctrans);
    LIB_FUNCTION("J3J1T9fjUik", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, towlower);
    LIB_FUNCTION("1uf1SQsj5go", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, towupper);
    LIB_FUNCTION("a4gLGspPEDM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, trunc);
    LIB_FUNCTION("Vo8rvWtZw3g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, truncf);
    LIB_FUNCTION("apdxz6cLMh8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, truncl);
    LIB_FUNCTION("BAYjQubSZT8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tzname);
    LIB_FUNCTION("gYFKAMoNEfo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, tzset);
    LIB_FUNCTION("-LFO7jhD5CE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ungetc);
    LIB_FUNCTION("Nz7J62MvgQs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ungetwc);
    LIB_FUNCTION("CRJcH8CnPSI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, unsetenv);
    LIB_FUNCTION("1nTKA7pN1jw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, utime);
    LIB_FUNCTION("qjBlw2cVMAM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vasprintf);
    LIB_FUNCTION("aoTkxU86Mr4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, verr);
    LIB_FUNCTION("7Pc0nOTw8po", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, verrc);
    LIB_FUNCTION("ItC2hTrYvHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, verrx);
    LIB_FUNCTION("pDBDcY6uLSA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vfprintf);
    LIB_FUNCTION("GhTZtaodo7o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vfprintf_s);
    LIB_FUNCTION("lckWSkHDBrY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vfscanf);
    LIB_FUNCTION("JjPXy-HX5dY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vfscanf_s);
    LIB_FUNCTION("M2bGWSqt764", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vfwprintf);
    LIB_FUNCTION("XX9KWzJvRf0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vfwprintf_s);
    LIB_FUNCTION("WF4fBmip+38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vfwscanf);
    LIB_FUNCTION("Wvm90I-TGl0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vfwscanf_s);
    LIB_FUNCTION("GMpvxPFW924", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vprintf);
    LIB_FUNCTION("YfJUGNPkbK4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vprintf_s);
    LIB_FUNCTION("j7Jk3yd3yC8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vscanf);
    LIB_FUNCTION("fQYpcUzy3zo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vscanf_s);
    LIB_FUNCTION("Q2V+iqvjgC0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vsnprintf);
    LIB_FUNCTION("rWSuTWY2JN0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vsnprintf_s);
    LIB_FUNCTION("8SKVXgeK1wY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vsnwprintf_s);
    LIB_FUNCTION("jbz9I9vkqkk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vsprintf);
    LIB_FUNCTION("+qitMEbkSWk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vsprintf_s);
    LIB_FUNCTION("UTrpOVLcoOA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vsscanf);
    LIB_FUNCTION("tfNbpqL3D0M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vsscanf_s);
    LIB_FUNCTION("u0XOsuOmOzc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vswprintf);
    LIB_FUNCTION("oDoV9tyHTbA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vswprintf_s);
    LIB_FUNCTION("KGotca3AjYw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vswscanf);
    LIB_FUNCTION("39HHkIWrWNo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vswscanf_s);
    LIB_FUNCTION("zxecOOffO68", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vsyslog);
    LIB_FUNCTION("s67G-KeDKOo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vwarn);
    LIB_FUNCTION("BfAsxVvQVTQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vwarnc);
    LIB_FUNCTION("iH+oMJn8YPk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vwarnx);
    LIB_FUNCTION("QuF2rZGE-v8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vwprintf);
    LIB_FUNCTION("XPrliF5n-ww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vwprintf_s);
    LIB_FUNCTION("QNwdOK7HfJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vwscanf);
    LIB_FUNCTION("YgZ6qvFH3QI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, vwscanf_s);
    LIB_FUNCTION("3Rhy2gXDhwc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, warn);
    LIB_FUNCTION("AqUBdZqHZi4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, warnc);
    LIB_FUNCTION("aNJaYyn0Ujo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, warnx);
    LIB_FUNCTION("y9OoA+P5cjk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcrtomb);
    LIB_FUNCTION("oAlR5z2iiCA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcrtomb_s);
    LIB_FUNCTION("KZm8HUIX2Rw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcscat);
    LIB_FUNCTION("MqeMaVUiyU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcscat_s);
    LIB_FUNCTION("Ezzq78ZgHPs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcschr);
    LIB_FUNCTION("pNtJdE3x49E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcscmp);
    LIB_FUNCTION("fV2xHER+bKE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcscoll);
    LIB_FUNCTION("FM5NPnLqBc8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcscpy);
    LIB_FUNCTION("6f5f-qx4ucA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcscpy_s);
    LIB_FUNCTION("7eNus40aGuk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcscspn);
    LIB_FUNCTION("XbVXpf5WF28", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsftime);
    LIB_FUNCTION("WkkeywLJcgU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcslen);
    LIB_FUNCTION("pA9N3VIgEZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsncat);
    LIB_FUNCTION("VxG0990tP3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsncat_s);
    LIB_FUNCTION("E8wCoUEbfzk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsncmp);
    LIB_FUNCTION("0nV21JjYCH8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsncpy);
    LIB_FUNCTION("Slmz4HMpNGs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsncpy_s);
    LIB_FUNCTION("K+v+cnmGoH4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsnlen_s);
    LIB_FUNCTION("H4MCONF+Gps", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcspbrk);
    LIB_FUNCTION("g3ShSirD50I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsrchr);
    LIB_FUNCTION("sOOMlZoy1pg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsrtombs);
    LIB_FUNCTION("79s2tnYQI6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsrtombs_s);
    LIB_FUNCTION("x9uumWcxhXU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsspn);
    LIB_FUNCTION("WDpobjImAb4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsstr);
    LIB_FUNCTION("7-a7sBHeUQ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstod);
    LIB_FUNCTION("7SXNu+0KBYQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstof);
    LIB_FUNCTION("ljFisaQPwYQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstoimax);
    LIB_FUNCTION("8ngzWNZzFJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstok);
    LIB_FUNCTION("dsXnVxORFdc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstok_s);
    LIB_FUNCTION("d3dMyWORw8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstol);
    LIB_FUNCTION("LEbYWl9rBc8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstold);
    LIB_FUNCTION("34nH7v2xvNQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstoll);
    LIB_FUNCTION("v7S7LhP2OJc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstombs);
    LIB_FUNCTION("sZLrjx-yEx4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstombs_s);
    LIB_FUNCTION("5AYcEn7aoro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstoul);
    LIB_FUNCTION("DAbZ-Vfu6lQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstoull);
    LIB_FUNCTION("1e-q5iIukH8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcstoumax);
    LIB_FUNCTION("VuMMb5CfpEw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wcsxfrm);
    LIB_FUNCTION("CL7VJxznu6g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wctob);
    LIB_FUNCTION("7PxmvOEX3oc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wctomb);
    LIB_FUNCTION("y3V0bIq38NE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wctomb_s);
    LIB_FUNCTION("seyrqIc4ovc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wctrans);
    LIB_FUNCTION("+3PtYiUxl-U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wctype);
    LIB_FUNCTION("fnUEjBCNRVU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wmemchr);
    LIB_FUNCTION("QJ5xVfKkni0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wmemcmp);
    LIB_FUNCTION("fL3O02ypZFE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wmemcpy);
    LIB_FUNCTION("BTsuaJ6FxKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wmemcpy_s);
    LIB_FUNCTION("Noj9PsJrsa8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wmemmove);
    LIB_FUNCTION("F8b+Wb-YQVs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wmemmove_s);
    LIB_FUNCTION("Al8MZJh-4hM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wmemset);
    LIB_FUNCTION("OGVdXU3E-xg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wprintf);
    LIB_FUNCTION("FEtOJURNey0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wprintf_s);
    LIB_FUNCTION("D8JBAR3RiZQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wscanf);
    LIB_FUNCTION("RV7X3FrWfTI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, wscanf_s);
    LIB_FUNCTION("inwDBwEvw18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, xtime_get);
    LIB_FUNCTION("RvsFE8j3C38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, y0);
    LIB_FUNCTION("+tfKv1vt0QQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, y0f);
    LIB_FUNCTION("vh9aGR3ALP0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, y1);
    LIB_FUNCTION("gklG+J87Pq4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, y1f);
    LIB_FUNCTION("eWSt5lscApo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, yn);
    LIB_FUNCTION("wdPaII721tY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, ynf);
    LIB_FUNCTION("GG6441JdYkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 Func_186EB8E3525D6240);
    LIB_FUNCTION("QZ9YgTk+yrE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 Func_419F5881393ECAB1);
    LIB_FUNCTION("bGuDd3kWVKQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 Func_6C6B8377791654A4);
    LIB_FUNCTION("f9LVyN8Ky8g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 Func_7FD2D5C8DF0ACBC8);
    LIB_FUNCTION("wUqJ0psUjDo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 Func_C14A89D29B148C3A);
};

} // namespace Libraries::LibcInternal