#include "LibSceVideoOut.h"

#include <debug.h>

#include "../../../Util/Log.h"
#include "../Loader/Elf.h"
#include "Libs.h"
#include "UserManagement/UsrMngCodes.h"
#include "VideoOut/VideoOutCodes.h"
#include "Graphics/video_out.h"

namespace HLE::Libs::LibSceVideoOut {

int32_t PS4_SYSV_ABI sceVideoOutGetFlipStatus(int32_t handle /*, SceVideoOutFlipStatus* status*/) {
    BREAKPOINT();
    return 0;
}

int32_t PS4_SYSV_ABI sceVideoOutSubmitFlip(int32_t handle, int32_t bufferIndex, int32_t flipMode, int64_t flipArg) {
    BREAKPOINT();
    return 0;
}
int32_t PS4_SYSV_ABI sceVideoOutRegisterBuffers(int32_t handle, int32_t startIndex, void* const* addresses, int32_t bufferNum /*,
                                       const SceVideoOutBufferAttribute* attribute*/) {
    BREAKPOINT();
    return 0;
}
int32_t PS4_SYSV_ABI sceVideoOutAddFlipEvent(/*SceKernelEqueue eq,*/ int32_t handle, void* udata) {
    // BREAKPOINT();
    PRINT_DUMMY_FUNCTION_NAME();
    return 0;
}
int32_t PS4_SYSV_ABI sceVideoOutSetFlipRate(int32_t handle, int32_t rate) {
    BREAKPOINT();
    return 0;
}
int32_t PS4_SYSV_ABI sceVideoOutGetResolutionStatus(int32_t handle /*, SceVideoOutResolutionStatus* status*/) {
    BREAKPOINT();
    return 0;
}

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
int32_t PS4_SYSV_ABI sceVideoOutIsFlipPending(int32_t handle) {
    BREAKPOINT();
    return 0;
}
void LibSceVideoOut_Register(SymbolsResolver* sym) {
    LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutGetFlipStatus);
    LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSubmitFlip);
    LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutRegisterBuffers);
    LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutAddFlipEvent);
    LIB_FUNCTION("CBiu4mCE1DA", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSetFlipRate);
    LIB_FUNCTION("i6-sR91Wt-4", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, HLE::Libs::Graphics::VideoOut::sceVideoOutSetBufferAttribute);
    LIB_FUNCTION("6kPnj51T62Y", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutGetResolutionStatus);
    LIB_FUNCTION("Up36PTk687E", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutOpen);
    LIB_FUNCTION("zgXifHT9ErY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutIsFlipPending);
}

};  // namespace HLE::Libs::LibSceVideoOut