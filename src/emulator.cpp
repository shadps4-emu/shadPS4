#include "emulator.h"
#include "Core/PS4/HLE/Graphics/video_out.h"

namespace Emulator {
void emuInit() {}
void emuRun() {
    for (;;) {
        HLE::Libs::Graphics::VideoOut::videoOutFlip(100000);//flip every 0.1 sec
    }
}
}  // namespace emulator