#include "Libs.h"

#include "LibC.h"
#include "LibKernel.h"
#include "LibSceGnmDriver.h"
#include <Core/PS4/HLE/Graphics/video_out.h>

namespace HLE::Libs {

void Init_HLE_Libs(SymbolsResolver *sym) {
    LibC::LibC_Register(sym);
    LibKernel::LibKernel_Register(sym);
    Graphics::VideoOut::videoOutRegisterLib(sym);
    LibSceGnmDriver::LibSceGnmDriver_Register(sym);
}
}  // namespace HLE::Libs