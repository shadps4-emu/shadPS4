#include "libc.h"

#include <debug.h>
#include <stdlib.h>

#include "Emulator/Util/singleton.h"
#include "Util/log.h"
#include "core/PS4/HLE/Libs.h"
#include "core/hle/libraries/libc/libc.h"
#include "core/hle/libraries/libc/libc_cxa.h"
#include "core/hle/libraries/libc/libc_math.h"
#include "core/hle/libraries/libc/libc_stdio.h"
#include "core/hle/libraries/libc/libc_stdlib.h"
#include "core/hle/libraries/libc/libc_string.h"

namespace Core::Libraries::LibC {
constexpr bool log_file_libc = true;  // disable it to disable logging
static u32 g_need_sceLibc = 1;

using cxa_destructor_func_t = void (*)(void*);

struct CxaDestructor {
    cxa_destructor_func_t destructor_func;
    void* destructor_object;
    void* module_id;
};

struct CContext {
    std::vector<CxaDestructor> cxa;
};

static PS4_SYSV_ABI int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle) {
    auto* cc = singleton<CContext>::instance();
    CxaDestructor c{};
    c.destructor_func = func;
    c.destructor_object = arg;
    c.module_id = dso_handle;
    cc->cxa.push_back(c);
    return 0;
}

void PS4_SYSV_ABI __cxa_finalize(void* d) { __debugbreak(); }

void PS4_SYSV_ABI __cxa_pure_virtual() { __debugbreak(); }

static PS4_SYSV_ABI void init_env() { PRINT_DUMMY_FUNCTION_NAME(); }

static PS4_SYSV_ABI void catchReturnFromMain(int status) { LOG_INFO_IF(log_file_libc, "catchReturnFromMain returned ={}\n", status); }

static PS4_SYSV_ABI void _Assert() {
    PRINT_DUMMY_FUNCTION_NAME();
    BREAKPOINT();
}

PS4_SYSV_ABI void _ZdlPv(void* ptr) { std::free(ptr); }
PS4_SYSV_ABI void _ZSt11_Xbad_allocv() {
    PRINT_DUMMY_FUNCTION_NAME();
    BREAKPOINT();
}
PS4_SYSV_ABI void _ZSt14_Xlength_errorPKc() {
    PRINT_DUMMY_FUNCTION_NAME();
    BREAKPOINT();
}
PS4_SYSV_ABI void* _Znwm(u64 count) {
    if (count == 0) {
        LOG_ERROR_IF(log_file_libc, "_Znwm count ={}\n", count);
        BREAKPOINT();
    }
    void* ptr = std::malloc(count);
    return ptr;
}

void libcSymbolsRegister(SymbolsResolver* sym) {
    // cxa functions
    LIB_FUNCTION("3GPpjQdAMTw", "libc", 1, "libc", 1, 1, __cxa_guard_acquire);
    LIB_FUNCTION("9rAeANT2tyE", "libc", 1, "libc", 1, 1, __cxa_guard_release);
    LIB_FUNCTION("2emaaluWzUw", "libc", 1, "libc", 1, 1, __cxa_guard_abort);

    // stdlib functions
    LIB_FUNCTION("uMei1W9uyNo", "libc", 1, "libc", 1, 1, exit);
    LIB_FUNCTION("8G2LB+A3rzg", "libc", 1, "libc", 1, 1, atexit);
    LIB_FUNCTION("gQX+4GDQjpM", "libc", 1, "libc", 1, 1, malloc);
    LIB_FUNCTION("tIhsqj0qsFE", "libc", 1, "libc", 1, 1, free);
    LIB_FUNCTION("cpCOXWMgha0", "libc", 1, "libc", 1, 1, rand);
    LIB_FUNCTION("AEJdIVZTEmo", "libc", 1, "libc", 1, 1, qsort);

    // math functions
    LIB_FUNCTION("EH-x713A99c", "libc", 1, "libc", 1, 1, atan2f);
    LIB_FUNCTION("QI-x0SL8jhw", "libc", 1, "libc", 1, 1, acosf);
    LIB_FUNCTION("ZE6RNL+eLbk", "libc", 1, "libc", 1, 1, tanf);
    LIB_FUNCTION("GZWjF-YIFFk", "libc", 1, "libc", 1, 1, asinf);
    LIB_FUNCTION("9LCjpWyQ5Zc", "libc", 1, "libc", 1, 1, pow);
    LIB_FUNCTION("cCXjU72Z0Ow", "libc", 1, "libc", 1, 1, _Sin);
    LIB_FUNCTION("ZtjspkJQ+vw", "libc", 1, "libc", 1, 1, _Fsin);
    LIB_FUNCTION("dnaeGXbjP6E", "libc", 1, "libc", 1, 1, exp2);

    // string functions
    LIB_FUNCTION("Ovb2dSJOAuE", "libc", 1, "libc", 1, 1, strcmp);
    LIB_FUNCTION("j4ViWNHEgww", "libc", 1, "libc", 1, 1, strlen);
    LIB_FUNCTION("6sJWiWSRuqk", "libc", 1, "libc", 1, 1, strncpy);
    LIB_FUNCTION("+P6FRGH4LfA", "libc", 1, "libc", 1, 1, memmove);
    LIB_FUNCTION("kiZSXIWd9vg", "libc", 1, "libc", 1, 1, strcpy);
    LIB_FUNCTION("Ls4tzzhimqQ", "libc", 1, "libc", 1, 1, strcat);
    LIB_FUNCTION("DfivPArhucg", "libc", 1, "libc", 1, 1, memcmp);
    LIB_FUNCTION("Q3VBxCXhUHs", "libc", 1, "libc", 1, 1, memcpy);
    LIB_FUNCTION("8zTFvBIAIN8", "libc", 1, "libc", 1, 1, memset);

    // stdio functions
    LIB_FUNCTION("hcuQgD53UxM", "libc", 1, "libc", 1, 1, printf);
    LIB_FUNCTION("Q2V+iqvjgC0", "libc", 1, "libc", 1, 1, vsnprintf);
    LIB_FUNCTION("YQ0navp+YIc", "libc", 1, "libc", 1, 1, puts);

    // misc
    LIB_OBJ("P330P3dFF68", "libc", 1, "libc", 1, 1, &g_need_sceLibc);
    LIB_FUNCTION("bzQExy189ZI", "libc", 1, "libc", 1, 1, init_env);
    LIB_FUNCTION("XKRegsFpEpk", "libc", 1, "libc", 1, 1, catchReturnFromMain);
    LIB_FUNCTION("-QgqOT5u2Vk", "libc", 1, "libc", 1, 1, _Assert);
    LIB_FUNCTION("z+P+xCnWLBk", "libc", 1, "libc", 1, 1, _ZdlPv);
    LIB_FUNCTION("eT2UsmTewbU", "libc", 1, "libc", 1, 1, _ZSt11_Xbad_allocv);
    LIB_FUNCTION("tQIo+GIPklo", "libc", 1, "libc", 1, 1, _ZSt14_Xlength_errorPKc);
    LIB_FUNCTION("fJnpuVVBbKk", "libc", 1, "libc", 1, 1, _Znwm);
    LIB_FUNCTION("tsvEmnenz48", "libc", 1, "libc", 1, 1, __cxa_atexit);
    LIB_FUNCTION("H2e8t5ScQGc", "libc", 1, "libc", 1, 1, __cxa_finalize);
    LIB_FUNCTION("zr094EQ39Ww", "libc", 1, "libc", 1, 1, __cxa_pure_virtual);
}

};  // namespace Core::Libraries::LibC