#include "Lib/Timer.h"

namespace Emulator::emuTimer {
static Lib::Timer timer;

void start() { timer.Start(); }

double getTimeMsec() { return timer.GetTimeMsec(); }

u64 getTimeCounter() { return timer.GetTicks(); }

u64 getTimeFrequency() { return timer.GetFrequency(); }

}  // namespace Emulator::emuTimer