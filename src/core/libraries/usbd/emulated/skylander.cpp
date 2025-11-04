//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "skylander.h"

#include <mutex>

namespace Libraries::Usbd {

SkylanderPortal::SkylanderPortal() {}

void SkylanderPortal::LoadFigure(std::string file_name, u8 pad, u8 slot) {
    Common::FS::IOFile file(file_name, Common::FS::FileAccessMode::ReadWrite);
    std::array<u8, 0x40 * 0x10> data;
    ASSERT(file.Read(data) == data.size());
    ui_skylanders[slot] = LoadSkylander(data.data(), std::move(file));
}

void SkylanderPortal::RemoveFigure(u8 pad, u8 slot, bool full_remove) {
    std::lock_guard lock(sky_mutex);
    auto& thesky = skylanders[ui_skylanders[slot]];

    if (thesky.status & 1) {
        thesky.status = 2;
        thesky.queued_status.push(Skylander::REMOVING);
        thesky.queued_status.push(Skylander::REMOVED);
        thesky.Save();
        thesky.skyFile.Close();
    }
}

u8 SkylanderPortal::LoadSkylander(u8* buf, Common::FS::IOFile file) {
    std::lock_guard lock(sky_mutex);

    u32 skySerial = 0;
    for (int i = 3; i > -1; i--) {
        skySerial <<= 8;
        skySerial |= buf[i];
    }
    u8 foundSlot = 0xFF;

    // mimics spot retaining on the portal
    for (auto i = 0; i < MAX_SKYLANDERS; i++) {
        if ((skylanders[i].status & 1) == 0) {
            if (skylanders[i].last_id == skySerial) {
                foundSlot = i;
                break;
            }

            if (i < foundSlot) {
                foundSlot = i;
            }
        }
    }

    if (foundSlot != 0xFF) {
        auto& skylander = skylanders[foundSlot];
        memcpy(skylander.data.data(), buf, skylander.data.size());
        skylander.skyFile = std::move(file);
        skylander.status = Skylander::ADDED;
        skylander.queued_status.push(Skylander::ADDED);
        skylander.queued_status.push(Skylander::READY);
        skylander.last_id = skySerial;
    }
    return foundSlot;
}

void SkylanderPortal::Activate() {
    const std::lock_guard lock(sky_mutex);
    if (m_activated) {
        // If the portal was already active no change is needed
        return;
    }

    // If not we need to advertise change to all the figures present on the portal
    for (auto& s : skylanders) {
        if (s.status & 1) {
            s.queued_status.push(Skylander::ADDED);
            s.queued_status.push(Skylander::READY);
        }
    }

    m_activated = true;
}

void SkylanderPortal::Deactivate() {
    const std::lock_guard lock(sky_mutex);

    for (auto& s : skylanders) {
        // check if at the end of the updates there would be a figure on the portal
        if (!s.queued_status.empty()) {
            s.status = s.queued_status.back();
            s.queued_status = std::queue<u8>();
        }

        s.status &= 1;
    }

    m_activated = false;
}

// Side:
// 0x00 = right
// 0x01 = left and right
// 0x02 = left
// 0x03 = trap
void SkylanderPortal::SetLEDs(u8 side, u8 red, u8 green, u8 blue) {
    const std::lock_guard lock(sky_mutex);
    if (side == 0x00) {
        m_color_right.red = red;
        m_color_right.green = green;
        m_color_right.blue = blue;
    } else if (side == 0x01) {
        m_color_right.red = red;
        m_color_right.green = green;
        m_color_right.blue = blue;

        m_color_left.red = red;
        m_color_left.green = green;
        m_color_left.blue = blue;
    } else if (side == 0x02) {
        m_color_left.red = red;
        m_color_left.green = green;
        m_color_left.blue = blue;
    } else if (side == 0x03) {
        m_color_trap.red = red;
        m_color_trap.green = green;
        m_color_trap.blue = blue;
    }
}

std::array<u8, 64> SkylanderPortal::GetStatus() {
    const std::lock_guard lock(sky_mutex);

    u32 status = 0;
    u8 active = 0x00;

    if (m_activated) {
        active = 0x01;
    }

    for (int i = MAX_SKYLANDERS - 1; i >= 0; i--) {
        auto& s = skylanders[i];

        if (!s.queued_status.empty()) {
            s.status = s.queued_status.front();
            s.queued_status.pop();
        }
        status <<= 2;
        status |= s.status;
    }

    std::array<u8, 64> interrupt_response = {0x53,   0x00, 0x00, 0x00, 0x00, m_interrupt_counter++,
                                             active, 0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00,   0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00,   0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00,   0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00,   0x00};
    memcpy(&interrupt_response[1], &status, sizeof(status));
    return interrupt_response;
}

void SkylanderPortal::QueryBlock(u8 sky_num, u8 block, u8* reply_buf) {
    if (!IsSkylanderNumberValid(sky_num) || !IsBlockNumberValid(block))
        return;

    std::lock_guard lock(sky_mutex);

    const auto& skylander = skylanders[sky_num];

    reply_buf[0] = 'Q';
    reply_buf[2] = block;
    if (skylander.status & Skylander::READY) {
        reply_buf[1] = (0x10 | sky_num);
        memcpy(reply_buf + 3, skylander.data.data() + (16 * block), 16);
    } else {
        reply_buf[1] = 0x01;
    }
}

void SkylanderPortal::WriteBlock(u8 sky_num, u8 block, const u8* to_write_buf, u8* reply_buf) {
    if (!IsSkylanderNumberValid(sky_num) || !IsBlockNumberValid(block))
        return;

    std::lock_guard lock(sky_mutex);

    auto& skylander = skylanders[sky_num];

    reply_buf[0] = 'W';
    reply_buf[2] = block;

    if (skylander.status & 1) {
        reply_buf[1] = (0x10 | sky_num);
        memcpy(skylander.data.data() + (block * 16), to_write_buf, 16);
        skylander.Save();
    } else {
        reply_buf[1] = 0x01;
    }
}

bool SkylanderPortal::IsSkylanderNumberValid(u8 sky_num) {
    return sky_num < MAX_SKYLANDERS;
}

bool SkylanderPortal::IsBlockNumberValid(u8 block) {
    return block < 64;
}

libusb_endpoint_descriptor* SkylanderBackend::FillEndpointDescriptorPair() {
    return m_endpoint_descriptors.data();
}

libusb_interface_descriptor* SkylanderBackend::FillInterfaceDescriptor(
    libusb_endpoint_descriptor* descs) {
    m_interface_descriptors[0].endpoint = descs;
    return m_interface_descriptors.data();
}

libusb_config_descriptor* SkylanderBackend::FillConfigDescriptor(libusb_interface* inter) {
    m_config_descriptors[0].interface = inter;
    return m_config_descriptors.data();
}

libusb_device_descriptor* SkylanderBackend::FillDeviceDescriptor() {
    return m_device_descriptors.data();
}

s32 SkylanderBackend::ControlTransfer(libusb_device_handle* dev_handle, u8 bmRequestType,
                                      u8 bRequest, u16 wValue, u16 wIndex, u8* data, u16 wLength,
                                      u32 timeout) {
    if (bmRequestType != 0x21) {
        return LIBUSB_ERROR_PIPE;
    }
    if (bRequest != 0x09) {
        return 8;
    }
    // Data to be sent back via the control transfer immediately
    std::array<u8, 64> control_response = {};
    s32 expected_count = 0;
    // Data to be queued to be sent back via the Interrupt Transfer (if needed)
    std::array<u8, 64> interrupt_response = {};
    switch (data[0]) {
    case 'A': {
        // Activation
        // Command	{ 'A', (00 | 01), 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 }
        // Response	{ 'A', (00 | 01),
        // ff, 77, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00 }
        // The 2nd byte of the command is whether to activate (0x01) or deactivate (0x00) the
        // portal. The response echos back the activation byte as the 2nd byte of the response. The
        // 3rd and 4th bytes of the response appear to vary from wired to wireless. On wired
        // portals, the bytes appear to always be ff 77. On wireless portals, during activation the
        // 3rd byte appears to count down from ff (possibly a battery power indication) and during
        // deactivation ed and eb responses have been observed. The 4th byte appears to always be 00
        // for wireless portals.

        // Wii U Wireless: 41 01 f4 00 41 00 ed 00 41 01 f4 00 41 00 eb 00 41 01 f3 00 41 00 ed 00
        if (wLength == 2) {
            control_response = {data[0], data[1]};
            interrupt_response = {0x41, data[1], 0xFF, 0x77, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00,    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00,    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00,    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            m_queries.push(interrupt_response);
            expected_count = 10;
            m_skylander_portal->Activate();
        }
        break;
    }
    case 'C': {
        // Color
        // Command	{ 'C', 12, 34, 56, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 }
        // Response	{ 'C', 12, 34, 56, 00, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00 }
        // The 3 bytes {12, 34, 56} are RGB values.

        // This command should set the color of the LED in the portal, however this appears
        // deprecated in most of the recent portals. On portals that do not have LEDs, this command
        // is silently ignored and do not require a response.
        if (wLength == 4) {
            m_skylander_portal->SetLEDs(0x01, data[1], data[2], data[3]);
            control_response = {0x43, data[1], data[2], data[3]};
            expected_count = 12;
        }
        break;
    }
    case 'J': {
        // Sided color
        // The 2nd byte is the side
        // 0x00: right
        // 0x02: left

        // The 3rd, 4th and 5th bytes are red, green and blue

        // The 6th and 7th bytes form a little-endian short for how long the fade duration should be
        // in milliseconds.
        // For example, 500 milliseconds becomes 0xF4, 0x01
        if (wLength == 7) {
            control_response = {data[0], data[1], data[2], data[3], data[4], data[5], data[6]};
            expected_count = 15;
            interrupt_response = {data[0]};
            m_queries.push(interrupt_response);
            m_skylander_portal->SetLEDs(data[1], data[2], data[3], data[4]);
        }
        break;
    }
    case 'L': {
        // Light
        // This command is used while playing audio through the portal

        // The 2nd bytes is the position
        // 0x00: right
        // 0x01: trap led
        // 0x02: left

        // The 3rd, 4th and 5th bytes are red, green and blue
        // the trap led is white-only
        // increasing or decreasing the values results in a brighter or dimmer light
        if (wLength == 5) {
            control_response = {data[0], data[1], data[2], data[3], data[4]};
            expected_count = 13;

            u8 side = data[1];
            if (side == 0x02) {
                side = 0x04;
            }
            m_skylander_portal->SetLEDs(side, data[2], data[3], data[4]);
        }
        break;
    }
    case 'M': {
        // Audio Firmware version
        // Respond with version obtained from Trap Team wired portal
        if (wLength == 2) {
            control_response = {data[0], data[1]};
            expected_count = 10;
            interrupt_response = {data[0], data[1], 0x00, 0x19};
            m_queries.push(interrupt_response);
        }
        break;
    }
        // Query
        // Command	{ 'Q', 10, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 }
        // Response	{ 'Q', 10, 00, 00, 00, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00 }
        // In the command the 2nd byte indicates which Skylander to query data
        // from. Index starts at 0x10 for the 1st Skylander (as reported in the Status command.) The
        // 16th Skylander indexed would be 0x20. The 3rd byte indicate which block to read from.

        // A response with the 2nd byte of 0x01 indicates an error in the read. Otherwise, the
        // response indicates the Skylander's index in the 2nd byte, the block read in the 3rd byte,
        // data (16 bytes) is contained in bytes 4-19.

        // A Skylander has 64 blocks of data indexed from 0x00 to 0x3f. SwapForce characters have 2
        // character indexes, these may not be sequential.
    case 'Q': {
        if (wLength == 3) {
            const u8 sky_num = data[1] & 0xF;
            const u8 block = data[2];
            m_skylander_portal->QueryBlock(sky_num, block, interrupt_response.data());
            m_queries.push(interrupt_response);
            control_response = {data[0], data[1], data[2]};
            expected_count = 11;
        }
        break;
    }
    case 'R': {
        // Ready
        // Command	{ 'R', 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 }
        // Response	{ 'R', 02, 0a, 03, 02, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00 }
        // The 4 byte sequence after the R (0x52) is unknown, but appears consistent based on device
        // type.
        if (wLength == 2) {
            control_response = {0x52, 0x00};
            interrupt_response = {0x52, 0x02, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            m_queries.push(interrupt_response);
            expected_count = 10;
        }
        break;
    }
        // Status
        // Command	{ 'S', 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 }
        // Response	{ 'S', 55, 00, 00, 55, 3e,
        // (00|01), 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00, 00 }
        // Status is the default command. If you open the HID device and
        // activate the portal, you will get status outputs.

        // The 4 bytes {55, 00, 00, 55} are the status of characters on the portal. The 4 bytes are
        // treated as a 32-bit binary array. Each unique Skylander placed on a board is represented
        // by 2 bits starting with the first Skylander in the least significant bit. This bit is
        // present whenever the Skylander is added or present on the portal. When the Skylander is
        // added to the board, both bits are set in the next status message as a one-time signal.
        // When a Skylander is removed from the board, only the most significant bit of the 2 bits
        // is set.

        // Different portals can track a different number of RFID tags. The Wii Wireless portal
        // tracks 4, the Wired portal can track 8. The maximum number of unique Skylanders tracked
        // at any time is 16, after which new Skylanders appear to cycle unused bits.

        // Certain Skylanders, e.g. SwapForce Skylanders, are represented as 2 ID in the bit array.
        // This may be due to the presence of 2 RFIDs, one for each half of the Skylander.

        // The 6th byte {3e} is a counter and increments by one. It will roll over when reaching
        // {ff}.

        // The purpose of the (00\|01) byte at the 7th position appear to indicate if the portal has
        // been activated: {01} when active and {00} when deactivated.
    case 'S': {
        if (wLength == 1) {
            // The Status interrupt responses are automatically handled via the GetStatus method
            control_response = {data[0]};
            expected_count = 9;
        }
        break;
    }
    case 'V': {
        if (wLength == 4) {
            control_response = {data[0], data[1], data[2], data[3]};
            expected_count = 12;
        }
        break;
    }
        // Write
        // Command	{ 'W', 10, 00, 01, 02, 03, 04, 05, 06, 07, 08, 09, 0a, 0b, 0c, 0d, 0e, 0f, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 }
        // Response	{ 'W', 00, 00, 00, 00, 00,
        // 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00,
        // 00, 00, 00, 00 }
        // In the command the 2nd byte indicates which Skylander to query data from. Index starts at
        // 0x10 for the 1st Skylander (as reported in the Status command.) The 16th Skylander
        // indexed would be 0x20.

        // 3rd byte is the block to write to.

        // Bytes 4 - 19 ({ 01, 02, 03, 04, 05, 06, 07, 08, 09, 0a, 0b, 0c, 0d, 0e, 0f }) are the
        // data to write.

        // The response does not appear to return the id of the Skylander being written, the 2nd
        // byte is 0x00; however, the 3rd byte echos the block that was written (0x00 in example
        // above.)

    case 'W': {
        if (wLength == 19) {
            const u8 sky_num = data[1] & 0xF;
            const u8 block = data[2];
            m_skylander_portal->WriteBlock(sky_num, block, &data[3], interrupt_response.data());
            m_queries.push(interrupt_response);
            control_response = {data[0],  data[1],  data[2],  data[3],  data[4],
                                data[5],  data[6],  data[7],  data[8],  data[9],
                                data[10], data[11], data[12], data[13], data[14],
                                data[15], data[16], data[17], data[18]};
            expected_count = 27;
        }
        break;
    }
    default:
        LOG_ERROR(Lib_Usbd, "Unhandled Skylander Portal Query: {}", data[0]);
        break;
    }
    return expected_count;
}

libusb_transfer_status SkylanderBackend::HandleAsyncTransfer(libusb_transfer* transfer) {
    switch (transfer->endpoint) {
    case 0x81:
        if (m_queries.empty()) {
            memcpy(transfer->buffer, m_skylander_portal->GetStatus().data(), 32);
        } else {
            memcpy(transfer->buffer, m_queries.front().data(), 32);
            m_queries.pop();
        }
        transfer->length = 32;
        break;
    case 0x02:
        LOG_INFO(Lib_Usbd, "OUT ENDPOINT");
        break;

    default:
        break;
    }
    return LIBUSB_TRANSFER_COMPLETED;
}

void Skylander::Save() {
    if (!skyFile.IsOpen())
        return;

    skyFile.Seek(0);
    skyFile.Write(data);
}
} // namespace Libraries::Usbd