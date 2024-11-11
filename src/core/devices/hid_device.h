//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "base_device.h"

namespace Core::Devices {

class HidDevice final : BaseDevice {
    u32 handle;
    u32 user_id{};

    int MouseCallback(u64 cmd, Common::VaCtx* args);

    int GenericCallback(u64 cmd, Common::VaCtx* args);

    int (HidDevice::*m_callback)(u64 cmd, Common::VaCtx* args) = &HidDevice::GenericCallback;

public:
    static BaseDevice* Create(u32 handle, const char*, int, u16);

    explicit HidDevice(u32 handle);

    ~HidDevice() override;

    int ioctl(u64 cmd, Common::VaCtx* args) override;
};

} // namespace Core::Devices
