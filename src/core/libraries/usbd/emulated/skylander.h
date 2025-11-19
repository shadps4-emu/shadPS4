//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include "common/io_file.h"
#include "core/libraries/usbd/usb_backend.h"

namespace Libraries::Usbd {

constexpr u16 SKY_BLOCK_COUNT = 0x40;
constexpr u16 SKY_BLOCK_SIZE = 0x10;
constexpr u16 SKY_FIGURE_SIZE = SKY_BLOCK_COUNT * SKY_BLOCK_SIZE;
constexpr u8 MAX_SKYLANDERS = 16;

struct Skylander final {
    Common::FS::IOFile skyFile;
    u8 status = 0;
    std::queue<u8> queued_status;
    std::array<u8, SKY_FIGURE_SIZE> data{};
    u32 last_id = 0;
    void Save();

    enum : u8 { REMOVED = 0, READY = 1, REMOVING = 2, ADDED = 3 };
};

struct SkylanderLEDColor final {
    u8 red = 0;
    u8 green = 0;
    u8 blue = 0;
};

class SkylanderPortal final : public UsbEmulatedImpl {
public:
    SkylanderPortal();
    ~SkylanderPortal() override = default;

    void Activate();
    void Deactivate();
    void SetLEDs(u8 side, u8 r, u8 g, u8 b);

    std::array<u8, 64> GetStatus();
    void QueryBlock(u8 sky_num, u8 block, u8* reply_buf);
    void WriteBlock(u8 sky_num, u8 block, const u8* to_write_buf, u8* reply_buf);

    void LoadFigure(std::string file_name, u8 pad, u8 slot) override;
    void RemoveFigure(u8 pad, u8 slot, bool full_remove) override;
    void MoveFigure(u8 new_pad, u8 new_index, u8 old_pad, u8 old_index) override {}
    void TempRemoveFigure(u8 index) override {}
    void CancelRemoveFigure(u8 index) override {}

    u8 LoadSkylander(u8* buf, Common::FS::IOFile file);

protected:
    std::mutex sky_mutex;

private:
    static bool IsSkylanderNumberValid(u8 sky_num);
    static bool IsBlockNumberValid(u8 block);

    bool m_activated = true;
    bool m_status_updated = false;
    u8 m_interrupt_counter = 0;
    SkylanderLEDColor m_color_right = {};
    SkylanderLEDColor m_color_left = {};
    SkylanderLEDColor m_color_trap = {};

    std::array<Skylander, MAX_SKYLANDERS> skylanders;
    std::array<u8, MAX_SKYLANDERS> ui_skylanders;
};

class SkylanderBackend final : public UsbEmulatedBackend {
protected:
    libusb_endpoint_descriptor* FillEndpointDescriptorPair() override;
    libusb_interface_descriptor* FillInterfaceDescriptor(
        libusb_endpoint_descriptor* descs) override;
    libusb_config_descriptor* FillConfigDescriptor(libusb_interface* inter) override;
    libusb_device_descriptor* FillDeviceDescriptor() override;

    s32 ControlTransfer(libusb_device_handle* dev_handle, u8 bmRequestType, u8 bRequest, u16 wValue,
                        u16 wIndex, u8* data, u16 wLength, u32 timeout) override;

    libusb_transfer_status HandleAsyncTransfer(libusb_transfer* transfer) override;

    std::shared_ptr<UsbEmulatedImpl> GetImplRef() override {
        return m_skylander_portal;
    }

private:
    std::shared_ptr<SkylanderPortal> m_skylander_portal = std::make_shared<SkylanderPortal>();

    std::array<u8, 9> m_endpoint_out_extra = {0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0x1d, 0x00};
    std::vector<libusb_endpoint_descriptor> m_endpoint_descriptors = {
        {0x7, 0x5, 0x81, 0x3, 0x40, 0x1, 0x0, 0x0},
        {0x7, 0x5, 0x2, 0x3, 0x40, 0x1, 0x0, 0x0, m_endpoint_out_extra.data(), 9}};
    std::vector<libusb_interface_descriptor> m_interface_descriptors = {
        {0x9, 0x4, 0x0, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0}};
    std::vector<libusb_config_descriptor> m_config_descriptors = {
        {0x9, 0x2, 0x29, 0x1, 0x1, 0x0, 0x80, 0xFA}};
    std::vector<libusb_device_descriptor> m_device_descriptors = {
        {0x12, 0x1, 0x200, 0x0, 0x0, 0x0, 0x40, 0x1430, 0x150, 0x100, 0x1, 0x2, 0x0, 0x1}};

    std::queue<std::array<u8, 64>> m_queries;
};
} // namespace Libraries::Usbd