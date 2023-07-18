#include "LibSceGnmDriver.h"
#include "Libs.h"
#include "../Loader/Elf.h"

namespace HLE::Libs::LibSceGnmDriver {

	int sceGnmAddEqEvent(/* SceKernelEqueue eq, EqEventType id,*/ void* udata)
	{ return 0;
	}
	bool sceGnmAreSubmitsAllowed()
	{ 
		return true;
	}

	int /* WorkloadStatus*/ sceGnmBeginWorkload(uint64_t* workload /*, WorkloadStream stream*/)
	{ 
		return 0;
	}

	int /* WorkloadStatus*/ sceGnmCreateWorkloadStream(/* WorkloadStream* workloadStream,*/ const char* name)
	{ 
		return 0;
	}
    void sceGnmDebugHardwareStatus(/* HardwareStatus flag*/) {

	}
    void sceGnmSetGsRingSizes(/* GsRingSizeSetup esgsRingSize, GsRingSizeSetup gsvsRingSize*/)
	{

	}
    int32_t sceGnmSetWaveLimitMultipliers(uint16_t targetPipeMask, uint8_t gfxRatio, const uint8_t (*pipeRatios)[7])
	{ return 0;
    }

    int /*MipStatsError*/ sceGnmSetupMipStatsReport(void* outputBuffer, uint32_t sizeInBytes, uint8_t intervalsBetweenReports,
                                              uint8_t numReportsBeforeReset /*, MipStatsResetForce mipStatsResetForce*/)
    {
        return 0;
    }

    int sceGnmSubmitCommandBuffers(uint32_t count, void* dcb_gpu_addrs[], const uint32_t* dcb_sizes_in_bytes, void* ccb_gpu_addrs[],
        const uint32_t* ccb_sizes_in_bytes)
    {
        return 0;
    }

    int sceGnmSubmitAndFlipCommandBuffers(uint32_t count, void* dcb_gpu_addrs[], const uint32_t* dcb_sizes_in_bytes,
        void* ccb_gpu_addrs[], const uint32_t* ccb_sizes_in_bytes, int handle, int index,
        int flip_mode, int64_t flip_arg)
    {
        return 0;
    }
    void sceGnmDingDong(u32 ring_id, u32 offset_dw)
    {
    
    }
    bool sceRazorIsLoaded()
    { return true;// hmm???
    }
    int sceGnmDeleteEqEvent(/* SceKernelEqueue eq, EqEventType id*/)
    { return 0;
    }
    int32_t sceGnmSubmitDone()
    { return 0;
    }
    int /* MipStatsError*/ sceGnmDisableMipStatsReport()
    { return 0;
    }
    int sceGnmSubmitAndFlipCommandBuffersForWorkload()
    { return 0;
    }
    int sceGnmSubmitCommandBuffersForWorkload()
    { return 0;
    }
    int /* WorkloadStatus*/ sceGnmDestroyWorkloadStream(/*WorkloadStream workloadStream*/)
    { return 0;
    }
    void sceGnmDingDongForWorkload()
    {

    }
    void sceGnmDriverCaptureInProgress() {}
    void sceGnmUnmapComputeQueue(){}
    void sceGnmDriverTraceInProgress(){}
    void sceGnmDriverTriggerCapture(){}
    void sceGnmEndWorkload(){}
    void sceGnmFlushGarlic(){}
    void sceGnmGetEqEventType(){}
    void sceGnmGetEqTimeStamp(){}
    void sceGnmGetGpuBlockStatus(){}
    void sceGnmGetGpuInfoStatus(){}
    void sceGnmGetLastWaitedAddress(){}
    void sceGnmGetNumTcaUnits(){}
    void sceGnmGetOffChipTessellationBufferSize(){}
    void sceGnmGetPhysicalCounterFromVirtualized(){}
    void sceGnmGetProtectionFaultTimeStamp(){}
    void sceGnmGetShaderProgramBaseAddress(){}
    void sceGnmGetShaderStatus(){}
    void sceGnmGetTheTessellationFactorRingBufferBaseAddress(){}
    void sceGnmIsUserPaEnabled(){}
    void sceGnmLogicalCuIndexToPhysicalCuIndex(){}
    void sceGnmLogicalCuMaskToPhysicalCuMask(){}
    void sceGnmMapComputeQueue(){}
    void sceGnmMapComputeQueueWithPriority(){}
    void sceRazorCaptureImmediate(){}
    void sceGnmRequestFlipAndSubmitDone(){}
    void sceGnmRequestFlipAndSubmitDoneForWorkload(){}
    void sceGnmRequestMipStatsReportAndReset(){}

	void LibSceGnmDriver_Register(SymbolsResolver* sym)
	{ 
		LIB_FUNCTION("b0xyllnVY-I", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmAddEqEvent);
        LIB_FUNCTION("b08AgtPlHPg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmAreSubmitsAllowed);
        LIB_FUNCTION("ihxrbsoSKWc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmBeginWorkload);
        LIB_FUNCTION("5udAm+6boVg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmCreateWorkloadStream);
        LIB_FUNCTION("qpGITzPE+Zc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDebugHardwareStatus);
        LIB_FUNCTION("jtkqXpAOY6w", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetGsRingSizes);
        LIB_FUNCTION("XiyzNZ9J4nQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetWaveLimitMultipliers);
		LIB_FUNCTION("+xuDhxlWRPg", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSetupMipStatsReport);
        LIB_FUNCTION("zwY0YV91TTI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSubmitCommandBuffers);
        LIB_FUNCTION("xbxNatawohc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSubmitAndFlipCommandBuffers);
        LIB_FUNCTION("Ga6r7H6Y0RI", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSubmitAndFlipCommandBuffersForWorkload);
        LIB_FUNCTION("f33OrruQYbM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceRazorIsLoaded);
        LIB_FUNCTION("jRcI8VcgTz4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSubmitCommandBuffersForWorkload);
        LIB_FUNCTION("PVT+fuoS9gU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDeleteEqEvent);
        LIB_FUNCTION("yvZ73uQUqrk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmSubmitDone);
        LIB_FUNCTION("UtObDRQiGbs", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDestroyWorkloadStream);
        LIB_FUNCTION("bX5IbRvECXk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDingDong);
        LIB_FUNCTION("byXlqupd8cE", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDingDongForWorkload);
        LIB_FUNCTION("HHo1BAljZO8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDisableMipStatsReport);
        LIB_FUNCTION("TLV4mswiZ4A", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDriverCaptureInProgress);
        LIB_FUNCTION("ArSg-TGinhk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmUnmapComputeQueue);
        LIB_FUNCTION("R6z1xM3pW-w", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDriverTraceInProgress);
        LIB_FUNCTION("d88anrgNoKY", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmDriverTriggerCapture);
        LIB_FUNCTION("Fa3x75OOLRA", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmEndWorkload);
        LIB_FUNCTION("iBt3Oe00Kvc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmFlushGarlic);
        LIB_FUNCTION("UoYY0DWMC0U", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetEqEventType);
        LIB_FUNCTION("H7-fgvEutM0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetEqTimeStamp);
        LIB_FUNCTION("oL4hGI1PMpw", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetGpuBlockStatus);
        LIB_FUNCTION("tZCSL5ulnB4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetGpuInfoStatus);
        LIB_FUNCTION("iFirFzgYsvw", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetLastWaitedAddress);
        LIB_FUNCTION("KnldROUkWJY", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetNumTcaUnits);
        LIB_FUNCTION("FFVZcCu3zWU", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetOffChipTessellationBufferSize);
        LIB_FUNCTION("dewXw5roLs0", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetPhysicalCounterFromVirtualized);
        LIB_FUNCTION("fzJdEihTFV4", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetProtectionFaultTimeStamp);
        LIB_FUNCTION("nEyFbYUloIM", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetShaderProgramBaseAddress);
        LIB_FUNCTION("k7iGTvDQPLQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetShaderStatus);
        LIB_FUNCTION("ln33zjBrfjk", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmGetTheTessellationFactorRingBufferBaseAddress);
        LIB_FUNCTION("jg33rEKLfVs", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmIsUserPaEnabled);
        LIB_FUNCTION("26PM5Mzl8zc", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmLogicalCuIndexToPhysicalCuIndex);
        LIB_FUNCTION("RU74kek-N0c", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmLogicalCuMaskToPhysicalCuMask);
        LIB_FUNCTION("29oKvKXzEZo", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmMapComputeQueue);
        LIB_FUNCTION("A+uGq+3KFtQ", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmMapComputeQueueWithPriority);
        LIB_FUNCTION("u9YKpRRHe-M", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceRazorCaptureImmediate);
        LIB_FUNCTION("gObODli-OH8", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmRequestFlipAndSubmitDone);
        LIB_FUNCTION("6YRHhh5mHCs", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmRequestFlipAndSubmitDoneForWorkload);
        LIB_FUNCTION("f85orjx7qts", "libSceGnmDriver", 1, "libSceGnmDriver", 1, 1, sceGnmRequestMipStatsReportAndReset);
	}
    
};