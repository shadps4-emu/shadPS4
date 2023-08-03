#include "Libs.h"

#include "LibC.h"
#include "LibKernel.h"
#include "LibSceGnmDriver.h"
#include "LibSceVideoOut.h"

namespace HLE::Libs {

void Init_HLE_Libs(SymbolsResolver *sym) {
    LibC::LibC_Register(sym);
    LibKernel::LibKernel_Register(sym);
    LibSceVideoOut::LibSceVideoOut_Register(sym);
    LibSceGnmDriver::LibSceGnmDriver_Register(sym);
}
}  // namespace HLE::Libs