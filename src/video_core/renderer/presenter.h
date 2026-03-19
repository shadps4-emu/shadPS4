// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/types.h"
#include "video_core/renderer/backend.h"

namespace Frontend {
class WindowSDL;
}

namespace Libraries::VideoOut {
struct BufferAttributeGroup;
}

namespace VideoCore::Render {

class IFrameHandle {
public:
    virtual ~IFrameHandle() = default;
};

class IPresenter {
public:
    virtual ~IPresenter() = default;

    virtual Frontend::WindowSDL& GetWindow() = 0;
    virtual DisplaySettings GetDisplaySettings() const = 0;
    virtual void SetDisplaySettings(const DisplaySettings& settings) = 0;

    virtual bool IsHDRSupported() const = 0;
    virtual void SetHDR(bool enable) = 0;

    virtual void RegisterVideoOutSurface(const Libraries::VideoOut::BufferAttributeGroup& attribute,
                                         VAddr cpu_address) = 0;
    virtual std::unique_ptr<IFrameHandle> PrepareFrame(
        const Libraries::VideoOut::BufferAttributeGroup& attribute, VAddr cpu_address) = 0;
    virtual std::unique_ptr<IFrameHandle> PrepareBlankFrame(bool present_thread) = 0;
    virtual std::unique_ptr<IFrameHandle> PrepareLastFrame() = 0;
    virtual void Present(std::unique_ptr<IFrameHandle> frame, bool is_reusing_frame = false) = 0;
};

} // namespace VideoCore::Render
