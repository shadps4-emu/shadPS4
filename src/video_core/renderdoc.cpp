// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <renderdoc_app.h>
#include "common/assert.h"
#include "video_core/renderdoc.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace VideoCore {

RENDERDOC_API_1_6_0* rdoc_api{};

void LoadRenderDoc() {
#ifdef WIN32
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
        const auto RENDERDOC_GetAPI =
            reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(mod, "RENDERDOC_GetAPI"));
        const s32 ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdoc_api);
        ASSERT(ret == 1);
    }
#else
#ifdef ANDROID
    static constexpr const char RENDERDOC_LIB[] = "libVkLayer_GLES_RenderDoc.so";
#else
    static constexpr const char RENDERDOC_LIB[] = "librenderdoc.so";
#endif
    if (void* mod = dlopen(RENDERDOC_LIB, RTLD_NOW | RTLD_NOLOAD)) {
        const auto RENDERDOC_GetAPI =
            reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod, "RENDERDOC_GetAPI"));
        const s32 ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdoc_api);
        ASSERT(ret == 1);
    }
#endif
}

void StartCapture() {
    if (rdoc_api) {
        rdoc_api->StartFrameCapture(nullptr, nullptr);
    }
}

void EndCapture() {
    if (rdoc_api) {
        rdoc_api->EndFrameCapture(nullptr, nullptr);
    }
}

} // namespace VideoCore
