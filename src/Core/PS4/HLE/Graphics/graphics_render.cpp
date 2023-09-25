#include "graphics_render.h"
#include "Util/Singleton.h"
#include "emulator.h"

void GPU::renderCreateCtx() {
    auto* render_ctx = Singleton<RenderCtx>::Instance();

    render_ctx->setGraphicCtx(Emulator::getGraphicCtx());
}
