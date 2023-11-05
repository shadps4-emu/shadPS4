#include "common/timer.h"

namespace Emulator::emuTimer {

static Common::Timer timer;

void start() {
    timer.Start();
}

double getTimeMsec() {
    return timer.GetTimeMsec();
}

u64 getTimeCounter() {
    return timer.GetTicks();
}

u64 getTimeFrequency() {
    return timer.GetFrequency();
}

}  // namespace Emulator::emuTimer
