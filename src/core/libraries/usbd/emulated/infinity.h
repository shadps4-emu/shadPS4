//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>

#include "common/io_file.h"
#include "core/libraries/usbd/usb_backend.h"

namespace Libraries::Usbd {

constexpr u16 INFINITY_BLOCK_COUNT = 0x14;
constexpr u16 INFINITY_BLOCK_SIZE = 0x10;
constexpr u16 INFINITY_FIGURE_SIZE = INFINITY_BLOCK_COUNT * INFINITY_BLOCK_SIZE;
constexpr u8 MAX_INFINITY_FIGURES = 9;

struct InfinityFigure final {
    Common::FS::IOFile infFile;
    std::array<u8, INFINITY_FIGURE_SIZE> data{};
    bool present = false;
    u8 order_added = 255;
    void Save();
};

class InfinityBase final : public UsbEmulatedImpl {
public:
    InfinityBase();
    ~InfinityBase() override = default;

    void GetBlankResponse(u8 sequence, std::array<u8, 32>& reply_buf);
    void DescrambleAndSeed(u8* buf, u8 sequence, std::array<u8, 32>& reply_buf);
    void GetNextAndScramble(u8 sequence, std::array<u8, 32>& reply_buf);
    void GetPresentFigures(u8 sequence, std::array<u8, 32>& reply_buf);
    void QueryBlock(u8 fig_num, u8 block, std::array<u8, 32>& reply_buf, u8 sequence);
    void WriteBlock(u8 fig_num, u8 block, const u8* to_write_buf, std::array<u8, 32>& reply_buf,
                    u8 sequence);
    void GetFigureIdentifier(u8 fig_num, u8 sequence, std::array<u8, 32>& reply_buf);
    std::optional<std::array<u8, 32>> PopAddedRemovedResponse();

    void LoadFigure(std::string file_name, u8 pad, u8 slot) override;
    void RemoveFigure(u8 pad, u8 slot, bool full_remove) override;
    void MoveFigure(u8 new_pad, u8 new_index, u8 old_pad, u8 old_index) override {}
    void TempRemoveFigure(u8 index) override {}
    void CancelRemoveFigure(u8 index) override {}

    void LoadInfinityFigure(const std::array<u8, 0x14 * 0x10>& buf, Common::FS::IOFile file,
                            u8 position);

protected:
    std::mutex infinity_mutex;
    std::array<InfinityFigure, MAX_INFINITY_FIGURES> infinity_figures;

private:
    u8 GenerateChecksum(const std::array<u8, 32>& data, int num_of_bytes) const;
    u32 Descramble(u64 num_to_descramble);
    u64 Scramble(u32 num_to_scramble, u32 garbage);
    void GenerateSeed(u32 seed);
    u32 GetNext();
    InfinityFigure& GetFigureByOrder(u8 order_added);
    u8 DeriveFigurePosition(u8 position);

    u32 random_a = 0;
    u32 random_b = 0;
    u32 random_c = 0;
    u32 random_d = 0;

    u8 m_figure_order = 0;
    std::queue<std::array<u8, 32>> m_figure_added_removed_responses;
};

class InfinityBackend final : public UsbEmulatedBackend {
protected:
    libusb_endpoint_descriptor* FillEndpointDescriptorPair() override;
    libusb_interface_descriptor* FillInterfaceDescriptor(
        libusb_endpoint_descriptor* descs) override;
    libusb_config_descriptor* FillConfigDescriptor(libusb_interface* inter) override;
    libusb_device_descriptor* FillDeviceDescriptor() override;

    s32 ControlTransfer(libusb_device_handle* dev_handle, u8 bmRequestType, u8 bRequest, u16 wValue,
                        u16 wIndex, u8* data, u16 wLength, u32 timeout) override {
        return LIBUSB_SUCCESS;
    }

    libusb_transfer_status HandleAsyncTransfer(libusb_transfer* transfer) override;

    std::shared_ptr<UsbEmulatedImpl> GetImplRef() override {
        return m_infinity_base;
    }

private:
    std::shared_ptr<InfinityBase> m_infinity_base = std::make_shared<InfinityBase>();

    std::array<u8, 9> m_endpoint_out_extra = {0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0x1d, 0x00};
    std::vector<libusb_endpoint_descriptor> m_endpoint_descriptors = {
        {0x7, 0x5, 0x81, 0x3, 0x20, 0x1, 0x0, 0x0},
        {0x7, 0x5, 0x1, 0x3, 0x20, 0x1, 0x0, 0x0, m_endpoint_out_extra.data(), 9}};
    std::vector<libusb_interface_descriptor> m_interface_descriptors = {
        {0x9, 0x4, 0x0, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0}};
    std::vector<libusb_config_descriptor> m_config_descriptors = {
        {0x9, 0x2, 0x29, 0x1, 0x1, 0x0, 0x80, 0xFA}};
    std::vector<libusb_device_descriptor> m_device_descriptors = {
        {0x12, 0x1, 0x200, 0x0, 0x0, 0x0, 0x20, 0x0E6F, 0x0129, 0x200, 0x1, 0x2, 0x3, 0x1}};

    std::queue<std::array<u8, 32>> m_queries;
};
} // namespace Libraries::Usbd