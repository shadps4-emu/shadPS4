#include "LibSceGnmDriver.h"
#include "Libs.h"
#include "../Loader/Elf.h"
#include <Util/log.h>
#include <debug.h>
#include <Core/PS4/GPU/gpu_memory.h>
#include <emulator.h>

namespace HLE::Libs::LibSceGnmDriver {

    int32_t sceGnmSubmitDone()
    { 
        PRINT_DUMMY_FUNCTION_NAME();
        return 0;
    }

    void sceGnmFlushGarlic() { PRINT_FUNCTION_NAME();
        GPU::flushGarlic(Emu::getGraphicCtx());
    }

	void LibSceGnmDriver_Register(SymbolsResolver* sym)
	{ 
        LIB_FUNCTION("yvZ73uQUqrk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSubmitDone);
        LIB_FUNCTION("iBt3Oe00Kvc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmFlushGarlic);
	}
    
};