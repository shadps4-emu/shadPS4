#pragma once

#include <types.h>
#include "printf.h"

namespace Emulator::HLE::Libraries::LibC {

	//HLE functions
	PS4_SYSV_ABI int printf(VA_ARGS);
}