#include "Libs.h"
#include "LibC.h"

namespace HLE::Libs {

	void Init_HLE_Libs(SymbolsResolver *sym)
	{
		LibC::LibC_RegisterFunc(sym);
	}
}