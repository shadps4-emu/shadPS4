#include "LibSceVideoOut.h"
#include "Libs.h"
#include "../Loader/Elf.h"

namespace HLE::Libs::LibSceVideoOut {

	int32_t sceVideoOutGetFlipStatus(int32_t handle /*, SceVideoOutFlipStatus* status*/){ 
		return 0;
	}

	int32_t sceVideoOutSubmitFlip(int32_t handle, int32_t bufferIndex, int32_t flipMode,int64_t flipArg){ 
		return 0;
	}
    int32_t sceVideoOutRegisterBuffers(int32_t handle, int32_t startIndex, void* const* addresses, int32_t bufferNum /*,
                                       const SceVideoOutBufferAttribute* attribute*/) {
        return 0;
	}
    int32_t sceVideoOutAddFlipEvent(/*SceKernelEqueue eq,*/ int32_t handle, void* udata) { 
		return 0;
	}
	int32_t sceVideoOutSetFlipRate(int32_t handle, int32_t rate) { 
		return 0;
	}
    void sceVideoOutSetBufferAttribute(/* SceVideoOutBufferAttribute* attribute,*/ uint32_t pixelFormat, uint32_t tilingMode, uint32_t aspectRatio,
		uint32_t width, uint32_t height, uint32_t pitchInPixel)
	{

	}
    int32_t sceVideoOutGetResolutionStatus(int32_t handle /*, SceVideoOutResolutionStatus* status*/)
	{ return 0;
	}
    int32_t sceVideoOutOpen(/* SceUserServiceUserId userId,*/ int32_t busType, int32_t index, const void* param) { return 0;
	}
	int32_t sceVideoOutIsFlipPending(int32_t handle) { return 0;
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