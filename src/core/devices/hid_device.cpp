//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <core/libraries/system/msgdialog_ui.h>

#include "common/logging/log.h"
#include "common/singleton.h"
#include "common/va_ctx.h"
#include "core/libraries/error_codes.h"
#include "hid_device.h"
#include "input/mouse.h"
#include "ioccom.h"

namespace Core::Devices {

struct InitHidRaw {
    u32 user_id;
    u32 type;
    u32 _unknown2;
};
static_assert(sizeof(InitHidRaw) == 0xC);

constexpr auto HID_CMD_INIT_HANDLE_FOR_USER_ID = _IOW('H', 0x2, InitHidRaw);

int HidDevice::GenericCallback(u64 cmd, Common::VaCtx* args) {
    LOG_TRACE(Core_Devices, "HID({:X}) generic: ioctl cmd={:X}", handle, cmd);
    if (cmd == HID_CMD_INIT_HANDLE_FOR_USER_ID) {
        auto data = vaArgPtr<InitHidRaw>(&args->va_list);
        this->user_id = data->user_id;
        if (data->type == 0) {
            LOG_INFO(Core_Devices, "HID({:X}) open: type=mouse, user_id={}", handle, data->user_id);
            this->m_callback = &HidDevice::MouseCallback;
            // FIXME Replace by status bar
            if (!Common::Singleton<Input::GameMouse>::Instance()->m_connected) {
                using namespace ::Libraries::MsgDialog;
                ShowMsgDialog(MsgDialogState(MsgDialogState::UserState{
                                  .type = ButtonType::YESNO,
                                  .msg = "Game wants to use your mouse.\nDo you want to allow it?",
                              }),
                              false, [](DialogResult result) {
                                  if (result.buttonId == ButtonId::YES) {
                                      auto* mouse = Common::Singleton<Input::GameMouse>::Instance();
                                      mouse->m_connected = true;
                                  }
                              });
            }
        } else {
            LOG_WARNING(Core_Devices, "HID({:X}) open: unknown type={}", handle, data->type);
        }
        return static_cast<int>(handle);
    }
    LOG_WARNING(Core_Devices, "HID({:X}) generic: unknown ioctl cmd = {:X}", handle, cmd);
    return ORBIS_KERNEL_ERROR_ENOTTY;
}

BaseDevice* HidDevice::Create(u32 handle, const char*, int, u16) {
    return new HidDevice(handle);
}

HidDevice::HidDevice(u32 handle) : handle{handle} {}

HidDevice::~HidDevice() = default;

int HidDevice::ioctl(u64 cmd, Common::VaCtx* args) {
    return (this->*m_callback)(cmd, args);
}

} // namespace Core::Devices