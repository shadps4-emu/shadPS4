//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/singleton.h"
#include "hid_device.h"
#include "input/mouse.h"
#include "ioccom.h"

struct MouseInfo {};

struct MouseDataEntry {
    std::array<u8, 8> _unknown1;
    s16 x;
    s16 y;
    u8 buttons;
    bool system_capture;
    s8 wheel;
    s8 tilt;
    std::array<u8, 8> _reserved;
};

static_assert(sizeof(MouseDataEntry) == 0x18);

struct MouseData {
    u32 handle;
    std::array<u8, 4> _unknown1;
    MouseDataEntry* data;
    int max_data;
    std::array<u8, 4> _unknown2;
    int* read_count;
};
static_assert(sizeof(MouseData) == 0x20);

constexpr auto HID_CMD_READ_MOUSE_STATE = _IOW('H', 0x19, MouseData);

namespace Core::Devices {

int HidDevice::MouseCallback(u64 cmd, Common::VaCtx* args) {
    switch (cmd) {
    case HID_CMD_READ_DEVICE_INFO: {
        auto info = vaArgPtr<DeviceInfo<MouseInfo>>(&args->va_list);
        info->data = nullptr;
        // TODO Fill data with the mouse info
        return ORBIS_OK;
    }
    case HID_CMD_READ_MOUSE_STATE: {
        auto mouse_data = vaArgPtr<MouseData>(&args->va_list);
        auto* mouse = Common::Singleton<Input::GameMouse>::Instance();
        if (!mouse->m_connected) {
            *mouse_data->read_count = 0;
            return 0;
        }
        std::array<Input::MouseState, 64> states;
        int total = std::min((int)states.size(), mouse_data->max_data);
        total = mouse->ReadStates(states.data(), total);
        for (int i = 0; i < total; ++i) {
            const auto& s = states[i];
            mouse_data->data[i] = MouseDataEntry{
                .x = static_cast<s16>(s.x_axis),
                .y = static_cast<s16>(s.y_axis),
                .buttons = static_cast<u8>(s.button_state),
                .system_capture = false,
                .wheel = static_cast<s8>(s.wheel),
                .tilt = static_cast<s8>(s.tilt),
            };
        }
        return total;
    }
    default:
        LOG_WARNING(Core_Devices, "HID({:X}) mouse: unknown ioctl request: {:X}", handle, cmd);
        return ORBIS_OK;
    }
}

} // namespace Core::Devices
