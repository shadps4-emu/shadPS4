#include "LibC.h"

#include <debug.h>
#include <pthread.h>

#include "../Loader/Elf.h"
#include "Emulator/HLE/Libraries/LibC/libc.h"
#include "Emulator/HLE/Libraries/LibC/libc_cxa.h"
#include "ErrorCodes.h"
#include "Libs.h"

namespace HLE::Libs::LibC {

static u32 g_need_sceLibc = 1;

static PS4_SYSV_ABI void init_env()  // every game/demo should probably
{
    // dummy no need atm
}

static PS4_SYSV_ABI void catchReturnFromMain(int status) {
    // dummy
}

static PS4_SYSV_ABI void _Assert() { BREAKPOINT(); }

PS4_SYSV_ABI int puts(const char* s) {
    std::puts(s);
    return SCE_OK;
}

PS4_SYSV_ABI int rand() { return std::rand(); }

PS4_SYSV_ABI void _ZdlPv(void* ptr) { std::free(ptr); }
PS4_SYSV_ABI void _ZSt11_Xbad_allocv() { BREAKPOINT(); }
PS4_SYSV_ABI void _ZSt14_Xlength_errorPKc() { BREAKPOINT(); }
PS4_SYSV_ABI void* _Znwm(u64 count) {
    if (count == 0) {
        BREAKPOINT();
    }
    void* ptr = std::malloc(count);
    return ptr;
}

float PS4_SYSV_ABI _Fsin(float arg) { return std::sinf(arg); }

typedef int(PS4_SYSV_ABI* pfunc_QsortCmp)(const void*, const void*);
thread_local static pfunc_QsortCmp compair_ps4;

int qsort_compair(const void* arg1, const void* arg2) { return compair_ps4(arg1, arg2); }

void PS4_SYSV_ABI qsort(void* ptr, size_t count,size_t size, int(PS4_SYSV_ABI* comp)(const void*, const void*)) {
    compair_ps4 = comp;
    std::qsort(ptr, count, size, qsort_compair);
}


void LibC_Register(SymbolsResolver* sym) {
    LIB_FUNCTION("bzQExy189ZI", "libc", 1, "libc", 1, 1, init_env);
    LIB_FUNCTION("3GPpjQdAMTw", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::Cxa::__cxa_guard_acquire);
    LIB_FUNCTION("9rAeANT2tyE", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::Cxa::__cxa_guard_release);
    LIB_FUNCTION("2emaaluWzUw", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::Cxa::__cxa_guard_abort);
    LIB_FUNCTION("DfivPArhucg", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::memcmp);
    LIB_FUNCTION("Q3VBxCXhUHs", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::memcpy);
    LIB_FUNCTION("8zTFvBIAIN8", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::memset);
    LIB_FUNCTION("XKRegsFpEpk", "libc", 1, "libc", 1, 1, catchReturnFromMain);
    LIB_FUNCTION("uMei1W9uyNo", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::exit);
    LIB_FUNCTION("8G2LB+A3rzg", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::atexit);
    LIB_FUNCTION("-QgqOT5u2Vk", "libc", 1, "libc", 1, 1, _Assert);
    LIB_FUNCTION("hcuQgD53UxM", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::printf);
    LIB_FUNCTION("Q2V+iqvjgC0", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::vsnprintf);
    LIB_FUNCTION("YQ0navp+YIc", "libc", 1, "libc", 1, 1, puts);
    LIB_FUNCTION("cpCOXWMgha0", "libc", 1, "libc", 1, 1, rand);
    LIB_FUNCTION("ZtjspkJQ+vw", "libc", 1, "libc", 1, 1, _Fsin);
    LIB_FUNCTION("AEJdIVZTEmo", "libc", 1, "libc", 1, 1, qsort);
    LIB_FUNCTION("Ovb2dSJOAuE", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::strcmp);
    LIB_FUNCTION("gQX+4GDQjpM", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::malloc);
    LIB_FUNCTION("tIhsqj0qsFE", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::free);
    LIB_FUNCTION("j4ViWNHEgww", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::strlen);
    LIB_FUNCTION("6sJWiWSRuqk", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::strncpy);
    LIB_FUNCTION("+P6FRGH4LfA", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::memmove);
    LIB_FUNCTION("EH-x713A99c", "libc", 1, "libc", 1, 1, Emulator::HLE::Libraries::LibC::atan2f);
    LIB_OBJ("P330P3dFF68", "libc", 1, "libc", 1, 1, &HLE::Libs::LibC::g_need_sceLibc);

    LIB_FUNCTION("z+P+xCnWLBk", "libc", 1, "libc", 1, 1, _ZdlPv);
    LIB_FUNCTION("eT2UsmTewbU", "libc", 1, "libc", 1, 1, _ZSt11_Xbad_allocv);
    LIB_FUNCTION("tQIo+GIPklo", "libc", 1, "libc", 1, 1, _ZSt14_Xlength_errorPKc);
    LIB_FUNCTION("fJnpuVVBbKk", "libc", 1, "libc", 1, 1, _Znwm);
}

};  // namespace HLE::Libs::LibC