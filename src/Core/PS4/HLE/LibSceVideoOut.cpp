#include "LibSceVideoOut.h"

#include <debug.h>

#include "../../../Util/Log.h"
#include "../Loader/Elf.h"
#include "Graphics/video_out.h"
#include "Libs.h"
#include "UserManagement/UsrMngCodes.h"
#include "VideoOut/VideoOutCodes.h"

namespace HLE::Libs::LibSceVideoOut {

void LibSceVideoOut_Register(SymbolsResolver* sym) {
    LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutGetFlipStatus);
    LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutSubmitFlip);
    LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutRegisterBuffers);
    LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutAddFlipEvent);
    LIB_FUNCTION("CBiu4mCE1DA", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutSetFlipRate);
    LIB_FUNCTION("i6-sR91Wt-4", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutSetBufferAttribute);
    LIB_FUNCTION("6kPnj51T62Y", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutGetResolutionStatus);
    LIB_FUNCTION("Up36PTk687E", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutOpen);
    LIB_FUNCTION("zgXifHT9ErY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutIsFlipPending);
}

};  // namespace HLE::Libs::LibSceVideoOut