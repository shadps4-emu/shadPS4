#include "Libs.h"

#include "LibC.h"
#include "LibKernel.h"
#include "LibSceGnmDriver.h"
#include <Core/PS4/HLE/Graphics/video_out.h>
#include "Emulator/HLE/Libraries/LibUserService/user_service.h"
#include "Emulator/HLE/Libraries/LibPad/pad.h"
#include <Emulator/HLE/Libraries/LibSystemService/system_service.h>

namespace HLE::Libs {

void Init_HLE_Libs(SymbolsResolver *sym) {
    LibC::LibC_Register(sym);
    LibKernel::LibKernel_Register(sym);
    Graphics::VideoOut::videoOutRegisterLib(sym);
    LibSceGnmDriver::LibSceGnmDriver_Register(sym);
    Emulator::HLE::Libraries::LibUserService::libUserService_Register(sym);
    Emulator::HLE::Libraries::LibPad::libPad_Register(sym);
    Emulator::HLE::Libraries::LibSystemService::libSystemService_Register(sym);
}
}  // namespace HLE::Libs