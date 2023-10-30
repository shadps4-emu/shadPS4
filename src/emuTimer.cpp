#include "Lib/Timer.h"

namespace Emulator::emuTimer {
static Lib::Timer timer;

void start() { timer.Start(); }

double getTimeMsec() { return timer.GetTimeMsec(); }

}