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

constexpr u8 DIMEMSIONS_BLOCK_COUNT = 0x2D;
constexpr u8 DIMENSIONS_BLOCK_SIZE = 0x04;
constexpr u8 DIMENSIONS_FIGURE_SIZE = DIMEMSIONS_BLOCK_COUNT * DIMENSIONS_BLOCK_SIZE;
constexpr u8 MAX_DIMENSIONS_FIGURES = 7;

struct DimensionsFigure final {
    Common::FS::IOFile dimFile;
    std::array<u8, DIMENSIONS_FIGURE_SIZE> data{};
    u8 index = 255;
    u8 pad = 255;
    u32 id = 0;
    void Save();
};

class DimensionsToypad final : public UsbEmulatedImpl {
public:
    DimensionsToypad();
    ~DimensionsToypad() override = default;

    static void GetBlankResponse(u8 type, u8 sequence, std::array<u8, 32>& reply_buf);
    void GenerateRandomNumber(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf);
    void InitializeRNG(u32 seed);
    void GetChallengeResponse(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf);
    void QueryBlock(u8 index, u8 page, std::array<u8, 32>& reply_buf, u8 sequence);
    void WriteBlock(u8 index, u8 page, const u8* to_write_buf, std::array<u8, 32>& reply_buf,
                    u8 sequence);
    void GetModel(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf);
    std::optional<std::array<u8, 32>> PopAddedRemovedResponse();

    void LoadFigure(std::string file_name, u8 pad, u8 slot) override;
    void RemoveFigure(u8 pad, u8 slot, bool full_remove) override;
    void MoveFigure(u8 new_pad, u8 new_index, u8 old_pad, u8 old_index) override;
    void TempRemoveFigure(u8 index) override;
    void CancelRemoveFigure(u8 index) override;

    u32 LoadDimensionsFigure(const std::array<u8, 0x2D * 0x04>& buf, Common::FS::IOFile file,
                             u8 pad, u8 index);

protected:
    std::mutex m_dimensions_mutex;
    std::array<DimensionsFigure, MAX_DIMENSIONS_FIGURES> m_figures{};

private:
    static void RandomUID(u8* uid_buffer);
    static u8 GenerateChecksum(const std::array<u8, 32>& data, u32 num_of_bytes);
    static std::array<u8, 8> Decrypt(const u8* buf, std::optional<std::array<u8, 16>> key);
    static std::array<u8, 8> Encrypt(const u8* buf, std::optional<std::array<u8, 16>> key);
    static std::array<u8, 16> GenerateFigureKey(const std::array<u8, 0x2D * 0x04>& buf);
    static u32 Scramble(const std::array<u8, 7>& uid, u8 count);
    static std::array<u8, 4> PWDGenerate(const std::array<u8, 7>& uid);
    static std::array<u8, 4> DimensionsRandomize(const std::vector<u8>& key, u8 count);
    static u32 GetFigureId(const std::array<u8, 0x2D * 0x04>& buf);
    u32 GetNext();
    DimensionsFigure& GetFigureByIndex(u8 index);

    u32 m_random_a{};
    u32 m_random_b{};
    u32 m_random_c{};
    u32 m_random_d{};

    u8 m_figure_order = 0;
    std::queue<std::array<u8, 32>> m_figure_added_removed_responses;
};

class DimensionsBackend final : public UsbEmulatedBackend {
protected:
    libusb_endpoint_descriptor* FillEndpointDescriptorPair() override;
    libusb_interface_descriptor* FillInterfaceDescriptor(
        libusb_endpoint_descriptor* descs) override;
    libusb_config_descriptor* FillConfigDescriptor(libusb_interface* inter) override;
    libusb_device_descriptor* FillDeviceDescriptor() override;

    s32 GetMaxPacketSize(libusb_device* dev, u8 endpoint) override {
        return 32;
    }

    s32 SubmitTransfer(libusb_transfer* transfer) override;

    libusb_transfer_status HandleAsyncTransfer(libusb_transfer* transfer) override;

    std::shared_ptr<UsbEmulatedImpl> GetImplRef() override {
        return m_dimensions_toypad;
    }

    std::mutex m_query_mutex;
    std::queue<std::array<u8, 32>> m_queries;

private:
    std::shared_ptr<DimensionsToypad> m_dimensions_toypad = std::make_shared<DimensionsToypad>();

    std::array<u8, 9> m_endpoint_out_extra = {0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0x1d, 0x00};
    std::vector<libusb_endpoint_descriptor> m_endpoint_descriptors = {
        {0x7, 0x5, 0x81, 0x3, 0x20, 0x1, 0x0, 0x0}, {0x7, 0x5, 0x1, 0x3, 0x20, 0x1, 0x0, 0x0}};
    std::vector<libusb_interface_descriptor> m_interface_descriptors = {
        {0x9, 0x4, 0x0, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0}};
    std::vector<libusb_config_descriptor> m_config_descriptors = {
        {0x9, 0x2, 0x29, 0x1, 0x1, 0x0, 0x80, 0xFA}};
    std::vector<libusb_device_descriptor> m_device_descriptors = {
        {0x12, 0x1, 0x200, 0x0, 0x0, 0x0, 0x20, 0x0E6F, 0x0241, 0x200, 0x1, 0x2, 0x0, 0x1}};
};
} // namespace Libraries::Usbd