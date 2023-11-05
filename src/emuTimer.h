#pragma once

#include "common/types.h"

namespace Emulator::emuTimer {
void start();
double getTimeMsec();
u64 getTimeCounter();
u64 getTimeFrequency();
}  // namespace Emulator::emuTimer
