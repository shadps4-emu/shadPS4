#pragma once
#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibC {

	void LibC_Register(SymbolsResolver* sym);

	//functions
    static PS4_SYSV_ABI void init_env();
    static PS4_SYSV_ABI void _Assert();
    static PS4_SYSV_ABI void catchReturnFromMain(int status);
    int PS4_SYSV_ABI __cxa_guard_acquire(u64* guard_object);
    void PS4_SYSV_ABI __cxa_guard_release(u64* guard_object); 
   
};