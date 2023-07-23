#pragma once
#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibC {

	void LibC_Register(SymbolsResolver* sym);

	//functions
    static PS4_SYSV_ABI void init_env();
    static PS4_SYSV_ABI void exit(int code);
    static PS4_SYSV_ABI void catchReturnFromMain(int status);
    int PS4_SYSV_ABI __cxa_guard_acquire(u64* guard_object);
    int PS4_SYSV_ABI memcmp(const void* s1, const void* s2, size_t n);
    void* PS4_SYSV_ABI memcpy(void* dest, const void* src, size_t n);

};