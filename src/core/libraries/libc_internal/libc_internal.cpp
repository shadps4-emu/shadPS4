// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include <csetjmp>
#include <string>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libc_internal/libc_internal_io.h"
#include "core/libraries/libc_internal/libc_internal_memory.h"
#include "core/libraries/libc_internal/libc_internal_mspace.h"
#include "core/libraries/libc_internal/libc_internal_str.h"
#include "core/libraries/libc_internal/libc_internal_stream.h"
#include "core/libraries/libs.h"
#include "libc_internal.h"

namespace Libraries::LibcInternal {

s32 PS4_SYSV_ABI internal_sceLibcHeapGetTraceInfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___absvdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___absvsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___absvti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___adddf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___addsf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___addvdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___addvsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___addvti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ashldi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ashlti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ashrdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ashrti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_compare_exchange() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_compare_exchange_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_compare_exchange_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_compare_exchange_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_compare_exchange_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_compare_exchange_n() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_exchange() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_exchange_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_exchange_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_exchange_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_exchange_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_exchange_n() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_add_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_add_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_add_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_add_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_and_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_and_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_and_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_and_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_or_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_or_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_or_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_or_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_sub_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_sub_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_sub_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_sub_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_xor_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_xor_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_xor_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_fetch_xor_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_is_lock_free() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_load() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_load_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_load_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_load_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_load_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_load_n() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_store() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_store_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_store_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_store_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_store_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___atomic_store_n() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cleanup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___clzdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___clzsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___clzti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cmpdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cmpti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ctzdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ctzsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ctzti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_allocate_dependent_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_allocate_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_atexit(void (*func)(), void* arg, void* dso_handle) {
    LOG_ERROR(Lib_LibcInternal, "(TEST) called"); // todo idek what I'm doing with this
    std::atexit(func);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_bad_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_bad_typeid() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_begin_catch() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_call_unexpected() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_current_exception_type() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_current_primary_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_decrement_exception_refcount() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_demangle() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_end_catch() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_finalize() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_free_dependent_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_free_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_get_exception_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_get_globals() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_get_globals_fast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_guard_abort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_guard_acquire() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_guard_release() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_increment_exception_refcount() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_pure_virtual() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_rethrow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_rethrow_primary_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___cxa_throw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divdc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divdf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divmoddi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divmodsi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divsc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divsf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___divxc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___dynamic_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___eqdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___eqsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___extendsfdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fe_dfl_env() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fedisableexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___feenableexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fflush() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ffsdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ffsti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixdfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixdfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixdfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixsfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixsfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixsfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixunsdfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixunsdfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixunsdfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixunssfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixunssfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixunssfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixunsxfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixunsxfsi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixunsxfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixxfdi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fixxfti() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatdidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatdisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatdixf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatsidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatsisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floattidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floattisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floattixf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatundidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatundisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatundixf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatunsidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatunsisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatuntidf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatuntisf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___floatuntixf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fpclassifyd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fpclassifyf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___fpclassifyl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___gedf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___gesf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___gtdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___gtsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___gxx_personality_v0() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___inet_addr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___inet_aton() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___inet_ntoa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___inet_ntoa_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isfinite() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isfinitef() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isfinitel() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isinf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isinff() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isinfl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isnan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isnanf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isnanl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isnormal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isnormalf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isnormall() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___isthreaded() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___kernel_cos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___kernel_cosdf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___kernel_rem_pio2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___kernel_sin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___kernel_sindf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ledf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___lesf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___longjmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___lshrdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___lshrti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ltdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ltsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mb_cur_max() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mb_sb_limit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___moddi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___modsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___modti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___muldc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___muldf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___muldi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mulodi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mulosi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___muloti4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mulsc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mulsf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___multi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mulvdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mulvsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mulvti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___mulxc3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___nedf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___negdf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___negdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___negsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___negti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___negvdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___negvsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___negvti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___nesf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___opendir2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___paritydi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___paritysi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___parityti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___popcountdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___popcountsi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___popcountti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___powidf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___powisf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___powixf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___signbit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___signbitf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___signbitl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___srefill() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___srget() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___stderrp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___stdinp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___stdoutp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___subdf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___subsf3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___subvdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___subvsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___subvti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___swbuf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___sync_fetch_and_add_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___sync_fetch_and_and_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___sync_fetch_and_or_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___sync_fetch_and_sub_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___sync_fetch_and_xor_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___sync_lock_test_and_set_16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___truncdfsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ucmpdi2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___ucmpti2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___udivdi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___udivmoddi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___udivmodsi4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___udivmodti4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___udivsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___udivti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___umoddi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___umodsi3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___umodti3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___unorddf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal___unordsf2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Assert() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_strong() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_strong_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_strong_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_strong_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_strong_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_weak() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_weak_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_weak_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_weak_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_compare_exchange_weak_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_copy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_exchange() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_exchange_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_exchange_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_exchange_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_exchange_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_add_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_add_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_add_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_add_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_and_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_and_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_and_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_and_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_or_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_or_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_or_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_or_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_sub_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_sub_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_sub_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_sub_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_xor_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_xor_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_xor_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_fetch_xor_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_flag_clear() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_flag_test_and_set() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_is_lock_free_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_is_lock_free_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_is_lock_free_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_is_lock_free_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_load_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_load_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_load_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_load_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_signal_fence() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_store_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_store_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_store_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_store_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atomic_thread_fence() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atqexit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Atthreadexit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Btowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Call_once() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Call_onceEx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Clocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Closreg() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_broadcast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_destroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_do_broadcast_at_thread_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_init_with_name() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_register_at_thread_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_signal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_timedwait() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_unregister_at_thread_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cnd_wait() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Cosh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Costate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__CTinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Ctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__CurrentRuneLocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__CWcsxfrm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Daysto() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Dbl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Dclass() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__DefaultRuneLocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Deletegloballocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Denorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Dint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Divide() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Dnorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Do_call() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Dscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Dsign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Dtento() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Dtest() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Dunscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Eps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Erf_one() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Erf_small() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Erfc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__err() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Errno() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Exp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fac_tidy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fail_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FAtan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FCosh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDclass() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDenorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDivide() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDnorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDsign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDtento() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDtest() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FDunscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FEps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Feraise() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FErf_one() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FErf_small() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FErfc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_add_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_and_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_and_seq_cst_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_and_seq_cst_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_and_seq_cst_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_or_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_or_seq_cst_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_or_seq_cst_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_or_seq_cst_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_xor_8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_xor_seq_cst_1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_xor_seq_cst_2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fetch_xor_seq_cst_4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FExp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FFpcomp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FGamma_big() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fgpos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FHypot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Files() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FInf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FLog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FLogpoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Flt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fltrounds() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FNan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fofind() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fofree() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Foprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fpcomp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FPlsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FPmsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FPoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FPow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FQuad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FQuadph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FRecip() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FRint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Frprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FRteps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FSin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FSincos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FSinh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FSnan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fspos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FTan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FTgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Fwprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXbig() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_addh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_addx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_getw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_invx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_ldexpx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_movx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_mulh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_mulx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_setn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_setw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_sqrtx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FXp_subx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__FZero() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Gamma_big() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Genld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Gentime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getcloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getctyptab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getdst() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Geterrno() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getfld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getfloat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getgloballocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getmbcurmax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getpcostate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getpctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getpmbstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__getprogname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getptimes() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getptolower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getptoupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getpwcostate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getpwcstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getpwctrtab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getpwctytab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Gettime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Getzone() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Hugeval() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Hypot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Inf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__init_env() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__init_tls() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Isdst() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Iswctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LAtan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LCosh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Ldbl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDclass() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDenorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDivide() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDnorm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDsign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDtento() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDtest() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Ldtob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LDunscale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LEps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LErf_one() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LErf_small() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LErfc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LExp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LFpcomp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LGamma_big() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LHypot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LInf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Litob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LLog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LLogpoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LNan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Lock_shared_ptr_spin_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Lock_spin_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Lockfilelock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Locksyslock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Locsum() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Loctab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Locterm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Locvar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Log() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Logpoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LPlsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LPmsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LPoly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LPow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LQuad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LQuadph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LRecip() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LRint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LRteps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LSin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LSincos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LSinh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LSnan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LTan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LTgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXbig() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_addh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_addx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_getw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_invx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_ldexpx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_movx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_mulh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_mulx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_setn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_setw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_sqrtx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LXp_subx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__LZero() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Makeloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Makestab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Makewct() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mbcurmax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mbstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mbtowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mbtowcx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtx_current_owns() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtx_destroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtx_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtx_init_with_name() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtx_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtx_timedlock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtx_trylock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtx_unlock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtxdst() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtxinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtxlock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Mtxunlock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Nan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__new_setup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Nnl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__PathLocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__PJP_C_Copyright() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__PJP_CPP_Copyright() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Plsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Pmsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Poly() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Pow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Putfld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Putstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Puttxt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Quad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Quadph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Randseed() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__readdir_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Readloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Recip() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__reclaim_telldir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Restore_state() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Rint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Rteps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__rtld_addr_phdr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__rtld_atfork_post() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__rtld_atfork_pre() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__rtld_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__rtld_get_stack_prot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__rtld_thread_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Save_state() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__SceLibcDebugOut() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__SceLibcTelemetoryOut() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__seekdir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Setgloballocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Shared_ptr_flag() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Sin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Sincos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Sinh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Skip() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Snan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stderr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stdin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Stdout() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tan() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_abort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_create() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_current() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_detach() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_equal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_id() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_join() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_lt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_sleep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_start() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_start_with_attr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_start_with_name() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_start_with_name_attr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Thrd_yield() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__thread_autoinit_dummy_decl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__thread_autoinit_dummy_decl_stub() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__thread_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__thread_init_stub() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Times() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Costate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Ctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Errno() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Mbcurmax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Mbstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Times() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Tolotab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Touptab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__WCostate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Wcstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Wctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tls_setup__Wctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tolotab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Touptab() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Towctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tss_create() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tss_delete() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tss_get() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tss_set() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Ttotm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Tzoff() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Unlock_shared_ptr_spin_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Unlock_spin_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Unlockfilelock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Unlocksyslock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Unwind_Backtrace() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Unwind_GetIP() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Unwind_Resume() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Unwind_Resume_or_Rethrow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Vacopy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__warn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WCostate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Wcscollx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Wcsftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Wcstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Wcsxfrmx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Wctob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Wctomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Wctombx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Wctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Wctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WFrprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WFwprep() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WGenld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WGetfld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WGetfloat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WGetint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WGetstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WLdtob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WLitob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WPutfld() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WPutstr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WPuttxt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStod() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStodx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStoflt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStofx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStold() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStoldx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStopfx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStoul() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStoull() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__WStoxflt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xbig() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_addh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_addx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_getw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_invx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_ldexpx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_movx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_mulh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_mulx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_setn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_setw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_sqrtx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xp_subx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xtime_diff_to_ts() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xtime_get_ticks() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Xtime_to_ts() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdaPvm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdaPvmRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdaPvmSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdaPvRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdaPvS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdaPvSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdaPvSt11align_val_tRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdlPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdlPvm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdlPvmRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdlPvmSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdlPvRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdlPvS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdlPvSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZdlPvSt11align_val_tRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Zero() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt10moneypunctIcLb0EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt10moneypunctIcLb1EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt10moneypunctIwLb0EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt10moneypunctIwLb1EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt14_Error_objectsIiE14_System_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt14_Error_objectsIiE15_Generic_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt20_Future_error_objectIiE14_Future_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt7codecvtIcc9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt7collateIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt7collateIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt8messagesIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt8messagesIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt8numpunctIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZGVNSt8numpunctIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
internal__ZGVZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
internal__ZGVZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv116__enum_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv116__enum_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv116__enum_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv117__array_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv117__array_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv117__array_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv117__class_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv117__class_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv117__class_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv117__pbase_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv117__pbase_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv117__pbase_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv119__pointer_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv119__pointer_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv119__pointer_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv120__function_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv120__function_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv120__function_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv120__si_class_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv120__si_class_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv120__si_class_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv121__vmi_class_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv121__vmi_class_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv121__vmi_class_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv123__fundamental_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv123__fundamental_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv123__fundamental_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv129__pointer_to_member_type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv129__pointer_to_member_type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN10__cxxabiv129__pointer_to_member_type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN6Dinkum7codecvt10_Cvt_checkEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN6Dinkum7threads10lock_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN6Dinkum7threads10lock_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN6Dinkum7threads17_Throw_lock_errorEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN6Dinkum7threads21_Throw_resource_errorEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN6Dinkum7threads21thread_resource_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZN6Dinkum7threads21thread_resource_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__Znam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZnamRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZnamSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZnamSt11align_val_tRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSbIwSt11char_traitsIwESaIwEE5_XlenEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSbIwSt11char_traitsIwESaIwEE5_XranEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSs5_XlenEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSs5_XranEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt10bad_typeid4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt10bad_typeid8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt11logic_error4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt11logic_error8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt12bad_weak_ptr4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt12codecvt_base11do_encodingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt12codecvt_base13do_max_lengthEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt12future_error4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt12future_error8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt12system_error8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt13bad_exception8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt13runtime_error4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt14error_category10equivalentEiRKSt15error_condition() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt14error_category10equivalentERKSt10error_codei() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt14error_category23default_error_conditionEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt17bad_function_call4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt18bad_variant_access4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt22_Future_error_category4nameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt22_Future_error_category7messageEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt22_System_error_category23default_error_conditionEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt22_System_error_category4nameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt22_System_error_category7messageEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt23_Generic_error_category4nameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt23_Generic_error_category7messageEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIcE10do_tolowerEc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIcE10do_tolowerEPcPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIcE10do_toupperEc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIcE10do_toupperEPcPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIcE8do_widenEc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIcE8do_widenEPKcS2_Pc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIcE9do_narrowEcc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIcE9do_narrowEPKcS2_cPc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE10do_scan_isEsPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE10do_tolowerEPwPKw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE10do_tolowerEw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE10do_toupperEPwPKw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE10do_toupperEw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE11do_scan_notEsPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE5do_isEPKwS2_Ps() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE5do_isEsw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE8do_widenEc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE8do_widenEPKcS2_Pw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE9do_narrowEPKwS2_cPc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt5ctypeIwE9do_narrowEwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIcE11do_groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIcE13do_neg_formatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIcE13do_pos_formatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIcE14do_curr_symbolEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIcE14do_frac_digitsEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIcE16do_decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIcE16do_negative_signEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIcE16do_positive_signEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIcE16do_thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIwE11do_groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIwE13do_neg_formatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIwE13do_pos_formatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIwE14do_curr_symbolEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIwE14do_frac_digitsEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIwE16do_decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIwE16do_negative_signEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIwE16do_positive_signEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7_MpunctIwE16do_thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIcc9_MbstatetE10do_unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIcc9_MbstatetE16do_always_noconvEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIcc9_MbstatetE2inERS0_PKcS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIcc9_MbstatetE3outERS0_PKcS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIcc9_MbstatetE5do_inERS0_PKcS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIcc9_MbstatetE6do_outERS0_PKcS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIcc9_MbstatetE6lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIcc9_MbstatetE7unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIcc9_MbstatetE9do_lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDic9_MbstatetE10do_unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDic9_MbstatetE11do_encodingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDic9_MbstatetE13do_max_lengthEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDic9_MbstatetE16do_always_noconvEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDic9_MbstatetE5do_inERS0_PKcS4_RS4_PDiS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDic9_MbstatetE6do_outERS0_PKDiS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDic9_MbstatetE9do_lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDsc9_MbstatetE10do_unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDsc9_MbstatetE11do_encodingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDsc9_MbstatetE13do_max_lengthEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDsc9_MbstatetE16do_always_noconvEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDsc9_MbstatetE5do_inERS0_PKcS4_RS4_PDsS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDsc9_MbstatetE6do_outERS0_PKDsS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIDsc9_MbstatetE9do_lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIwc9_MbstatetE10do_unshiftERS0_PcS3_RS3_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIwc9_MbstatetE11do_encodingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIwc9_MbstatetE13do_max_lengthEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIwc9_MbstatetE16do_always_noconvEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIwc9_MbstatetE5do_inERS0_PKcS4_RS4_PwS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIwc9_MbstatetE6do_outERS0_PKwS4_RS4_PcS6_RS6_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7codecvtIwc9_MbstatetE9do_lengthERS0_PKcS4_m() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIcE10do_compareEPKcS2_S2_S2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIcE12do_transformEPKcS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIcE4hashEPKcS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIcE7compareEPKcS2_S2_S2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIcE7do_hashEPKcS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIcE9transformEPKcS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIwE10do_compareEPKwS2_S2_S2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIwE12do_transformEPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIwE4hashEPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIwE7compareEPKwS2_S2_S2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIwE7do_hashEPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt7collateIwE9transformEPKwS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8bad_cast4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8bad_cast8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8ios_base7failure8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIcE3getEiiiRKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIcE4openERKSsRKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIcE5closeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIcE6do_getEiiiRKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIcE7do_openERKSsRKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIcE8do_closeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIwE3getEiiiRKSbIwSt11char_traitsIwESaIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIwE4openERKSsRKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIwE5closeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIwE6do_getEiiiRKSbIwSt11char_traitsIwESaIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIwE7do_openERKSsRKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8messagesIwE8do_closeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE11do_groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE11do_truenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE12do_falsenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE13decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE13thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE16do_decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE16do_thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE8groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE8truenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIcE9falsenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE11do_groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE11do_truenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE12do_falsenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE13decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE13thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE16do_decimal_pointEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE16do_thousands_sepEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE8groupingEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE8truenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt8numpunctIwE9falsenameEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt9bad_alloc4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt9bad_alloc8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt9exception4whatEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt9exception6_RaiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNKSt9exception8_DoraiseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSbIwSt11char_traitsIwESaIwEE5_CopyEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSbIwSt11char_traitsIwESaIwEE5eraseEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSbIwSt11char_traitsIwESaIwEE6appendEmw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSbIwSt11char_traitsIwESaIwEE6appendERKS2_mm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSbIwSt11char_traitsIwESaIwEE6assignEmw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSbIwSt11char_traitsIwESaIwEE6assignEPKwm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSbIwSt11char_traitsIwESaIwEE6assignERKS2_mm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSbIwSt11char_traitsIwESaIwEE6insertEmmw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSiD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSiD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSo6sentryC2ERSo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSo6sentryD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSs5_CopyEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSs5eraseEmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSs6appendEmc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSs6appendERKSsmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSs6assignEmc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSs6assignEPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSs6assignERKSsmm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSs6insertEmmc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10bad_typeidD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10bad_typeidD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10bad_typeidD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem10_Close_dirEPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem10_Copy_fileEPKcS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem10_File_sizeEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem11_EquivalentEPKcS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem11_Remove_dirEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem12_Current_getERA260_c() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem12_Current_setEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem16_Last_write_timeEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem18_Xfilesystem_errorEPKcRKNS_4pathES4_St10error_code() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem18_Xfilesystem_errorEPKcRKNS_4pathESt10error_code() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem18_Xfilesystem_errorEPKcSt10error_code() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem20_Set_last_write_timeEPKcl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem5_StatEPKcPNS_5permsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem6_ChmodEPKcNS_5permsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem6_LstatEPKcPNS_5permsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem7_RenameEPKcS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem7_ResizeEPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem7_UnlinkEPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem8_StatvfsEPKcRNS_10space_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem9_Make_dirEPKcS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem9_Open_dirERA260_cPKcRiRNS_9file_typeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10filesystem9_Read_dirERA260_cPvRNS_9file_typeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EE4intlE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb0EED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EE4intlE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIcLb1EED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EE4intlE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb0EED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EE4intlE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt10moneypunctIwLb1EED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt11logic_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt11logic_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt11logic_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt11range_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt11range_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt11range_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt11regex_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt11regex_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt11regex_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12bad_weak_ptrD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12bad_weak_ptrD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12bad_weak_ptrD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12domain_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12domain_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12domain_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12future_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12future_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12future_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12length_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12length_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12length_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12out_of_rangeD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12out_of_rangeD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12out_of_rangeD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders2_1E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders2_2E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders2_3E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders2_4E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders2_5E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders2_6E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders2_7E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders2_8E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders2_9E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_11E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_12E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_13E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_14E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_15E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_16E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_17E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_18E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_19E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12placeholders3_20E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12system_errorC2ESt10error_codePKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12system_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12system_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt12system_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13_Num_int_base10is_boundedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13_Num_int_base10is_integerE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13_Num_int_base14is_specializedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13_Num_int_base5radixE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13_Num_int_base8is_exactE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13_Num_int_base9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13_Regex_traitsIcE6_NamesE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13_Regex_traitsIwE6_NamesE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13bad_exceptionD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13bad_exceptionD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13bad_exceptionD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE4syncEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE5_LockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE5imbueERKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE5uflowEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE6setbufEPci() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE7_UnlockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
internal__ZNSt13basic_filebufIcSt11char_traitsIcEE7seekoffElNSt5_IosbIiE8_SeekdirENS4_9_OpenmodeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
internal__ZNSt13basic_filebufIcSt11char_traitsIcEE7seekposESt4fposI9_MbstatetENSt5_IosbIiE9_OpenmodeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE8overflowEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE9_EndwriteEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE9pbackfailEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEE9underflowEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIcSt11char_traitsIcEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE4syncEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE5_LockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE5imbueERKSt6locale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE5uflowEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE6setbufEPwi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE7_UnlockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
internal__ZNSt13basic_filebufIwSt11char_traitsIwEE7seekoffElNSt5_IosbIiE8_SeekdirENS4_9_OpenmodeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
internal__ZNSt13basic_filebufIwSt11char_traitsIwEE7seekposESt4fposI9_MbstatetENSt5_IosbIiE9_OpenmodeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE8overflowEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE9_EndwriteEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE9pbackfailEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEE9underflowEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13basic_filebufIwSt11char_traitsIwEED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13runtime_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13runtime_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt13runtime_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Error_objectsIiE14_System_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Error_objectsIiE15_Generic_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base10has_denormE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base10is_boundedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base10is_integerE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base11round_styleE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base12has_infinityE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base13has_quiet_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base14is_specializedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base15has_denorm_lossE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base15tinyness_beforeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base17has_signaling_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base5radixE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base5trapsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base8is_exactE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base9is_iec559E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14_Num_ldbl_base9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14error_categoryD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIaE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIaE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIaE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIbE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIbE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIbE9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIbE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIcE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIcE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIcE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIdE12max_digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIdE12max_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIdE12min_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIdE14max_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIdE14min_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIdE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIdE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIDiE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIDiE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIDiE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIDsE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIDsE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIDsE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIeE12max_digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIeE12max_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIeE12min_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIeE14max_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIeE14min_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIeE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIeE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIfE12max_digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIfE12max_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIfE12min_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIfE14max_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIfE14min_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIfE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIfE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIhE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIhE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIhE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIiE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIiE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIiE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIjE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIjE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIjE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIlE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIlE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIlE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsImE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsImE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsImE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIsE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIsE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIsE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsItE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsItE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsItE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIwE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIwE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIwE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIxE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIxE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIxE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIyE6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIyE8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14numeric_limitsIyE9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14overflow_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14overflow_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt14overflow_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base10has_denormE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base10is_boundedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base10is_integerE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base11round_styleE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base12has_infinityE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base13has_quiet_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base14is_specializedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base15has_denorm_lossE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base15tinyness_beforeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base17has_signaling_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base5radixE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base5trapsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base8is_exactE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base9is_iec559E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15_Num_float_base9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15underflow_errorD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15underflow_errorD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt15underflow_errorD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt16invalid_argumentD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt16invalid_argumentD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt16invalid_argumentD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt16nested_exceptionD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt16nested_exceptionD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt16nested_exceptionD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt17bad_function_callD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt17bad_function_callD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt17bad_function_callD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt18bad_variant_accessD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt18bad_variant_accessD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt20_Future_error_objectIiE14_Future_objectE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt20bad_array_new_lengthD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt20bad_array_new_lengthD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt20bad_array_new_lengthD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt22_Future_error_categoryD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt22_Future_error_categoryD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt22_System_error_categoryD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt22_System_error_categoryD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt23_Generic_error_categoryD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt23_Generic_error_categoryD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt3pmr19new_delete_resourceEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt3pmr20get_default_resourceEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt3pmr20null_memory_resourceEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt3pmr20set_default_resourceEPNS_15memory_resourceE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt4_Pad7_LaunchEPKcPP12pthread_attrPP7pthread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt4_Pad7_LaunchEPKcPP7pthread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt4_Pad7_LaunchEPP12pthread_attrPP7pthread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt4_Pad7_LaunchEPP7pthread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt4_Pad8_ReleaseEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt4_PadC2EPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt4_PadC2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt4_PadD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt4_PadD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt5ctypeIcE10table_sizeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt5ctypeIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt5ctypeIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt5ctypeIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt5ctypeIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt5ctypeIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt5ctypeIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_Mutex5_LockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_Mutex7_UnlockEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_MutexC1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_MutexC2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_MutexD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_MutexD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_Winit9_Init_cntE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_WinitC1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called"); // GRR
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_WinitC2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_WinitD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6_WinitD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6chrono12steady_clock12is_monotonicE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6chrono12steady_clock9is_steadyE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6chrono12system_clock12is_monotonicE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6chrono12system_clock9is_steadyE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale16_GetgloballocaleEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale16_SetgloballocaleEPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale2id7_Id_cntE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale5_InitEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale5emptyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale5facet7_DecrefEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale5facet7_IncrefEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale5facet9_RegisterEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale6globalERKS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_Locimp7_AddfacEPNS_5facetEm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_Locimp8_ClocptrE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_Locimp8_MakelocERKSt8_LocinfoiPS0_PKS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_Locimp9_MakewlocERKSt8_LocinfoiPS0_PKS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_Locimp9_MakexlocERKSt8_LocinfoiPS0_PKS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_LocimpC1Eb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_LocimpC1ERKS0_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_LocimpC2Eb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_LocimpC2ERKS0_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_LocimpD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_LocimpD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7_LocimpD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6locale7classicEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6localeD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt6thread20hardware_concurrencyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIcE5_InitERKSt8_Locinfob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIcEC2Emb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIcEC2EPKcmbb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIwE5_InitERKSt8_Locinfob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIwEC2Emb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIwEC2EPKcmbb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7_MpunctIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetE7_GetcatEPPKNSt6locale5facetEPKS2_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIcc9_MbstatetED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIDic9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIDic9_MbstatetED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIDic9_MbstatetED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIDsc9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIDsc9_MbstatetED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIDsc9_MbstatetED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIwc9_MbstatetE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIwc9_MbstatetED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7codecvtIwc9_MbstatetED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIcED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt7collateIwED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8_Locinfo8_AddcatsEiPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8_LocinfoC1EiPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8_LocinfoC1EPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8_LocinfoC1ERKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8_LocinfoC2EiPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8_LocinfoC2EPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8_LocinfoC2ERKSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8_LocinfoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8_LocinfoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8bad_castD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8bad_castD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8bad_castD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base4Init9_Init_cntE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base4InitC1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called"); // alien isolation
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base4InitC2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called"); // GRR
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base4InitD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base4InitD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base5_SyncE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base5clearENSt5_IosbIiE8_IostateEb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base6_IndexE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base7_AddstdEPS_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base7failureD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base7failureD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_base7failureD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_baseD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_baseD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8ios_baseD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIcED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwE5_InitERKSt8_Locinfo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwEC1EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwEC1ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwEC2EPKcm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwEC2ERKSt8_Locinfom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8messagesIwED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcE5_InitERKSt8_Locinfob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcE5_TidyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcEC1EPKcmb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcEC2EPKcmb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIcED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwE2idE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwE5_InitERKSt8_Locinfob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwE5_TidyEv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwE7_GetcatEPPKNSt6locale5facetEPKS1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwEC1Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwEC1EPKcmb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwEC1ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwEC2Em() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwEC2EPKcmb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwEC2ERKSt8_Locinfomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwED0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwED1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt8numpunctIwED2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base10has_denormE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base10is_boundedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base10is_integerE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base11round_styleE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base12has_infinityE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base12max_digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base12max_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base12min_exponentE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base13has_quiet_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base14is_specializedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base14max_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base14min_exponent10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base15has_denorm_lossE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base15tinyness_beforeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base17has_signaling_NaNE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base5radixE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base5trapsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base6digitsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base8digits10E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base8is_exactE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base9is_iec559E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base9is_moduloE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9_Num_base9is_signedE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9bad_allocD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9bad_allocD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9bad_allocD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9exception18_Set_raise_handlerEPFvRKS_E() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9exceptionD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9exceptionD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9exceptionD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9type_infoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9type_infoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZNSt9type_infoD2Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZnwmRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZnwmSt11align_val_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZnwmSt11align_val_tRKSt9nothrow_t() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt10_Rng_abortPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt10adopt_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt10defer_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt10unexpectedv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt11_Xbad_allocv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt11setiosflagsNSt5_IosbIiE9_FmtflagsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt11try_to_lock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt12setprecisioni() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt13_Cl_charnames() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt13_Execute_onceRSt9once_flagPFiPvS1_PS1_ES1_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt13_Syserror_mapi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt13_Xregex_errorNSt15regex_constants10error_typeE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt13get_terminatev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt13resetiosflagsNSt5_IosbIiE9_FmtflagsE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt13set_terminatePFvvE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14_Atomic_assertPKcS0_() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14_Cl_wcharnames() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14_Debug_messagePKcS0_j() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14_Raise_handler() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14_Random_devicev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14_Throw_C_errori() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14_Xlength_errorPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14_Xout_of_rangePKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14get_unexpectedv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt14set_unexpectedPFvvE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt15_sceLibcLocinfoPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt15_Xruntime_errorPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt15future_categoryv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt15get_new_handlerv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt15set_new_handlerPFvvE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt15system_categoryv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt16_Throw_Cpp_errori() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt16_Xoverflow_errorPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt16generic_categoryv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt17_Future_error_mapi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt18_String_cpp_unused() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt18_Xinvalid_argumentPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt18uncaught_exceptionv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt19_Throw_future_errorRKSt10error_code() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt19_Xbad_function_callv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt21_sceLibcClassicLocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt22_Get_future_error_whati() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt22_Random_device_entropyv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt25_Rethrow_future_exceptionSt13exception_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt3cin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt4_Fpz() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt4cerr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt4clog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt4cout() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt4setwi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt4wcin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt5wcerr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt5wclog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt5wcout() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt6_ThrowRKSt9exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt6ignore() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt7_BADOFF() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt7_FiopenPKcNSt5_IosbIiE9_OpenmodeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt7_FiopenPKwNSt5_IosbIiE9_OpenmodeEi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt7_MP_AddPyy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt7_MP_GetPy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt7_MP_MulPyyy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt7_MP_RemPyy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt7nothrow() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt7setbasei() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt8_XLgammad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt8_XLgammae() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt8_XLgammaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt9_LStrcollIcEiPKT_S2_S2_S2_PKSt8_Collvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt9_LStrcollIwEiPKT_S2_S2_S2_PKSt8_Collvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt9_LStrxfrmIcEmPT_S1_PKS0_S3_PKSt8_Collvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt9_LStrxfrmIwEmPT_S1_PKS0_S3_PKSt8_Collvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZSt9terminatev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTId() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIDh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv116__enum_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv117__array_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv117__class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv117__pbase_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv119__pointer_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv120__function_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv120__si_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv121__vmi_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv123__fundamental_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN10__cxxabiv129__pointer_to_member_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN6Dinkum7threads10lock_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIN6Dinkum7threads21thread_resource_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTINSt6locale5facetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTINSt6locale7_LocimpE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTINSt8ios_base7failureE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPDh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKDh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPKy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIPy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt10bad_typeid() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt10ctype_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt10money_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt10moneypunctIcLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt10moneypunctIcLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt10moneypunctIwLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt10moneypunctIwLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt11_Facet_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt11logic_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt11range_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt11regex_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt12bad_weak_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt12codecvt_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt12domain_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt12future_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt12length_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt12out_of_range() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt12system_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt13bad_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt13basic_filebufIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt13basic_filebufIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt13messages_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt13runtime_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt14error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt14overflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt15underflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt16invalid_argument() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt16nested_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt17bad_function_call() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt18bad_variant_access() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt20bad_array_new_length() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt22_Future_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt22_System_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt23_Generic_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt4_Pad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt5_IosbIiE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt5ctypeIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt5ctypeIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt7_MpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt7_MpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt7codecvtIcc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt7codecvtIDic9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt7codecvtIDsc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt7codecvtIwc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt7collateIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt7collateIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt8bad_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt8ios_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt8messagesIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt8messagesIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt8numpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt8numpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt9bad_alloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt9basic_iosIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt9basic_iosIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt9exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt9time_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTISt9type_info() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTIy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv116__enum_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv117__array_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv117__class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv117__pbase_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv119__pointer_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv120__function_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv120__si_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv121__vmi_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv123__fundamental_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN10__cxxabiv129__pointer_to_member_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN6Dinkum7threads10lock_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSN6Dinkum7threads21thread_resource_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSNSt6locale5facetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSNSt6locale7_LocimpE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSNSt8ios_base7failureE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKDi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKDn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKDs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKe() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKh() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKj() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPKy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSPy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt10bad_typeid() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt10ctype_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt10money_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt10moneypunctIcLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt10moneypunctIcLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt10moneypunctIwLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt10moneypunctIwLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt11_Facet_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt11logic_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt11range_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt11regex_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt12bad_weak_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt12codecvt_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt12domain_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt12future_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt12length_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt12out_of_range() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt12system_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt13bad_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt13basic_filebufIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt13basic_filebufIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt13messages_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt13runtime_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt14error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt14overflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt15underflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt16invalid_argument() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt16nested_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt17bad_function_call() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt18bad_variant_access() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt20bad_array_new_length() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt22_Future_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt22_System_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt23_Generic_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt4_Pad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt5_IosbIiE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt5ctypeIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt5ctypeIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt7_MpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt7_MpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt7codecvtIcc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt7codecvtIDic9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt7codecvtIDsc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt7codecvtIwc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt7collateIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt7collateIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt8bad_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt8ios_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt8messagesIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt8messagesIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt8numpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt8numpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt9bad_alloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt9basic_iosIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt9basic_iosIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt9exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt9time_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSSt9type_info() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTSy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTv0_n24_NSiD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTv0_n24_NSiD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTv0_n24_NSoD0Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTv0_n24_NSoD1Ev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv116__enum_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv117__array_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv117__class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv117__pbase_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv119__pointer_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv120__function_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv120__si_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv121__vmi_class_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv123__fundamental_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN10__cxxabiv129__pointer_to_member_type_infoE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN6Dinkum7threads10lock_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVN6Dinkum7threads21thread_resource_errorE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVNSt6locale7_LocimpE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVNSt8ios_base7failureE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt10bad_typeid() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt10moneypunctIcLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt10moneypunctIcLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt10moneypunctIwLb0EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt10moneypunctIwLb1EE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt11logic_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt11range_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt11regex_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt12bad_weak_ptr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt12domain_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt12future_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt12length_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt12out_of_range() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt12system_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt13bad_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt13basic_filebufIcSt11char_traitsIcEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt13basic_filebufIwSt11char_traitsIwEE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt13runtime_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt14error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt14overflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt15underflow_error() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt16invalid_argument() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt16nested_exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt17bad_function_call() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt18bad_variant_access() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt20bad_array_new_length() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt22_Future_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt22_System_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt23_Generic_error_category() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt4_Pad() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt5ctypeIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt5ctypeIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt7_MpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt7_MpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt7codecvtIcc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt7codecvtIDic9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt7codecvtIDsc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt7codecvtIwc9_MbstatetE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt7collateIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt7collateIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt8bad_cast() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt8ios_base() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt8messagesIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt8messagesIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt8numpunctIcE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt8numpunctIwE() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt9bad_alloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt9exception() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal__ZTVSt9type_info() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
internal__ZZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
internal__ZZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_abort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_abort_handler_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_alarm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_aligned_alloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_asctime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_asctime_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_at_quick_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atexit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atoi() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atol() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_atoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_basename() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_basename_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_bcmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_bcopy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_bsearch() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_bsearch_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_btowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_bzero() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_c16rtomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_c32rtomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_calloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_cbrt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_cbrtf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_cbrtl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_clearerr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_clearerr_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_clock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_clock_1700() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_closedir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_copysign() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_copysignf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_copysignl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ctime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ctime_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_daemon() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_daylight() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_devname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_devname_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_difftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_dirname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_div() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_drand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_drem() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_dremf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_erand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_erf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_erfc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_erfcf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_erfcl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_erff() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_erfl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_err() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_err_set_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_err_set_file() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_errc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_errx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fclose() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fcloseall() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fdim() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fdimf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fdiml() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fdopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fdopendir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_feclearexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fedisableexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_feenableexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fegetenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fegetexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fegetexceptflag() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fegetround() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fegettrapenable() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_feholdexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_feof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_feof_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_feraiseexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ferror() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ferror_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fesetenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fesetexceptflag() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fesetround() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fesettrapenable() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fetestexcept() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_feupdateenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fflush() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fgetc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fgetln() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fgetpos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fgets() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fgetwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fgetws() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fileno() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fileno_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_finite() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_finitef() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_flockfile() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_flsl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fmaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fmal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fopen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fpurge() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fputc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fputs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fputwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fputws() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fread() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_freeifaddrs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_freopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_freopen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fseek() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fseeko() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fsetpos() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fstatvfs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ftell() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ftello() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ftrylockfile() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_funlockfile() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fwide() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_fwrite() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_gamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_gamma_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_gammaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_gammaf_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getc_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getchar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getchar_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getcwd() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_gethostname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getifaddrs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getopt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getopt_long() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getopt_long_only() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getprogname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_gets() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_gets_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_getwchar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_gmtime(time_t* timer) {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_gmtime_s(time_t* timer, u64 flags) {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_hypot() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_hypot3() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_hypot3f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_hypot3l() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_hypotf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_hypotl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ignore_handler_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_index() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_inet_addr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_inet_aton() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_inet_ntoa() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_inet_ntoa_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_initstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isalnum() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isalpha() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isblank() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iscntrl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isdigit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isgraph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isprint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ispunct() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isspace() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswalnum() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswalpha() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswblank() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswcntrl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswdigit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswgraph() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswlower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswprint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswpunct() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswspace() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_iswxdigit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_isxdigit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_j0() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_j0f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_j1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_j1f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_jn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_jnf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_jrand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_labs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lcong48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ldexp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ldexpf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ldexpl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ldiv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lgamma_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lgammaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lgammaf_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lgammal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_llabs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lldiv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_llrint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_llrintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_llrintl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_llround() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_llroundf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_llroundl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_localeconv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_localtime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_localtime_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_longjmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lrand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lrint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lrintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_lrintl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_makecontext() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mblen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbrlen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbrtoc16() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbrtoc32() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbrtowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbsinit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbsrtowcs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbsrtowcs_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbstowcs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbstowcs_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mbtowc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mergesort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mktime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_modf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_modff() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_modfl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_mrand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nearbyint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nearbyintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nearbyintl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_Need_sceLibcInternal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nextafter() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nextafterf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nextafterl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nexttoward() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nexttowardf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nexttowardl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_nrand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_opendir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_optarg() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_opterr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_optind() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_optopt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_optreset() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_perror() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawn_file_actions_addclose() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawn_file_actions_adddup2() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawn_file_actions_addopen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawn_file_actions_destroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawn_file_actions_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_destroy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_getflags() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_getpgroup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_getschedparam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_getschedpolicy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_getsigdefault() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_getsigmask() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_init() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_setflags() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_setpgroup() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_setschedparam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_setschedpolicy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_setsigdefault() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnattr_setsigmask() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_posix_spawnp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_psignal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_putc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_putc_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_putchar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_putchar_unlocked() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_putenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_puts() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_putw() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_putwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_putwchar() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_qsort() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_qsort_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_quick_exit() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_rand() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_rand_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_random() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_readdir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_readdir_r() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_realpath() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_remainderf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_remainderl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_remove() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_remquo() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_remquof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_remquol() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_rewind() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_rewinddir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_rindex() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_rint() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_rintf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_rintl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_round() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_roundf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_roundl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scalb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scalbf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scalbln() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scalblnf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scalblnl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scalbn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scalbnf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_scalbnl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcDebugOut() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcHeapGetAddressRanges() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcHeapMutexCalloc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcHeapMutexFree() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcHeapSetAddressRangeCallback() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcHeapSetTraceMarker() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcHeapUnsetTraceMarker() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcInternalMemoryGetWakeAddr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcInternalMemoryMutexEnable() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcInternalSetMallocCallback() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sceLibcOnce() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_seed48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_seekdir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_set_constraint_handler_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_setbuf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_setenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_setjmp(std::jmp_buf buf) {
    LOG_ERROR(Lib_LibcInternal, "(TEST) called");
    return _setjmp(buf); // todo this feels platform specific but maybe not
}

s32 PS4_SYSV_ABI internal_setlocale() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_setstate() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_setvbuf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sigblock() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_siginterrupt() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_signalcontext() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_signgam() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_significand() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_significandf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sigsetmask() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sigvec() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_srand48() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_srandom() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_srandomdev() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_statvfs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_stderr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_stdin() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_stdout() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_stpcpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sys_nsig() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sys_siglist() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_sys_signame() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_syslog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_telldir() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tgamma() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tgammaf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tgammal() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_time() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_timezone() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tolower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_toupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_towctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_towlower() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_towupper() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_trunc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_truncf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_truncl() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tzname() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_tzset() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ungetc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ungetwc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_unsetenv() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_utime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_verr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_verrc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_verrx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vsyslog() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vwarn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vwarnc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_vwarnx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_warn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_warnc() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_warnx() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcrtomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcrtomb_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcscat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcscat_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcschr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcscmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcscoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcscpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcscpy_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcscspn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsftime() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcslen() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsncat() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsncat_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsncmp() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsncpy() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsncpy_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsnlen_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcspbrk() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsrchr() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsrtombs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsrtombs_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsspn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstod() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstof() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstoimax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstok() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstok_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstol() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstold() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstoll() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstombs() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstombs_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstoul() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstoull() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcstoumax() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wcsxfrm() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wctob() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wctomb() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wctomb_s() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wctrans() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_wctype() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_xtime_get() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_y0() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_y0f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_y1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_y1f() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_yn() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_ynf() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_Func_186EB8E3525D6240() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_Func_419F5881393ECAB1() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_Func_6C6B8377791654A4() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_Func_7FD2D5C8DF0ACBC8() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI internal_Func_C14A89D29B148C3A() {
    LOG_ERROR(Lib_LibcInternal, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceLibcInternal(Core::Loader::SymbolsResolver* sym) {
    RegisterlibSceLibcInternalMspace(sym);
    RegisterlibSceLibcInternalIo(sym);
    RegisterlibSceLibcInternalMemory(sym);
    RegisterlibSceLibcInternalStr(sym);
    RegisterlibSceLibcInternalStream(sym);

    LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcHeapGetTraceInfo);
    LIB_FUNCTION("ys1W6EwuVw4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___absvdi2);
    LIB_FUNCTION("2HED9ow7Zjc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___absvsi2);
    LIB_FUNCTION("v9XNTmsmz+M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___absvti2);
    LIB_FUNCTION("3CAYAjL-BLs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___adddf3);
    LIB_FUNCTION("mhIInD5nz8I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___addsf3);
    LIB_FUNCTION("8gG-+co6LfM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___addvdi3);
    LIB_FUNCTION("gsnW-FWQqZo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___addvsi3);
    LIB_FUNCTION("IjlonFkCFDs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___addvti3);
    LIB_FUNCTION("CS91br93fag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ashldi3);
    LIB_FUNCTION("ECUHmdEfhic", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ashlti3);
    LIB_FUNCTION("fSZ+gbf8Ekc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ashrdi3);
    LIB_FUNCTION("7+0ouwmGDww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ashrti3);
    LIB_FUNCTION("ClfCoK1Zeb4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_compare_exchange);
    LIB_FUNCTION("ZwapHUAcijE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_compare_exchange_1);
    LIB_FUNCTION("MwiKdf6QFvI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_compare_exchange_2);
    LIB_FUNCTION("lku-VgKK0RE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_compare_exchange_4);
    LIB_FUNCTION("tnlAgPCKyTk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_compare_exchange_8);
    LIB_FUNCTION("hsn2TaF3poY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_compare_exchange_n);
    LIB_FUNCTION("5i8mTQeo9hs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_exchange);
    LIB_FUNCTION("z8lecpCHpqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_exchange_1);
    LIB_FUNCTION("HDvFM0iZYXo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_exchange_2);
    LIB_FUNCTION("yit-Idli5gU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_exchange_4);
    LIB_FUNCTION("UOz27kgch8k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_exchange_8);
    LIB_FUNCTION("oCH4efUlxZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_exchange_n);
    LIB_FUNCTION("Qb86Y5QldaE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_add_1);
    LIB_FUNCTION("wEImmi0YYQM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_add_2);
    LIB_FUNCTION("U8pDVMfBDUY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_add_4);
    LIB_FUNCTION("SqcnaljoFBw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_add_8);
    LIB_FUNCTION("Q3-0HGD3Y48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_and_1);
    LIB_FUNCTION("A71XWS1kKqA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_and_2);
    LIB_FUNCTION("E-XEmpL9i1A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_and_4);
    LIB_FUNCTION("xMksIr3nXug", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_and_8);
    LIB_FUNCTION("LvLuiirFk8U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_or_1);
    LIB_FUNCTION("aSNAf0kxC+Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_or_2);
    LIB_FUNCTION("AFRS4-8aOSo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_or_4);
    LIB_FUNCTION("5ZKavcBG7eM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_or_8);
    LIB_FUNCTION("HWBJOsgJBT8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_sub_1);
    LIB_FUNCTION("yvhjR7PTRgc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_sub_2);
    LIB_FUNCTION("-mUC21i8WBQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_sub_4);
    LIB_FUNCTION("K+k1HlhjyuA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_sub_8);
    LIB_FUNCTION("aWc+LyHD1vk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_xor_1);
    LIB_FUNCTION("PZoM-Yn6g2Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_xor_2);
    LIB_FUNCTION("pPdYDr1KDsI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_xor_4);
    LIB_FUNCTION("Dw3ieb2rMmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_fetch_xor_8);
    LIB_FUNCTION("JZWEhLSIMoQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_is_lock_free);
    LIB_FUNCTION("+iy+BecyFVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_load);
    LIB_FUNCTION("cWgvLiSJSOQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_load_1);
    LIB_FUNCTION("ufqiLmjiBeM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_load_2);
    LIB_FUNCTION("F+m2tOMgeTo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_load_4);
    LIB_FUNCTION("8KwflkOtvZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_load_8);
    LIB_FUNCTION("Q6oqEnefZQ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_load_n);
    LIB_FUNCTION("sV6ry-Fd-TM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_store);
    LIB_FUNCTION("ZF6hpsTZ2m8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_store_1);
    LIB_FUNCTION("-JjkEief9No", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_store_2);
    LIB_FUNCTION("4tDF0D+qdWk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_store_4);
    LIB_FUNCTION("DEQmHCl-EGU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_store_8);
    LIB_FUNCTION("GdwuPYbVpP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___atomic_store_n);
    LIB_FUNCTION("XGNIEdRyYPo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cleanup);
    LIB_FUNCTION("gCf7+aGEhnU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___clzdi2);
    LIB_FUNCTION("ptL8XWgpGS4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___clzsi2);
    LIB_FUNCTION("jPywoVsPVR8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___clzti2);
    LIB_FUNCTION("OvbYtSGnzFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cmpdi2);
    LIB_FUNCTION("u2kPEkUHfsg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cmpti2);
    LIB_FUNCTION("yDPuV0SXp7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ctzdi2);
    LIB_FUNCTION("2NvhgiBTcVE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ctzsi2);
    LIB_FUNCTION("olBDzD1rX2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ctzti2);
    LIB_FUNCTION("IJKVjsmxxWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_allocate_dependent_exception);
    LIB_FUNCTION("cfAXurvfl5o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_allocate_exception);
    LIB_FUNCTION("tsvEmnenz48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_atexit);
    LIB_FUNCTION("pBxafllkvt0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_bad_cast);
    LIB_FUNCTION("xcc6DTcL8QA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_bad_typeid);
    LIB_FUNCTION("3cUUypQzMiI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_begin_catch);
    LIB_FUNCTION("usKbuvy2hQg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_call_unexpected);
    LIB_FUNCTION("BxPeH9TTcs4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_current_exception_type);
    LIB_FUNCTION("RY8mQlhg7mI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_current_primary_exception);
    LIB_FUNCTION("MQFPAqQPt1s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_decrement_exception_refcount);
    LIB_FUNCTION("zMCYAqNRllc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_demangle);
    LIB_FUNCTION("lX+4FNUklF0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_end_catch);
    LIB_FUNCTION("H2e8t5ScQGc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_finalize);
    LIB_FUNCTION("kBxt5LwtLA4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_free_dependent_exception);
    LIB_FUNCTION("nOIEswYD4Ig", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_free_exception);
    LIB_FUNCTION("Y6Sl4Xw7gfA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_get_exception_ptr);
    LIB_FUNCTION("3rJJb81CDM4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_get_globals);
    LIB_FUNCTION("uCRed7SvX5E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_get_globals_fast);
    LIB_FUNCTION("2emaaluWzUw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_guard_abort);
    LIB_FUNCTION("3GPpjQdAMTw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_guard_acquire);
    LIB_FUNCTION("9rAeANT2tyE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_guard_release);
    LIB_FUNCTION("PsrRUg671K0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_increment_exception_refcount);
    LIB_FUNCTION("zr094EQ39Ww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_pure_virtual);
    LIB_FUNCTION("ZL9FV4mJXxo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_rethrow);
    LIB_FUNCTION("qKQiNX91IGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_rethrow_primary_exception);
    LIB_FUNCTION("vkuuLfhnSZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___cxa_throw);
    LIB_FUNCTION("eTP9Mz4KkY4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divdc3);
    LIB_FUNCTION("mdGgLADsq8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divdf3);
    LIB_FUNCTION("9daYeu+0Y-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divdi3);
    LIB_FUNCTION("1rs4-h7Fq9U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divmoddi4);
    LIB_FUNCTION("rtBENmz8Iwc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divmodsi4);
    LIB_FUNCTION("dcaiFCKtoDg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divsc3);
    LIB_FUNCTION("nufufTB4jcI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divsf3);
    LIB_FUNCTION("zdJ3GXAcI9M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divsi3);
    LIB_FUNCTION("XU4yLKvcDh0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divti3);
    LIB_FUNCTION("SNdBm+sNfM4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___divxc3);
    LIB_FUNCTION("hMAe+TWS9mQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___dynamic_cast);
    LIB_FUNCTION("8F52nf7VDS8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___eqdf2);
    LIB_FUNCTION("LmXIpdHppBM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___eqsf2);
    LIB_FUNCTION("6zU++1tayjA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___extendsfdf2);
    LIB_FUNCTION("CVoT4wFYleE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fe_dfl_env);
    LIB_FUNCTION("1IB0U3rUtBw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fedisableexcept);
    LIB_FUNCTION("NDOLSTFT1ns", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___feenableexcept);
    LIB_FUNCTION("E1iwBYkG3CM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fflush);
    LIB_FUNCTION("r3tNGoVJ2YA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ffsdi2);
    LIB_FUNCTION("b54DvYZEHj4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ffsti2);
    LIB_FUNCTION("q9SHp+5SOOQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixdfdi);
    LIB_FUNCTION("saNCRNfjeeg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixdfsi);
    LIB_FUNCTION("cY4yCWdcTXE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixdfti);
    LIB_FUNCTION("0eoyU-FoNyk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixsfdi);
    LIB_FUNCTION("3qQmz11yFaA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixsfsi);
    LIB_FUNCTION("IHq2IaY4UGg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixsfti);
    LIB_FUNCTION("h8nbSvw0s+M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixunsdfdi);
    LIB_FUNCTION("6WwFtNvnDag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixunsdfsi);
    LIB_FUNCTION("rLuypv9iADw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixunsdfti);
    LIB_FUNCTION("Qa6HUR3h1k4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixunssfdi);
    LIB_FUNCTION("NcZqFTG-RBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixunssfsi);
    LIB_FUNCTION("mCESRUqZ+mw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixunssfti);
    LIB_FUNCTION("DG8dDx9ZV70", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixunsxfdi);
    LIB_FUNCTION("dtMu9zCDn3s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixunsxfsi);
    LIB_FUNCTION("l0qC0BR1F44", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixunsxfti);
    LIB_FUNCTION("31g+YJf1fHk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixxfdi);
    LIB_FUNCTION("usQDRS-1HZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fixxfti);
    LIB_FUNCTION("BMVIEbwpP+8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatdidf);
    LIB_FUNCTION("2SSK3UFPqgQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatdisf);
    LIB_FUNCTION("MVPtIf3MtL8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatdixf);
    LIB_FUNCTION("X7A21ChFXPQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatsidf);
    LIB_FUNCTION("rdht7pwpNfM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatsisf);
    LIB_FUNCTION("EtpM9Qdy8D4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floattidf);
    LIB_FUNCTION("VlDpPYOXL58", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floattisf);
    LIB_FUNCTION("dJvVWc2jOP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floattixf);
    LIB_FUNCTION("1RNxpXpVWs4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatundidf);
    LIB_FUNCTION("9tnIVFbvOrw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatundisf);
    LIB_FUNCTION("3A9RVSwG8B0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatundixf);
    LIB_FUNCTION("OdvMJCV7Oxo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatunsidf);
    LIB_FUNCTION("RC3VBr2l94o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatunsisf);
    LIB_FUNCTION("ibs6jIR0Bw0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatuntidf);
    LIB_FUNCTION("KLfd8g4xp+c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatuntisf);
    LIB_FUNCTION("OdzLUcBLhb4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___floatuntixf);
    LIB_FUNCTION("qlWiRfOJx1A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fpclassifyd);
    LIB_FUNCTION("z7aCCd9hMsI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fpclassifyf);
    LIB_FUNCTION("zwV79ZJ9qAU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___fpclassifyl);
    LIB_FUNCTION("hXA24GbAPBk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___gedf2);
    LIB_FUNCTION("mdLGxBXl6nk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___gesf2);
    LIB_FUNCTION("1PvImz6yb4M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___gtdf2);
    LIB_FUNCTION("ICY0Px6zjjo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___gtsf2);
    LIB_FUNCTION("XwLA5cTHjt4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___gxx_personality_v0);
    LIB_FUNCTION("7p7kTAJcuGg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___inet_addr);
    LIB_FUNCTION("a7ToDPsIQrc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___inet_aton);
    LIB_FUNCTION("6i5aLrxRhG0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___inet_ntoa);
    LIB_FUNCTION("H2QD+kNpa+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___inet_ntoa_r);
    LIB_FUNCTION("dhK16CKwhQg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isfinite);
    LIB_FUNCTION("Q8pvJimUWis", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isfinitef);
    LIB_FUNCTION("3-zCDXatSU4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isfinitel);
    LIB_FUNCTION("V02oFv+-JzA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isinf);
    LIB_FUNCTION("rDMyAf1Jhug", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isinff);
    LIB_FUNCTION("gLGmR9aan4c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isinfl);
    LIB_FUNCTION("GfxAp9Xyiqs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isnan);
    LIB_FUNCTION("lA94ZgT+vMM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isnanf);
    LIB_FUNCTION("YBRHNH4+dDo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isnanl);
    LIB_FUNCTION("fGPRa6T+Cu8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isnormal);
    LIB_FUNCTION("WkYnBHFsmW4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isnormalf);
    LIB_FUNCTION("S3nFV6TR1Dw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isnormall);
    LIB_FUNCTION("q1OvUam0BJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___isthreaded);
    LIB_FUNCTION("-m7FIvSBbMM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___kernel_cos);
    LIB_FUNCTION("7ruwcMCJVGI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___kernel_cosdf);
    LIB_FUNCTION("GLNDoAYNlLQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___kernel_rem_pio2);
    LIB_FUNCTION("zpy7LnTL5p0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___kernel_sin);
    LIB_FUNCTION("2Lvc7KWtErs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___kernel_sindf);
    LIB_FUNCTION("F78ECICRxho", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ledf2);
    LIB_FUNCTION("hbiV9vHqTgo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___lesf2);
    LIB_FUNCTION("9mKjVppFsL0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___longjmp);
    LIB_FUNCTION("18E1gOH7cmk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___lshrdi3);
    LIB_FUNCTION("1iRAqEqEL0Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___lshrti3);
    LIB_FUNCTION("tcBJa2sYx0w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ltdf2);
    LIB_FUNCTION("259y57ZdZ3I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ltsf2);
    LIB_FUNCTION("77pL1FoD4I4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mb_cur_max);
    LIB_FUNCTION("fGYLBr2COwA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mb_sb_limit);
    LIB_FUNCTION("gQFVRFgFi48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___moddi3);
    LIB_FUNCTION("k0vARyJi9oU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___modsi3);
    LIB_FUNCTION("J8JRHcUKWP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___modti3);
    LIB_FUNCTION("D4Hf-0ik5xU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___muldc3);
    LIB_FUNCTION("O+Bv-zodKLw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___muldf3);
    LIB_FUNCTION("Hf8hPlDoVsw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___muldi3);
    LIB_FUNCTION("wVbBBrqhwdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mulodi4);
    LIB_FUNCTION("DDxNvs1a9jM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mulosi4);
    LIB_FUNCTION("+X-5yNFPbDw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___muloti4);
    LIB_FUNCTION("y+E+IUZYVmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mulsc3);
    LIB_FUNCTION("BXmn6hA5o0M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mulsf3);
    LIB_FUNCTION("zhAIFVIN1Ds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___multi3);
    LIB_FUNCTION("Uyfpss5cZDE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mulvdi3);
    LIB_FUNCTION("tFgzEdfmEjI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mulvsi3);
    LIB_FUNCTION("6gc1Q7uu244", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mulvti3);
    LIB_FUNCTION("gZWsDrmeBsg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___mulxc3);
    LIB_FUNCTION("ocyIiJnJW24", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___nedf2);
    LIB_FUNCTION("tWI4Ej9k9BY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___negdf2);
    LIB_FUNCTION("Rj4qy44yYUw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___negdi2);
    LIB_FUNCTION("4f+Q5Ka3Ex0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___negsf2);
    LIB_FUNCTION("Zofiv1PMmR4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___negti2);
    LIB_FUNCTION("fh54IRxGBUQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___negvdi2);
    LIB_FUNCTION("7xnsvjuqtZQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___negvsi2);
    LIB_FUNCTION("QW-f9vYgI7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___negvti2);
    LIB_FUNCTION("OWZ3ZLkgye8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___nesf2);
    LIB_FUNCTION("KOy7MeQ7OAU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___opendir2);
    LIB_FUNCTION("RDeUB6JGi1U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___paritydi2);
    LIB_FUNCTION("9xUnIQ53Ao4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___paritysi2);
    LIB_FUNCTION("vBP4ytNRXm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___parityti2);
    LIB_FUNCTION("m4S+lkRvTVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___popcountdi2);
    LIB_FUNCTION("IBn9qjWnXIw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___popcountsi2);
    LIB_FUNCTION("l1wz5R6cIxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___popcountti2);
    LIB_FUNCTION("H+8UBOwfScI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___powidf2);
    LIB_FUNCTION("EiMkgQsOfU0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___powisf2);
    LIB_FUNCTION("DSI7bz2Jt-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___powixf2);
    LIB_FUNCTION("Rw4J-22tu1U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___signbit);
    LIB_FUNCTION("CjQROLB88a4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___signbitf);
    LIB_FUNCTION("Cj81LPErPCc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___signbitl);
    LIB_FUNCTION("fIskTFX9p68", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___srefill);
    LIB_FUNCTION("yDnwZsMnX0Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___srget);
    LIB_FUNCTION("as8Od-tH1BI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___stderrp);
    LIB_FUNCTION("bgAcsbcEznc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___stdinp);
    LIB_FUNCTION("zqJhBxAKfsc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___stdoutp);
    LIB_FUNCTION("HLDcfGUMNWY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___subdf3);
    LIB_FUNCTION("FeyelHfQPzo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___subsf3);
    LIB_FUNCTION("+kvyBGa+5VI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___subvdi3);
    LIB_FUNCTION("y8j-jP6bHW4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___subvsi3);
    LIB_FUNCTION("cbyLM5qrvHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___subvti3);
    LIB_FUNCTION("TP6INgQ6N4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___swbuf);
    LIB_FUNCTION("+WLgzxv5xYA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___sync_fetch_and_add_16);
    LIB_FUNCTION("XmAquprnaGM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___sync_fetch_and_and_16);
    LIB_FUNCTION("GE4I2XAd4G4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___sync_fetch_and_or_16);
    LIB_FUNCTION("Z2I0BWPANGY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___sync_fetch_and_sub_16);
    LIB_FUNCTION("d5Q-h2wF+-E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___sync_fetch_and_xor_16);
    LIB_FUNCTION("ufZdCzu8nME", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___sync_lock_test_and_set_16);
    LIB_FUNCTION("2M9VZGYPHLI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___truncdfsf2);
    LIB_FUNCTION("SZk+FxWXdAs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ucmpdi2);
    LIB_FUNCTION("dLmvQfG8am4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___ucmpti2);
    LIB_FUNCTION("tX8ED4uIAsQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___udivdi3);
    LIB_FUNCTION("EWWEBA+Ldw8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___udivmoddi4);
    LIB_FUNCTION("PPdIvXwUQwA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___udivmodsi4);
    LIB_FUNCTION("lcNk3Ar5rUQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___udivmodti4);
    LIB_FUNCTION("PxP1PFdu9OQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___udivsi3);
    LIB_FUNCTION("802pFCwC9w0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___udivti3);
    LIB_FUNCTION("+wj27DzRPpo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___umoddi3);
    LIB_FUNCTION("p4vYrlsVpDE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___umodsi3);
    LIB_FUNCTION("ELSr5qm4K1M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___umodti3);
    LIB_FUNCTION("EDvkw0WaiOw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___unorddf2);
    LIB_FUNCTION("z0OhwgG3Bik", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal___unordsf2);
    LIB_FUNCTION("-QgqOT5u2Vk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Assert);
    LIB_FUNCTION("FHErahnajkw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atan);
    LIB_FUNCTION("kBpWlgVZLm4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_strong);
    LIB_FUNCTION("SwJ-E2FImAo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_strong_1);
    LIB_FUNCTION("qXkZo1LGnfk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_strong_2);
    LIB_FUNCTION("s+LfDF7LKxM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_strong_4);
    LIB_FUNCTION("SZrEVfvcHuA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_strong_8);
    LIB_FUNCTION("FOe7cAuBjh8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_weak);
    LIB_FUNCTION("rBbtKToRRq4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_weak_1);
    LIB_FUNCTION("sDOFamOKWBI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_weak_2);
    LIB_FUNCTION("0AgCOypbQ90", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_weak_4);
    LIB_FUNCTION("bNFLV9DJxdc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_compare_exchange_weak_8);
    LIB_FUNCTION("frx6Ge5+Uco", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_copy);
    LIB_FUNCTION("qvTpLUKwq7Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_exchange);
    LIB_FUNCTION("KHJflcH9s84", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_exchange_1);
    LIB_FUNCTION("TbuLWpWuJmc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_exchange_2);
    LIB_FUNCTION("-EgDt569OVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_exchange_4);
    LIB_FUNCTION("+xoGf-x7nJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_exchange_8);
    LIB_FUNCTION("cO0ldEk3Uko", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_add_1);
    LIB_FUNCTION("9kSWQ8RGtVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_add_2);
    LIB_FUNCTION("iPBqs+YUUFw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_add_4);
    LIB_FUNCTION("QVsk3fWNbp0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_add_8);
    LIB_FUNCTION("UVDWssRNEPM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_and_1);
    LIB_FUNCTION("PnfhEsZ-5uk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_and_2);
    LIB_FUNCTION("Pn2dnvUmbRA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_and_4);
    LIB_FUNCTION("O6LEoHo2qSQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_and_8);
    LIB_FUNCTION("K49mqeyzLSk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_or_1);
    LIB_FUNCTION("SVIiJg5eppY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_or_2);
    LIB_FUNCTION("R5X1i1zcapI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_or_4);
    LIB_FUNCTION("++In3PHBZfw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_or_8);
    LIB_FUNCTION("-Zfr0ZQheg4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_sub_1);
    LIB_FUNCTION("ovtwh8IO3HE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_sub_2);
    LIB_FUNCTION("2HnmKiLmV6s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_sub_4);
    LIB_FUNCTION("T8lH8xXEwIw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_sub_8);
    LIB_FUNCTION("Z9gbzf7fkMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_xor_1);
    LIB_FUNCTION("rpl4rhpUhfg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_xor_2);
    LIB_FUNCTION("-GVEj2QODEI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_xor_4);
    LIB_FUNCTION("XKenFBsoh1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_fetch_xor_8);
    LIB_FUNCTION("4CVc6G8JrvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_flag_clear);
    LIB_FUNCTION("Ou6QdDy1f7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_flag_test_and_set);
    LIB_FUNCTION("RBPhCcRhyGI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_is_lock_free_1);
    LIB_FUNCTION("QhORYaNkS+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_is_lock_free_2);
    LIB_FUNCTION("cRYyxdZo1YQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_is_lock_free_4);
    LIB_FUNCTION("-3ZujD7JX9c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_is_lock_free_8);
    LIB_FUNCTION("XAqAE803zMg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_load_1);
    LIB_FUNCTION("aYVETR3B8wk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_load_2);
    LIB_FUNCTION("cjZEuzHkgng", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_load_4);
    LIB_FUNCTION("ea-rVHyM3es", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_load_8);
    LIB_FUNCTION("HfKQ6ZD53sM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_signal_fence);
    LIB_FUNCTION("VRX+Ul1oSgE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_store_1);
    LIB_FUNCTION("6WR6sFxcd40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_store_2);
    LIB_FUNCTION("HMRMLOwOFIQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_store_4);
    LIB_FUNCTION("2uKxXHAKynI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_store_8);
    LIB_FUNCTION("-7vr7t-uto8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atomic_thread_fence);
    LIB_FUNCTION("M6nCy6H8Hs4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atqexit);
    LIB_FUNCTION("IHiK3lL7CvI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Atthreadexit);
    LIB_FUNCTION("aMucxariNg8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Btowc);
    LIB_FUNCTION("fttiF7rDdak", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Call_once);
    LIB_FUNCTION("G1kDk+5L6dU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Call_onceEx);
    LIB_FUNCTION("myTyhGbuDBw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Clocale);
    LIB_FUNCTION("mgNGxmJltOY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Closreg);
    LIB_FUNCTION("VsP3daJgmVA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_broadcast);
    LIB_FUNCTION("7yMFgcS8EPA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_destroy);
    LIB_FUNCTION("vyLotuB6AS4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_do_broadcast_at_thread_exit);
    LIB_FUNCTION("SreZybSRWpU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_init);
    LIB_FUNCTION("2B+V3qCqz4s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_init_with_name);
    LIB_FUNCTION("DV2AdZFFEh8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_register_at_thread_exit);
    LIB_FUNCTION("0uuqgRz9qfo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_signal);
    LIB_FUNCTION("McaImWKXong", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_timedwait);
    LIB_FUNCTION("wpuIiVoCWcM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_unregister_at_thread_exit);
    LIB_FUNCTION("vEaqE-7IZYc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cnd_wait);
    LIB_FUNCTION("KeOZ19X8-Ug", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Cosh);
    LIB_FUNCTION("gguxDbgbG74", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Costate);
    LIB_FUNCTION("YUKO57czb+0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__CTinfo);
    LIB_FUNCTION("eul2MC3gaYs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Ctype);
    LIB_FUNCTION("chlN6g6UbGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__CurrentRuneLocale);
    LIB_FUNCTION("6aEXAPYpaEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__CWcsxfrm);
    LIB_FUNCTION("ZlsoRa7pcuI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Daysto);
    LIB_FUNCTION("e+hi-tOrDZU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Dbl);
    LIB_FUNCTION("+5OuLYpRB28", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Dclass);
    LIB_FUNCTION("lWGF8NHv880", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__DefaultRuneLocale);
    LIB_FUNCTION("H0FQnSWp1es", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Deletegloballocale);
    LIB_FUNCTION("COSADmn1ROg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Denorm);
    LIB_FUNCTION("-vyIrREaQ0g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Dint);
    LIB_FUNCTION("VGhcd0QwhhY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Divide);
    LIB_FUNCTION("NApYynEzlco", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Dnorm);
    LIB_FUNCTION("4QwxZ3U0OK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Do_call);
    LIB_FUNCTION("FMU7jRhYCRU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Dscale);
    LIB_FUNCTION("zvl6nrvd0sE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Dsign);
    LIB_FUNCTION("vCQLavj-3CM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Dtento);
    LIB_FUNCTION("b-xTWRgI1qw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Dtest);
    LIB_FUNCTION("4Wt5uzHO98o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Dunscale);
    LIB_FUNCTION("E4SYYdwWV28", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Eps);
    LIB_FUNCTION("HmdaOhdCr88", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Erf_one);
    LIB_FUNCTION("DJXyKhVrAD8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Erf_small);
    LIB_FUNCTION("aQURHgjHo-w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Erfc);
    LIB_FUNCTION("UhKI6z9WWuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__err);
    LIB_FUNCTION("u4FNPlIIAtw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Errno);
    LIB_FUNCTION("wZi5ly2guNw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Exit);
    LIB_FUNCTION("yL91YD-WTBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Exp);
    LIB_FUNCTION("chzmnjxxVtk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fac_tidy);
    LIB_FUNCTION("D+fkILS7EK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fail_s);
    LIB_FUNCTION("us3bDnDzd70", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FAtan);
    LIB_FUNCTION("PdnFCFqKGqA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FCosh);
    LIB_FUNCTION("LSp+r7-JWwc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDclass);
    LIB_FUNCTION("JG1MkIFKnT8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDenorm);
    LIB_FUNCTION("HcpmBnY1RGE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDint);
    LIB_FUNCTION("fuzzBVdpRG0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDivide);
    LIB_FUNCTION("0NwCmZv7XcU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDnorm);
    LIB_FUNCTION("SSvY6pTRAgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDscale);
    LIB_FUNCTION("6ei1eQn2WIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDsign);
    LIB_FUNCTION("SoNnx4Ejxw8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDtento);
    LIB_FUNCTION("mnufPlYbnN0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDtest);
    LIB_FUNCTION("41SqJvOe8lA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FDunscale);
    LIB_FUNCTION("OviE3yVSuTU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FEps);
    LIB_FUNCTION("z1EfxK6E0ts", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Feraise);
    LIB_FUNCTION("dST+OsSWbno", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FErf_one);
    LIB_FUNCTION("qEvDssa4tOE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FErf_small);
    LIB_FUNCTION("qwR1gtp-WS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FErfc);
    LIB_FUNCTION("JbQw6W62UwI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_add_8);
    LIB_FUNCTION("pxFnS1okTFE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_and_8);
    LIB_FUNCTION("zQQIrnpCoFA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_and_seq_cst_1);
    LIB_FUNCTION("xROUuk7ItMM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_and_seq_cst_2);
    LIB_FUNCTION("jQuruQuMlyo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_and_seq_cst_4);
    LIB_FUNCTION("ixWEOmOBavk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_or_8);
    LIB_FUNCTION("2+6K-2tWaok", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_or_seq_cst_1);
    LIB_FUNCTION("-egu08GJ0lw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_or_seq_cst_2);
    LIB_FUNCTION("gI9un1H-fZk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_or_seq_cst_4);
    LIB_FUNCTION("YhaOeniKcoA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_xor_8);
    LIB_FUNCTION("E2YhT7m79kM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_xor_seq_cst_1);
    LIB_FUNCTION("fgXJvOSrqfg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_xor_seq_cst_2);
    LIB_FUNCTION("cqcY17uV3dI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fetch_xor_seq_cst_4);
    LIB_FUNCTION("-3pU5y1utmI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FExp);
    LIB_FUNCTION("EBkab3s8Jto", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FFpcomp);
    LIB_FUNCTION("cNGg-Y7JQQw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FGamma_big);
    LIB_FUNCTION("dYJJbxnyb74", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fgpos);
    LIB_FUNCTION("DS03EjPDtFo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FHypot);
    LIB_FUNCTION("qG50MWOiS-Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Files);
    LIB_FUNCTION("QWCTbYI14dA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FInf);
    LIB_FUNCTION("jjjRS7l1MPM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FLog);
    LIB_FUNCTION("OAE3YU396YQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FLogpoly);
    LIB_FUNCTION("+SeQg8c1WC0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Flt);
    LIB_FUNCTION("Jo9ON-AX9eU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fltrounds);
    LIB_FUNCTION("VVgqI3B2bfk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FNan);
    LIB_FUNCTION("xGT4Mc55ViQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fofind);
    LIB_FUNCTION("jVDuvE3s5Bs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fofree);
    LIB_FUNCTION("sQL8D-jio7U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fopen);
    LIB_FUNCTION("dREVnZkAKRE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Foprep);
    LIB_FUNCTION("vhPKxN6zs+A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fpcomp);
    LIB_FUNCTION("cfpRP3h9F6o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FPlsw);
    LIB_FUNCTION("IdWhZ0SM7JA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FPmsw);
    LIB_FUNCTION("5AN3vhTZ7f8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FPoly);
    LIB_FUNCTION("A98W3Iad6xE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FPow);
    LIB_FUNCTION("y9Ur6T0A0p8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FQuad);
    LIB_FUNCTION("PDQbEFcV4h0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FQuadph);
    LIB_FUNCTION("lP9zfrhtpBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FRecip);
    LIB_FUNCTION("TLvAYmLtdVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FRint);
    LIB_FUNCTION("9s3P+LCvWP8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Frprep);
    LIB_FUNCTION("XwRd4IpNEss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FRteps);
    LIB_FUNCTION("fQ+SWrQUQBg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FSincos);
    LIB_FUNCTION("O4L+0oCN9zA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FSinh);
    LIB_FUNCTION("UCjpTas5O3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FSnan);
    LIB_FUNCTION("A+Y3xfrWLLo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fspos);
    LIB_FUNCTION("iBrTJkDlQv8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FTan);
    LIB_FUNCTION("odPHnVL-rFg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FTgamma);
    LIB_FUNCTION("4F9pQjbh8R8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Fwprep);
    LIB_FUNCTION("3uW2ESAzsKo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXbig);
    LIB_FUNCTION("1EyHxzcz6AM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_addh);
    LIB_FUNCTION("1b+IhPTX0nk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_addx);
    LIB_FUNCTION("e1y7KVAySrY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_getw);
    LIB_FUNCTION("OVqW4uElSrc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_invx);
    LIB_FUNCTION("7GgGIxmwA6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_ldexpx);
    LIB_FUNCTION("DVZmEd0ipSg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_movx);
    LIB_FUNCTION("W+lrIwAQVUk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_mulh);
    LIB_FUNCTION("o1B4dkvesMc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_mulx);
    LIB_FUNCTION("ikHTMeh60B0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_setn);
    LIB_FUNCTION("5zWUVRtR8xg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_setw);
    LIB_FUNCTION("pNWIpeE5Wv4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_sqrtx);
    LIB_FUNCTION("HD9vSXqj6zI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FXp_subx);
    LIB_FUNCTION("LrXu7E+GLDY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__FZero);
    LIB_FUNCTION("7FjitE7KKm4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Gamma_big);
    LIB_FUNCTION("vakoyx9nkqo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Genld);
    LIB_FUNCTION("bRN9BzEkm4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Gentime);
    LIB_FUNCTION("2MfMa3456FI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getcloc);
    LIB_FUNCTION("i1N28hWcD-4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getctyptab);
    LIB_FUNCTION("Jcu0Wl1-XbE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getdst);
    LIB_FUNCTION("M1xC101lsIU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Geterrno);
    LIB_FUNCTION("bItEABINEm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getfld);
    LIB_FUNCTION("7iFNNuNyXxw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getfloat);
    LIB_FUNCTION("8Jr4cvRM6EM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getgloballocale);
    LIB_FUNCTION("PWmDp8ZTS9k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getint);
    LIB_FUNCTION("U52BlHBvYvE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getmbcurmax);
    LIB_FUNCTION("bF4eWOM5ouo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getpcostate);
    LIB_FUNCTION("sUP1hBaouOw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getpctype);
    LIB_FUNCTION("QxqK-IdpumU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getpmbstate);
    LIB_FUNCTION("iI6kGxgXzcU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__getprogname);
    LIB_FUNCTION("8xXiEPby8h8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getptimes);
    LIB_FUNCTION("1uJgoVq3bQU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getptolower);
    LIB_FUNCTION("rcQCUr0EaRU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getptoupper);
    LIB_FUNCTION("hzsdjKbFD7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getpwcostate);
    LIB_FUNCTION("zS94yyJRSUs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getpwcstate);
    LIB_FUNCTION("RLdcWoBjmT4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getpwctrtab);
    LIB_FUNCTION("uF8hDs1CqWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getpwctytab);
    LIB_FUNCTION("g8ozp2Zrsj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Gettime);
    LIB_FUNCTION("Wz9CvcF5jn4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Getzone);
    LIB_FUNCTION("ac102y6Rjjc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Hugeval);
    LIB_FUNCTION("wUa+oPQvFZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Hypot);
    LIB_FUNCTION("HIhqigNaOns", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Inf);
    LIB_FUNCTION("bzQExy189ZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__init_env);
    LIB_FUNCTION("6NCOqr3cD74", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__init_tls);
    LIB_FUNCTION("Sb26PiOiFtE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Isdst);
    LIB_FUNCTION("CyXs2l-1kNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Iswctype);
    LIB_FUNCTION("2Aw366Jn07s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LAtan);
    LIB_FUNCTION("moDSeLQGJFQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LCosh);
    LIB_FUNCTION("43u-nm1hQc8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Ldbl);
    LIB_FUNCTION("hdcGjNpcr4w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDclass);
    LIB_FUNCTION("O7zxyNnSDDA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDenorm);
    LIB_FUNCTION("alNWe8glQH4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDint);
    LIB_FUNCTION("HPGLb8Qo6as", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDivide);
    LIB_FUNCTION("ldbrWsQk+2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDnorm);
    LIB_FUNCTION("s-Ml8NxBKf4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDscale);
    LIB_FUNCTION("islhay8zGWo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDsign);
    LIB_FUNCTION("PEU-SAfo5+0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDtento);
    LIB_FUNCTION("A+1YXWOGpuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDtest);
    LIB_FUNCTION("3BbBNPjfkYI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Ldtob);
    LIB_FUNCTION("ArZF2KISb5k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LDunscale);
    LIB_FUNCTION("DzkYNChIvmw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LEps);
    LIB_FUNCTION("urrv9v-Ge6g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LErf_one);
    LIB_FUNCTION("MHyK+d+72V0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LErf_small);
    LIB_FUNCTION("PG4HVe4X+Oc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LErfc);
    LIB_FUNCTION("se3uU7lRMHY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LExp);
    LIB_FUNCTION("-BwLPxElT7U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LFpcomp);
    LIB_FUNCTION("-svZFUiG3T4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LGamma_big);
    LIB_FUNCTION("IRUFZywbTT0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LHypot);
    LIB_FUNCTION("2h3jY75zMkI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LInf);
    LIB_FUNCTION("+GxvfSLhoAo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Litob);
    LIB_FUNCTION("4iFgTDd9NFs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LLog);
    LIB_FUNCTION("niPixjs0l2w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LLogpoly);
    LIB_FUNCTION("zqASRvZg6d0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LNan);
    LIB_FUNCTION("JHvEnCQLPQI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Locale);
    LIB_FUNCTION("fRWufXAccuI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Lock_shared_ptr_spin_lock);
    LIB_FUNCTION("Cv-8x++GS9A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Lock_spin_lock);
    LIB_FUNCTION("vZkmJmvqueY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Lockfilelock);
    LIB_FUNCTION("kALvdgEv5ME", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Locksyslock);
    LIB_FUNCTION("sYz-SxY8H6M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Locsum);
    LIB_FUNCTION("rvNWRuHY6AQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Loctab);
    LIB_FUNCTION("4ifo9QGrO5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Locterm);
    LIB_FUNCTION("ElU3kEY8q+I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Locvar);
    LIB_FUNCTION("zXCi78bYrEI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Log);
    LIB_FUNCTION("bqcStLRGIXw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Logpoly);
    LIB_FUNCTION("W-T-amhWrkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LPlsw);
    LIB_FUNCTION("gso5X06CFkI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LPmsw);
    LIB_FUNCTION("c6Qa0P3XKYQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LPoly);
    LIB_FUNCTION("3Z8jD-HTKr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LPow);
    LIB_FUNCTION("fXCPTDS0tD0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LQuad);
    LIB_FUNCTION("CbRhl+RoYEM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LQuadph);
    LIB_FUNCTION("XrXTtRA7U08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LRecip);
    LIB_FUNCTION("fJmOr59K8QY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LRint);
    LIB_FUNCTION("gobJundphD0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LRteps);
    LIB_FUNCTION("m-Bu5Tzr82A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LSin);
    LIB_FUNCTION("g3dK2qlzwnM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LSincos);
    LIB_FUNCTION("ZWy6IcBqs1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LSinh);
    LIB_FUNCTION("rQ-70s51wrc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LSnan);
    LIB_FUNCTION("mM4OblD9xWM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LTan);
    LIB_FUNCTION("jq4Srfnz9K8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LTgamma);
    LIB_FUNCTION("WSaroeRDE5Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXbig);
    LIB_FUNCTION("QEr-PxGUoic", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_addh);
    LIB_FUNCTION("BuP7bDPGEcQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_addx);
    LIB_FUNCTION("TnublTBYXTI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_getw);
    LIB_FUNCTION("FAreWopdBvE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_invx);
    LIB_FUNCTION("7O-vjsHecbY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_ldexpx);
    LIB_FUNCTION("wlEdSSqaz+E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_movx);
    LIB_FUNCTION("riets0BFHMY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_mulh);
    LIB_FUNCTION("LbLiLA2biaI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_mulx);
    LIB_FUNCTION("hjDoZ6qbkmQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_setn);
    LIB_FUNCTION("kHVXc-AWbMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_setw);
    LIB_FUNCTION("IPjwywTNR8w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_sqrtx);
    LIB_FUNCTION("YCVxOE0lHOI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LXp_subx);
    LIB_FUNCTION("A-cEjaZBDwA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__LZero);
    LIB_FUNCTION("tTDqwhYbUUo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Makeloc);
    LIB_FUNCTION("AnTtuRUE1cE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Makestab);
    LIB_FUNCTION("QalEczzSNqc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Makewct);
    LIB_FUNCTION("pCWh67X1PHU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mbcurmax);
    LIB_FUNCTION("wKsLxRq5+fg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mbstate);
    LIB_FUNCTION("sij3OtJpHFc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mbtowc);
    LIB_FUNCTION("-9SIhUr4Iuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mbtowcx);
    LIB_FUNCTION("VYQwFs4CC4Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtx_current_owns);
    LIB_FUNCTION("5Lf51jvohTQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtx_destroy);
    LIB_FUNCTION("YaHc3GS7y7g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtx_init);
    LIB_FUNCTION("tgioGpKtmbE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtx_init_with_name);
    LIB_FUNCTION("iS4aWbUonl0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtx_lock);
    LIB_FUNCTION("hPzYSd5Nasc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtx_timedlock);
    LIB_FUNCTION("k6pGNMwJB08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtx_trylock);
    LIB_FUNCTION("gTuXQwP9rrs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtx_unlock);
    LIB_FUNCTION("LaPaA6mYA38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtxdst);
    LIB_FUNCTION("z7STeF6abuU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtxinit);
    LIB_FUNCTION("pE4Ot3CffW0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtxlock);
    LIB_FUNCTION("cMwgSSmpE5o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Mtxunlock);
    LIB_FUNCTION("8e2KBTO08Po", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Nan);
    LIB_FUNCTION("KNNNbyRieqQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__new_setup);
    LIB_FUNCTION("Ss3108pBuZY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Nnl);
    LIB_FUNCTION("TMhLRjcQMw8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__PathLocale);
    LIB_FUNCTION("OnHQSrOHTks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__PJP_C_Copyright);
    LIB_FUNCTION("Cqpti4y-D3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__PJP_CPP_Copyright);
    LIB_FUNCTION("1hGmBh83dL8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Plsw);
    LIB_FUNCTION("hrv1BgM68kU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Pmsw);
    LIB_FUNCTION("aINUE2xbhZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Poly);
    LIB_FUNCTION("ECh+p-LRG6Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Pow);
    LIB_FUNCTION("rnxaQ+2hSMI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Putfld);
    LIB_FUNCTION("Fot-m6M2oKE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Putstr);
    LIB_FUNCTION("ijAqq39g4dI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Puttxt);
    LIB_FUNCTION("TQPr-IeNIS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Quad);
    LIB_FUNCTION("VecRKuKdXdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Quadph);
    LIB_FUNCTION("5qtcuXWt5Xc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Randseed);
    LIB_FUNCTION("-u3XfqNumMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__readdir_unlocked);
    LIB_FUNCTION("MxZ4Lc35Rig", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Readloc);
    LIB_FUNCTION("YLEc5sPn1Rg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Recip);
    LIB_FUNCTION("7ZFy2m9rc5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__reclaim_telldir);
    LIB_FUNCTION("77uWF3Z2q90", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Restore_state);
    LIB_FUNCTION("TzxDRcns9e4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Rint);
    LIB_FUNCTION("W8tdMlttt3o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Rteps);
    LIB_FUNCTION("5FoE+V3Aj0c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__rtld_addr_phdr);
    LIB_FUNCTION("R2QKo3hBLkw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__rtld_atfork_post);
    LIB_FUNCTION("i-7v8+UGglc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__rtld_atfork_pre);
    LIB_FUNCTION("dmHEVUqDYGw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__rtld_error);
    LIB_FUNCTION("AdYYKgtPlqg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__rtld_get_stack_prot);
    LIB_FUNCTION("gRw4XrztJ4Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__rtld_thread_init);
    LIB_FUNCTION("0ITXuJOqrSk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Save_state);
    LIB_FUNCTION("vhETq-NiqJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__SceLibcDebugOut);
    LIB_FUNCTION("-hOAbTf3Cqc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__SceLibcTelemetoryOut);
    LIB_FUNCTION("Do3zPpsXj1o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__seekdir);
    LIB_FUNCTION("bEk+WGOU90I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Setgloballocale);
    LIB_FUNCTION("KDFy-aPxHVE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Shared_ptr_flag);
    LIB_FUNCTION("j9SGTLclro8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Sincos);
    LIB_FUNCTION("MU25eqxSDTw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Sinh);
    LIB_FUNCTION("Jp6dZm7545A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Skip);
    LIB_FUNCTION("fmHLr9P3dOk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Snan);
    LIB_FUNCTION("H8AprKeZtNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stderr);
    LIB_FUNCTION("1TDo-ImqkJc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stdin);
    LIB_FUNCTION("2sWzhYqFH4E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Stdout);
    LIB_FUNCTION("szUft0jERdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal__Tan);
    LIB_FUNCTION("z-OrNOmb6kk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tgamma);
    LIB_FUNCTION("JOV1XY47eQA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_abort);
    LIB_FUNCTION("SkRR6WxCTcE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_create);
    LIB_FUNCTION("n2-b3O8qvqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_current);
    LIB_FUNCTION("L7f7zYwBvZA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_detach);
    LIB_FUNCTION("BnV7WrHdPLU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_equal);
    LIB_FUNCTION("cFsD9R4iY50", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_exit);
    LIB_FUNCTION("np6xXcXEnXE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_id);
    LIB_FUNCTION("YvmY5Jf0VYU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_join);
    LIB_FUNCTION("OCbJ96N1utA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_lt);
    LIB_FUNCTION("jfRI3snge3o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_sleep);
    LIB_FUNCTION("0JidN6q9yGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_start);
    LIB_FUNCTION("gsqXCd6skKs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_start_with_attr);
    LIB_FUNCTION("ihfGsjLjmOM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_start_with_name);
    LIB_FUNCTION("ShanbBWU3AA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_start_with_name_attr);
    LIB_FUNCTION("exNzzCAQuWM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Thrd_yield);
    LIB_FUNCTION("kQelMBYgkK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__thread_autoinit_dummy_decl);
    LIB_FUNCTION("dmUgzUX3nXU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__thread_autoinit_dummy_decl_stub);
    LIB_FUNCTION("PJW+-O4pj6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__thread_init);
    LIB_FUNCTION("1kIpmZX1jw8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__thread_init_stub);
    LIB_FUNCTION("+9ypoH8eJko", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Times);
    LIB_FUNCTION("hbY3mFi8XY0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Costate);
    LIB_FUNCTION("JoeZJ15k5vU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Ctype);
    LIB_FUNCTION("uhYTnarXhZM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Errno);
    LIB_FUNCTION("jr5yQE9WYdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Locale);
    LIB_FUNCTION("7TIcrP513IM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Mbcurmax);
    LIB_FUNCTION("YYG-8VURgXA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Mbstate);
    LIB_FUNCTION("0Hu7rUmhqJM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Times);
    LIB_FUNCTION("hM7qvmxBTx8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Tolotab);
    LIB_FUNCTION("UlJSnyS473g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Touptab);
    LIB_FUNCTION("YUdPel2w8as", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__WCostate);
    LIB_FUNCTION("2d5UH9BnfVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Wcstate);
    LIB_FUNCTION("RYwqCx0hU5c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Wctrans);
    LIB_FUNCTION("KdAc8glk2+8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tls_setup__Wctype);
    LIB_FUNCTION("J-oDqE1N8R4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tolotab);
    LIB_FUNCTION("KmfpnwO2lkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Touptab);
    LIB_FUNCTION("DbEnA+MnVIw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Towctrans);
    LIB_FUNCTION("amHyU7v8f-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tss_create);
    LIB_FUNCTION("g5cG7QtKLTA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tss_delete);
    LIB_FUNCTION("lOVQnEJEzvY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tss_get);
    LIB_FUNCTION("ibyFt0bPyTk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tss_set);
    LIB_FUNCTION("4TTbo2SxxvA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Ttotm);
    LIB_FUNCTION("Csx50UU9b18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Tzoff);
    LIB_FUNCTION("1HYEoANqZ1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Unlock_shared_ptr_spin_lock);
    LIB_FUNCTION("aHUFijEWlxQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Unlock_spin_lock);
    LIB_FUNCTION("0x7rx8TKy2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Unlockfilelock);
    LIB_FUNCTION("9nf8joUTSaQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Unlocksyslock);
    LIB_FUNCTION("s62MgBhosjU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Unwind_Backtrace);
    LIB_FUNCTION("sETNbyWsEHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Unwind_GetIP);
    LIB_FUNCTION("f1zwJ3jAI2k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Unwind_Resume);
    LIB_FUNCTION("xUsJSLsdv9I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Unwind_Resume_or_Rethrow);
    LIB_FUNCTION("xRycekLYXdc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Vacopy);
    LIB_FUNCTION("XQFE8Y58WZM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__warn);
    LIB_FUNCTION("Nd2Y2X6oR18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WCostate);
    LIB_FUNCTION("wmkXFgerLnM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Wcscollx);
    LIB_FUNCTION("RGc3P3UScjY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Wcsftime);
    LIB_FUNCTION("IvP-B8lC89k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Wcstate);
    LIB_FUNCTION("cmIyU0BNYeo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Wcsxfrmx);
    LIB_FUNCTION("oK6C1fys3dU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Wctob);
    LIB_FUNCTION("bSgY14j4Ov4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Wctomb);
    LIB_FUNCTION("stv1S3BKfgw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Wctombx);
    LIB_FUNCTION("DYamMikEv2M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Wctrans);
    LIB_FUNCTION("PlDgAP2AS7M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Wctype);
    LIB_FUNCTION("VbczgfwgScA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WFrprep);
    LIB_FUNCTION("hDuyUWUBrDU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WFwprep);
    LIB_FUNCTION("BYcXjG3Lw-o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WGenld);
    LIB_FUNCTION("Z6CCOW8TZVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WGetfld);
    LIB_FUNCTION("LcHsLn97kcE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WGetfloat);
    LIB_FUNCTION("dWz3HtMMpPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WGetint);
    LIB_FUNCTION("nVS8UHz1bx0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WGetstr);
    LIB_FUNCTION("kUcinoWwBr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WLdtob);
    LIB_FUNCTION("XkT6YSShQcE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WLitob);
    LIB_FUNCTION("0ISumvb2U5o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WPutfld);
    LIB_FUNCTION("Fh1GjwqvCpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WPutstr);
    LIB_FUNCTION("Kkgg8mWU2WE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WPuttxt);
    LIB_FUNCTION("4nRn+exUJAM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStod);
    LIB_FUNCTION("RlewTNtWEcg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStodx);
    LIB_FUNCTION("GUuiOcxL-r0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStof);
    LIB_FUNCTION("FHJlhz0wVts", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStoflt);
    LIB_FUNCTION("JZ9gGlJ22hg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStofx);
    LIB_FUNCTION("w3gRFGRjpZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStold);
    LIB_FUNCTION("waexoHL0Bf4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStoldx);
    LIB_FUNCTION("OmDPJeJXkBM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStoll);
    LIB_FUNCTION("43PYQ2fMT8k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStopfx);
    LIB_FUNCTION("JhVR7D4Ax6Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStoul);
    LIB_FUNCTION("9iCP-hTL5z8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStoull);
    LIB_FUNCTION("DmUIy7m0cyE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__WStoxflt);
    LIB_FUNCTION("QSfaClY45dM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xbig);
    LIB_FUNCTION("ijc7yCXzXsc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_addh);
    LIB_FUNCTION("ycMCyFmWJnU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_addx);
    LIB_FUNCTION("Zb70g9IUs98", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_getw);
    LIB_FUNCTION("f41T4clGlzY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_invx);
    LIB_FUNCTION("c9S0tXDhMBQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_ldexpx);
    LIB_FUNCTION("Zm2LLWgxWu8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_movx);
    LIB_FUNCTION("aOtpC3onyJo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_mulh);
    LIB_FUNCTION("jatbHyxH3ek", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_mulx);
    LIB_FUNCTION("lahbB4B2ugY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_setn);
    LIB_FUNCTION("bIfFaqUwy3I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_setw);
    LIB_FUNCTION("m0uSPrCsVdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_sqrtx);
    LIB_FUNCTION("w178uGYbIJs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xp_subx);
    LIB_FUNCTION("Df1BO64nU-k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xtime_diff_to_ts);
    LIB_FUNCTION("Cj+Fw5q1tUo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xtime_get_ticks);
    LIB_FUNCTION("Zs8Xq-ce3rY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Xtime_to_ts);
    LIB_FUNCTION("FOt55ZNaVJk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdaPvm);
    LIB_FUNCTION("7lCihI18N9I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdaPvmRKSt9nothrow_t);
    LIB_FUNCTION("Y1RR+IQy6Pg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdaPvmSt11align_val_t);
    LIB_FUNCTION("m-fSo3EbxNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdaPvRKSt9nothrow_t);
    LIB_FUNCTION("Suc8W0QPxjw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdaPvS_);
    LIB_FUNCTION("v09ZcAhZzSc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdaPvSt11align_val_t);
    LIB_FUNCTION("dH3ucvQhfSY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdaPvSt11align_val_tRKSt9nothrow_t);
    LIB_FUNCTION("z+P+xCnWLBk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdlPv);
    LIB_FUNCTION("lYDzBVE5mZs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdlPvm);
    LIB_FUNCTION("7VPIYFpwU2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdlPvmRKSt9nothrow_t);
    LIB_FUNCTION("nwujzxOPXzQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdlPvmSt11align_val_t);
    LIB_FUNCTION("McsGnqV6yRE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdlPvRKSt9nothrow_t);
    LIB_FUNCTION("1vo6qqqa9F4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdlPvS_);
    LIB_FUNCTION("bZx+FFSlkUM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdlPvSt11align_val_t);
    LIB_FUNCTION("Dt9kllUFXS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZdlPvSt11align_val_tRKSt9nothrow_t);
    LIB_FUNCTION("bPtdppw1+7I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Zero);
    LIB_FUNCTION("Bc4ozvHb4Kg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt10moneypunctIcLb0EE2idE);
    LIB_FUNCTION("yzcKSTTCz1M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt10moneypunctIcLb1EE2idE);
    LIB_FUNCTION("tfmEv+TVGFU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt10moneypunctIwLb0EE2idE);
    LIB_FUNCTION("ksNM8H72JHg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt10moneypunctIwLb1EE2idE);
    LIB_FUNCTION("2G1UznxkcgU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt14_Error_objectsIiE14_System_objectE);
    LIB_FUNCTION("DjLpZIMEkks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt14_Error_objectsIiE15_Generic_objectE);
    LIB_FUNCTION("DSnq6xesUo8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt20_Future_error_objectIiE14_Future_objectE);
    LIB_FUNCTION("agAYSUes238", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt7codecvtIcc9_MbstatetE2idE);
    LIB_FUNCTION("gC0DGo+7PVc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt7collateIcE2idE);
    LIB_FUNCTION("jaLGUrwYX84", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt7collateIwE2idE);
    LIB_FUNCTION("4ZnE1sEyX3g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt8messagesIcE2idE);
    LIB_FUNCTION("oqYAk3zpC64", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt8messagesIwE2idE);
    LIB_FUNCTION("iHZb2839dBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt8numpunctIcE2idE);
    LIB_FUNCTION("8ArIPXBlkgM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZGVNSt8numpunctIwE2idE);
    LIB_FUNCTION(
        "0ND8MZiuTR8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
        internal__ZGVZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit);
    LIB_FUNCTION(
        "O2wxIdbMcMQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
        internal__ZGVZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit);
    LIB_FUNCTION("CjzjU2nFUWw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv116__enum_type_infoD0Ev);
    LIB_FUNCTION("upwSZWmYwqE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv116__enum_type_infoD1Ev);
    LIB_FUNCTION("iQiT26+ZGnA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv116__enum_type_infoD2Ev);
    LIB_FUNCTION("R5nRbLQT3oI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv117__array_type_infoD0Ev);
    LIB_FUNCTION("1ZMchlkvNNQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv117__array_type_infoD1Ev);
    LIB_FUNCTION("ckFrsyD2830", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv117__array_type_infoD2Ev);
    LIB_FUNCTION("XAk7W3NcpTY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv117__class_type_infoD0Ev);
    LIB_FUNCTION("goLVqD-eiIY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv117__class_type_infoD1Ev);
    LIB_FUNCTION("xXM1q-ayw2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv117__class_type_infoD2Ev);
    LIB_FUNCTION("GLxD5v2uMHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv117__pbase_type_infoD0Ev);
    LIB_FUNCTION("vIJPARS8imE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv117__pbase_type_infoD1Ev);
    LIB_FUNCTION("krshE4JAE6M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv117__pbase_type_infoD2Ev);
    LIB_FUNCTION("64180GwMVro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv119__pointer_type_infoD0Ev);
    LIB_FUNCTION("bhfgrK+MZdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv119__pointer_type_infoD1Ev);
    LIB_FUNCTION("vCLVhOcdQMY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv119__pointer_type_infoD2Ev);
    LIB_FUNCTION("kVvGL9aF5gg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv120__function_type_infoD0Ev);
    LIB_FUNCTION("dsQ5Xwhl9no", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv120__function_type_infoD1Ev);
    LIB_FUNCTION("NtqD4Q0vUUI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv120__function_type_infoD2Ev);
    LIB_FUNCTION("Fg4w+h9wAMA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv120__si_class_type_infoD0Ev);
    LIB_FUNCTION("6aEkwkEOGRg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv120__si_class_type_infoD1Ev);
    LIB_FUNCTION("bWHwovVFfqc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv120__si_class_type_infoD2Ev);
    LIB_FUNCTION("W5k0jlyBpgM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv121__vmi_class_type_infoD0Ev);
    LIB_FUNCTION("h-a7+0UuK7Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv121__vmi_class_type_infoD1Ev);
    LIB_FUNCTION("yYIymfQGl2E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv121__vmi_class_type_infoD2Ev);
    LIB_FUNCTION("YsZuwZrJZlU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv123__fundamental_type_infoD0Ev);
    LIB_FUNCTION("kzWL2iOsv0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv123__fundamental_type_infoD1Ev);
    LIB_FUNCTION("0jk9oqKd2Gw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv123__fundamental_type_infoD2Ev);
    LIB_FUNCTION("NdeDffcZ-30", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv129__pointer_to_member_type_infoD0Ev);
    LIB_FUNCTION("KaZ3xf5c9Vc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv129__pointer_to_member_type_infoD1Ev);
    LIB_FUNCTION("Re3Lpk8mwEo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN10__cxxabiv129__pointer_to_member_type_infoD2Ev);
    LIB_FUNCTION("yc-vi84-2aE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN6Dinkum7codecvt10_Cvt_checkEmm);
    LIB_FUNCTION("PG-2ZeVVWZE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN6Dinkum7threads10lock_errorD0Ev);
    LIB_FUNCTION("vX+NfOHOI5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN6Dinkum7threads10lock_errorD1Ev);
    LIB_FUNCTION("o27+xO5NBZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN6Dinkum7threads17_Throw_lock_errorEv);
    LIB_FUNCTION("cjmJLdYnT5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN6Dinkum7threads21_Throw_resource_errorEv);
    LIB_FUNCTION("Q+ZnPMGI4M0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN6Dinkum7threads21thread_resource_errorD0Ev);
    LIB_FUNCTION("NOaB7a1PZl8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZN6Dinkum7threads21thread_resource_errorD1Ev);
    LIB_FUNCTION("hdm0YfMa7TQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__Znam);
    LIB_FUNCTION("Jh5qUcwiSEk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZnamRKSt9nothrow_t);
    LIB_FUNCTION("kn-rKRB0pfY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZnamSt11align_val_t);
    LIB_FUNCTION("s2eGsgUF9vk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZnamSt11align_val_tRKSt9nothrow_t);
    LIB_FUNCTION("ZRRBeuLmHjc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSbIwSt11char_traitsIwESaIwEE5_XlenEv);
    LIB_FUNCTION("GvYZax3i-Qk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSbIwSt11char_traitsIwESaIwEE5_XranEv);
    LIB_FUNCTION("pDtTdJ2sIz0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSs5_XlenEv);
    LIB_FUNCTION("AzBnOt1DouU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSs5_XranEv);
    LIB_FUNCTION("BbXxNgTW1x4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt10bad_typeid4whatEv);
    LIB_FUNCTION("WMw8eIs0kjM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt10bad_typeid8_DoraiseEv);
    LIB_FUNCTION("++ge3jYlHW8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt11logic_error4whatEv);
    LIB_FUNCTION("YTM5eyFGh2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt11logic_error8_DoraiseEv);
    LIB_FUNCTION("OzMS0BqVUGQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt12bad_weak_ptr4whatEv);
    LIB_FUNCTION("MwAySqTo+-M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt12codecvt_base11do_encodingEv);
    LIB_FUNCTION("FOsY+JAyXow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt12codecvt_base13do_max_lengthEv);
    LIB_FUNCTION("5sfbtNAdt-M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt12future_error4whatEv);
    LIB_FUNCTION("-syPONaWjqw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt12future_error8_DoraiseEv);
    LIB_FUNCTION("uWZBRxLAvEg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt12system_error8_DoraiseEv);
    LIB_FUNCTION("kTlQY47fo88", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt13bad_exception8_DoraiseEv);
    LIB_FUNCTION("2iW5Fv+kFxs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt13runtime_error4whatEv);
    LIB_FUNCTION("GthClwqQAZs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt14error_category10equivalentEiRKSt15error_condition);
    LIB_FUNCTION("9hB8AwIqQfs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt14error_category10equivalentERKSt10error_codei);
    LIB_FUNCTION("8SDojuZyQaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt14error_category23default_error_conditionEi);
    LIB_FUNCTION("XVu4--EWzcM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt17bad_function_call4whatEv);
    LIB_FUNCTION("+5IOQncui3c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt18bad_variant_access4whatEv);
    LIB_FUNCTION("Q0VsWTapQ4M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt22_Future_error_category4nameEv);
    LIB_FUNCTION("nWfZplDjbxQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt22_Future_error_category7messageEi);
    LIB_FUNCTION("ww3UUl317Ng", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt22_System_error_category23default_error_conditionEi);
    LIB_FUNCTION("dXy+lFOiaQA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt22_System_error_category4nameEv);
    LIB_FUNCTION("HpAlvhNKb2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt22_System_error_category7messageEi);
    LIB_FUNCTION("xvVR0CBPFAM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt23_Generic_error_category4nameEv);
    LIB_FUNCTION("KZ++filsCL4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt23_Generic_error_category7messageEi);
    LIB_FUNCTION("WXOoCK+kqwY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIcE10do_tolowerEc);
    LIB_FUNCTION("2w+4Mo2DPro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIcE10do_tolowerEPcPKc);
    LIB_FUNCTION("mnq3tbhCs34", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIcE10do_toupperEc);
    LIB_FUNCTION("7glioH0t9HM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIcE10do_toupperEPcPKc);
    LIB_FUNCTION("zwcNQT0Avck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIcE8do_widenEc);
    LIB_FUNCTION("W5OtP+WC800", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIcE8do_widenEPKcS2_Pc);
    LIB_FUNCTION("4SnCJmLL27U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIcE9do_narrowEcc);
    LIB_FUNCTION("-nCVE3kBjjA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIcE9do_narrowEPKcS2_cPc);
    LIB_FUNCTION("pSQz254t3ug", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE10do_scan_isEsPKwS2_);
    LIB_FUNCTION("Ej0X1EwApwM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE10do_tolowerEPwPKw);
    LIB_FUNCTION("ATYj6zra5W0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE10do_tolowerEw);
    LIB_FUNCTION("r1W-NtOi6E8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE10do_toupperEPwPKw);
    LIB_FUNCTION("JsZjB3TnZ8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE10do_toupperEw);
    LIB_FUNCTION("Kbe+LHOer9o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE11do_scan_notEsPKwS2_);
    LIB_FUNCTION("D0lUMKU+ELI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE5do_isEPKwS2_Ps);
    LIB_FUNCTION("rh7L-TliPoc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE5do_isEsw);
    LIB_FUNCTION("h3PbnNnZ-gI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE8do_widenEc);
    LIB_FUNCTION("KM0b6O8show", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE8do_widenEPKcS2_Pw);
    LIB_FUNCTION("Vf68JAD5rbM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE9do_narrowEPKwS2_cPc);
    LIB_FUNCTION("V+c0E+Uqcww", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt5ctypeIwE9do_narrowEwc);
    LIB_FUNCTION("aUNISsPboqE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIcE11do_groupingEv);
    LIB_FUNCTION("uUDq10y4Raw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIcE13do_neg_formatEv);
    LIB_FUNCTION("E64hr8yXoXw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIcE13do_pos_formatEv);
    LIB_FUNCTION("VhyjwJugIe8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIcE14do_curr_symbolEv);
    LIB_FUNCTION("C3LC9A6SrVE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIcE14do_frac_digitsEv);
    LIB_FUNCTION("tZj4yquwuhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIcE16do_decimal_pointEv);
    LIB_FUNCTION("Rqu9OmkKY+M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIcE16do_negative_signEv);
    LIB_FUNCTION("ARZszYKvDWg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIcE16do_positive_signEv);
    LIB_FUNCTION("6aFwTNpqTP8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIcE16do_thousands_sepEv);
    LIB_FUNCTION("ckD5sIxo+Co", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIwE11do_groupingEv);
    LIB_FUNCTION("UzmR8lOfyqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIwE13do_neg_formatEv);
    LIB_FUNCTION("zF2GfKzBgqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIwE13do_pos_formatEv);
    LIB_FUNCTION("ypq5jFNRIJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIwE14do_curr_symbolEv);
    LIB_FUNCTION("ij-yZhH9YjY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIwE14do_frac_digitsEv);
    LIB_FUNCTION("v8P1X84ytFY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIwE16do_decimal_pointEv);
    LIB_FUNCTION("zkUC74aJxpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIwE16do_negative_signEv);
    LIB_FUNCTION("PWFePkVdv9w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIwE16do_positive_signEv);
    LIB_FUNCTION("XX+xiPXAN8I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7_MpunctIwE16do_thousands_sepEv);
    LIB_FUNCTION("iCWgjeqMHvg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIcc9_MbstatetE10do_unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("7tIwDZyyKHo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIcc9_MbstatetE16do_always_noconvEv);
    LIB_FUNCTION("TNexGlwiVEQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIcc9_MbstatetE2inERS0_PKcS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("14xKj+SV72o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIcc9_MbstatetE3outERS0_PKcS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("P+q1OLiErP0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIcc9_MbstatetE5do_inERS0_PKcS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("Uc+-Sx0UZ3U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIcc9_MbstatetE6do_outERS0_PKcS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("ikBt0lqkxPc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIcc9_MbstatetE6lengthERS0_PKcS4_m);
    LIB_FUNCTION("N7z+Dnk1cS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIcc9_MbstatetE7unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("Xk7IZcfHDD8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIcc9_MbstatetE9do_lengthERS0_PKcS4_m);
    LIB_FUNCTION("c6Lyc6xOp4o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDic9_MbstatetE10do_unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("DDnr3lDwW8I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDic9_MbstatetE11do_encodingEv);
    LIB_FUNCTION("E5NdzqEmWuY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDic9_MbstatetE13do_max_lengthEv);
    LIB_FUNCTION("NQ81EZ7CL6w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDic9_MbstatetE16do_always_noconvEv);
    LIB_FUNCTION("PJ2UDX9Tvwg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDic9_MbstatetE5do_inERS0_PKcS4_RS4_PDiS6_RS6_);
    LIB_FUNCTION("eoW60zcLT8Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDic9_MbstatetE6do_outERS0_PKDiS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("m6rjfL4aMcA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDic9_MbstatetE9do_lengthERS0_PKcS4_m);
    LIB_FUNCTION("RYTHR81Y-Mc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDsc9_MbstatetE10do_unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("Mo6K-pUyNhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDsc9_MbstatetE11do_encodingEv);
    LIB_FUNCTION("BvRS0cGTd6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDsc9_MbstatetE13do_max_lengthEv);
    LIB_FUNCTION("9Vyfb-I-9xw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDsc9_MbstatetE16do_always_noconvEv);
    LIB_FUNCTION("+uPwCGfmJHs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDsc9_MbstatetE5do_inERS0_PKcS4_RS4_PDsS6_RS6_);
    LIB_FUNCTION("0FKwlv9iH1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDsc9_MbstatetE6do_outERS0_PKDsS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("lynApfiP6Lw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIDsc9_MbstatetE9do_lengthERS0_PKcS4_m);
    LIB_FUNCTION("oDtGxrzLXMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIwc9_MbstatetE10do_unshiftERS0_PcS3_RS3_);
    LIB_FUNCTION("4fPIrH+z+E4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIwc9_MbstatetE11do_encodingEv);
    LIB_FUNCTION("5BQIjX7Y5YU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIwc9_MbstatetE13do_max_lengthEv);
    LIB_FUNCTION("KheIhkaSrlA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIwc9_MbstatetE16do_always_noconvEv);
    LIB_FUNCTION("WAPkmrXx2o8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIwc9_MbstatetE5do_inERS0_PKcS4_RS4_PwS6_RS6_);
    LIB_FUNCTION("ABFE75lbcDc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIwc9_MbstatetE6do_outERS0_PKwS4_RS4_PcS6_RS6_);
    LIB_FUNCTION("G1zcPUEvY7U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7codecvtIwc9_MbstatetE9do_lengthERS0_PKcS4_m);
    LIB_FUNCTION("1eEXfeW6wrI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIcE10do_compareEPKcS2_S2_S2_);
    LIB_FUNCTION("gYlF567r3-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIcE12do_transformEPKcS2_);
    LIB_FUNCTION("6vYXzFD-mrk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIcE4hashEPKcS2_);
    LIB_FUNCTION("Q-8lQCGMj4g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIcE7compareEPKcS2_S2_S2_);
    LIB_FUNCTION("GSAXi4F1SlM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIcE7do_hashEPKcS2_);
    LIB_FUNCTION("XaSxLBnqcWE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIcE9transformEPKcS2_);
    LIB_FUNCTION("roztnFEs5Es", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIwE10do_compareEPKwS2_S2_S2_);
    LIB_FUNCTION("Zxe-nQMgxHM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIwE12do_transformEPKwS2_);
    LIB_FUNCTION("entYnoIu+fc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIwE4hashEPKwS2_);
    LIB_FUNCTION("n-3HFZvDwBw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIwE7compareEPKwS2_S2_S2_);
    LIB_FUNCTION("cWaCDW+Dc9U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIwE7do_hashEPKwS2_);
    LIB_FUNCTION("81uX7PzrtG8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt7collateIwE9transformEPKwS2_);
    LIB_FUNCTION("6CPwoi-cFZM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8bad_cast4whatEv);
    LIB_FUNCTION("NEemVJeMwd0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8bad_cast8_DoraiseEv);
    LIB_FUNCTION("F27xQUBtNdU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8ios_base7failure8_DoraiseEv);
    LIB_FUNCTION("XxsPrrWJ52I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIcE3getEiiiRKSs);
    LIB_FUNCTION("U2t+WvMQj8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIcE4openERKSsRKSt6locale);
    LIB_FUNCTION("EeBQ7253LkE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIcE5closeEi);
    LIB_FUNCTION("vbgCuYKySLI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIcE6do_getEiiiRKSs);
    LIB_FUNCTION("HeBwePMtuFs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIcE7do_openERKSsRKSt6locale);
    LIB_FUNCTION("rRmMX83UL1E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIcE8do_closeEi);
    LIB_FUNCTION("Ea+awuQ5Bm8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIwE3getEiiiRKSbIwSt11char_traitsIwESaIwEE);
    LIB_FUNCTION("TPq0HfoACeI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIwE4openERKSsRKSt6locale);
    LIB_FUNCTION("GGoH7e6SZSY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIwE5closeEi);
    LIB_FUNCTION("UM6rGQxnEMg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIwE6do_getEiiiRKSbIwSt11char_traitsIwESaIwEE);
    LIB_FUNCTION("zSehLdHI0FA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIwE7do_openERKSsRKSt6locale);
    LIB_FUNCTION("AjkxQBlsOOY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8messagesIwE8do_closeEi);
    LIB_FUNCTION("cnNz2ftNwEU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE11do_groupingEv);
    LIB_FUNCTION("nRf0VQ++OEw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE11do_truenameEv);
    LIB_FUNCTION("ozLi0i4r6ds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE12do_falsenameEv);
    LIB_FUNCTION("klWxQ2nKAHY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE13decimal_pointEv);
    LIB_FUNCTION("QGSIlqfIU2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE13thousands_sepEv);
    LIB_FUNCTION("JXzQGOtumdM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE16do_decimal_pointEv);
    LIB_FUNCTION("zv1EMhI7R1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE16do_thousands_sepEv);
    LIB_FUNCTION("JWplGh2O0Rs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE8groupingEv);
    LIB_FUNCTION("fXUuZEw7C24", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE8truenameEv);
    LIB_FUNCTION("3+VwUA8-QPI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIcE9falsenameEv);
    LIB_FUNCTION("2BmJdX269kI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE11do_groupingEv);
    LIB_FUNCTION("nvSsAW7tcX8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE11do_truenameEv);
    LIB_FUNCTION("-amctzWbEtw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE12do_falsenameEv);
    LIB_FUNCTION("leSFwTZZuE4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE13decimal_pointEv);
    LIB_FUNCTION("2Olt9gqOauQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE13thousands_sepEv);
    LIB_FUNCTION("mzRlAVX65hQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE16do_decimal_pointEv);
    LIB_FUNCTION("Utj8Sh5L0jE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE16do_thousands_sepEv);
    LIB_FUNCTION("VsJCpXqMPJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE8groupingEv);
    LIB_FUNCTION("3M20pLo9Gdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE8truenameEv);
    LIB_FUNCTION("LDbKkgI-TZg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt8numpunctIwE9falsenameEv);
    LIB_FUNCTION("xvRvFtnUk3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt9bad_alloc4whatEv);
    LIB_FUNCTION("pS-t9AJblSM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt9bad_alloc8_DoraiseEv);
    LIB_FUNCTION("apPZ6HKZWaQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt9exception4whatEv);
    LIB_FUNCTION("DuW5ZqZv-70", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt9exception6_RaiseEv);
    LIB_FUNCTION("tyHd3P7oDrU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNKSt9exception8_DoraiseEv);
    LIB_FUNCTION("Ti86LmOKvr0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSbIwSt11char_traitsIwESaIwEE5_CopyEmm);
    LIB_FUNCTION("TgEb5a+nOnk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSbIwSt11char_traitsIwESaIwEE5eraseEmm);
    LIB_FUNCTION("nF8-CM+tro4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSbIwSt11char_traitsIwESaIwEE6appendEmw);
    LIB_FUNCTION("hSUcSStZEHM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSbIwSt11char_traitsIwESaIwEE6appendERKS2_mm);
    LIB_FUNCTION("8oO55jndPRg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSbIwSt11char_traitsIwESaIwEE6assignEmw);
    LIB_FUNCTION("IJmeA5ayVJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSbIwSt11char_traitsIwESaIwEE6assignEPKwm);
    LIB_FUNCTION("piJabTDQRVs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSbIwSt11char_traitsIwESaIwEE6assignERKS2_mm);
    LIB_FUNCTION("w2GyuoXCnkw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSbIwSt11char_traitsIwESaIwEE6insertEmmw);
    LIB_FUNCTION("6ZDv6ZusiFg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSiD0Ev);
    LIB_FUNCTION("tJU-ttrsXsk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSiD1Ev);
    LIB_FUNCTION("gVTWlvyBSIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSo6sentryC2ERSo);
    LIB_FUNCTION("nk+0yTWvoRE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSo6sentryD2Ev);
    LIB_FUNCTION("lTTrDj5OIwQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSoD0Ev);
    LIB_FUNCTION("HpCeP12cuNY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSoD1Ev);
    LIB_FUNCTION("9HILqEoh24E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSs5_CopyEmm);
    LIB_FUNCTION("0Ir3jiT4V6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSs5eraseEmm);
    LIB_FUNCTION("QqBWUNEfIAo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSs6appendEmc);
    LIB_FUNCTION("qiR-4jx1abE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSs6appendERKSsmm);
    LIB_FUNCTION("ikjnoeemspQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSs6assignEmc);
    LIB_FUNCTION("xSxPHmpcNzY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSs6assignEPKcm);
    LIB_FUNCTION("pGxNI4JKfmY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSs6assignERKSsmm);
    LIB_FUNCTION("KDgQWX1eDeo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSs6insertEmmc);
    LIB_FUNCTION("MHA0XR2YHoQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10bad_typeidD0Ev);
    LIB_FUNCTION("vzh0qoLIEuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10bad_typeidD1Ev);
    LIB_FUNCTION("tkZ7jVV6wJ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10bad_typeidD2Ev);
    LIB_FUNCTION("xGbaQPsHCFI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem10_Close_dirEPv);
    LIB_FUNCTION("PbCV7juCZVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem10_Copy_fileEPKcS1_);
    LIB_FUNCTION("SQ02ZA5E-UE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem10_File_sizeEPKc);
    LIB_FUNCTION("XD9FmX1mavU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem11_EquivalentEPKcS1_);
    LIB_FUNCTION("YDQxE4cIwa4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem11_Remove_dirEPKc);
    LIB_FUNCTION("8VKAqiw7lC0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem12_Current_getERA260_c);
    LIB_FUNCTION("Yl10kSufa5k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem12_Current_setEPKc);
    LIB_FUNCTION("HCB1auZdcmo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem16_Last_write_timeEPKc);
    LIB_FUNCTION("Wut42WAe7Rw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem18_Xfilesystem_errorEPKcRKNS_4pathES4_St10error_code);
    LIB_FUNCTION("C6-7Mo5WbwU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem18_Xfilesystem_errorEPKcRKNS_4pathESt10error_code);
    LIB_FUNCTION("B0CeIhQty7Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem18_Xfilesystem_errorEPKcSt10error_code);
    LIB_FUNCTION("VSk+sij2mwg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem20_Set_last_write_timeEPKcl);
    LIB_FUNCTION("EBwahsMLokw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem5_StatEPKcPNS_5permsE);
    LIB_FUNCTION("XyKw6Hs1P9Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem6_ChmodEPKcNS_5permsE);
    LIB_FUNCTION("o1qlZJqrvmI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem6_LstatEPKcPNS_5permsE);
    LIB_FUNCTION("srwl1hhFoUI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem7_RenameEPKcS1_);
    LIB_FUNCTION("O4mPool-pow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem7_ResizeEPKcm);
    LIB_FUNCTION("Iok1WdvAROg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem7_UnlinkEPKc);
    LIB_FUNCTION("SdKk439pgjg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem8_StatvfsEPKcRNS_10space_infoE);
    LIB_FUNCTION("x7pQExTeqBY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem9_Make_dirEPKcS1_);
    LIB_FUNCTION("8iuHpl+kg8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem9_Open_dirERA260_cPKcRiRNS_9file_typeE);
    LIB_FUNCTION("w5CGykBBU5M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10filesystem9_Read_dirERA260_cPvRNS_9file_typeE);
    LIB_FUNCTION("eF26YAKQWKA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EE2idE);
    LIB_FUNCTION("UbuTnKIXyCk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EE4intlE);
    LIB_FUNCTION("mU88GYCirhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("tYBLm0BoQdQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EEC1Em);
    LIB_FUNCTION("5afBJmEfUQI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EEC1EPKcm);
    LIB_FUNCTION("wrR3T5i7gpY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("5DFeXjP+Plg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EEC2Em);
    LIB_FUNCTION("aNfpdhcsMWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EEC2EPKcm);
    LIB_FUNCTION("uQFv8aNF8Jc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("sS5fF+fht2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EED0Ev);
    LIB_FUNCTION("3cW6MrkCKt0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EED1Ev);
    LIB_FUNCTION("mbGmSOLXgN0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb0EED2Ev);
    LIB_FUNCTION("PgiTG7nVxXE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EE2idE);
    LIB_FUNCTION("XhdnPX5bosc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EE4intlE);
    LIB_FUNCTION("BuxsERsopss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("nbTAoMwiO38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EEC1Em);
    LIB_FUNCTION("9S960jA8tB0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EEC1EPKcm);
    LIB_FUNCTION("TRn3cMU4mjY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("kPELiw9L-gw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EEC2Em);
    LIB_FUNCTION("RxJnJ-HoySc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EEC2EPKcm);
    LIB_FUNCTION("7e3DrnZea-Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("tcdvTUlPnL0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EED0Ev);
    LIB_FUNCTION("wT+HL7oqjYc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EED1Ev);
    LIB_FUNCTION("F7CUCpiasec", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIcLb1EED2Ev);
    LIB_FUNCTION("mhoxSElvH0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EE2idE);
    LIB_FUNCTION("D0gqPsqeZac", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EE4intlE);
    LIB_FUNCTION("0OjBJZd9VeM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("McUBYCqjLMg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EEC1Em);
    LIB_FUNCTION("jna5sqISK4s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EEC1EPKcm);
    LIB_FUNCTION("dHs7ndrQBiI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("bBGvmspg3Xs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EEC2Em);
    LIB_FUNCTION("5bQqdR3hTZw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EEC2EPKcm);
    LIB_FUNCTION("1kvQkOSaaVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("95MaQlRbfC8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EED0Ev);
    LIB_FUNCTION("ki5SQPsB4m4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EED1Ev);
    LIB_FUNCTION("6F1JfiING18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb0EED2Ev);
    LIB_FUNCTION("XUs40umcJLQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EE2idE);
    LIB_FUNCTION("8vEBRx0O1fc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EE4intlE);
    LIB_FUNCTION("HmcMLz3cPkM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("X69UlAXF-Oc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EEC1Em);
    LIB_FUNCTION("pyBabUesXN4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EEC1EPKcm);
    LIB_FUNCTION("uROsAczW6OU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("sTaUDDnGOpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EEC2Em);
    LIB_FUNCTION("GS1AvxBwVgY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EEC2EPKcm);
    LIB_FUNCTION("H0a2QXvgHOk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("fWuQSVGOivA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EED0Ev);
    LIB_FUNCTION("OM0FnA7Tldk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EED1Ev);
    LIB_FUNCTION("uVOxy7dQTFc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt10moneypunctIwLb1EED2Ev);
    LIB_FUNCTION("fn1i72X18Gs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt11logic_errorD0Ev);
    LIB_FUNCTION("i726T0BHbOU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt11logic_errorD1Ev);
    LIB_FUNCTION("wgDImKoGKCM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt11logic_errorD2Ev);
    LIB_FUNCTION("efXnxYFN5oE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt11range_errorD0Ev);
    LIB_FUNCTION("NnNaWa16OvE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt11range_errorD1Ev);
    LIB_FUNCTION("XgmUR6WSeXg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt11range_errorD2Ev);
    LIB_FUNCTION("ASUJmlcHSLo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt11regex_errorD0Ev);
    LIB_FUNCTION("gDsvnPIkLIE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt11regex_errorD1Ev);
    LIB_FUNCTION("X2wfcFYusTk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt11regex_errorD2Ev);
    LIB_FUNCTION("JyAoulEqA1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12bad_weak_ptrD0Ev);
    LIB_FUNCTION("jAO1IJKMhE4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12bad_weak_ptrD1Ev);
    LIB_FUNCTION("2R2j1QezUGM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12bad_weak_ptrD2Ev);
    LIB_FUNCTION("q89N9L8q8FU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12domain_errorD0Ev);
    LIB_FUNCTION("7P1Wm-5KgAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12domain_errorD1Ev);
    LIB_FUNCTION("AsShnG3DulM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12domain_errorD2Ev);
    LIB_FUNCTION("rNYLEsL7M0k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12future_errorD0Ev);
    LIB_FUNCTION("fuyXHeERajE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12future_errorD1Ev);
    LIB_FUNCTION("XFh0C66aEms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12future_errorD2Ev);
    LIB_FUNCTION("QS7CASjt4FU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12length_errorD0Ev);
    LIB_FUNCTION("n3y8Rn9hXJo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12length_errorD1Ev);
    LIB_FUNCTION("NjJfVHJL2Gg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12length_errorD2Ev);
    LIB_FUNCTION("TqvziWHetnw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12out_of_rangeD0Ev);
    LIB_FUNCTION("Kcb+MNSzZcc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12out_of_rangeD1Ev);
    LIB_FUNCTION("cCXMypoz4Vs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12out_of_rangeD2Ev);
    LIB_FUNCTION("+CnX+ZDO8qg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders2_1E);
    LIB_FUNCTION("GHsPYRKjheE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders2_2E);
    LIB_FUNCTION("X1C-YhubpGY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders2_3E);
    LIB_FUNCTION("fjnxuk9ucsE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders2_4E);
    LIB_FUNCTION("jxlpClEsfJQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders2_5E);
    LIB_FUNCTION("-cgB1bQG6jo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders2_6E);
    LIB_FUNCTION("Vj+KUu5khTE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders2_7E);
    LIB_FUNCTION("9f-LMAJYGCY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders2_8E);
    LIB_FUNCTION("RlB3+37KJaE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders2_9E);
    LIB_FUNCTION("b8ySy0pHgSQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_10E);
    LIB_FUNCTION("or0CNRlAEeE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_11E);
    LIB_FUNCTION("BO1r8DPhmyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_12E);
    LIB_FUNCTION("eeeT3NKKQZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_13E);
    LIB_FUNCTION("s0V1HJcZWEs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_14E);
    LIB_FUNCTION("94OiPulKcao", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_15E);
    LIB_FUNCTION("XOEdRCackI4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_16E);
    LIB_FUNCTION("pP76ElRLm78", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_17E);
    LIB_FUNCTION("WVB9rXLAUFs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_18E);
    LIB_FUNCTION("BE7U+QsixQA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_19E);
    LIB_FUNCTION("dFhgrqyzqhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12placeholders3_20E);
    LIB_FUNCTION("oly3wSwEJ2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12system_errorC2ESt10error_codePKc);
    LIB_FUNCTION("cyy-9ntjWT8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12system_errorD0Ev);
    LIB_FUNCTION("3qWXO9GTUYU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12system_errorD1Ev);
    LIB_FUNCTION("it6DDrqKGvo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt12system_errorD2Ev);
    LIB_FUNCTION("Ntg7gSs99PY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13_Num_int_base10is_boundedE);
    LIB_FUNCTION("90T0XESrYzU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13_Num_int_base10is_integerE);
    LIB_FUNCTION("WFTOZxDfpbQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13_Num_int_base14is_specializedE);
    LIB_FUNCTION("ongs2C6YZgA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13_Num_int_base5radixE);
    LIB_FUNCTION("VET8UnnaQKo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13_Num_int_base8is_exactE);
    LIB_FUNCTION("rZ5sEWyLqa4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13_Num_int_base9is_moduloE);
    LIB_FUNCTION("diSRws0Ppxg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13_Regex_traitsIcE6_NamesE);
    LIB_FUNCTION("xsRN6gUx-DE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13_Regex_traitsIwE6_NamesE);
    LIB_FUNCTION("lX9M5u0e48k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13bad_exceptionD0Ev);
    LIB_FUNCTION("t6egllDqQ2M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13bad_exceptionD1Ev);
    LIB_FUNCTION("iWNC2tkDgxw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13bad_exceptionD2Ev);
    LIB_FUNCTION("VNaqectsZNs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE4syncEv);
    LIB_FUNCTION("9biiDDejX3Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE5_LockEv);
    LIB_FUNCTION("qM0gUepGWT0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE5imbueERKSt6locale);
    LIB_FUNCTION("jfr41GGp2jA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE5uflowEv);
    LIB_FUNCTION("SYFNsz9K2rs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE6setbufEPci);
    LIB_FUNCTION("yofHspnD9us", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE7_UnlockEv);
    LIB_FUNCTION(
        "7oio2Gs1GNk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
        internal__ZNSt13basic_filebufIcSt11char_traitsIcEE7seekoffElNSt5_IosbIiE8_SeekdirENS4_9_OpenmodeE);
    LIB_FUNCTION(
        "rsS5cBMihAM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
        internal__ZNSt13basic_filebufIcSt11char_traitsIcEE7seekposESt4fposI9_MbstatetENSt5_IosbIiE9_OpenmodeE);
    LIB_FUNCTION("oYMRgkQHoJM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE8overflowEi);
    LIB_FUNCTION("JTwt9OTgk1k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE9_EndwriteEv);
    LIB_FUNCTION("jerxcj2Xnbg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE9pbackfailEi);
    LIB_FUNCTION("Nl6si1CfINw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEE9underflowEv);
    LIB_FUNCTION("MYCRRmc7cDA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEED0Ev);
    LIB_FUNCTION("Yc2gZRtDeNQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEED1Ev);
    LIB_FUNCTION("gOxGOQmSVU0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIcSt11char_traitsIcEED2Ev);
    LIB_FUNCTION("+WvmZi3216M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE4syncEv);
    LIB_FUNCTION("GYTma8zq0NU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE5_LockEv);
    LIB_FUNCTION("kmzNbhlkddA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE5imbueERKSt6locale);
    LIB_FUNCTION("VrXGNMTgNdE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE5uflowEv);
    LIB_FUNCTION("wAcnCK2HCeI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE6setbufEPwi);
    LIB_FUNCTION("ryl2DYMxlZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE7_UnlockEv);
    LIB_FUNCTION(
        "g7gjCDEedJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
        internal__ZNSt13basic_filebufIwSt11char_traitsIwEE7seekoffElNSt5_IosbIiE8_SeekdirENS4_9_OpenmodeE);
    LIB_FUNCTION(
        "10VcrHqHAlw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
        internal__ZNSt13basic_filebufIwSt11char_traitsIwEE7seekposESt4fposI9_MbstatetENSt5_IosbIiE9_OpenmodeE);
    LIB_FUNCTION("PjH5dZGfQHQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE8overflowEi);
    LIB_FUNCTION("cV6KpJiF0Ck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE9_EndwriteEv);
    LIB_FUNCTION("NeiFvKblpZM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE9pbackfailEi);
    LIB_FUNCTION("hXsvfky362s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEE9underflowEv);
    LIB_FUNCTION("JJ-mkOhdook", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEED0Ev);
    LIB_FUNCTION("XcuCO1YXaRs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEED1Ev);
    LIB_FUNCTION("aC9OWBGjvxA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13basic_filebufIwSt11char_traitsIwEED2Ev);
    LIB_FUNCTION("94dk1V7XfYw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13runtime_errorD0Ev);
    LIB_FUNCTION("uBlwRfRb-CM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13runtime_errorD1Ev);
    LIB_FUNCTION("oe9tS0VztYk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt13runtime_errorD2Ev);
    LIB_FUNCTION("3CtP20nk8fs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Error_objectsIiE14_System_objectE);
    LIB_FUNCTION("fMfCVl0JvfE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Error_objectsIiE15_Generic_objectE);
    LIB_FUNCTION("y8PXwxTZ9Hc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base10has_denormE);
    LIB_FUNCTION("G4Pw4hv6NKc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base10is_boundedE);
    LIB_FUNCTION("Zwn1Rlbirio", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base10is_integerE);
    LIB_FUNCTION("M+F+0jd4+Y0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base11round_styleE);
    LIB_FUNCTION("f06wGEmo5Pk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base12has_infinityE);
    LIB_FUNCTION("xd7O9oMO+nI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base13has_quiet_NaNE);
    LIB_FUNCTION("8hyOiMUD36c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base14is_specializedE);
    LIB_FUNCTION("F+ehGYUe36Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base15has_denorm_lossE);
    LIB_FUNCTION("0JlZYApT0UM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base15tinyness_beforeE);
    LIB_FUNCTION("ec8jeC2LMOc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base17has_signaling_NaNE);
    LIB_FUNCTION("7tACjdACOBM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base5radixE);
    LIB_FUNCTION("7gc-QliZnMc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base5trapsE);
    LIB_FUNCTION("4PL4SkJXTos", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base8is_exactE);
    LIB_FUNCTION("tsiBm2NZQfo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base9is_iec559E);
    LIB_FUNCTION("c27lOSHxPA4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base9is_moduloE);
    LIB_FUNCTION("LV2FB+f1MJE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14_Num_ldbl_base9is_signedE);
    LIB_FUNCTION("g8Jw7V6mn8k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14error_categoryD2Ev);
    LIB_FUNCTION("KQTHP-ij0yo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIaE6digitsE);
    LIB_FUNCTION("btueF8F0fQE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIaE8digits10E);
    LIB_FUNCTION("iBrS+wbpuT0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIaE9is_signedE);
    LIB_FUNCTION("x1vTXM-GLCE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIbE6digitsE);
    LIB_FUNCTION("lnOqjnXNTwQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIbE8digits10E);
    LIB_FUNCTION("qOkciFIHghY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIbE9is_moduloE);
    LIB_FUNCTION("0mi6NtGz04Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIbE9is_signedE);
    LIB_FUNCTION("nlxVZWbqzsU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIcE6digitsE);
    LIB_FUNCTION("VVK0w0uxDLE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIcE8digits10E);
    LIB_FUNCTION("M+AMxjxwWlA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIcE9is_signedE);
    LIB_FUNCTION("hqVKCQr0vU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIdE12max_digits10E);
    LIB_FUNCTION("fjI2ddUGZZs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIdE12max_exponentE);
    LIB_FUNCTION("AwdlDnuQ6c0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIdE12min_exponentE);
    LIB_FUNCTION("VmOyIzWFNKs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIdE14max_exponent10E);
    LIB_FUNCTION("odyn6PGg5LY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIdE14min_exponent10E);
    LIB_FUNCTION("xQtNieUQLVg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIdE6digitsE);
    LIB_FUNCTION("EXW20cJ3oNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIdE8digits10E);
    LIB_FUNCTION("Zhtj6WalERg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIDiE6digitsE);
    LIB_FUNCTION("r1k-y+1yDcQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIDiE8digits10E);
    LIB_FUNCTION("TEMThaOLu+c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIDiE9is_signedE);
    LIB_FUNCTION("EL+4ceAj+UU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIDsE6digitsE);
    LIB_FUNCTION("vEdl5Er9THU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIDsE8digits10E);
    LIB_FUNCTION("ZaOkYNQyQ6g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIDsE9is_signedE);
    LIB_FUNCTION("u16WKNmQUNg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIeE12max_digits10E);
    LIB_FUNCTION("bzmM0dI80jM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIeE12max_exponentE);
    LIB_FUNCTION("ERYMucecNws", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIeE12min_exponentE);
    LIB_FUNCTION("tUo2aRfWs5I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIeE14max_exponent10E);
    LIB_FUNCTION("3+5qZWL6APo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIeE14min_exponent10E);
    LIB_FUNCTION("NLHWcHpvMss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIeE6digitsE);
    LIB_FUNCTION("JYZigPvvB6c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIeE8digits10E);
    LIB_FUNCTION("MFqdrWyu9Ls", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIfE12max_digits10E);
    LIB_FUNCTION("L29QQz-6+X8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIfE12max_exponentE);
    LIB_FUNCTION("SPlcBQ4pIZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIfE12min_exponentE);
    LIB_FUNCTION("R8xUpEJwAA8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIfE14max_exponent10E);
    LIB_FUNCTION("n+NFkoa0VD0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIfE14min_exponent10E);
    LIB_FUNCTION("W6qgdoww-3k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIfE6digitsE);
    LIB_FUNCTION("J7d2Fq6Mb0k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIfE8digits10E);
    LIB_FUNCTION("T1YYqsPgrn0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIhE6digitsE);
    LIB_FUNCTION("uTiJLq4hayE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIhE8digits10E);
    LIB_FUNCTION("o0WexTj82pU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIhE9is_signedE);
    LIB_FUNCTION("ZvahxWPLKm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIiE6digitsE);
    LIB_FUNCTION("aQjlTguvFMw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIiE8digits10E);
    LIB_FUNCTION("GST3YemNZD8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIiE9is_signedE);
    LIB_FUNCTION("-jpk31lZR6E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIjE6digitsE);
    LIB_FUNCTION("csNIBfF6cyI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIjE8digits10E);
    LIB_FUNCTION("P9XP5U7AfXs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIjE9is_signedE);
    LIB_FUNCTION("31lJOpD3GGk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIlE6digitsE);
    LIB_FUNCTION("4MdGVqrsl7s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIlE8digits10E);
    LIB_FUNCTION("4llda2Y+Q+4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIlE9is_signedE);
    LIB_FUNCTION("7AaHj1O8-gI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsImE6digitsE);
    LIB_FUNCTION("h9RyP3R30HI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsImE8digits10E);
    LIB_FUNCTION("FXrK1DiAosQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsImE9is_signedE);
    LIB_FUNCTION("QO6Q+6WPgy0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIsE6digitsE);
    LIB_FUNCTION("kW5K7R4rXy8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIsE8digits10E);
    LIB_FUNCTION("L0nMzhz-axs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIsE9is_signedE);
    LIB_FUNCTION("4r9P8foa6QQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsItE6digitsE);
    LIB_FUNCTION("OQorbmM+NbA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsItE8digits10E);
    LIB_FUNCTION("vyqQpWI+O48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsItE9is_signedE);
    LIB_FUNCTION("Tlfgn9TIWkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIwE6digitsE);
    LIB_FUNCTION("mdcx6KcRIkE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIwE8digits10E);
    LIB_FUNCTION("YVacrIa4L0c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIwE9is_signedE);
    LIB_FUNCTION("LN2bC6QtGQE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIxE6digitsE);
    LIB_FUNCTION("OwcpepSk5lg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIxE8digits10E);
    LIB_FUNCTION("mmrSzkWDrgA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIxE9is_signedE);
    LIB_FUNCTION("v7XHt2HwUVI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIyE6digitsE);
    LIB_FUNCTION("Eubj+4g8dWA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIyE8digits10E);
    LIB_FUNCTION("F2uQDOc7fMo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14numeric_limitsIyE9is_signedE);
    LIB_FUNCTION("y1dYQsc67ys", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14overflow_errorD0Ev);
    LIB_FUNCTION("XilOsTdCZuM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14overflow_errorD1Ev);
    LIB_FUNCTION("OypvNf3Uq3c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt14overflow_errorD2Ev);
    LIB_FUNCTION("q-WOrJNOlhI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base10has_denormE);
    LIB_FUNCTION("XbD-A2MEsS4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base10is_boundedE);
    LIB_FUNCTION("mxv24Oqmp0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base10is_integerE);
    LIB_FUNCTION("9AcX4Qk47+o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base11round_styleE);
    LIB_FUNCTION("MIKN--3fORE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base12has_infinityE);
    LIB_FUNCTION("nxdioQgDF2E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base13has_quiet_NaNE);
    LIB_FUNCTION("N03wZLr2RrE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base14is_specializedE);
    LIB_FUNCTION("rhJg5tjs83Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base15has_denorm_lossE);
    LIB_FUNCTION("EzuahjKzeGQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base15tinyness_beforeE);
    LIB_FUNCTION("uMMG8cuJNr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base17has_signaling_NaNE);
    LIB_FUNCTION("1KngsM7trps", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base5radixE);
    LIB_FUNCTION("mMPu4-jx9oI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base5trapsE);
    LIB_FUNCTION("J5QA0ZeLmhs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base8is_exactE);
    LIB_FUNCTION("JwPU+6+T20M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base9is_iec559E);
    LIB_FUNCTION("HU3yzCPz3GQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base9is_moduloE);
    LIB_FUNCTION("S7kkgAPGxLQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15_Num_float_base9is_signedE);
    LIB_FUNCTION("iHILAmwYRGY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15underflow_errorD0Ev);
    LIB_FUNCTION("ywv2X-q-9is", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15underflow_errorD1Ev);
    LIB_FUNCTION("xiqd+QkuYXc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt15underflow_errorD2Ev);
    LIB_FUNCTION("1GhiIeIpkms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt16invalid_argumentD0Ev);
    LIB_FUNCTION("oQDS9nX05Qg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt16invalid_argumentD1Ev);
    LIB_FUNCTION("ddr7Ie4u5Nw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt16invalid_argumentD2Ev);
    LIB_FUNCTION("za50kXyi3SA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt16nested_exceptionD0Ev);
    LIB_FUNCTION("+qKS53qzWdA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt16nested_exceptionD1Ev);
    LIB_FUNCTION("8R00hgzXQDY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt16nested_exceptionD2Ev);
    LIB_FUNCTION("q9rMtHuXvZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt17bad_function_callD0Ev);
    LIB_FUNCTION("YEDrb1pSx2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt17bad_function_callD1Ev);
    LIB_FUNCTION("NqMgmxSA1rc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt17bad_function_callD2Ev);
    LIB_FUNCTION("8DNJW5tX-A8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt18bad_variant_accessD0Ev);
    LIB_FUNCTION("U3b5A2LEiTc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt18bad_variant_accessD1Ev);
    LIB_FUNCTION("QUeUgxy7PTA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt20_Future_error_objectIiE14_Future_objectE);
    LIB_FUNCTION("-UKRka-33sM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt20bad_array_new_lengthD0Ev);
    LIB_FUNCTION("XO3N4SBvCy0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt20bad_array_new_lengthD1Ev);
    LIB_FUNCTION("15lB7flw-9w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt20bad_array_new_lengthD2Ev);
    LIB_FUNCTION("WDKzMM-uuLE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt22_Future_error_categoryD0Ev);
    LIB_FUNCTION("xsXQD5ybREw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt22_Future_error_categoryD1Ev);
    LIB_FUNCTION("Dc4ZMWmPMl8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt22_System_error_categoryD0Ev);
    LIB_FUNCTION("hVQgfGhJz3U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt22_System_error_categoryD1Ev);
    LIB_FUNCTION("YBrp9BlADaA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt23_Generic_error_categoryD0Ev);
    LIB_FUNCTION("MAalgQhejPc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt23_Generic_error_categoryD1Ev);
    LIB_FUNCTION("9G32u5RRYxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt3pmr19new_delete_resourceEv);
    LIB_FUNCTION("d2u38zs4Pe8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt3pmr20get_default_resourceEv);
    LIB_FUNCTION("eWMGI7B7Lyc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt3pmr20null_memory_resourceEv);
    LIB_FUNCTION("TKYsv0jdvRw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt3pmr20set_default_resourceEPNS_15memory_resourceE);
    LIB_FUNCTION("H7-7Z3ixv-w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt4_Pad7_LaunchEPKcPP12pthread_attrPP7pthread);
    LIB_FUNCTION("PBbZjsL6nfc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt4_Pad7_LaunchEPKcPP7pthread);
    LIB_FUNCTION("fLBZMOQh-3Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt4_Pad7_LaunchEPP12pthread_attrPP7pthread);
    LIB_FUNCTION("xZqiZvmcp9k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt4_Pad7_LaunchEPP7pthread);
    LIB_FUNCTION("a-z7wxuYO2E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt4_Pad8_ReleaseEv);
    LIB_FUNCTION("uhnb6dnXOnc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt4_PadC2EPKc);
    LIB_FUNCTION("dGYo9mE8K2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt4_PadC2Ev);
    LIB_FUNCTION("XyJPhPqpzMw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt4_PadD1Ev);
    LIB_FUNCTION("gjLRZgfb3i0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt4_PadD2Ev);
    LIB_FUNCTION("rX58aCQCMS4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt5ctypeIcE10table_sizeE);
    LIB_FUNCTION("Cv+zC4EjGMA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt5ctypeIcE2idE);
    LIB_FUNCTION("p8-44cVeO84", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt5ctypeIcED0Ev);
    LIB_FUNCTION("tPsGA6EzNKA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt5ctypeIcED1Ev);
    LIB_FUNCTION("VmqsS6auJzo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt5ctypeIwE2idE);
    LIB_FUNCTION("zOPA5qnbW2U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt5ctypeIwED0Ev);
    LIB_FUNCTION("P0383AW3Y9A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt5ctypeIwED1Ev);
    LIB_FUNCTION("U54NBtdj6UY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_Mutex5_LockEv);
    LIB_FUNCTION("7OCTkL2oWyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_Mutex7_UnlockEv);
    LIB_FUNCTION("2KNnG2Z9zJA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_MutexC1Ev);
    LIB_FUNCTION("log6zy2C9iQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_MutexC2Ev);
    LIB_FUNCTION("djHbPE+TFIo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_MutexD1Ev);
    LIB_FUNCTION("j7e7EQBD6ZA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_MutexD2Ev);
    LIB_FUNCTION("0WY1SH7eoIs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_Winit9_Init_cntE);
    LIB_FUNCTION("-Bl9-SZ2noc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_WinitC1Ev);
    LIB_FUNCTION("57mMrw0l-40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_WinitC2Ev);
    LIB_FUNCTION("Uw3OTZFPNt4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_WinitD1Ev);
    LIB_FUNCTION("2yOarodWACE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6_WinitD2Ev);
    LIB_FUNCTION("z83caOn94fM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6chrono12steady_clock12is_monotonicE);
    LIB_FUNCTION("vHy+a4gLBfs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6chrono12steady_clock9is_steadyE);
    LIB_FUNCTION("jCX3CPIVB8I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6chrono12system_clock12is_monotonicE);
    LIB_FUNCTION("88EyUEoBX-E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6chrono12system_clock9is_steadyE);
    LIB_FUNCTION("hEQ2Yi4PJXA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale16_GetgloballocaleEv);
    LIB_FUNCTION("1TaQLyPDJEY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale16_SetgloballocaleEPv);
    LIB_FUNCTION("H4fcpQOpc08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale2id7_Id_cntE);
    LIB_FUNCTION("9rMML086SEE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale5_InitEv);
    LIB_FUNCTION("K-5mtupQZ4g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale5emptyEv);
    LIB_FUNCTION("AgxEl+HeWRQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale5facet7_DecrefEv);
    LIB_FUNCTION("-EgSegeAKl4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale5facet7_IncrefEv);
    LIB_FUNCTION("QW2jL1J5rwY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale5facet9_RegisterEv);
    LIB_FUNCTION("ptwhA0BQVeE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale6globalERKS_);
    LIB_FUNCTION("uuga3RipCKQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_Locimp7_AddfacEPNS_5facetEm);
    LIB_FUNCTION("9FF+T5Xks9E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_Locimp8_ClocptrE);
    LIB_FUNCTION("5r801ZWiJJI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_Locimp8_MakelocERKSt8_LocinfoiPS0_PKS_);
    LIB_FUNCTION("BcbHFSrcg3Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_Locimp9_MakewlocERKSt8_LocinfoiPS0_PKS_);
    LIB_FUNCTION("fkFGlPdquqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_Locimp9_MakexlocERKSt8_LocinfoiPS0_PKS_);
    LIB_FUNCTION("6b3KIjPD33k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_LocimpC1Eb);
    LIB_FUNCTION("WViwxtEKxHk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_LocimpC1ERKS0_);
    LIB_FUNCTION("zrmR88ClfOs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_LocimpC2Eb);
    LIB_FUNCTION("dsJKehuajH4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_LocimpC2ERKS0_);
    LIB_FUNCTION("bleKr8lOLr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_LocimpD0Ev);
    LIB_FUNCTION("aD-iqbVlHmQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_LocimpD1Ev);
    LIB_FUNCTION("So6gSmJMYDs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7_LocimpD2Ev);
    LIB_FUNCTION("Uq5K8tl8I9U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6locale7classicEv);
    LIB_FUNCTION("pMWnITHysPc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6localeD1Ev);
    LIB_FUNCTION("CHrhwd8QSBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt6thread20hardware_concurrencyEv);
    LIB_FUNCTION("m7qAgircaZY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIcE5_InitERKSt8_Locinfob);
    LIB_FUNCTION("zWSNYg14Uag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIcEC2Emb);
    LIB_FUNCTION("0il9qdo6fhs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIcEC2EPKcmbb);
    LIB_FUNCTION("Lzj4ws7DlhQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIcED0Ev);
    LIB_FUNCTION("0AeC+qCELEA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIcED1Ev);
    LIB_FUNCTION("iCoD0EOIbTM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIwE5_InitERKSt8_Locinfob);
    LIB_FUNCTION("Pr1yLzUe230", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIwEC2Emb);
    LIB_FUNCTION("TDhjx3nyaoU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIwEC2EPKcmbb);
    LIB_FUNCTION("8UeuxGKjQr8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIwED0Ev);
    LIB_FUNCTION("0TADyPWrobI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7_MpunctIwED1Ev);
    LIB_FUNCTION("eVFYZnYNDo0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetE2idE);
    LIB_FUNCTION("iZCHNahj++4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("zyhiiLKndO8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetE7_GetcatEPPKNSt6locale5facetEPKS2_);
    LIB_FUNCTION("XhwSbwsBdx0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetEC1Em);
    LIB_FUNCTION("3YCLxZqgIdo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetEC1ERKSt8_Locinfom);
    LIB_FUNCTION("e5Hwcntvd8c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetEC2Em);
    LIB_FUNCTION("4qHwSTPt-t8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetEC2ERKSt8_Locinfom);
    LIB_FUNCTION("2TYdayAO39E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetED0Ev);
    LIB_FUNCTION("djNkrJKTb6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetED1Ev);
    LIB_FUNCTION("to7GggwECZU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIcc9_MbstatetED2Ev);
    LIB_FUNCTION("u2MAta5SS84", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIDic9_MbstatetE2idE);
    LIB_FUNCTION("vwMx2NhWdLw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIDic9_MbstatetED0Ev);
    LIB_FUNCTION("TuhGCIxgLvA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIDic9_MbstatetED1Ev);
    LIB_FUNCTION("xM5re58mxj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIDsc9_MbstatetE2idE);
    LIB_FUNCTION("zYHryd8vd0w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIDsc9_MbstatetED0Ev);
    LIB_FUNCTION("Oeo9tUbzW7s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIDsc9_MbstatetED1Ev);
    LIB_FUNCTION("FjZCPmK0SbA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIwc9_MbstatetE2idE);
    LIB_FUNCTION("9BI3oYkCTCU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIwc9_MbstatetED0Ev);
    LIB_FUNCTION("0fkFA3za2N8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7codecvtIwc9_MbstatetED1Ev);
    LIB_FUNCTION("7brRfHVVAlI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcE2idE);
    LIB_FUNCTION("CKlZ-H-D1CE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("BSVJqITGCyI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("Oo1r8jKGZQ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcEC1Em);
    LIB_FUNCTION("splBMMcF3yk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcEC1EPKcm);
    LIB_FUNCTION("raLgIUi3xmk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcEC1ERKSt8_Locinfom);
    LIB_FUNCTION("tkqNipin1EI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcEC2Em);
    LIB_FUNCTION("VClCrMDyydE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcEC2EPKcm);
    LIB_FUNCTION("L71JAnoQees", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcEC2ERKSt8_Locinfom);
    LIB_FUNCTION("Lt4407UMs2o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcED0Ev);
    LIB_FUNCTION("8pXCeme0FC4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcED1Ev);
    LIB_FUNCTION("dP5zwQ2Yc8g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIcED2Ev);
    LIB_FUNCTION("irGo1yaJ-vM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwE2idE);
    LIB_FUNCTION("LxKs-IGDsFU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("2wz4rthdiy8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("d-MOtyu8GAk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwEC1Em);
    LIB_FUNCTION("fjHAU8OSaW8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwEC1EPKcm);
    LIB_FUNCTION("wggIIjWSt-E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwEC1ERKSt8_Locinfom);
    LIB_FUNCTION("HQbgeUdQyyw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwEC2Em);
    LIB_FUNCTION("PSAw7g1DD24", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwEC2EPKcm);
    LIB_FUNCTION("2PoQu-K2qXk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwEC2ERKSt8_Locinfom);
    LIB_FUNCTION("ig4VDIRc21Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwED0Ev);
    LIB_FUNCTION("ZO3a6HfALTQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwED1Ev);
    LIB_FUNCTION("84wIPnwBGiU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt7collateIwED2Ev);
    LIB_FUNCTION("WkAsdy5CUAo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8_Locinfo8_AddcatsEiPKc);
    LIB_FUNCTION("L1Ze94yof2I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8_LocinfoC1EiPKc);
    LIB_FUNCTION("hqi8yMOCmG0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8_LocinfoC1EPKc);
    LIB_FUNCTION("2aSk2ruCP0E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8_LocinfoC1ERKSs);
    LIB_FUNCTION("i180MNC9p4c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8_LocinfoC2EiPKc);
    LIB_FUNCTION("pN02FS5SPgg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8_LocinfoC2EPKc);
    LIB_FUNCTION("ReK9U6EUWuU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8_LocinfoC2ERKSs);
    LIB_FUNCTION("p6LrHjIQMdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8_LocinfoD1Ev);
    LIB_FUNCTION("YXVCU6PdgZk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8_LocinfoD2Ev);
    LIB_FUNCTION("2MK5Lr9pgQc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8bad_castD0Ev);
    LIB_FUNCTION("47RvLSo2HN8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8bad_castD1Ev);
    LIB_FUNCTION("rF07weLXJu8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8bad_castD2Ev);
    LIB_FUNCTION("QZb07KKwTU0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base4Init9_Init_cntE);
    LIB_FUNCTION("sqWytnhYdEg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base4InitC1Ev);
    LIB_FUNCTION("bTQcNwRc8hE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base4InitC2Ev);
    LIB_FUNCTION("kxXCvcat1cM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base4InitD1Ev);
    LIB_FUNCTION("bxLH5WHgMBY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base4InitD2Ev);
    LIB_FUNCTION("8tL6yJaX1Ro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base5_SyncE);
    LIB_FUNCTION("QXJCcrXoqpU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base5clearENSt5_IosbIiE8_IostateEb);
    LIB_FUNCTION("4EkPKYzOjPc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base6_IndexE);
    LIB_FUNCTION("LTov9gMEqCU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base7_AddstdEPS_);
    LIB_FUNCTION("x7vtyar1sEY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base7failureD0Ev);
    LIB_FUNCTION("N2f485TmJms", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base7failureD1Ev);
    LIB_FUNCTION("fjG5plxblj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_base7failureD2Ev);
    LIB_FUNCTION("I5jcbATyIWo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_baseD0Ev);
    LIB_FUNCTION("X9D8WWSG3As", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_baseD1Ev);
    LIB_FUNCTION("P8F2oavZXtY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8ios_baseD2Ev);
    LIB_FUNCTION("lA+PfiZ-S5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcE2idE);
    LIB_FUNCTION("eLB2+1+mVvg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("96Ev+CE1luE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("6gCBQs1mIi4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcEC1Em);
    LIB_FUNCTION("W0w8TGzAu0Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcEC1EPKcm);
    LIB_FUNCTION("SD403oMc1pQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcEC1ERKSt8_Locinfom);
    LIB_FUNCTION("6DBUo0dty1k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcEC2Em);
    LIB_FUNCTION("qF3mHeMAHVk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcEC2EPKcm);
    LIB_FUNCTION("969Euioo12Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcEC2ERKSt8_Locinfom);
    LIB_FUNCTION("jy9urODH0Wo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcED0Ev);
    LIB_FUNCTION("34mi8lteNTs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcED1Ev);
    LIB_FUNCTION("yDdbQr1oLOc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIcED2Ev);
    LIB_FUNCTION("n1Y6pGR-8AU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwE2idE);
    LIB_FUNCTION("Zz-RfDtowlo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwE5_InitERKSt8_Locinfo);
    LIB_FUNCTION("XghI4vmw8mU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("n4+3hznhkU4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwEC1Em);
    LIB_FUNCTION("4Srtnk+NpC4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwEC1EPKcm);
    LIB_FUNCTION("RrTMGyPhYU4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwEC1ERKSt8_Locinfom);
    LIB_FUNCTION("x8PFBjJhH7E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwEC2Em);
    LIB_FUNCTION("DlDsyX+XsoA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwEC2EPKcm);
    LIB_FUNCTION("DDQjbwNC31E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwEC2ERKSt8_Locinfom);
    LIB_FUNCTION("gMwkpZNI9Us", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwED0Ev);
    LIB_FUNCTION("6sAaleB7Zgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwED1Ev);
    LIB_FUNCTION("I-e7Dxo087A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8messagesIwED2Ev);
    LIB_FUNCTION("9iXtwvGVFRI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcE2idE);
    LIB_FUNCTION("1LvbNeZZJ-o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcE5_InitERKSt8_Locinfob);
    LIB_FUNCTION("fFnht9SPed8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcE5_TidyEv);
    LIB_FUNCTION("zCB24JBovnQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("TEtyeXjcZ0w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcEC1Em);
    LIB_FUNCTION("WK24j1F3rCU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcEC1EPKcmb);
    LIB_FUNCTION("CDm+TUClE7E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("1eVdDzPtzD4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcEC2Em);
    LIB_FUNCTION("yIn4l8OO1zA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcEC2EPKcmb);
    LIB_FUNCTION("Cb1hI+w9nyU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("Lf6h5krl2fA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcED0Ev);
    LIB_FUNCTION("qEob3o53s2M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcED1Ev);
    LIB_FUNCTION("xFva4yxsVW8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIcED2Ev);
    LIB_FUNCTION("XZNi3XtbWQ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwE2idE);
    LIB_FUNCTION("uiRALKOdAZk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwE5_InitERKSt8_Locinfob);
    LIB_FUNCTION("2YCDWkuFEy8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwE5_TidyEv);
    LIB_FUNCTION("SdXFaufpLIs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwE7_GetcatEPPKNSt6locale5facetEPKS1_);
    LIB_FUNCTION("XOgjMgZ3fjo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwEC1Em);
    LIB_FUNCTION("H+T2VJ91dds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwEC1EPKcmb);
    LIB_FUNCTION("s1EM2NdPf0Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwEC1ERKSt8_Locinfomb);
    LIB_FUNCTION("ElKI+ReiehU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwEC2Em);
    LIB_FUNCTION("m4kEqv7eGVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwEC2EPKcmb);
    LIB_FUNCTION("MQJQCxbLfM0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwEC2ERKSt8_Locinfomb);
    LIB_FUNCTION("VHBnRBxBg5E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwED0Ev);
    LIB_FUNCTION("lzK3uL1rWJY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwED1Ev);
    LIB_FUNCTION("XDm4jTtoEbo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt8numpunctIwED2Ev);
    LIB_FUNCTION("cDHRgSXYdqA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base10has_denormE);
    LIB_FUNCTION("v9HHsaa42qE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base10is_boundedE);
    LIB_FUNCTION("EgSIYe3IYso", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base10is_integerE);
    LIB_FUNCTION("XXiGcYa5wtg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base11round_styleE);
    LIB_FUNCTION("98w+P+GuFMU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base12has_infinityE);
    LIB_FUNCTION("qeA5qUg9xBk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base12max_digits10E);
    LIB_FUNCTION("E4gWXl6V2J0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base12max_exponentE);
    LIB_FUNCTION("KqdclsYd24w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base12min_exponentE);
    LIB_FUNCTION("gF5aGNmzWSg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base13has_quiet_NaNE);
    LIB_FUNCTION("RCWKbkEaDAU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base14is_specializedE);
    LIB_FUNCTION("Dl4hxL59YF4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base14max_exponent10E);
    LIB_FUNCTION("zBHGQsN5Yfw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base14min_exponent10E);
    LIB_FUNCTION("96Bg8h09w+o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base15has_denorm_lossE);
    LIB_FUNCTION("U0FdtOUjUPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base15tinyness_beforeE);
    LIB_FUNCTION("fSdpGoYfYs8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base17has_signaling_NaNE);
    LIB_FUNCTION("Xb9FhMysEHo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base5radixE);
    LIB_FUNCTION("suaBxzlL0p0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base5trapsE);
    LIB_FUNCTION("ejBz8a8TCWU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base6digitsE);
    LIB_FUNCTION("M-KRaPj9tQQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base8digits10E);
    LIB_FUNCTION("bUQLE6uEu10", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base8is_exactE);
    LIB_FUNCTION("0ZdjsAWtbG8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base9is_iec559E);
    LIB_FUNCTION("qO4MLGs1o58", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base9is_moduloE);
    LIB_FUNCTION("5DzttCF356U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9_Num_base9is_signedE);
    LIB_FUNCTION("qb6A7pSgAeY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9bad_allocD0Ev);
    LIB_FUNCTION("khbdMADH4cQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9bad_allocD1Ev);
    LIB_FUNCTION("WiH8rbVv5s4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9bad_allocD2Ev);
    LIB_FUNCTION("Bin7e2UR+a0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9exception18_Set_raise_handlerEPFvRKS_E);
    LIB_FUNCTION("+Nc8JGdVLQs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9exceptionD0Ev);
    LIB_FUNCTION("BgZcGDh7o9g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9exceptionD1Ev);
    LIB_FUNCTION("MOBxtefPZUg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9exceptionD2Ev);
    LIB_FUNCTION("N5nZ3wQw+Vc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9type_infoD0Ev);
    LIB_FUNCTION("LLssoYjMOow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9type_infoD1Ev);
    LIB_FUNCTION("zb3436295XU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZNSt9type_infoD2Ev);
    LIB_FUNCTION("ryUxD-60bKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZnwmRKSt9nothrow_t);
    LIB_FUNCTION("3yxLpdKD0RA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZnwmSt11align_val_t);
    LIB_FUNCTION("iQXBbJbfT5k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZnwmSt11align_val_tRKSt9nothrow_t);
    LIB_FUNCTION("VrWUXy1gqn8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt10_Rng_abortPKc);
    LIB_FUNCTION("Zmeuhg40yNI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt10adopt_lock);
    LIB_FUNCTION("mhR3IufA7fE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt10defer_lock);
    LIB_FUNCTION("lKfKm6INOpQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt10unexpectedv);
    LIB_FUNCTION("eT2UsmTewbU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt11_Xbad_allocv);
    LIB_FUNCTION("L7Vnk06zC2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt11setiosflagsNSt5_IosbIiE9_FmtflagsE);
    LIB_FUNCTION("kFYQ4d6jVls", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt11try_to_lock);
    LIB_FUNCTION("1h8hFQghR7w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt12setprecisioni);
    LIB_FUNCTION("ybn35k-I+B0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt13_Cl_charnames);
    LIB_FUNCTION("DiGVep5yB5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt13_Execute_onceRSt9once_flagPFiPvS1_PS1_ES1_);
    LIB_FUNCTION("PDX01ziUz+4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt13_Syserror_mapi);
    LIB_FUNCTION("UWyL6KoR96U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt13_Xregex_errorNSt15regex_constants10error_typeE);
    LIB_FUNCTION("Zb+hMspRR-o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt13get_terminatev);
    LIB_FUNCTION("qMXslRdZVj4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt13resetiosflagsNSt5_IosbIiE9_FmtflagsE);
    LIB_FUNCTION("NG1phipELJE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt13set_terminatePFvvE);
    LIB_FUNCTION("u2tMGOLaqnE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14_Atomic_assertPKcS0_);
    LIB_FUNCTION("T+zVxpVaaTo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14_Cl_wcharnames);
    LIB_FUNCTION("Zn44KZgJtWY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14_Debug_messagePKcS0_j);
    LIB_FUNCTION("u0yN6tzBors", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14_Raise_handler);
    LIB_FUNCTION("Nmtr628eA3A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14_Random_devicev);
    LIB_FUNCTION("bRujIheWlB0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14_Throw_C_errori);
    LIB_FUNCTION("tQIo+GIPklo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14_Xlength_errorPKc);
    LIB_FUNCTION("ozMAr28BwSY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14_Xout_of_rangePKc);
    LIB_FUNCTION("zPWCqkg7V+o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14get_unexpectedv);
    LIB_FUNCTION("Eva2i4D5J6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt14set_unexpectedPFvvE);
    LIB_FUNCTION("zugltxeIXM0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt15_sceLibcLocinfoPKc);
    LIB_FUNCTION("y7aMFEkj4PE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt15_Xruntime_errorPKc);
    LIB_FUNCTION("vI85k3GQcz8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt15future_categoryv);
    LIB_FUNCTION("CkY0AVa-frg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt15get_new_handlerv);
    LIB_FUNCTION("RqeErO3cFHU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt15set_new_handlerPFvvE);
    LIB_FUNCTION("aotaAaQK6yc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt15system_categoryv);
    LIB_FUNCTION("W0j6vCxh9Pc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt16_Throw_Cpp_errori);
    LIB_FUNCTION("saSUnPWLq9E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt16_Xoverflow_errorPKc);
    LIB_FUNCTION("YxwfcCH5Q0I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt16generic_categoryv);
    LIB_FUNCTION("0r8rbw2SFqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt17_Future_error_mapi);
    LIB_FUNCTION("VoUwme28y4w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt18_String_cpp_unused);
    LIB_FUNCTION("NU-T4QowTNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt18_Xinvalid_argumentPKc);
    LIB_FUNCTION("Q1BL70XVV0o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt18uncaught_exceptionv);
    LIB_FUNCTION("PjwbqtUehPU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt19_Throw_future_errorRKSt10error_code);
    LIB_FUNCTION("MELi-cKqWq0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt19_Xbad_function_callv);
    LIB_FUNCTION("Qoo175Ig+-k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt21_sceLibcClassicLocale);
    LIB_FUNCTION("cPNeOAYgB0A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt22_Get_future_error_whati);
    LIB_FUNCTION("mDIHE-aaRaI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt22_Random_device_entropyv);
    LIB_FUNCTION("DNbsDRZ-ntI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt25_Rethrow_future_exceptionSt13exception_ptr);
    LIB_FUNCTION("2WVBaSdGIds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt3cin);
    LIB_FUNCTION("wiR+rIcbnlc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt4_Fpz);
    LIB_FUNCTION("TVfbf1sXt0A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt4cerr);
    LIB_FUNCTION("jSquWN7i7lc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt4clog);
    LIB_FUNCTION("5PfqUBaQf4g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt4cout);
    LIB_FUNCTION("vU9svJtEnWc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt4setwi);
    LIB_FUNCTION("2bASh0rEeXI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt4wcin);
    LIB_FUNCTION("CvJ3HUPlMIY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt5wcerr);
    LIB_FUNCTION("awr1A2VAVZQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt5wclog);
    LIB_FUNCTION("d-YRIvO0jXI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt5wcout);
    LIB_FUNCTION("pDFe-IgbTPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt6_ThrowRKSt9exception);
    LIB_FUNCTION("kr5ph+wVAtU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt6ignore);
    LIB_FUNCTION("FQ9NFbBHb5Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt7_BADOFF);
    LIB_FUNCTION("vYWK2Pz8vGE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt7_FiopenPKcNSt5_IosbIiE9_OpenmodeEi);
    LIB_FUNCTION("aqyjhIx7jaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt7_FiopenPKwNSt5_IosbIiE9_OpenmodeEi);
    LIB_FUNCTION("QfPuSqhub7o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt7_MP_AddPyy);
    LIB_FUNCTION("lrQSsTRMMr4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt7_MP_GetPy);
    LIB_FUNCTION("Gav1Xt1Ce+c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt7_MP_MulPyyy);
    LIB_FUNCTION("Ozk+Z6QnlTY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt7_MP_RemPyy);
    LIB_FUNCTION("NLwJ3q+64bY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt7nothrow);
    LIB_FUNCTION("4rrOHCHAV1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt7setbasei);
    LIB_FUNCTION("yYk819F9TyU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt8_XLgammad);
    LIB_FUNCTION("bl0DPP6kFBk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt8_XLgammae);
    LIB_FUNCTION("DWMcG8yogkY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt8_XLgammaf);
    LIB_FUNCTION("X1DNtCe22Ks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt9_LStrcollIcEiPKT_S2_S2_S2_PKSt8_Collvec);
    LIB_FUNCTION("m6uU37-b27s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt9_LStrcollIwEiPKT_S2_S2_S2_PKSt8_Collvec);
    LIB_FUNCTION("V62E2Q8bJVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt9_LStrxfrmIcEmPT_S1_PKS0_S3_PKSt8_Collvec);
    LIB_FUNCTION("BloPUt1HCH0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt9_LStrxfrmIwEmPT_S1_PKS0_S3_PKSt8_Collvec);
    LIB_FUNCTION("qYhnoevd9bI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZSt9terminatev);
    LIB_FUNCTION("XO9ihAZCBcY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIa);
    LIB_FUNCTION("nEuTkSQAQFw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIb);
    LIB_FUNCTION("smeljzleGRQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIc);
    LIB_FUNCTION("iZrCfFRsE3Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTId);
    LIB_FUNCTION("ltRLAWAeSaM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIDh);
    LIB_FUNCTION("7TW4UgJjwJ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIDi);
    LIB_FUNCTION("SK0Syya+scs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIDn);
    LIB_FUNCTION("rkWOabkkpVo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIDs);
    LIB_FUNCTION("NlgA2fMtxl4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIe);
    LIB_FUNCTION("c5-Jw-LTekM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIf);
    LIB_FUNCTION("g-fUPD4HznU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIh);
    LIB_FUNCTION("St4apgcBNfo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIi);
    LIB_FUNCTION("MpiTv3MErEQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIj);
    LIB_FUNCTION("b5JSEuAHuDo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIl);
    LIB_FUNCTION("DoGS21ugIfI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIm);
    LIB_FUNCTION("2EEDQ6uHY2c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIn);
    LIB_FUNCTION("h1Eewgzowes", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv116__enum_type_infoE);
    LIB_FUNCTION("eD+mC6biMFI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv117__array_type_infoE);
    LIB_FUNCTION("EeOtHxoUkvM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv117__class_type_infoE);
    LIB_FUNCTION("dSBshTZ8JcA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv117__pbase_type_infoE);
    LIB_FUNCTION("YglrcQaNfds", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv119__pointer_type_infoE);
    LIB_FUNCTION("DZhZwYkJDCE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv120__function_type_infoE);
    LIB_FUNCTION("N2VV+vnEYlw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv120__si_class_type_infoE);
    LIB_FUNCTION("gjLRFhKCMNE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv121__vmi_class_type_infoE);
    LIB_FUNCTION("dHw0YAjyIV4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv123__fundamental_type_infoE);
    LIB_FUNCTION("7tTpzMt-PzY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN10__cxxabiv129__pointer_to_member_type_infoE);
    LIB_FUNCTION("yZmHOKICuxg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN6Dinkum7threads10lock_errorE);
    LIB_FUNCTION("qcaIknDQLwE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIN6Dinkum7threads21thread_resource_errorE);
    LIB_FUNCTION("sJUU2ZW-yxU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTINSt6locale5facetE);
    LIB_FUNCTION("8Wc+t3BCF-k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTINSt6locale7_LocimpE);
    LIB_FUNCTION("sBCTjFk7Gi4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTINSt8ios_base7failureE);
    LIB_FUNCTION("Sn3TCBWJ5wo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIo);
    LIB_FUNCTION("Jk+LgZzCsi8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPa);
    LIB_FUNCTION("+qso2nVwQzg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPb);
    LIB_FUNCTION("M1jmeNsWVK8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPc);
    LIB_FUNCTION("3o0PDVnn1qA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPd);
    LIB_FUNCTION("7OO0uCJWILQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPDh);
    LIB_FUNCTION("DOBCPW6DL3w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPDi);
    LIB_FUNCTION("QvWOlLyuQ2o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPDn);
    LIB_FUNCTION("OkYxbdkrv64", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPDs);
    LIB_FUNCTION("96xdSFbiR7Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPe);
    LIB_FUNCTION("01FSgNK1wwA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPf);
    LIB_FUNCTION("ota-3+co4Jk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPh);
    LIB_FUNCTION("YstfcFbhwvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPi);
    LIB_FUNCTION("DQ9mChn0nnE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPj);
    LIB_FUNCTION("Ml1z3dYEVPM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKa);
    LIB_FUNCTION("WV94zKqwgxY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKb);
    LIB_FUNCTION("I4y33AOamns", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKc);
    LIB_FUNCTION("0G36SAiYUhQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKd);
    LIB_FUNCTION("NVCBWomXpcw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKDh);
    LIB_FUNCTION("50aDlGVFt5I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKDi);
    LIB_FUNCTION("liR+QkhejDk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKDn);
    LIB_FUNCTION("kzfj-YSkW7w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKDs);
    LIB_FUNCTION("7uX6IsXWwak", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKe);
    LIB_FUNCTION("2PXZUKjolAA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKf);
    LIB_FUNCTION("RKvygdQzGaY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKh);
    LIB_FUNCTION("sVUkO0TTpM8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKi);
    LIB_FUNCTION("4zhc1xNSIno", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKj);
    LIB_FUNCTION("Gr+ih5ipgNk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKl);
    LIB_FUNCTION("0cLFYdr1AGc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKm);
    LIB_FUNCTION("0Xxtiar8Ceg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKn);
    LIB_FUNCTION("hsttk-IbL1o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKo);
    LIB_FUNCTION("zqOGToT2dH8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKs);
    LIB_FUNCTION("WY7615THqKM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKt);
    LIB_FUNCTION("0g+zCGZ7dgQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKv);
    LIB_FUNCTION("jfqTdKTGbBI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKw);
    LIB_FUNCTION("sOz2j1Lxl48", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKx);
    LIB_FUNCTION("qTgw+f54K34", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPKy);
    LIB_FUNCTION("1+5ojo5J2xU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPl);
    LIB_FUNCTION("SPiW3NTO8I0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPm);
    LIB_FUNCTION("zUwmtNuJABI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPn);
    LIB_FUNCTION("A9PfIjQCOUw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPo);
    LIB_FUNCTION("nqpARwWZmjI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPs);
    LIB_FUNCTION("KUW22XiVxvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPt);
    LIB_FUNCTION("OJPn-YR1bow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPv);
    LIB_FUNCTION("7gj0BXUP3dc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPw);
    LIB_FUNCTION("9opd1ucwDqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPx);
    LIB_FUNCTION("a9KMkfXXUsE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIPy);
    LIB_FUNCTION("j97CjKJNtQI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIs);
    LIB_FUNCTION("U1CBVMD42HA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISi);
    LIB_FUNCTION("iLSavTYoxx0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISo);
    LIB_FUNCTION("H0aqk25W6BI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt10bad_typeid);
    LIB_FUNCTION("2GWRrgT8o20", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt10ctype_base);
    LIB_FUNCTION("IBtzswgYU3A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt10money_base);
    LIB_FUNCTION("2e96MkSXo3U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt10moneypunctIcLb0EE);
    LIB_FUNCTION("Ks2FIQJ2eDc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt10moneypunctIcLb1EE);
    LIB_FUNCTION("EnMjfRlO5f0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt10moneypunctIwLb0EE);
    LIB_FUNCTION("gBZnTFMk6N0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt10moneypunctIwLb1EE);
    LIB_FUNCTION("n7iD5r9+4Eo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt11_Facet_base);
    LIB_FUNCTION("x8LHSvl5N6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt11logic_error);
    LIB_FUNCTION("C0IYaaVSC1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt11range_error);
    LIB_FUNCTION("9-TRy4p-YTM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt11regex_error);
    LIB_FUNCTION("XtP9KKwyK9Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt12bad_weak_ptr);
    LIB_FUNCTION("dDIjj8NBxNA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt12codecvt_base);
    LIB_FUNCTION("5BIbzIuDxTQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt12domain_error);
    LIB_FUNCTION("DCY9coLQcVI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt12future_error);
    LIB_FUNCTION("cxqzgvGm1GI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt12length_error);
    LIB_FUNCTION("dKjhNUf9FBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt12out_of_range);
    LIB_FUNCTION("eDciML+moZs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt12system_error);
    LIB_FUNCTION("Z7NWh8jD+Nw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt13bad_exception);
    LIB_FUNCTION("STNAj1oxtpk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt13basic_filebufIcSt11char_traitsIcEE);
    LIB_FUNCTION("37CMzzbbHn8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt13basic_filebufIwSt11char_traitsIwEE);
    LIB_FUNCTION("WbBz4Oam3wM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt13messages_base);
    LIB_FUNCTION("bLPn1gfqSW8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt13runtime_error);
    LIB_FUNCTION("cbvW20xPgyc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt14error_category);
    LIB_FUNCTION("lt0mLhNwjs0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt14overflow_error);
    LIB_FUNCTION("oNRAB0Zs2+0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt15underflow_error);
    LIB_FUNCTION("XZzWt0ygWdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt16invalid_argument);
    LIB_FUNCTION("FtPFMdiURuM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt16nested_exception);
    LIB_FUNCTION("c33GAGjd7Is", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt17bad_function_call);
    LIB_FUNCTION("8rd5FvOFk+w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt18bad_variant_access);
    LIB_FUNCTION("lbLEAN+Y9iI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt20bad_array_new_length);
    LIB_FUNCTION("3aZN32UTqqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt22_Future_error_category);
    LIB_FUNCTION("QLqM1r9nPow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt22_System_error_category);
    LIB_FUNCTION("+Le0VsFb9mE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt23_Generic_error_category);
    LIB_FUNCTION("QQsnQ2bWkdM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt4_Pad);
    LIB_FUNCTION("sIvK5xl5pzw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt5_IosbIiE);
    LIB_FUNCTION("gZvNGjQsmf8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt5ctypeIcE);
    LIB_FUNCTION("Fj7VTFzlI3k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt5ctypeIwE);
    LIB_FUNCTION("weALTw0uesc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt7_MpunctIcE);
    LIB_FUNCTION("DaYYQBc+SY8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt7_MpunctIwE);
    LIB_FUNCTION("Cs3DBACRSY8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt7codecvtIcc9_MbstatetE);
    LIB_FUNCTION("+TtUFzALoDc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt7codecvtIDic9_MbstatetE);
    LIB_FUNCTION("v1WebHtIa24", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt7codecvtIDsc9_MbstatetE);
    LIB_FUNCTION("hbU5HOTy1HM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt7codecvtIwc9_MbstatetE);
    LIB_FUNCTION("fvgYbBEhXnc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt7collateIcE);
    LIB_FUNCTION("pphEhnnuXKA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt7collateIwE);
    LIB_FUNCTION("qOD-ksTkE08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt8bad_cast);
    LIB_FUNCTION("BJCgW9-OxLA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt8ios_base);
    LIB_FUNCTION("UFsKD1fd1-w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt8messagesIcE);
    LIB_FUNCTION("007PjrBCaUM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt8messagesIwE);
    LIB_FUNCTION("ddLNBT9ks2I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt8numpunctIcE);
    LIB_FUNCTION("A2TTRMAe6Sw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt8numpunctIwE);
    LIB_FUNCTION("DwH3gdbYfZo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt9bad_alloc);
    LIB_FUNCTION("7f4Nl2VS0gw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt9basic_iosIcSt11char_traitsIcEE);
    LIB_FUNCTION("RjWhdj0ztTs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt9basic_iosIwSt11char_traitsIwEE);
    LIB_FUNCTION("n2kx+OmFUis", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt9exception);
    LIB_FUNCTION("CVcmmf8VL40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt9time_base);
    LIB_FUNCTION("xX6s+z0q6oo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTISt9type_info);
    LIB_FUNCTION("Qd6zUdRhrhs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIt);
    LIB_FUNCTION("JrUnjJ-PCTg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIv);
    LIB_FUNCTION("qUxH+Damft4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIw);
    LIB_FUNCTION("8Ijx3Srynh0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIx);
    LIB_FUNCTION("KBBVmt8Td7c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTIy);
    LIB_FUNCTION("iOLTktXe6a0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSa);
    LIB_FUNCTION("M86y4bmx+WA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSb);
    LIB_FUNCTION("zGpCWBtVC0A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSc);
    LIB_FUNCTION("pMQUQSfX6ZE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSd);
    LIB_FUNCTION("DghzFjzLqaE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSDi);
    LIB_FUNCTION("FUvnVyCDhjg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSDn);
    LIB_FUNCTION("Z7+siBC690w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSDs);
    LIB_FUNCTION("KNgcEteI72I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSe);
    LIB_FUNCTION("aFMVMBzO5jk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSf);
    LIB_FUNCTION("BNC7IeJelZ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSh);
    LIB_FUNCTION("papHVcWkO5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSi);
    LIB_FUNCTION("1nylaCTiH08", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSj);
    LIB_FUNCTION("k9kErpz2Sv8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSl);
    LIB_FUNCTION("OzMC6yz6Row", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSm);
    LIB_FUNCTION("au+YxKwehQM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv116__enum_type_infoE);
    LIB_FUNCTION("6BYT26CFh58", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv117__array_type_infoE);
    LIB_FUNCTION("8Vs1AjNm2mE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv117__class_type_infoE);
    LIB_FUNCTION("bPUMNZBqRqQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv117__pbase_type_infoE);
    LIB_FUNCTION("UVft3+rc06o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv119__pointer_type_infoE);
    LIB_FUNCTION("4ZXlZy7iRWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv120__function_type_infoE);
    LIB_FUNCTION("AQlqO860Ztc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv120__si_class_type_infoE);
    LIB_FUNCTION("I1Ru2fZJDoE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv121__vmi_class_type_infoE);
    LIB_FUNCTION("6WYrZgAyjuE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv123__fundamental_type_infoE);
    LIB_FUNCTION("K+w0ofCSsAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN10__cxxabiv129__pointer_to_member_type_infoE);
    LIB_FUNCTION("y-bbIiLALd4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN6Dinkum7threads10lock_errorE);
    LIB_FUNCTION("hmHH6DsLWgA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSN6Dinkum7threads21thread_resource_errorE);
    LIB_FUNCTION("RVDooP5gZ4s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSNSt6locale5facetE);
    LIB_FUNCTION("JjTc4SCuILE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSNSt6locale7_LocimpE);
    LIB_FUNCTION("C-3N+mEQli4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSNSt8ios_base7failureE);
    LIB_FUNCTION("p07Yvdjjoo4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPa);
    LIB_FUNCTION("ickyvjtMLm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPb);
    LIB_FUNCTION("jJtoPFrxG-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPc);
    LIB_FUNCTION("dIxG0L1esAI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPd);
    LIB_FUNCTION("TSMc8vgtvHI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPDi);
    LIB_FUNCTION("cj+ge8YLU7s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPDn);
    LIB_FUNCTION("mQCm5NmJORg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPDs);
    LIB_FUNCTION("N84qS6rJuGI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPe);
    LIB_FUNCTION("DN0xDLRXD2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPf);
    LIB_FUNCTION("HHVZLHmCfI4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPh);
    LIB_FUNCTION("g8phA3duRm8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPi);
    LIB_FUNCTION("bEbjy6yceWo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPj);
    LIB_FUNCTION("dSifrMdPVQ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKa);
    LIB_FUNCTION("qSiIrmgy1D8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKb);
    LIB_FUNCTION("wm9QKozFM+s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKc);
    LIB_FUNCTION("-7c7thUsi1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKd);
    LIB_FUNCTION("lFKA8eMU5PA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKDi);
    LIB_FUNCTION("2veyNsXFZuw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKDn);
    LIB_FUNCTION("qQ4c52GZlYw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKDs);
    LIB_FUNCTION("8Ce6O0B-KpA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKe);
    LIB_FUNCTION("emnRy3TNxFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKf);
    LIB_FUNCTION("thDTXTikSmc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKh);
    LIB_FUNCTION("3Fd+8Pk6fgE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKi);
    LIB_FUNCTION("6azovDgjxt0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKj);
    LIB_FUNCTION("QdPk9cbJrOY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKl);
    LIB_FUNCTION("ER8-AFoFDfM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKm);
    LIB_FUNCTION("5rD2lCo4688", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKs);
    LIB_FUNCTION("iWMhoHS8gqk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKt);
    LIB_FUNCTION("3op2--wf660", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKv);
    LIB_FUNCTION("h64u7Gu3-TM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKw);
    LIB_FUNCTION("3THnS7v0D+M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKx);
    LIB_FUNCTION("h+xQETZ+6Yo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPKy);
    LIB_FUNCTION("6cfcRTPD2zU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPl);
    LIB_FUNCTION("OXkzGA9WqVw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPm);
    LIB_FUNCTION("6XcQYYO2YMY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPs);
    LIB_FUNCTION("8OSy0MMQ7eI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPt);
    LIB_FUNCTION("s1b2SRBzSAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPv);
    LIB_FUNCTION("4r--aFJyPpI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPw);
    LIB_FUNCTION("pc4-Lqosxgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPx);
    LIB_FUNCTION("VJL9W+nOv1U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSPy);
    LIB_FUNCTION("VenLJSDuDXY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSs);
    LIB_FUNCTION("46haDPRVtPo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSi);
    LIB_FUNCTION("RgJjmluR+QA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSo);
    LIB_FUNCTION("ALcclvT4W3Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt10bad_typeid);
    LIB_FUNCTION("idsapmYZ49w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt10ctype_base);
    LIB_FUNCTION("VxObo0uiafo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt10money_base);
    LIB_FUNCTION("h+iBEkE50Zs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt10moneypunctIcLb0EE);
    LIB_FUNCTION("o4DiZqXId90", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt10moneypunctIcLb1EE);
    LIB_FUNCTION("MxGclWMtl4g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt10moneypunctIwLb0EE);
    LIB_FUNCTION("J+hjiBreZr4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt10moneypunctIwLb1EE);
    LIB_FUNCTION("FAah-AY8+vY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt11_Facet_base);
    LIB_FUNCTION("VNHXByo1yuY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt11logic_error);
    LIB_FUNCTION("msxwgUAPy-Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt11range_error);
    LIB_FUNCTION("UG6HJeH5GNI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt11regex_error);
    LIB_FUNCTION("P7l9+yBL5VU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt12bad_weak_ptr);
    LIB_FUNCTION("NXKsxT-x76M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt12codecvt_base);
    LIB_FUNCTION("2ud1bFeR0h8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt12domain_error);
    LIB_FUNCTION("YeBP0Rja7vc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt12future_error);
    LIB_FUNCTION("zEhcQGEiPik", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt12length_error);
    LIB_FUNCTION("eNW5jsFxS6k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt12out_of_range);
    LIB_FUNCTION("XRxuwvN++2w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt12system_error);
    LIB_FUNCTION("G8z7rz17xYM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt13bad_exception);
    LIB_FUNCTION("WYWf+rJuDVU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt13basic_filebufIcSt11char_traitsIcEE);
    LIB_FUNCTION("coVkgLzNtaw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt13basic_filebufIwSt11char_traitsIwEE);
    LIB_FUNCTION("N0EHkukBz6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt13messages_base);
    LIB_FUNCTION("CX3WC8qekJE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt13runtime_error);
    LIB_FUNCTION("u5zp3yXW5wA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt14error_category);
    LIB_FUNCTION("iy1lPjADRUs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt14overflow_error);
    LIB_FUNCTION("Uea1kfRJ7Oc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt15underflow_error);
    LIB_FUNCTION("KJutwrVUFUo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt16invalid_argument);
    LIB_FUNCTION("S8kp05fMCqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt16nested_exception);
    LIB_FUNCTION("ql6hz7ZOZTs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt17bad_function_call);
    LIB_FUNCTION("ObdBkrZylOg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt18bad_variant_access);
    LIB_FUNCTION("hBvqSQD5yNk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt20bad_array_new_length);
    LIB_FUNCTION("ouo2obDE6yU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt22_Future_error_category);
    LIB_FUNCTION("iwIUndpU5ZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt22_System_error_category);
    LIB_FUNCTION("88Fre+wfuT0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt23_Generic_error_category);
    LIB_FUNCTION("qR6GVq1IplU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt4_Pad);
    LIB_FUNCTION("uO6YxonQkJI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt5_IosbIiE);
    LIB_FUNCTION("jUQ+FlOMEHk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt5ctypeIcE);
    LIB_FUNCTION("1jUJO+TZm5k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt5ctypeIwE);
    LIB_FUNCTION("LfMY9H6d5mI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt7_MpunctIcE);
    LIB_FUNCTION("mh9Jro0tcjg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt7_MpunctIwE);
    LIB_FUNCTION("rf0BfDQG1KU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt7codecvtIcc9_MbstatetE);
    LIB_FUNCTION("Tt3ZSp9XD4E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt7codecvtIDic9_MbstatetE);
    LIB_FUNCTION("9XL3Tlgx6lc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt7codecvtIDsc9_MbstatetE);
    LIB_FUNCTION("YrYO5bTIPqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt7codecvtIwc9_MbstatetE);
    LIB_FUNCTION("wElyE0OmoRw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt7collateIcE);
    LIB_FUNCTION("BdfPxmlM9bs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt7collateIwE);
    LIB_FUNCTION("--fMWwCvo+c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt8bad_cast);
    LIB_FUNCTION("Nr+GiZ0tGAk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt8ios_base);
    LIB_FUNCTION("iUhx-JN27uI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt8messagesIcE);
    LIB_FUNCTION("5ViZYJRew6g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt8messagesIwE);
    LIB_FUNCTION("2ZqL1jnL8so", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt8numpunctIcE);
    LIB_FUNCTION("xuEUMolGMwU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt8numpunctIwE);
    LIB_FUNCTION("22g2xONdXV4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt9bad_alloc);
    LIB_FUNCTION("TuKJRIKcceA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt9basic_iosIcSt11char_traitsIcEE);
    LIB_FUNCTION("wYWYC8xNFOI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt9basic_iosIwSt11char_traitsIwEE);
    LIB_FUNCTION("H61hE9pLBmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt9exception);
    LIB_FUNCTION("5jX3QET-Jhw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt9time_base);
    LIB_FUNCTION("WG7lrmFxyKY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSSt9type_info);
    LIB_FUNCTION("f5zmgYKSpIY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSt);
    LIB_FUNCTION("mI0SR5s7kxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSv);
    LIB_FUNCTION("UXS8VgAnIP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSw);
    LIB_FUNCTION("N8KLCZc3Y1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSx);
    LIB_FUNCTION("kfuINXyHayQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTSy);
    LIB_FUNCTION("0bGGr4zLE3w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTv0_n24_NSiD0Ev);
    LIB_FUNCTION("+Uuj++A+I14", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTv0_n24_NSiD1Ev);
    LIB_FUNCTION("QJJ-4Dgm8YQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTv0_n24_NSoD0Ev);
    LIB_FUNCTION("kvqg376KsJo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTv0_n24_NSoD1Ev);
    LIB_FUNCTION("fjni7nkqJ4M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv116__enum_type_infoE);
    LIB_FUNCTION("aMQhMoYipk4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv117__array_type_infoE);
    LIB_FUNCTION("byV+FWlAnB4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv117__class_type_infoE);
    LIB_FUNCTION("7EirbE7st4E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv117__pbase_type_infoE);
    LIB_FUNCTION("aeHxLWwq0gQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv119__pointer_type_infoE);
    LIB_FUNCTION("CSEjkTYt5dw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv120__function_type_infoE);
    LIB_FUNCTION("pZ9WXcClPO8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv120__si_class_type_infoE);
    LIB_FUNCTION("9ByRMdo7ywg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv121__vmi_class_type_infoE);
    LIB_FUNCTION("G4XM-SS1wxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv123__fundamental_type_infoE);
    LIB_FUNCTION("2H51caHZU0Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN10__cxxabiv129__pointer_to_member_type_infoE);
    LIB_FUNCTION("WJU9B1OjRbA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN6Dinkum7threads10lock_errorE);
    LIB_FUNCTION("ouXHPXjKUL4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVN6Dinkum7threads21thread_resource_errorE);
    LIB_FUNCTION("QGkJzBs3WmU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVNSt6locale7_LocimpE);
    LIB_FUNCTION("yLE5H3058Ao", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVNSt8ios_base7failureE);
    LIB_FUNCTION("+8jItptyeQQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSi);
    LIB_FUNCTION("qjyK90UVVCM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSo);
    LIB_FUNCTION("jRLwj8TLcQY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt10bad_typeid);
    LIB_FUNCTION("XbFyGCk3G2s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt10moneypunctIcLb0EE);
    LIB_FUNCTION("MfyPz2J5E0I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt10moneypunctIcLb1EE);
    LIB_FUNCTION("RfpPDUaxVJM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt10moneypunctIwLb0EE);
    LIB_FUNCTION("APrAh-3-ICg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt10moneypunctIwLb1EE);
    LIB_FUNCTION("udTM6Nxx-Ng", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt11logic_error);
    LIB_FUNCTION("RbzWN8X21hY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt11range_error);
    LIB_FUNCTION("c-EfVOIbo8M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt11regex_error);
    LIB_FUNCTION("apHKv46QaCw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt12bad_weak_ptr);
    LIB_FUNCTION("oAidKrxuUv0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt12domain_error);
    LIB_FUNCTION("6-LMlTS1nno", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt12future_error);
    LIB_FUNCTION("cqvea9uWpvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt12length_error);
    LIB_FUNCTION("n+aUKkC-3sI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt12out_of_range);
    LIB_FUNCTION("Bq8m04PN1zw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt12system_error);
    LIB_FUNCTION("Gvp-ypl9t5E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt13bad_exception);
    LIB_FUNCTION("rSADYzp-RTU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt13basic_filebufIcSt11char_traitsIcEE);
    LIB_FUNCTION("Tx5Y+BQJrzs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt13basic_filebufIwSt11char_traitsIwEE);
    LIB_FUNCTION("-L+-8F0+gBc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt13runtime_error);
    LIB_FUNCTION("lF66NEAqanc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt14error_category);
    LIB_FUNCTION("Azw9C8cy7FY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt14overflow_error);
    LIB_FUNCTION("ZrFcJ-Ab0vw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt15underflow_error);
    LIB_FUNCTION("keXoyW-rV-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt16invalid_argument);
    LIB_FUNCTION("j6qwOi2Nb7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt16nested_exception);
    LIB_FUNCTION("wuOrunkpIrU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt17bad_function_call);
    LIB_FUNCTION("AZGKZIVok6U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt18bad_variant_access);
    LIB_FUNCTION("Z+vcX3rnECg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt20bad_array_new_length);
    LIB_FUNCTION("YHfG3-K23CY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt22_Future_error_category);
    LIB_FUNCTION("qTwVlzGoViY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt22_System_error_category);
    LIB_FUNCTION("UuVHsmfVOHU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt23_Generic_error_category);
    LIB_FUNCTION("CRoMIoZkYhU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt4_Pad);
    LIB_FUNCTION("GKWcAz6-G7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt5ctypeIcE);
    LIB_FUNCTION("qdHsu+gIxRo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt5ctypeIwE);
    LIB_FUNCTION("6gAhNHCNHxY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt7_MpunctIcE);
    LIB_FUNCTION("+hlZqs-XpUM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt7_MpunctIwE);
    LIB_FUNCTION("aK1Ymf-NhAs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt7codecvtIcc9_MbstatetE);
    LIB_FUNCTION("9H2BStEAAMg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt7codecvtIDic9_MbstatetE);
    LIB_FUNCTION("jlNI3SSF41o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt7codecvtIDsc9_MbstatetE);
    LIB_FUNCTION("H-TDszhsYuY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt7codecvtIwc9_MbstatetE);
    LIB_FUNCTION("ruZtIwbCFjk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt7collateIcE);
    LIB_FUNCTION("rZwUkaQ02J4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt7collateIwE);
    LIB_FUNCTION("tVHE+C8vGXk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt8bad_cast);
    LIB_FUNCTION("AJsqpbcCiwY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt8ios_base);
    LIB_FUNCTION("FnEnECMJGag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt8messagesIcE);
    LIB_FUNCTION("2FezsYwelgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt8messagesIwE);
    LIB_FUNCTION("lJnP-cn0cvQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt8numpunctIcE);
    LIB_FUNCTION("Gtsl8PUl40U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt8numpunctIwE);
    LIB_FUNCTION("EMNG6cHitlQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt9bad_alloc);
    LIB_FUNCTION("dCzeFfg9WWI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt9exception);
    LIB_FUNCTION("749AEdSd4Go", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal__ZTVSt9type_info);
    LIB_FUNCTION(
        "jfq92K8E1Vc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
        internal__ZZNSt13basic_filebufIcSt11char_traitsIcEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit);
    LIB_FUNCTION(
        "AoZRvn-vaq4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
        internal__ZZNSt13basic_filebufIwSt11char_traitsIwEE5_InitEP7__sFILENS2_7_InitflEE7_Stinit);
    LIB_FUNCTION("L1SBTkC+Cvw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_abort);
    LIB_FUNCTION("SmYrO79NzeI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_abort_handler_s);
    LIB_FUNCTION("DQXJraCc1rA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_alarm);
    LIB_FUNCTION("2Btkg8k24Zg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_aligned_alloc);
    LIB_FUNCTION("jT3xiGpA3B4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_asctime);
    LIB_FUNCTION("qPe7-h5Jnuc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_asctime_s);
    LIB_FUNCTION("HC8vbJStYVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_at_quick_exit);
    LIB_FUNCTION("8G2LB+A3rzg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atexit);
    LIB_FUNCTION("SRI6S9B+-a4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_atof);
    LIB_FUNCTION("fPxypibz2MY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_atoi);
    LIB_FUNCTION("+my9jdHCMIQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_atol);
    LIB_FUNCTION("fLcU5G6Qrjs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_atoll);
    LIB_FUNCTION("rg5JEBlKCuo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_basename);
    LIB_FUNCTION("vsK6LzRtRLI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_basename_r);
    LIB_FUNCTION("5TjaJwkLWxE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_bcmp);
    LIB_FUNCTION("RMo7j0iTPfA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_bcopy);
    LIB_FUNCTION("NesIgTmfF0Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_bsearch);
    LIB_FUNCTION("hzX87C+zDAY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_bsearch_s);
    LIB_FUNCTION("LEvm25Sxi7I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_btowc);
    LIB_FUNCTION("9oiX1kyeedA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_bzero);
    LIB_FUNCTION("EOLQfNZ9HpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_c16rtomb);
    LIB_FUNCTION("MzsycG6RYto", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_c32rtomb);
    LIB_FUNCTION("2X5agFjKxMc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_calloc);
    LIB_FUNCTION("5ZkEP3Rq7As", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_cbrt);
    LIB_FUNCTION("GlelR9EEeck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_cbrtf);
    LIB_FUNCTION("lO01m-JcDqM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_cbrtl);
    LIB_FUNCTION("St9nbxSoezk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_clearerr);
    LIB_FUNCTION("cYNk9M+7YkY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_clearerr_unlocked);
    LIB_FUNCTION("QZP6I9ZZxpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_clock);
    LIB_FUNCTION("n8onIBR4Qdk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_clock_1700);
    LIB_FUNCTION("XepdqehVYe4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_closedir);
    LIB_FUNCTION("BEFy1ZFv8Fw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_copysign);
    LIB_FUNCTION("x-04iOzl1xs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_copysignf);
    LIB_FUNCTION("j84nSG4V0B0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_copysignl);
    LIB_FUNCTION("0uAUs3hYuG4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ctime);
    LIB_FUNCTION("2UFh+YKfuzk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ctime_s);
    LIB_FUNCTION("Wn6I3wVATUE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_daemon);
    LIB_FUNCTION("tOicWgmk4ZI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_daylight);
    LIB_FUNCTION("fqLrWSWcGHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_devname);
    LIB_FUNCTION("BIALMFTZ75I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_devname_r);
    LIB_FUNCTION("-VVn74ZyhEs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_difftime);
    LIB_FUNCTION("E4wZaG1zSFc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_dirname);
    LIB_FUNCTION("2gbcltk3swE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_div);
    LIB_FUNCTION("WIg11rA+MRY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_drand48);
    LIB_FUNCTION("5OpjqFs8yv8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_drem);
    LIB_FUNCTION("Gt5RT417EGA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_dremf);
    LIB_FUNCTION("Fncgcl1tnXg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_erand48);
    LIB_FUNCTION("oXgaqGVnW5o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_erf);
    LIB_FUNCTION("arIKLlen2sg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_erfc);
    LIB_FUNCTION("IvF98yl5u4s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_erfcf);
    LIB_FUNCTION("f2YbMj0gBf8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_erfcl);
    LIB_FUNCTION("RePA3bDBJqo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_erff);
    LIB_FUNCTION("fNH4tsl7rB8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_erfl);
    LIB_FUNCTION("aeeMZ0XrNsY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_err);
    LIB_FUNCTION("9aODPZAKOmA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_err_set_exit);
    LIB_FUNCTION("FihG2CudUNs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_err_set_file);
    LIB_FUNCTION("L-jLYJFP9Mc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_errc);
    LIB_FUNCTION("t8sFCgJAClE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_errx);
    LIB_FUNCTION("uMei1W9uyNo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_exit);
    LIB_FUNCTION("uodLYyUip20", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fclose);
    LIB_FUNCTION("cBSM-YB7JVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fcloseall);
    LIB_FUNCTION("Zs4p6RemDxM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_fdim);
    LIB_FUNCTION("yb9iUBPkSS0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fdimf);
    LIB_FUNCTION("IMt+UO5YoQI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fdiml);
    LIB_FUNCTION("qdlHjTa9hQ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fdopen);
    LIB_FUNCTION("j+XjoRSIvwU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fdopendir);
    LIB_FUNCTION("cqt8emEH3kQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_feclearexcept);
    LIB_FUNCTION("y4WlO8qzHqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fedisableexcept);
    LIB_FUNCTION("utLW7uXm3Ss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_feenableexcept);
    LIB_FUNCTION("psx0YiAKm7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fegetenv);
    LIB_FUNCTION("VtRkfsD292M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fegetexcept);
    LIB_FUNCTION("myQDQapYJdw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fegetexceptflag);
    LIB_FUNCTION("AeZTCCm1Qqc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fegetround);
    LIB_FUNCTION("P38JvXuK-uE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fegettrapenable);
    LIB_FUNCTION("1kduKXMqx7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_feholdexcept);
    LIB_FUNCTION("LxcEU+ICu8U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_feof);
    LIB_FUNCTION("NuydofHcR1w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_feof_unlocked);
    LIB_FUNCTION("NIfFNcyeCTo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_feraiseexcept);
    LIB_FUNCTION("AHxyhN96dy4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ferror);
    LIB_FUNCTION("yxbGzBQC5xA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ferror_unlocked);
    LIB_FUNCTION("Q-bLp+b-RVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fesetenv);
    LIB_FUNCTION("FuxaUZsWTok", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fesetexceptflag);
    LIB_FUNCTION("hAJZ7-FBpEQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fesetround);
    LIB_FUNCTION("u5a7Ofymqlg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fesettrapenable);
    LIB_FUNCTION("pVjisbvtQKU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fetestexcept);
    LIB_FUNCTION("YXQ4gXamCrY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_feupdateenv);
    LIB_FUNCTION("MUjC4lbHrK4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fflush);
    LIB_FUNCTION("AEuF3F2f8TA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fgetc);
    LIB_FUNCTION("KKgUiHSYGRE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fgetln);
    LIB_FUNCTION("SHlt7EhOtqA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fgetpos);
    LIB_FUNCTION("KdP-nULpuGw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fgets);
    LIB_FUNCTION("bzbQ5zQ2Y3g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fgetwc);
    LIB_FUNCTION("F81hKe2k2tg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fgetws);
    LIB_FUNCTION("Fm-dmyywH9Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fileno);
    LIB_FUNCTION("kxm0z4T5mMI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fileno_unlocked);
    LIB_FUNCTION("TJFQFm+W3wg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_finite);
    LIB_FUNCTION("2nqzJ87zsB8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_finitef);
    LIB_FUNCTION("hGljHZEfF0U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_flockfile);
    LIB_FUNCTION("G3qjOUu7KnM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_flsl);
    LIB_FUNCTION("YKbL5KR6RDI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_fma);
    LIB_FUNCTION("RpTR+VY15ss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_fmaf);
    LIB_FUNCTION("uMeLdbwheag", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_fmal);
    LIB_FUNCTION("xeYO4u7uyJ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fopen);
    LIB_FUNCTION("NL836gOLANs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fopen_s);
    LIB_FUNCTION("y1Ch2nXs4IQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fpurge);
    LIB_FUNCTION("aZK8lNei-Qw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fputc);
    LIB_FUNCTION("QrZZdJ8XsX0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fputs);
    LIB_FUNCTION("1QJWxoB6pCo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fputwc);
    LIB_FUNCTION("-7nRJFXMxnM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fputws);
    LIB_FUNCTION("lbB+UlZqVG0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fread);
    LIB_FUNCTION("N2OjwJJGjeQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_freeifaddrs);
    LIB_FUNCTION("gkWgn0p1AfU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_freopen);
    LIB_FUNCTION("NdvAi34vV3g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_freopen_s);
    LIB_FUNCTION("rQFVBXp-Cxg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fseek);
    LIB_FUNCTION("pkYiKw09PRA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fseeko);
    LIB_FUNCTION("7PkSz+qnTto", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fsetpos);
    LIB_FUNCTION("6IM2up2+a-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fstatvfs);
    LIB_FUNCTION("Qazy8LmXTvw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ftell);
    LIB_FUNCTION("5qP1iVQkdck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ftello);
    LIB_FUNCTION("h05OHOMZNMw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ftrylockfile);
    LIB_FUNCTION("vAc9y8UQ31o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_funlockfile);
    LIB_FUNCTION("w6Aq68dFoP4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fwide);
    LIB_FUNCTION("MpxhMh8QFro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_fwrite);
    LIB_FUNCTION("BD-xV2fLe2M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_gamma);
    LIB_FUNCTION("q+AdV-EHiKc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_gamma_r);
    LIB_FUNCTION("sZ93QMbGRJY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_gammaf);
    LIB_FUNCTION("E3RYvWbYLgk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_gammaf_r);
    LIB_FUNCTION("8Q60JLJ6Rv4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_getc);
    LIB_FUNCTION("5tM252Rs2fc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getc_unlocked);
    LIB_FUNCTION("L3XZiuKqZUM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getchar);
    LIB_FUNCTION("H0pVDvSuAVQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getchar_unlocked);
    LIB_FUNCTION("DYivN1nO-JQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getcwd);
    LIB_FUNCTION("smbQukfxYJM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getenv);
    LIB_FUNCTION("-nvxBWa0iDs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_gethostname);
    LIB_FUNCTION("j-gWL6wDros", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getifaddrs);
    LIB_FUNCTION("VUtibKJCt1o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getopt);
    LIB_FUNCTION("8VVXJxB5nlk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getopt_long);
    LIB_FUNCTION("oths6jEyBqo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getopt_long_only);
    LIB_FUNCTION("7Psx1DlAyE4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getprogname);
    LIB_FUNCTION("Ok+SYcoL19Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_gets);
    LIB_FUNCTION("lb+HLLZkbbw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_gets_s);
    LIB_FUNCTION("AoLA2MRWJvc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_getw);
    LIB_FUNCTION("CosTELN5ETk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getwc);
    LIB_FUNCTION("n2mWDsholo8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_getwchar);
    LIB_FUNCTION("1mecP7RgI2A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_gmtime);
    LIB_FUNCTION("5bBacGLyLOs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_gmtime_s);
    LIB_FUNCTION("YFoOw5GkkK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_hypot);
    LIB_FUNCTION("2HzgScoQq9o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_hypot3);
    LIB_FUNCTION("xlRcc7Rcqgo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_hypot3f);
    LIB_FUNCTION("aDmly36AAgI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_hypot3l);
    LIB_FUNCTION("iz2shAGFIxc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_hypotf);
    LIB_FUNCTION("jJC7x18ge8k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_hypotl);
    LIB_FUNCTION("ODGONXcSmz4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ignore_handler_s);
    LIB_FUNCTION("t3RFHn0bTPg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_index);
    LIB_FUNCTION("sBBuXmJ5Kjk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_inet_addr);
    LIB_FUNCTION("ISTLytNGT0c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_inet_aton);
    LIB_FUNCTION("7iTp7O6VOXQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_inet_ntoa);
    LIB_FUNCTION("i3E1Ywn4t+8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_inet_ntoa_r);
    LIB_FUNCTION("IIUY-5hk-4k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_initstate);
    LIB_FUNCTION("4uJJNi+C9wk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isalnum);
    LIB_FUNCTION("+xU0WKT8mDc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isalpha);
    LIB_FUNCTION("lhnrCOBiTGo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isblank);
    LIB_FUNCTION("akpGErA1zdg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iscntrl);
    LIB_FUNCTION("JWBr5N8zyNE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isdigit);
    LIB_FUNCTION("rrgxakQtvc0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isgraph);
    LIB_FUNCTION("eGkOpTojJl4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isprint);
    LIB_FUNCTION("I6Z-684E2C4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ispunct);
    LIB_FUNCTION("wazw2x2m3DQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isspace);
    LIB_FUNCTION("wDmL2EH0CBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswalnum);
    LIB_FUNCTION("D-qDARDb1aM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswalpha);
    LIB_FUNCTION("p6DbM0OAHNo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswblank);
    LIB_FUNCTION("6A+1YZ79qFk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswcntrl);
    LIB_FUNCTION("45E7omS0vvc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswctype);
    LIB_FUNCTION("n0kT+8Eeizs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswdigit);
    LIB_FUNCTION("wjG0GyCyaP0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswgraph);
    LIB_FUNCTION("Ok8KPy3nFls", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswlower);
    LIB_FUNCTION("U7IhU4VEB-0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswprint);
    LIB_FUNCTION("AEPvEZkaLsU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswpunct);
    LIB_FUNCTION("vqtytrxgLMs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswspace);
    LIB_FUNCTION("1QcrrL9UDRQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswupper);
    LIB_FUNCTION("cjmSjRlnMAs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_iswxdigit);
    LIB_FUNCTION("srzSVSbKn7M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_isxdigit);
    LIB_FUNCTION("tcN0ngcXegg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_j0);
    LIB_FUNCTION("RmE3aE8WHuY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_j0f);
    LIB_FUNCTION("BNbWdC9Jg+4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_j1);
    LIB_FUNCTION("uVXcivvVHzU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_j1f);
    LIB_FUNCTION("QdE7Arjzxos", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_jn);
    LIB_FUNCTION("M5KJmq-gKM8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_jnf);
    LIB_FUNCTION("M7KmRg9CERk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_jrand48);
    LIB_FUNCTION("xzZiQgReRGE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_labs);
    LIB_FUNCTION("wTjDJ6In3Cg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lcong48);
    LIB_FUNCTION("JrwFIMzKNr0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ldexp);
    LIB_FUNCTION("kn0yiYeExgA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ldexpf);
    LIB_FUNCTION("aX8H2+BBlWE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ldexpl);
    LIB_FUNCTION("gfP0im5Z3g0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_ldiv);
    LIB_FUNCTION("o-kMHRBvkbQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lgamma);
    LIB_FUNCTION("EjL+gY1G2lk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lgamma_r);
    LIB_FUNCTION("i-ifjh3SLBU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lgammaf);
    LIB_FUNCTION("RlGUiqyKf9I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lgammaf_r);
    LIB_FUNCTION("lPYpsOb9s-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lgammal);
    LIB_FUNCTION("rHRr+131ATY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_llabs);
    LIB_FUNCTION("iVhJZvAO2aQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lldiv);
    LIB_FUNCTION("-431A-YBAks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_llrint);
    LIB_FUNCTION("KPsQA0pis8o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_llrintf);
    LIB_FUNCTION("6bRANWNYID0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_llrintl);
    LIB_FUNCTION("w-BvXF4O6xo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_llround);
    LIB_FUNCTION("eQhBFnTOp40", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_llroundf);
    LIB_FUNCTION("wRs5S54zjm0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_llroundl);
    LIB_FUNCTION("0hlfW1O4Aa4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_localeconv);
    LIB_FUNCTION("efhK-YSUYYQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_localtime);
    LIB_FUNCTION("fiiNDnNBKVY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_localtime_s);
    LIB_FUNCTION("lKEN2IebgJ0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_longjmp);
    LIB_FUNCTION("5IpoNfxu84U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lrand48);
    LIB_FUNCTION("VOKOgR7L-2Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lrint);
    LIB_FUNCTION("rcVv5ivMhY0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lrintf);
    LIB_FUNCTION("jp2e+RSrcow", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_lrintl);
    LIB_FUNCTION("GipcbdDM5cI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_makecontext);
    LIB_FUNCTION("hew0fReI2H0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mblen);
    LIB_FUNCTION("j6OnScWpu7k", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbrlen);
    LIB_FUNCTION("ogPDBoLmCcA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbrtoc16);
    LIB_FUNCTION("TEd4egxRmdE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbrtoc32);
    LIB_FUNCTION("qVHpv0PxouI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbrtowc);
    LIB_FUNCTION("UbnVmck+o10", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbsinit);
    LIB_FUNCTION("8hygs6D9KBY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbsrtowcs);
    LIB_FUNCTION("1NFvAuzw8dA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbsrtowcs_s);
    LIB_FUNCTION("VUzjXknPPBs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbstowcs);
    LIB_FUNCTION("tdcAqgCS+uI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbstowcs_s);
    LIB_FUNCTION("6eU9xX9oEdQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mbtowc);
    LIB_FUNCTION("HWEOv0+n7cU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mergesort);
    LIB_FUNCTION("n7AepwR0s34", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mktime);
    LIB_FUNCTION("0WMHDb5Dt94", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_modf);
    LIB_FUNCTION("3+UPM-9E6xY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_modff);
    LIB_FUNCTION("tG8pGyxdLEs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_modfl);
    LIB_FUNCTION("k-l0Jth-Go8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_mrand48);
    LIB_FUNCTION("cJLTwtKGXJk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nearbyint);
    LIB_FUNCTION("c+4r-T-tEIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nearbyintf);
    LIB_FUNCTION("6n23e0gIJ9s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nearbyintl);
    LIB_FUNCTION("ZT4ODD2Ts9o", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_Need_sceLibcInternal);
    LIB_FUNCTION("h+J60RRlfnk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nextafter);
    LIB_FUNCTION("3m2ro+Di+Ck", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nextafterf);
    LIB_FUNCTION("R0-hvihVoy0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nextafterl);
    LIB_FUNCTION("-Q6FYBO4sn0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nexttoward);
    LIB_FUNCTION("QaTrhMKUT18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nexttowardf);
    LIB_FUNCTION("ryyn6-WJm6U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nexttowardl);
    LIB_FUNCTION("3wcYIMz8LUo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_nrand48);
    LIB_FUNCTION("ay3uROQAc5A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_opendir);
    LIB_FUNCTION("zG0BNJOZdm4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_optarg);
    LIB_FUNCTION("yaFXXViLWPw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_opterr);
    LIB_FUNCTION("zCnSJWp-Qj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_optind);
    LIB_FUNCTION("FwzVaZ8Vnus", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_optopt);
    LIB_FUNCTION("CZNm+oNmB-I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_optreset);
    LIB_FUNCTION("EMutwaQ34Jo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_perror);
    LIB_FUNCTION("3Nr9caNHhyg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawn);
    LIB_FUNCTION("Heh4KJwyoX8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawn_file_actions_addclose);
    LIB_FUNCTION("LG6O0oW9bQU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawn_file_actions_adddup2);
    LIB_FUNCTION("Sj7y+JO5PcM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawn_file_actions_addopen);
    LIB_FUNCTION("Ud8CbISKRGM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawn_file_actions_destroy);
    LIB_FUNCTION("p--TkNVsXjA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawn_file_actions_init);
    LIB_FUNCTION("Hq9-2AMG+ks", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_destroy);
    LIB_FUNCTION("7BGUDQDJu-A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_getflags);
    LIB_FUNCTION("Q-GfRQNi66I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_getpgroup);
    LIB_FUNCTION("jbgqYhmVEGY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_getschedparam);
    LIB_FUNCTION("KUYSaO1qv0Y", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_getschedpolicy);
    LIB_FUNCTION("7pASQ1hhH00", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_getsigdefault);
    LIB_FUNCTION("wvqDod5pVZg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_getsigmask);
    LIB_FUNCTION("44hlATrd47U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_init);
    LIB_FUNCTION("UV4m0bznVtU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_setflags);
    LIB_FUNCTION("aPDKI3J8PqI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_setpgroup);
    LIB_FUNCTION("SFlW4kqPgU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_setschedparam);
    LIB_FUNCTION("fBne7gcou0s", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_setschedpolicy);
    LIB_FUNCTION("Ani6e+T-y6Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_setsigdefault);
    LIB_FUNCTION("wCavZQ+m1PA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnattr_setsigmask);
    LIB_FUNCTION("IUfBO5UIZNc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_posix_spawnp);
    LIB_FUNCTION("tjuEJo1obls", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_psignal);
    LIB_FUNCTION("tLB5+4TEOK0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_putc);
    LIB_FUNCTION("H-QeERgWuTM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_putc_unlocked);
    LIB_FUNCTION("m5wN+SwZOR4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_putchar);
    LIB_FUNCTION("v95AEAzqm+0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_putchar_unlocked);
    LIB_FUNCTION("t1ytXodWUH0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_putenv);
    LIB_FUNCTION("YQ0navp+YIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_puts);
    LIB_FUNCTION("DwcWtj3tSPA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_putw);
    LIB_FUNCTION("UZJnC81pUCw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_putwc);
    LIB_FUNCTION("aW9KhGC4cOo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_putwchar);
    LIB_FUNCTION("AEJdIVZTEmo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_qsort);
    LIB_FUNCTION("G7yOZJObV+4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_qsort_s);
    LIB_FUNCTION("qdGFBoLVNKI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_quick_exit);
    LIB_FUNCTION("cpCOXWMgha0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_rand);
    LIB_FUNCTION("dW3xsu3EgFI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_rand_r);
    LIB_FUNCTION("w1o05aHJT4c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_random);
    LIB_FUNCTION("lybyyKtP54c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_readdir);
    LIB_FUNCTION("J0kng1yac3M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_readdir_r);
    LIB_FUNCTION("vhtcIgZG-Lk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_realpath);
    LIB_FUNCTION("eS+MVq+Lltw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_remainderf);
    LIB_FUNCTION("MvdnffYU3jg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_remainderl);
    LIB_FUNCTION("MZO7FXyAPU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_remove);
    LIB_FUNCTION("XI0YDgH8x1c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_remquo);
    LIB_FUNCTION("AqpZU2Njrmk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_remquof);
    LIB_FUNCTION("Fwow0yyW0nI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_remquol);
    LIB_FUNCTION("3QIPIh-GDjw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_rewind);
    LIB_FUNCTION("kCKHi6JYtmM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_rewinddir);
    LIB_FUNCTION("CWiqHSTO5hk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_rindex);
    LIB_FUNCTION("LxGIYYKwKYc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_rint);
    LIB_FUNCTION("q5WzucyVSkM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_rintf);
    LIB_FUNCTION("Yy5yMiZHBIc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_rintl);
    LIB_FUNCTION("nlaojL9hDtA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_round);
    LIB_FUNCTION("DDHG1a6+3q0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_roundf);
    LIB_FUNCTION("8F1ctQaP0uk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_roundl);
    LIB_FUNCTION("HI4N2S6ZWpE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scalb);
    LIB_FUNCTION("rjak2Xm+4mE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scalbf);
    LIB_FUNCTION("7Jp3g-qTgZw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scalbln);
    LIB_FUNCTION("S6LHwvK4h8c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scalblnf);
    LIB_FUNCTION("NFxDIuqfmgw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scalblnl);
    LIB_FUNCTION("KGKBeVcqJjc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scalbn);
    LIB_FUNCTION("9fs1btfLoUs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scalbnf);
    LIB_FUNCTION("l3fh3QW0Tss", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_scalbnl);
    LIB_FUNCTION("aqqpmI7-1j0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcDebugOut);
    LIB_FUNCTION("Sj3fKG7MwMk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcHeapGetAddressRanges);
    LIB_FUNCTION("HFtbbWvBO+U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcHeapMutexCalloc);
    LIB_FUNCTION("jJKMkpqQr7I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcHeapMutexFree);
    LIB_FUNCTION("4iOzclpv1M0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcHeapSetAddressRangeCallback);
    LIB_FUNCTION("M6qiY0nhk54", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcHeapSetTraceMarker);
    LIB_FUNCTION("RlhJVAYLSqU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcHeapUnsetTraceMarker);
    LIB_FUNCTION("YrL-1y6vfyo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcInternalMemoryGetWakeAddr);
    LIB_FUNCTION("h8jq9ee4h5c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcInternalMemoryMutexEnable);
    LIB_FUNCTION("LXqt47GvaRA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcInternalSetMallocCallback);
    LIB_FUNCTION("HmgKoOWpUc8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sceLibcOnce);
    LIB_FUNCTION("2g5wco7AAHg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_seed48);
    LIB_FUNCTION("7WoI+lVawlc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_seekdir);
    LIB_FUNCTION("ENLfKJEZTjE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_set_constraint_handler_s);
    LIB_FUNCTION("vZMcAfsA31I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_setbuf);
    LIB_FUNCTION("M4YYbSFfJ8g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_setenv);
    LIB_FUNCTION("gNQ1V2vfXDE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_setjmp);
    LIB_FUNCTION("PtsB1Q9wsFA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_setlocale);
    LIB_FUNCTION("woQta4WRpk0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_setstate);
    LIB_FUNCTION("QMFyLoqNxIg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_setvbuf);
    LIB_FUNCTION("Bm3k7JQMN5w", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sigblock);
    LIB_FUNCTION("TsrS8nGDQok", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_siginterrupt);
    LIB_FUNCTION("SQGxZCv3aYk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_signalcontext);
    LIB_FUNCTION("5gOOC0kzW0c", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_signgam);
    LIB_FUNCTION("Az3tTyAy380", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_significand);
    LIB_FUNCTION("L2YaHYQdmHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_significandf);
    LIB_FUNCTION("cJvOg1KV8uc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sigsetmask);
    LIB_FUNCTION("yhxKO9LYc8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sigvec);
    LIB_FUNCTION("+KSnjvZ0NMc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_srand48);
    LIB_FUNCTION("sPC7XE6hfFY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_srandom);
    LIB_FUNCTION("a2MOZf++Wjg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_srandomdev);
    LIB_FUNCTION("ayTeobcoGj8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_statvfs);
    LIB_FUNCTION("+CUrIMpVuaM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_stderr);
    LIB_FUNCTION("omQZ36ESr98", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_stdin);
    LIB_FUNCTION("3eGXiXpFLt0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_stdout);
    LIB_FUNCTION("ZSnX-xZBGCg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_stpcpy);
    LIB_FUNCTION("ZD+Dp+-LsGg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sys_nsig);
    LIB_FUNCTION("yCdGspbNHZ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sys_siglist);
    LIB_FUNCTION("Y16fu+FC+3Q", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_sys_signame);
    LIB_FUNCTION("UNS2V4S097M", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_syslog);
    LIB_FUNCTION("RZA5RZawY04", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_telldir);
    LIB_FUNCTION("b7J3q7-UABY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_tgamma);
    LIB_FUNCTION("B2ZbqV9geCM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_tgammaf);
    LIB_FUNCTION("FU03r76UxaU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_tgammal);
    LIB_FUNCTION("wLlFkwG9UcQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_time);
    LIB_FUNCTION("-Oet9AHzwtU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_timezone);
    LIB_FUNCTION("PqF+kHW-2WQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_tolower);
    LIB_FUNCTION("TYE4irxSmko", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_toupper);
    LIB_FUNCTION("BEKIcbCV-MU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_towctrans);
    LIB_FUNCTION("J3J1T9fjUik", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_towlower);
    LIB_FUNCTION("1uf1SQsj5go", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_towupper);
    LIB_FUNCTION("a4gLGspPEDM", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_trunc);
    LIB_FUNCTION("Vo8rvWtZw3g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_truncf);
    LIB_FUNCTION("apdxz6cLMh8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_truncl);
    LIB_FUNCTION("BAYjQubSZT8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_tzname);
    LIB_FUNCTION("gYFKAMoNEfo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_tzset);
    LIB_FUNCTION("-LFO7jhD5CE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ungetc);
    LIB_FUNCTION("Nz7J62MvgQs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_ungetwc);
    LIB_FUNCTION("CRJcH8CnPSI", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_unsetenv);
    LIB_FUNCTION("1nTKA7pN1jw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_utime);
    LIB_FUNCTION("aoTkxU86Mr4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_verr);
    LIB_FUNCTION("7Pc0nOTw8po", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_verrc);
    LIB_FUNCTION("ItC2hTrYvHw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_verrx);
    LIB_FUNCTION("zxecOOffO68", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vsyslog);
    LIB_FUNCTION("s67G-KeDKOo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vwarn);
    LIB_FUNCTION("BfAsxVvQVTQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vwarnc);
    LIB_FUNCTION("iH+oMJn8YPk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_vwarnx);
    LIB_FUNCTION("3Rhy2gXDhwc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_warn);
    LIB_FUNCTION("AqUBdZqHZi4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_warnc);
    LIB_FUNCTION("aNJaYyn0Ujo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_warnx);
    LIB_FUNCTION("y9OoA+P5cjk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcrtomb);
    LIB_FUNCTION("oAlR5z2iiCA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcrtomb_s);
    LIB_FUNCTION("KZm8HUIX2Rw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcscat);
    LIB_FUNCTION("MqeMaVUiyU8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcscat_s);
    LIB_FUNCTION("Ezzq78ZgHPs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcschr);
    LIB_FUNCTION("pNtJdE3x49E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcscmp);
    LIB_FUNCTION("fV2xHER+bKE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcscoll);
    LIB_FUNCTION("FM5NPnLqBc8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcscpy);
    LIB_FUNCTION("6f5f-qx4ucA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcscpy_s);
    LIB_FUNCTION("7eNus40aGuk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcscspn);
    LIB_FUNCTION("XbVXpf5WF28", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsftime);
    LIB_FUNCTION("WkkeywLJcgU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcslen);
    LIB_FUNCTION("pA9N3VIgEZ4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsncat);
    LIB_FUNCTION("VxG0990tP3E", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsncat_s);
    LIB_FUNCTION("E8wCoUEbfzk", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsncmp);
    LIB_FUNCTION("0nV21JjYCH8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsncpy);
    LIB_FUNCTION("Slmz4HMpNGs", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsncpy_s);
    LIB_FUNCTION("K+v+cnmGoH4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsnlen_s);
    LIB_FUNCTION("H4MCONF+Gps", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcspbrk);
    LIB_FUNCTION("g3ShSirD50I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsrchr);
    LIB_FUNCTION("sOOMlZoy1pg", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsrtombs);
    LIB_FUNCTION("79s2tnYQI6I", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsrtombs_s);
    LIB_FUNCTION("x9uumWcxhXU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsspn);
    LIB_FUNCTION("7-a7sBHeUQ8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstod);
    LIB_FUNCTION("7SXNu+0KBYQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstof);
    LIB_FUNCTION("ljFisaQPwYQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstoimax);
    LIB_FUNCTION("8ngzWNZzFJU", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstok);
    LIB_FUNCTION("dsXnVxORFdc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstok_s);
    LIB_FUNCTION("d3dMyWORw8A", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstol);
    LIB_FUNCTION("LEbYWl9rBc8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstold);
    LIB_FUNCTION("34nH7v2xvNQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstoll);
    LIB_FUNCTION("v7S7LhP2OJc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstombs);
    LIB_FUNCTION("sZLrjx-yEx4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstombs_s);
    LIB_FUNCTION("5AYcEn7aoro", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstoul);
    LIB_FUNCTION("DAbZ-Vfu6lQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstoull);
    LIB_FUNCTION("1e-q5iIukH8", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcstoumax);
    LIB_FUNCTION("VuMMb5CfpEw", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wcsxfrm);
    LIB_FUNCTION("CL7VJxznu6g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wctob);
    LIB_FUNCTION("7PxmvOEX3oc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wctomb);
    LIB_FUNCTION("y3V0bIq38NE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wctomb_s);
    LIB_FUNCTION("seyrqIc4ovc", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wctrans);
    LIB_FUNCTION("+3PtYiUxl-U", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_wctype);
    LIB_FUNCTION("inwDBwEvw18", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_xtime_get);
    LIB_FUNCTION("RvsFE8j3C38", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_y0);
    LIB_FUNCTION("+tfKv1vt0QQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_y0f);
    LIB_FUNCTION("vh9aGR3ALP0", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_y1);
    LIB_FUNCTION("gklG+J87Pq4", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_y1f);
    LIB_FUNCTION("eWSt5lscApo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_yn);
    LIB_FUNCTION("wdPaII721tY", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1, internal_ynf);
    LIB_FUNCTION("GG6441JdYkA", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_Func_186EB8E3525D6240);
    LIB_FUNCTION("QZ9YgTk+yrE", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_Func_419F5881393ECAB1);
    LIB_FUNCTION("bGuDd3kWVKQ", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_Func_6C6B8377791654A4);
    LIB_FUNCTION("f9LVyN8Ky8g", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_Func_7FD2D5C8DF0ACBC8);
    LIB_FUNCTION("wUqJ0psUjDo", "libSceLibcInternal", 1, "libSceLibcInternal", 1, 1,
                 internal_Func_C14A89D29B148C3A);
};

} // namespace Libraries::LibcInternal