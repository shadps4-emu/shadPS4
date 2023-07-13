#pragma once
#include "../Loader/SymbolsResolver.h"

namespace HLE::Libs::LibC {

	void LibC_Register(SymbolsResolver* sym);
	//functions
	static void init_env();

};