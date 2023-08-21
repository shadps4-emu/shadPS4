#include "LibSceVideoOut.h"

#include <debug.h>

#include "../../../Util/Log.h"
#include "../Loader/Elf.h"
#include "Libs.h"
#include "UserManagement/UsrMngCodes.h"
#include "VideoOut/VideoOutCodes.h"
#include "Graphics/video_out.h"

namespace HLE::Libs::LibSceVideoOut {

s32 PS4_SYSV_ABI sceVideoOutOpen(SceUserServiceUserId userId, s32 busType, s32 index, const void* param) {
    PRINT_DUMMY_FUNCTION_NAME();
    if (userId != SCE_USER_SERVICE_USER_ID_SYSTEM) {
        BREAKPOINT();
    }
    if (busType != SCE_VIDEO_OUT_BUS_TYPE_MAIN) {
        BREAKPOINT();
    }
    if (index != 0) {
        BREAKPOINT();
    }
    if (param != nullptr) {
        BREAKPOINT();
    }
    return 1;  // dummy return TODO
}

void LibSceVideoOut_Register(SymbolsResolver* sym) {
    LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutGetFlipStatus);
    LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutSubmitFlip);
    LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutRegisterBuffers);
    LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutAddFlipEvent);
    LIB_FUNCTION("CBiu4mCE1DA", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutSetFlipRate);
    LIB_FUNCTION("i6-sR91Wt-4", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutSetBufferAttribute);
    LIB_FUNCTION("6kPnj51T62Y", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutGetResolutionStatus);
    LIB_FUNCTION("Up36PTk687E", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutOpen);
    LIB_FUNCTION("zgXifHT9ErY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutIsFlipPending);
}

};  // namespace HLE::Libs::LibSceVideoOut