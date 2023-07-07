#include "LibC.h"
#include "../Loader/Elf.h"

namespace HLE::Libs::LibC {

	static void init_env() //every game/demo should probably 
	{
		for(;;) {
			printf("__debugbreak\n");
		}
		//__debugbreak();//if we reach here it will be a great progress :D
	}

	void LibC_RegisterFunc(SymbolsResolver* sym)
	{
		//TODO this will be convert to macro probably once we decide how will it work and what's the best
		SymbolRes sr {};
		sr.name = "bzQExy189ZI";
		sr.library = "libc";
		sr.library_version = 1;
		sr.module = "libc";
		sr.module_version_major = 1;
		sr.module_version_minor = 1;
		sr.type = STT_FUN;
		auto func = reinterpret_cast<u64>(init_env);
		sym->AddSymbol(sr, func);
	}
};