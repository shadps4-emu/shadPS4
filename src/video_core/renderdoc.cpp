// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/config.h"
#include "video_core/renderdoc.h"

#include <renderdoc_app.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <filesystem>

namespace VideoCore {

enum class CaptureState {
    Idle,
    Triggered,
    InProgress,
};
static CaptureState capture_state{CaptureState::Idle};

RENDERDOC_API_1_6_0* rdoc_api{};

void LoadRenderDoc() {
#ifdef WIN32

    // Check if we are running by RDoc GUI
    HMODULE mod = GetModuleHandleA("renderdoc.dll");
    if (!mod && Config::isRdocEnabled()) {
        // If enabled in config, try to load RDoc runtime in offline mode
        HKEY h_reg_key;
        LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                    L"SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\", 0,
                                    KEY_READ, &h_reg_key);
        if (result != ERROR_SUCCESS) {
            return;
        }
        std::array<wchar_t, MAX_PATH> key_str{};
        DWORD str_sz_out{key_str.size()};
        result = RegQueryValueExW(h_reg_key, L"", 0, NULL, (LPBYTE)key_str.data(), &str_sz_out);
        if (result != ERROR_SUCCESS) {
            return;
        }

        std::filesystem::path path{key_str.cbegin(), key_str.cend()};
        path = path.parent_path().append("renderdoc.dll");
        const auto path_to_lib = path.generic_string();
        mod = LoadLibraryA(path_to_lib.c_str());
    }

    if (mod) {
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
    // Check if we are running by RDoc GUI
    void* mod = dlopen(RENDERDOC_LIB, RTLD_NOW | RTLD_NOLOAD);
    if (!mod && Config::isRdocEnabled()) {
        // If enabled in config, try to load RDoc runtime in offline mode
        if ((mod = dlopen(RENDERDOC_LIB, RTLD_NOW))) {
            const auto RENDERDOC_GetAPI =
                reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod, "RENDERDOC_GetAPI"));
            const s32 ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdoc_api);
            ASSERT(ret == 1);
        } else {
            LOG_ERROR(Render, "Cannot load RenderDoc: {}", dlerror());
        }
    }
#endif
    if (rdoc_api) {
        // Disable default capture keys as they suppose to trigger present-to-present capturing
        // and it is not what we want
        rdoc_api->SetCaptureKeys(nullptr, 0);

        // Also remove rdoc crash handler
        rdoc_api->UnloadCrashHandler();
    }
}

void StartCapture() {
    if (!rdoc_api) {
        return;
    }

    if (capture_state == CaptureState::Triggered) {
        rdoc_api->StartFrameCapture(nullptr, nullptr);
        capture_state = CaptureState::InProgress;
    }
}

void EndCapture() {
    if (!rdoc_api) {
        return;
    }

    if (capture_state == CaptureState::InProgress) {
        rdoc_api->EndFrameCapture(nullptr, nullptr);
        capture_state = CaptureState::Idle;
    }
}

void TriggerCapture() {
    if (capture_state == CaptureState::Idle) {
        capture_state = CaptureState::Triggered;
    }
}

void SetOutputDir(const std::filesystem::path& path, const std::string& prefix) {
    if (!rdoc_api) {
        return;
    }
    rdoc_api->SetCaptureFilePathTemplate(fmt::UTF((path / prefix).u8string()).data.data());
}

} // namespace VideoCore
