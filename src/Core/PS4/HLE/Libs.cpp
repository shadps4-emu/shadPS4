#include "Libs.h"
#include "LibC.h"
#include "LibKernel.h"

namespace HLE::Libs {

	void Init_HLE_Libs(SymbolsResolver *sym)
	{
		LibC::LibC_Register(sym);
        LibKernel::LibKernel_Register(sym);
	}
}