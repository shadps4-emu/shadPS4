#include "LibSceVideoOut.h"
#include "Libs.h"
#include "../Loader/Elf.h"
#include "../../../Debug.h"
#include "VideoOut/VideoOutCodes.h"
#include "UserManagement/UsrMngCodes.h"
#include "../../../Util/Log.h"

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
		//BREAKPOINT();
        LOG_INFO_IF(true, "dummy sceVideoOutAddFlipEvent\n");
        return 0;
	}
    int32_t PS4_SYSV_ABI sceVideoOutSetFlipRate(int32_t handle, int32_t rate) { 
		BREAKPOINT();
        return 0;
	}
    void PS4_SYSV_ABI sceVideoOutSetBufferAttribute(/* SceVideoOutBufferAttribute* attribute,*/ uint32_t pixelFormat, uint32_t tilingMode,
                                                    uint32_t aspectRatio,
		uint32_t width, uint32_t height, uint32_t pitchInPixel)
	{
        BREAKPOINT();
	}
    int32_t PS4_SYSV_ABI sceVideoOutGetResolutionStatus(int32_t handle /*, SceVideoOutResolutionStatus* status*/) {
        BREAKPOINT();
        return 0;
	}
    

    s32 PS4_SYSV_ABI sceVideoOutOpen(SceUserServiceUserId userId, s32 busType, s32 index, const void* param) 
	{ 
        if (userId != SCE_USER_SERVICE_USER_ID_SYSTEM)
        {
            BREAKPOINT();
        }
		if (busType != SCE_VIDEO_OUT_BUS_TYPE_MAIN)
		{
            BREAKPOINT();
		}
		if (index != 0)
		{
            BREAKPOINT();
		}
		if (param != nullptr)
		{
            BREAKPOINT();
		}
        LOG_INFO_IF(true, "dummy sceVideoOutOpen\n");
		return 1;//dummy return TODO
	}
    int32_t PS4_SYSV_ABI sceVideoOutIsFlipPending(int32_t handle) {
        BREAKPOINT();
        return 0;
	}
	void LibSceVideoOut_Register(SymbolsResolver* sym)
	{
        LIB_FUNCTION("SbU3dwp80lQ", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutGetFlipStatus); 
		LIB_FUNCTION("U46NwOiJpys", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSubmitFlip);
        LIB_FUNCTION("w3BY+tAEiQY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutRegisterBuffers);
        LIB_FUNCTION("HXzjK9yI30k", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutAddFlipEvent);
        LIB_FUNCTION("CBiu4mCE1DA", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSetFlipRate);
        LIB_FUNCTION("i6-sR91Wt-4", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutSetBufferAttribute);
        LIB_FUNCTION("6kPnj51T62Y", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutGetResolutionStatus);
        LIB_FUNCTION("Up36PTk687E", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutOpen);
        LIB_FUNCTION("zgXifHT9ErY", "libSceVideoOut", 1, "libSceVideoOut", 0, 0, sceVideoOutIsFlipPending);
	}

};