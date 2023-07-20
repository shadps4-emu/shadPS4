#include "LibC.h"
#include "Libs.h"
#include "../Loader/Elf.h"

namespace HLE::Libs::LibC {

	static u32 g_need_sceLibc = 1;

	static void init_env() //every game/demo should probably 
	{
		for(;;) {
			printf("life is a bitch but it did reach here\n");
		}
		//__debugbreak();//if we reach here it will be a great progress :D
	}

	int __cxa_guard_acquire(u64* guard_object)
	{ return 0;
	}

	int __cxa_guard_release(u64* guard_object) 
	{ return 0;
	}

	int memcmp(const void* s1, const void* s2, size_t n) {
        return ::memcmp(s1, s2, n);
    }

    void* memcpy(void* dest, const void* src, size_t n) {
        return ::memcpy(dest, src, n);
    }

	static void catchReturnFromMain(int status)
	{

	}
	static void exit(int code)
	{

	}
	static int atexit(void (*func)())
	{ return 0;
	}

	void LibC_Register(SymbolsResolver* sym)
	{
        LIB_FUNCTION("bzQExy189ZI", "libc", 1, "libc", 1, 1, init_env);
        LIB_FUNCTION("3GPpjQdAMTw", "libc", 1, "libc", 1, 1, __cxa_guard_acquire);
        LIB_FUNCTION("9rAeANT2tyE", "libc", 1, "libc", 1, 1, __cxa_guard_release);
        LIB_FUNCTION("DfivPArhucg", "libc", 1, "libc", 1, 1, memcmp);
        LIB_FUNCTION("Q3VBxCXhUHs", "libc", 1, "libc", 1, 1, memcpy);
        LIB_FUNCTION("XKRegsFpEpk", "libc", 1, "libc", 1, 1, catchReturnFromMain);
        LIB_FUNCTION("uMei1W9uyNo", "libc", 1, "libc", 1, 1, exit);
        LIB_FUNCTION("8G2LB+A3rzg", "libc", 1, "libc", 1, 1, atexit);

		LIB_OBJ("P330P3dFF68", "libc", 1, "libc", 1, 1, &HLE::Libs::LibC::g_need_sceLibc);
    }
    
};