#include "common/log.h"
#include "core/PS4/GPU/gpu_memory.h"
#include "core/hle/libraries/libscegnmdriver/libscegnmdriver.h"
#include "core/hle/libraries/libs.h"
#include "emulator.h"

namespace Core::Libraries::LibSceGnmDriver {

int32_t sceGnmSubmitDone() {
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}

void sceGnmFlushGarlic() {
    PRINT_FUNCTION_NAME();
    GPU::flushGarlic(Emu::getGraphicCtx());
}

void LibSceGnmDriver_Register(Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("yvZ73uQUqrk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSubmitDone);
    LIB_FUNCTION("iBt3Oe00Kvc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmFlushGarlic);
}
    
};
