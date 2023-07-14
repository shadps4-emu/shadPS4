#pragma once
#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibC {

	void LibC_Register(SymbolsResolver* sym);
	//functions
	static void init_env();
    static void exit(int code);
    static void catchReturnFromMain(int status);
    int __cxa_guard_acquire(u64* guard_object);
    int memcmp(const void* s1, const void* s2, size_t n);
    void* memcpy(void* dest, const void* src, size_t n);
};