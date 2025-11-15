//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "infinity.h"

#include <mutex>

namespace Libraries::Usbd {

InfinityBase::InfinityBase() {}

void InfinityBase::LoadFigure(std::string file_name, u8 pad, u8 slot) {
    Common::FS::IOFile file(file_name, Common::FS::FileAccessMode::ReadWrite);
    std::array<u8, INFINITY_FIGURE_SIZE> data;
    ASSERT(file.Read(data) == data.size());
    LoadInfinityFigure(data, std::move(file), slot);
}

void InfinityBase::RemoveFigure(u8 pad, u8 slot, bool full_remove) {
    std::lock_guard lock(infinity_mutex);
    InfinityFigure& figure = infinity_figures[slot];

    if (!figure.present) {
        return;
    }

    slot = DeriveFigurePosition(slot);
    if (slot == 0) {
        return;
    }

    figure.present = false;

    std::array<u8, 32> figure_change_response = {0xab, 0x04, slot, 0x09, figure.order_added, 0x01};
    figure_change_response[6] = GenerateChecksum(figure_change_response, 6);
    m_figure_added_removed_responses.push(figure_change_response);

    figure.Save();
    figure.infFile.Close();
}

void InfinityBase::LoadInfinityFigure(const std::array<u8, INFINITY_FIGURE_SIZE>& buf,
                                      Common::FS::IOFile file, u8 position) {
    std::lock_guard lock(infinity_mutex);
    u8 order_added;

    InfinityFigure& figure = infinity_figures[position];

    figure.infFile = std::move(file);
    memcpy(figure.data.data(), buf.data(), figure.data.size());
    figure.present = true;
    if (figure.order_added == 255) {
        figure.order_added = m_figure_order;
        m_figure_order++;
    }
    order_added = figure.order_added;

    position = DeriveFigurePosition(position);
    if (position == 0) {
        return;
    }

    std::array<u8, 32> figure_change_response = {0xab, 0x04, position, 0x09, order_added, 0x00};
    figure_change_response[6] = GenerateChecksum(figure_change_response, 6);
    m_figure_added_removed_responses.push(figure_change_response);
}

void InfinityBase::GetBlankResponse(u8 sequence, std::array<u8, 32>& reply_buf) {
    reply_buf[0] = 0xaa;
    reply_buf[1] = 0x01;
    reply_buf[2] = sequence;
    reply_buf[3] = GenerateChecksum(reply_buf, 3);
}

void InfinityBase::DescrambleAndSeed(u8* buf, u8 sequence, std::array<u8, 32>& reply_buf) {
    u64 value = u64(buf[4]) << 56 | u64(buf[5]) << 48 | u64(buf[6]) << 40 | u64(buf[7]) << 32 |
                u64(buf[8]) << 24 | u64(buf[9]) << 16 | u64(buf[10]) << 8 | u64(buf[11]);
    u32 seed = Descramble(value);
    GenerateSeed(seed);
    GetBlankResponse(sequence, reply_buf);
}

void InfinityBase::GetNextAndScramble(u8 sequence, std::array<u8, 32>& reply_buf) {
    const u32 next_random = GetNext();
    const u64 scrambled_next_random = Scramble(next_random, 0);
    reply_buf = {0xAA, 0x09, sequence};
    reply_buf[3] = u8((scrambled_next_random >> 56) & 0xFF);
    reply_buf[4] = u8((scrambled_next_random >> 48) & 0xFF);
    reply_buf[5] = u8((scrambled_next_random >> 40) & 0xFF);
    reply_buf[6] = u8((scrambled_next_random >> 32) & 0xFF);
    reply_buf[7] = u8((scrambled_next_random >> 24) & 0xFF);
    reply_buf[8] = u8((scrambled_next_random >> 16) & 0xFF);
    reply_buf[9] = u8((scrambled_next_random >> 8) & 0xFF);
    reply_buf[10] = u8(scrambled_next_random & 0xFF);
    reply_buf[11] = GenerateChecksum(reply_buf, 11);
}

void InfinityBase::GetPresentFigures(u8 sequence, std::array<u8, 32>& reply_buf) {
    int x = 3;
    for (u8 i = 0; i < infinity_figures.size(); i++) {
        u8 slot = i == 0 ? 0x10 : (i < 4) ? 0x20 : 0x30;
        if (infinity_figures[i].present) {
            reply_buf[x] = slot + infinity_figures[i].order_added;
            reply_buf[x + 1] = 0x09;
            x += 2;
        }
    }
    reply_buf[0] = 0xaa;
    reply_buf[1] = x - 2;
    reply_buf[2] = sequence;
    reply_buf[x] = GenerateChecksum(reply_buf, x);
}

void InfinityBase::QueryBlock(u8 fig_num, u8 block, std::array<u8, 32>& reply_buf, u8 sequence) {
    std::lock_guard lock(infinity_mutex);

    InfinityFigure& figure = GetFigureByOrder(fig_num);

    reply_buf[0] = 0xaa;
    reply_buf[1] = 0x12;
    reply_buf[2] = sequence;
    reply_buf[3] = 0x00;
    const u8 file_block = (block == 0) ? 1 : (block * 4);
    if (figure.present && file_block < 20) {
        memcpy(&reply_buf[4], figure.data.data() + (16 * file_block), 16);
    }
    reply_buf[20] = GenerateChecksum(reply_buf, 20);
}

void InfinityBase::WriteBlock(u8 fig_num, u8 block, const u8* to_write_buf,
                              std::array<u8, 32>& reply_buf, u8 sequence) {
    std::lock_guard lock(infinity_mutex);

    InfinityFigure& figure = GetFigureByOrder(fig_num);

    reply_buf[0] = 0xaa;
    reply_buf[1] = 0x02;
    reply_buf[2] = sequence;
    reply_buf[3] = 0x00;
    const u8 file_block = (block == 0) ? 1 : (block * 4);
    if (figure.present && file_block < 20) {
        memcpy(figure.data.data() + (file_block * 16), to_write_buf, 16);
        figure.Save();
    }
    reply_buf[4] = GenerateChecksum(reply_buf, 4);
}

void InfinityBase::GetFigureIdentifier(u8 fig_num, u8 sequence, std::array<u8, 32>& reply_buf) {
    std::lock_guard lock(infinity_mutex);

    InfinityFigure& figure = GetFigureByOrder(fig_num);

    reply_buf[0] = 0xaa;
    reply_buf[1] = 0x09;
    reply_buf[2] = sequence;
    reply_buf[3] = 0x00;

    if (figure.present) {
        memcpy(&reply_buf[4], figure.data.data(), 7);
    }
    reply_buf[11] = GenerateChecksum(reply_buf, 11);
}

std::optional<std::array<u8, 32>> InfinityBase::PopAddedRemovedResponse() {
    if (m_figure_added_removed_responses.empty())
        return std::nullopt;

    std::array<u8, 32> response = m_figure_added_removed_responses.front();
    m_figure_added_removed_responses.pop();
    return response;
}

u8 InfinityBase::GenerateChecksum(const std::array<u8, 32>& data, int num_of_bytes) const {
    int checksum = 0;
    for (int i = 0; i < num_of_bytes; i++) {
        checksum += data[i];
    }
    return (checksum & 0xFF);
}

u32 InfinityBase::Descramble(u64 num_to_descramble) {
    u64 mask = 0x8E55AA1B3999E8AA;
    u32 ret = 0;

    for (int i = 0; i < 64; i++) {
        if (mask & 0x8000000000000000) {
            ret = (ret << 1) | (num_to_descramble & 0x01);
        }

        num_to_descramble >>= 1;
        mask <<= 1;
    }

    return ret;
}

u64 InfinityBase::Scramble(u32 num_to_scramble, u32 garbage) {
    u64 mask = 0x8E55AA1B3999E8AA;
    u64 ret = 0;

    for (int i = 0; i < 64; i++) {
        ret <<= 1;

        if ((mask & 1) != 0) {
            ret |= (num_to_scramble & 1);
            num_to_scramble >>= 1;
        } else {
            ret |= (garbage & 1);
            garbage >>= 1;
        }

        mask >>= 1;
    }

    return ret;
}

void InfinityBase::GenerateSeed(u32 seed) {
    random_a = 0xF1EA5EED;
    random_b = seed;
    random_c = seed;
    random_d = seed;

    for (int i = 0; i < 23; i++) {
        GetNext();
    }
}

u32 InfinityBase::GetNext() {
    u32 a = random_a;
    u32 b = random_b;
    u32 c = random_c;
    u32 ret = std::rotl(random_b, 27);

    const u32 temp = (a + ((ret ^ 0xFFFFFFFF) + 1));
    b ^= std::rotl(c, 17);
    a = random_d;
    c += a;
    ret = b + temp;
    a += temp;

    random_c = a;
    random_a = b;
    random_b = c;
    random_d = ret;

    return ret;
}

InfinityFigure& InfinityBase::GetFigureByOrder(u8 order_added) {
    for (u8 i = 0; i < infinity_figures.size(); i++) {
        if (infinity_figures[i].order_added == order_added) {
            return infinity_figures[i];
        }
    }
    return infinity_figures[0];
}

u8 InfinityBase::DeriveFigurePosition(u8 position) {
    switch (position) {
    case 0:
    case 1:
    case 2:
        return 1;
    case 3:
    case 4:
    case 5:
        return 2;
    case 6:
    case 7:
    case 8:
        return 3;

    default:
        return 0;
    }
}

libusb_endpoint_descriptor* InfinityBackend::FillEndpointDescriptorPair() {
    return m_endpoint_descriptors.data();
}

libusb_interface_descriptor* InfinityBackend::FillInterfaceDescriptor(
    libusb_endpoint_descriptor* descs) {
    m_interface_descriptors[0].endpoint = descs;
    return m_interface_descriptors.data();
}

libusb_config_descriptor* InfinityBackend::FillConfigDescriptor(libusb_interface* inter) {
    m_config_descriptors[0].interface = inter;
    return m_config_descriptors.data();
}

libusb_device_descriptor* InfinityBackend::FillDeviceDescriptor() {
    return m_device_descriptors.data();
}

libusb_transfer_status InfinityBackend::HandleAsyncTransfer(libusb_transfer* transfer) {
    switch (transfer->endpoint) {
    case 0x81: {
        // Respond after FF command
        std::optional<std::array<u8, 32>> response = m_infinity_base->PopAddedRemovedResponse();
        if (response) {
            memcpy(transfer->buffer, response.value().data(), 0x20);
        } else if (!m_queries.empty()) {
            memcpy(transfer->buffer, m_queries.front().data(), 0x20);
            m_queries.pop();
        }
        break;
    }
    case 0x01: {
        const u8 command = transfer->buffer[2];
        const u8 sequence = transfer->buffer[3];
        LOG_INFO(Lib_Usbd, "Infinity Backend Transfer command: {:x}", command);

        std::array<u8, 32> q_result{};

        switch (command) {
        case 0x80: {
            q_result = {0xaa, 0x15, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x03, 0x02, 0x09, 0x09, 0x43,
                        0x20, 0x32, 0x62, 0x36, 0x36, 0x4b, 0x34, 0x99, 0x67, 0x31, 0x93, 0x8c};
            break;
        }
        case 0x81: {
            // Initiate Challenge
            m_infinity_base->DescrambleAndSeed(transfer->buffer, sequence, q_result);
            break;
        }
        case 0x83: {
            // Challenge Response
            m_infinity_base->GetNextAndScramble(sequence, q_result);
            break;
        }
        case 0x90:
        case 0x92:
        case 0x93:
        case 0x95:
        case 0x96: {
            // Color commands
            m_infinity_base->GetBlankResponse(sequence, q_result);
            break;
        }
        case 0xA1: {
            // Get Present Figures
            m_infinity_base->GetPresentFigures(sequence, q_result);
            break;
        }
        case 0xA2: {
            // Read Block from Figure
            m_infinity_base->QueryBlock(transfer->buffer[4], transfer->buffer[5], q_result,
                                        sequence);
            break;
        }
        case 0xA3: {
            // Write block to figure
            m_infinity_base->WriteBlock(transfer->buffer[4], transfer->buffer[5],
                                        &transfer->buffer[7], q_result, sequence);
            break;
        }
        case 0xB4: {
            // Get figure ID
            m_infinity_base->GetFigureIdentifier(transfer->buffer[4], sequence, q_result);
            break;
        }
        case 0xB5: {
            // Get status?
            m_infinity_base->GetBlankResponse(sequence, q_result);
            break;
        }
        default:
            LOG_ERROR(Lib_Usbd, "Unhandled Infinity Query: {}", command);
            break;
        }

        m_queries.push(q_result);
        break;
    }
    default:
        LOG_ERROR(Lib_Usbd, "Unhandled Infinity Endpoint: {}", transfer->endpoint);
        break;
    }
    return LIBUSB_TRANSFER_COMPLETED;
}

void InfinityFigure::Save() {
    if (!infFile.IsOpen())
        return;

    infFile.Seek(0);
    infFile.Write(data);
}
} // namespace Libraries::Usbd