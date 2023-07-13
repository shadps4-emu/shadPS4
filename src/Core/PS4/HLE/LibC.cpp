#include "LibC.h"
#include "Libs.h"
#include "../Loader/Elf.h"

namespace HLE::Libs::LibC {

	static void init_env() //every game/demo should probably 
	{
		for(;;) {
			printf("__debugbreak\n");
		}
		//__debugbreak();//if we reach here it will be a great progress :D
	}

	void LibC_Register(SymbolsResolver* sym)
	{
        LIB_FUNCTION("bzQExy189ZI", "libc", 1, "libc", 1, 1, init_env);
	}
};