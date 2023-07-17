#pragma once
#include "../Loader/SymbolsResolver.h"


#define LIB_FUNCTION(nid, lib, libversion, mod, moduleVersionMajor, moduleVersionMinor, function) \
    {\
	SymbolRes sr{}; \
	sr.name = nid; \
	sr.library = lib; \
	sr.library_version = libversion;\
	sr.module = mod;\
	sr.module_version_major = moduleVersionMajor;\
	sr.module_version_minor = moduleVersionMinor;\
	sr.type = STT_FUN;\
	auto func = reinterpret_cast<u64>(function);\
	sym->AddSymbol(sr, func);\
	}

#define LIB_OBJ(nid, lib, libversion, mod, moduleVersionMajor, moduleVersionMinor, function) \
    {                                                                                             \
        SymbolRes sr{};                                                                           \
        sr.name = nid;                                                                            \
        sr.library = lib;                                                                         \
        sr.library_version = libversion;                                                          \
        sr.module = mod;                                                                          \
        sr.module_version_major = moduleVersionMajor;                                             \
        sr.module_version_minor = moduleVersionMinor;                                             \
        sr.type = STT_OBJECT;                                                                        \
        auto func = reinterpret_cast<u64>(function);                                              \
        sym->AddSymbol(sr, func);                                                                 \
    }
namespace HLE::Libs {
	void Init_HLE_Libs(SymbolsResolver* sym);
}