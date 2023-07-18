#include "Libs.h"
#include "LibC.h"
#include "LibKernel.h"
#include "LibSceVideoOut.h"
#include "LibSceGnmDriver.h"

namespace HLE::Libs {

	void Init_HLE_Libs(SymbolsResolver *sym)
	{
		LibC::LibC_Register(sym);
        LibKernel::LibKernel_Register(sym);
        LibSceVideoOut::LibSceVideoOut_Register(sym);
        LibSceGnmDriver::LibSceGnmDriver_Register(sym);
	}
}