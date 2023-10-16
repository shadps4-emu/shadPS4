#pragma once
#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibC {

	void LibC_Register(SymbolsResolver* sym);

	//functions
    static PS4_SYSV_ABI void init_env();
    static PS4_SYSV_ABI void _Assert();
    static PS4_SYSV_ABI void catchReturnFromMain(int status);
   
};