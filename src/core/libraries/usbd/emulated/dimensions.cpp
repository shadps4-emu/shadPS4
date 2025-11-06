//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "dimensions.h"

#include <mutex>
#include <thread>

namespace Libraries::Usbd {

static constexpr std::array<u8, 16> COMMAND_KEY = {0x55, 0xFE, 0xF6, 0xB0, 0x62, 0xBF, 0x0B, 0x41,
                                                   0xC9, 0xB3, 0x7C, 0xB4, 0x97, 0x3E, 0x29, 0x7B};

static constexpr std::array<u8, 17> CHAR_CONSTANT = {0xB7, 0xD5, 0xD7, 0xE6, 0xE7, 0xBA,
                                                     0x3C, 0xA8, 0xD8, 0x75, 0x47, 0x68,
                                                     0xCF, 0x23, 0xE9, 0xFE, 0xAA};

static constexpr std::array<u8, 25> PWD_CONSTANT = {
    0x28, 0x63, 0x29, 0x20, 0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74,
    0x20, 0x4C, 0x45, 0x47, 0x4F, 0x20, 0x32, 0x30, 0x31, 0x34, 0xAA, 0xAA};

DimensionsToypad::DimensionsToypad() {}

void DimensionsToypad::LoadFigure(std::string file_name, u8 pad, u8 index) {
    Common::FS::IOFile file(file_name, Common::FS::FileAccessMode::ReadWrite);
    std::array<u8, 0x2D * 0x04> data;
    ASSERT(file.Read(data) == data.size());
    LoadDimensionsFigure(data, std::move(file), pad, index);
}

u32 DimensionsToypad::LoadDimensionsFigure(const std::array<u8, 0x2D * 0x04>& buf,
                                           Common::FS::IOFile file, u8 pad, u8 index) {
    std::lock_guard lock(m_dimensions_mutex);

    const u32 id = GetFigureId(buf);

    DimensionsFigure& figure = GetFigureByIndex(index);
    figure.dimFile = std::move(file);
    figure.id = id;
    figure.pad = pad;
    figure.index = index + 1;
    figure.data = buf;
    // When a figure is added to the toypad, respond to the game with the pad they were added to,
    // their index, the direction (0x00 in byte 6 for added) and their UID
    std::array<u8, 32> figureChangeResponse = {0x56,   0x0b,   figure.pad, 0x00,   figure.index,
                                               0x00,   buf[0], buf[1],     buf[2], buf[4],
                                               buf[5], buf[6], buf[7]};
    figureChangeResponse[13] = GenerateChecksum(figureChangeResponse, 13);
    m_figure_added_removed_responses.push(figureChangeResponse);

    return id;
}

void DimensionsToypad::RemoveFigure(u8 pad, u8 index, bool fullRemove) {
    std::lock_guard lock(m_dimensions_mutex);

    DimensionsFigure& figure = GetFigureByIndex(index);
    if (figure.index == 255)
        return;

    // When a figure is removed from the toypad, respond to the game with the pad they were removed
    // from, their index, the direction (0x01 in byte 6 for removed) and their UID
    if (fullRemove) {
        std::array<u8, 32> figureChangeResponse = {
            0x56,           0x0b,           figure.pad,     0x00,           figure.index,
            0x01,           figure.data[0], figure.data[1], figure.data[2], figure.data[4],
            figure.data[5], figure.data[6], figure.data[7]};
        figureChangeResponse[13] = GenerateChecksum(figureChangeResponse, 13);
        m_figure_added_removed_responses.push(figureChangeResponse);
        figure.Save();
        figure.dimFile.Close();
    }

    figure.index = 255;
    figure.pad = 255;
    figure.id = 0;
}

void DimensionsToypad::MoveFigure(u8 new_pad, u8 new_index, u8 old_pad, u8 old_index) {
    if (old_index == new_index) {
        // Don't bother removing and loading again, just send response to the game
        CancelRemoveFigure(new_index);
        return;
    }

    // When moving figures between spaces on the toypad, remove any figure from the space they are
    // moving to, then remove them from their current space, then load them to the space they are
    // moving to
    RemoveFigure(new_pad, new_index, true);

    DimensionsFigure& figure = GetFigureByIndex(old_index);
    const std::array<u8, 0x2D * 0x04> data = figure.data;
    Common::FS::IOFile inFile = std::move(figure.dimFile);

    RemoveFigure(old_pad, old_index, false);

    LoadDimensionsFigure(data, std::move(inFile), new_pad, new_index);
}

void DimensionsToypad::TempRemoveFigure(u8 index) {
    std::lock_guard lock(m_dimensions_mutex);

    DimensionsFigure& figure = GetFigureByIndex(index);
    if (figure.index == 255)
        return;

    // Send a response to the game that the figure has been "Picked up" from existing slot,
    // until either the movement is cancelled, or user chooses a space to move to
    std::array<u8, 32> figureChangeResponse = {
        0x56,           0x0b,           figure.pad,     0x00,           figure.index,
        0x01,           figure.data[0], figure.data[1], figure.data[2], figure.data[4],
        figure.data[5], figure.data[6], figure.data[7]};

    figureChangeResponse[13] = GenerateChecksum(figureChangeResponse, 13);
    m_figure_added_removed_responses.push(figureChangeResponse);
}

void DimensionsToypad::CancelRemoveFigure(u8 index) {
    std::lock_guard lock(m_dimensions_mutex);

    DimensionsFigure& figure = GetFigureByIndex(index);
    if (figure.index == 255)
        return;

    // Cancel the previous movement of the figure
    std::array<u8, 32> figureChangeResponse = {
        0x56,           0x0b,           figure.pad,     0x00,           figure.index,
        0x00,           figure.data[0], figure.data[1], figure.data[2], figure.data[4],
        figure.data[5], figure.data[6], figure.data[7]};

    figureChangeResponse[13] = GenerateChecksum(figureChangeResponse, 13);
    m_figure_added_removed_responses.push(figureChangeResponse);
}

u8 DimensionsToypad::GenerateChecksum(const std::array<u8, 32>& data, u32 num_of_bytes) {
    int checksum = 0;
    ASSERT(num_of_bytes <= data.size());
    for (u8 i = 0; i < num_of_bytes; i++) {
        checksum += data[i];
    }
    return (checksum & 0xFF);
}

void DimensionsToypad::GetBlankResponse(u8 type, u8 sequence, std::array<u8, 32>& reply_buf) {
    reply_buf[0] = 0x55;
    reply_buf[1] = type;
    reply_buf[2] = sequence;
    reply_buf[3] = GenerateChecksum(reply_buf, 3);
}

void DimensionsToypad::GenerateRandomNumber(const u8* buf, u8 sequence,
                                            std::array<u8, 32>& reply_buf) {
    // Decrypt payload into an 8 byte array
    const std::array<u8, 8> value = Decrypt(buf, std::nullopt);
    // Seed is the first 4 bytes (little endian) of the decrypted payload
    const u32 seed = (u32&)value[0];
    // Confirmation is the second 4 bytes (big endian) of the decrypted payload
    // const u32 conf = (u32be&)value[4];
    // Initialize rng using the seed from decrypted payload
    InitializeRNG(seed);
    // Encrypt 8 bytes, first 4 bytes is the decrypted confirmation from payload, 2nd 4 bytes are
    // blank
    std::array<u8, 8> value_to_encrypt = {value[4], value[5], value[6], value[7], 0, 0, 0, 0};
    const std::array<u8, 8> encrypted = Encrypt(value_to_encrypt.data(), std::nullopt);
    reply_buf[0] = 0x55;
    reply_buf[1] = 0x09;
    reply_buf[2] = sequence;
    // Copy encrypted value to response data
    std::memcpy(&reply_buf[3], encrypted.data(), encrypted.size());
    reply_buf[11] = GenerateChecksum(reply_buf, 11);
}

void DimensionsToypad::InitializeRNG(u32 seed) {
    m_random_a = 0xF1EA5EED;
    m_random_b = seed;
    m_random_c = seed;
    m_random_d = seed;

    for (int i = 0; i < 42; i++) {
        GetNext();
    }
}

u32 DimensionsToypad::GetNext() {
    const u32 e = m_random_a - std::rotl(m_random_b, 21);
    m_random_a = m_random_b ^ std::rotl(m_random_c, 19);
    m_random_b = m_random_c + std::rotl(m_random_d, 6);
    m_random_c = m_random_d + e;
    m_random_d = e + m_random_a;
    return m_random_d;
}

std::array<u8, 8> DimensionsToypad::Decrypt(const u8* buf, std::optional<std::array<u8, 16>> key) {
    // Value to decrypt is separated in to two little endian 32 bit unsigned integers
    u32 data_one = u32(buf[0]) | (u32(buf[1]) << 8) | (u32(buf[2]) << 16) | (u32(buf[3]) << 24);
    u32 data_two = u32(buf[4]) | (u32(buf[5]) << 8) | (u32(buf[6]) << 16) | (u32(buf[7]) << 24);

    // Use the key as 4 32 bit little endian unsigned integers
    u32 key_one;
    u32 key_two;
    u32 key_three;
    u32 key_four;

    if (key) {
        key_one = u32(key.value()[0]) | (u32(key.value()[1]) << 8) | (u32(key.value()[2]) << 16) |
                  (u32(key.value()[3]) << 24);
        key_two = u32(key.value()[4]) | (u32(key.value()[5]) << 8) | (u32(key.value()[6]) << 16) |
                  (u32(key.value()[7]) << 24);
        key_three = u32(key.value()[8]) | (u32(key.value()[9]) << 8) |
                    (u32(key.value()[10]) << 16) | (u32(key.value()[11]) << 24);
        key_four = u32(key.value()[12]) | (u32(key.value()[13]) << 8) |
                   (u32(key.value()[14]) << 16) | (u32(key.value()[15]) << 24);
    } else {
        key_one = u32(COMMAND_KEY[0]) | (u32(COMMAND_KEY[1]) << 8) | (u32(COMMAND_KEY[2]) << 16) |
                  (u32(COMMAND_KEY[3]) << 24);
        key_two = u32(COMMAND_KEY[4]) | (u32(COMMAND_KEY[5]) << 8) | (u32(COMMAND_KEY[6]) << 16) |
                  (u32(COMMAND_KEY[7]) << 24);
        key_three = u32(COMMAND_KEY[8]) | (u32(COMMAND_KEY[9]) << 8) |
                    (u32(COMMAND_KEY[10]) << 16) | (u32(COMMAND_KEY[11]) << 24);
        key_four = u32(COMMAND_KEY[12]) | (u32(COMMAND_KEY[13]) << 8) |
                   (u32(COMMAND_KEY[14]) << 16) | (u32(COMMAND_KEY[15]) << 24);
    }

    u32 sum = 0xC6EF3720;
    constexpr u32 delta = 0x9E3779B9;

    for (int i = 0; i < 32; i++) {
        data_two -=
            (((data_one << 4) + key_three) ^ (data_one + sum) ^ ((data_one >> 5) + key_four));
        data_one -= (((data_two << 4) + key_one) ^ (data_two + sum) ^ ((data_two >> 5) + key_two));
        sum -= delta;
    }

    ASSERT_MSG(sum == 0, "Decryption failed, sum inequal to 0");

    std::array<u8, 8> decrypted = {u8(data_one & 0xFF),         u8((data_one >> 8) & 0xFF),
                                   u8((data_one >> 16) & 0xFF), u8((data_one >> 24) & 0xFF),
                                   u8(data_two & 0xFF),         u8((data_two >> 8) & 0xFF),
                                   u8((data_two >> 16) & 0xFF), u8((data_two >> 24) & 0xFF)};
    return decrypted;
}

std::array<u8, 8> DimensionsToypad::Encrypt(const u8* buf, std::optional<std::array<u8, 16>> key) {
    // Value to encrypt is separated in to two little endian 32 bit unsigned integers
    u32 data_one = u32(buf[0]) | (u32(buf[1]) << 8) | (u32(buf[2]) << 16) | (u32(buf[3]) << 24);
    u32 data_two = u32(buf[4]) | (u32(buf[5]) << 8) | (u32(buf[6]) << 16) | (u32(buf[7]) << 24);

    // Use the key as 4 32 bit little endian unsigned integers
    u32 key_one;
    u32 key_two;
    u32 key_three;
    u32 key_four;

    if (key) {
        key_one = u32(key.value()[0]) | (u32(key.value()[1]) << 8) | (u32(key.value()[2]) << 16) |
                  (u32(key.value()[3]) << 24);
        key_two = u32(key.value()[4]) | (u32(key.value()[5]) << 8) | (u32(key.value()[6]) << 16) |
                  (u32(key.value()[7]) << 24);
        key_three = u32(key.value()[8]) | (u32(key.value()[9]) << 8) |
                    (u32(key.value()[10]) << 16) | (u32(key.value()[11]) << 24);
        key_four = u32(key.value()[12]) | (u32(key.value()[13]) << 8) |
                   (u32(key.value()[14]) << 16) | (u32(key.value()[15]) << 24);
    } else {
        key_one = u32(COMMAND_KEY[0]) | (u32(COMMAND_KEY[1]) << 8) | (u32(COMMAND_KEY[2]) << 16) |
                  (u32(COMMAND_KEY[3]) << 24);
        key_two = u32(COMMAND_KEY[4]) | (u32(COMMAND_KEY[5]) << 8) | (u32(COMMAND_KEY[6]) << 16) |
                  (u32(COMMAND_KEY[7]) << 24);
        key_three = u32(COMMAND_KEY[8]) | (u32(COMMAND_KEY[9]) << 8) |
                    (u32(COMMAND_KEY[10]) << 16) | (u32(COMMAND_KEY[11]) << 24);
        key_four = u32(COMMAND_KEY[12]) | (u32(COMMAND_KEY[13]) << 8) |
                   (u32(COMMAND_KEY[14]) << 16) | (u32(COMMAND_KEY[15]) << 24);
    }

    u32 sum = 0;
    u32 delta = 0x9E3779B9;

    for (int i = 0; i < 32; i++) {
        sum += delta;
        data_one += (((data_two << 4) + key_one) ^ (data_two + sum) ^ ((data_two >> 5) + key_two));
        data_two +=
            (((data_one << 4) + key_three) ^ (data_one + sum) ^ ((data_one >> 5) + key_four));
    }

    std::array<u8, 8> encrypted = {u8(data_one & 0xFF),         u8((data_one >> 8) & 0xFF),
                                   u8((data_one >> 16) & 0xFF), u8((data_one >> 24) & 0xFF),
                                   u8(data_two & 0xFF),         u8((data_two >> 8) & 0xFF),
                                   u8((data_two >> 16) & 0xFF), u8((data_two >> 24) & 0xFF)};
    return encrypted;
}

std::array<u8, 16> DimensionsToypad::GenerateFigureKey(const std::array<u8, 0x2D * 0x04>& buf) {
    std::array<u8, 7> uid = {buf[0], buf[1], buf[2], buf[4], buf[5], buf[6], buf[7]};

    u32 scrambleA = Scramble(uid, 3);
    u32 scrambleB = Scramble(uid, 4);
    u32 scrambleC = Scramble(uid, 5);
    u32 scrambleD = Scramble(uid, 6);

    return {
        u8((scrambleA >> 24) & 0xFF), u8((scrambleA >> 16) & 0xFF), u8((scrambleA >> 8) & 0xFF),
        u8(scrambleA & 0xFF),         u8((scrambleB >> 24) & 0xFF), u8((scrambleB >> 16) & 0xFF),
        u8((scrambleB >> 8) & 0xFF),  u8(scrambleB & 0xFF),         u8((scrambleC >> 24) & 0xFF),
        u8((scrambleC >> 16) & 0xFF), u8((scrambleC >> 8) & 0xFF),  u8(scrambleC & 0xFF),
        u8((scrambleD >> 24) & 0xFF), u8((scrambleD >> 16) & 0xFF), u8((scrambleD >> 8) & 0xFF),
        u8(scrambleD & 0xFF)};
}

u32 DimensionsToypad::Scramble(const std::array<u8, 7>& uid, u8 count) {
    std::vector<u8> to_scramble;
    to_scramble.reserve(uid.size() + CHAR_CONSTANT.size());
    for (u8 x : uid) {
        to_scramble.push_back(x);
    }
    for (u8 c : CHAR_CONSTANT) {
        to_scramble.push_back(c);
    }
    to_scramble[(count * 4) - 1] = 0xaa;

    std::array<u8, 4> randomized = DimensionsRandomize(to_scramble, count);

    return (u32(randomized[0]) << 24) | (u32(randomized[1]) << 16) | (u32(randomized[2]) << 8) |
           u32(randomized[3]);
}

std::array<u8, 4> DimensionsToypad::PWDGenerate(const std::array<u8, 7>& uid) {
    std::vector<u8> pwdCalc = {PWD_CONSTANT.begin(), PWD_CONSTANT.end() - 1};
    for (u8 i = 0; i < uid.size(); i++) {
        pwdCalc.insert(pwdCalc.begin() + i, uid[i]);
    }

    return DimensionsRandomize(pwdCalc, 8);
}

std::array<u8, 4> DimensionsToypad::DimensionsRandomize(const std::vector<u8>& key, u8 count) {
    u32 scrambled = 0;
    for (u8 i = 0; i < count; i++) {
        const u32 v4 = std::rotr(scrambled, 25);
        const u32 v5 = std::rotr(scrambled, 10);
        const u32 b = u32(key[i * 4]) | (u32(key[(i * 4) + 1]) << 8) |
                      (u32(key[(i * 4) + 2]) << 16) | (u32(key[(i * 4) + 3]) << 24);
        scrambled = b + v4 + v5 - scrambled;
    }
    return {u8(scrambled & 0xFF), u8(scrambled >> 8 & 0xFF), u8(scrambled >> 16 & 0xFF),
            u8(scrambled >> 24 & 0xFF)};
}

u32 DimensionsToypad::GetFigureId(const std::array<u8, 0x2D * 0x04>& buf) {
    const std::array<u8, 16> figure_key = GenerateFigureKey(buf);

    const std::array<u8, 8> decrypted = Decrypt(&buf[36 * 4], figure_key);

    const u32 fig_num = u32(decrypted[0]) | (u32(decrypted[1]) << 8) | (u32(decrypted[2]) << 16) |
                        (u32(decrypted[3]) << 24);
    // Characters have their model number encrypted in page 36
    if (fig_num < 1000) {
        return fig_num;
    }
    // Vehicles/Gadgets have their model number written as little endian in page 36
    return u32(buf[36 * 4]) | (u32(buf[(36 * 4) + 1]) << 8) | (u32(buf[(36 * 4) + 2]) << 16) |
           (u32(buf[(36 * 4) + 3]) << 24);
}

DimensionsFigure& DimensionsToypad::GetFigureByIndex(u8 index) {
    return m_figures[index];
}

void DimensionsToypad::RandomUID(u8* uid_buffer) {
    uid_buffer[0] = 0x04;
    uid_buffer[7] = 0x80;

    for (u8 i = 1; i < 7; i++) {
        u8 random = rand() % 255;
        uid_buffer[i] = random;
    }
}

void DimensionsToypad::GetChallengeResponse(const u8* buf, u8 sequence,
                                            std::array<u8, 32>& reply_buf) {
    // Decrypt payload into an 8 byte array
    const std::array<u8, 8> value = Decrypt(buf, std::nullopt);
    // Confirmation is the first 4 bytes of the decrypted payload
    // const u32 conf = read_from_ptr<be_t<u32>>(value);
    // Generate next random number based on RNG
    const u32 next_random = GetNext();
    // Encrypt an 8 byte array, first 4 bytes are the next random number (little endian)
    // followed by the confirmation from the decrypted payload
    std::array<u8, 8> value_to_encrypt = {u8(next_random & 0xFF),
                                          u8((next_random >> 8) & 0xFF),
                                          u8((next_random >> 16) & 0xFF),
                                          u8((next_random >> 24) & 0xFF),
                                          value[0],
                                          value[1],
                                          value[2],
                                          value[3]};
    const std::array<u8, 8> encrypted = Encrypt(value_to_encrypt.data(), std::nullopt);
    reply_buf[0] = 0x55;
    reply_buf[1] = 0x09;
    reply_buf[2] = sequence;
    // Copy encrypted value to response data
    std::memcpy(&reply_buf[3], encrypted.data(), encrypted.size());
    reply_buf[11] = GenerateChecksum(reply_buf, 11);
}

void DimensionsToypad::QueryBlock(u8 index, u8 page, std::array<u8, 32>& reply_buf, u8 sequence) {
    std::lock_guard lock(m_dimensions_mutex);

    reply_buf[0] = 0x55;
    reply_buf[1] = 0x12;
    reply_buf[2] = sequence;
    reply_buf[3] = 0x00;

    // Index from game begins at 1 rather than 0, so minus 1 here
    if (const u8 figure_index = index - 1; figure_index < MAX_DIMENSIONS_FIGURES) {
        const DimensionsFigure& figure = GetFigureByIndex(figure_index);

        // Query 4 pages of 4 bytes from the figure, copy this to the response
        if (figure.index != 255 && (4 * page) < ((0x2D * 4) - 16)) {
            std::memcpy(&reply_buf[4], figure.data.data() + (4 * page), 16);
        }
    }
    reply_buf[20] = GenerateChecksum(reply_buf, 20);
}

void DimensionsToypad::WriteBlock(u8 index, u8 page, const u8* to_write_buf,
                                  std::array<u8, 32>& reply_buf, u8 sequence) {
    std::lock_guard lock(m_dimensions_mutex);

    reply_buf[0] = 0x55;
    reply_buf[1] = 0x02;
    reply_buf[2] = sequence;
    reply_buf[3] = 0x00;

    // Index from game begins at 1 rather than 0, so minus 1 here
    if (const u8 figure_index = index - 1; figure_index < MAX_DIMENSIONS_FIGURES) {
        DimensionsFigure& figure = GetFigureByIndex(figure_index);

        // Copy 4 bytes to the page on the figure requested by the game
        if (figure.index != 255 && page < 0x2D) {
            // Id is written to page 36
            if (page == 36) {
                figure.id = u32(to_write_buf[0]) | (u32(to_write_buf[1]) << 8) |
                            (u32(to_write_buf[2]) << 16) | (u32(to_write_buf[3]) << 24);
            }
            std::memcpy(figure.data.data() + (page * 4), to_write_buf, 4);
            figure.Save();
        }
    }
    reply_buf[4] = GenerateChecksum(reply_buf, 4);
}

void DimensionsToypad::GetModel(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf) {
    // Decrypt payload to 8 byte array, byte 1 is the index, 4-7 are the confirmation
    const std::array<u8, 8> value = Decrypt(buf, std::nullopt);
    const u8 index = value[0];
    // const u32 conf = read_from_ptr<be_t<u32>>(value, 4);
    std::array<u8, 8> value_to_encrypt = {};
    // Response is the figure's id (little endian) followed by the confirmation from payload
    // Index from game begins at 1 rather than 0, so minus 1 here
    if (const u8 figure_index = index - 1; figure_index < MAX_DIMENSIONS_FIGURES) {
        const DimensionsFigure& figure = GetFigureByIndex(figure_index);
        value_to_encrypt = {u8(figure.id & 0xFF),
                            u8((figure.id >> 8) & 0xFF),
                            u8((figure.id >> 16) & 0xFF),
                            u8((figure.id >> 24) & 0xFF),
                            value[4],
                            value[5],
                            value[6],
                            value[7]};
    }
    const std::array<u8, 8> encrypted = Encrypt(value_to_encrypt.data(), std::nullopt);
    reply_buf[0] = 0x55;
    reply_buf[1] = 0x0a;
    reply_buf[2] = sequence;
    reply_buf[3] = 0x00;
    // Copy encrypted message to response
    std::memcpy(&reply_buf[4], encrypted.data(), encrypted.size());
    reply_buf[12] = GenerateChecksum(reply_buf, 12);
}

std::optional<std::array<u8, 32>> DimensionsToypad::PopAddedRemovedResponse() {
    std::lock_guard lock(m_dimensions_mutex);

    if (m_figure_added_removed_responses.empty()) {
        return std::nullopt;
    }

    std::array<u8, 32> response = m_figure_added_removed_responses.front();
    m_figure_added_removed_responses.pop();
    return response;
}

libusb_endpoint_descriptor* DimensionsBackend::FillEndpointDescriptorPair() {
    return m_endpoint_descriptors.data();
}

libusb_interface_descriptor* DimensionsBackend::FillInterfaceDescriptor(
    libusb_endpoint_descriptor* descs) {
    m_interface_descriptors[0].endpoint = descs;
    return m_interface_descriptors.data();
}

libusb_config_descriptor* DimensionsBackend::FillConfigDescriptor(libusb_interface* inter) {
    m_config_descriptors[0].interface = inter;
    return m_config_descriptors.data();
}

libusb_device_descriptor* DimensionsBackend::FillDeviceDescriptor() {
    return m_device_descriptors.data();
}

libusb_transfer_status DimensionsBackend::HandleAsyncTransfer(libusb_transfer* transfer) {
    ASSERT(transfer->length == 32);

    switch (transfer->endpoint) {
    case 0x81: {
        // Read Endpoint, wait to respond with either an added/removed figure response, or a queued
        // response from a previous write
        bool responded = false;
        while (!responded) {
            std::lock_guard lock(m_query_mutex);
            std::optional<std::array<u8, 32>> response =
                m_dimensions_toypad->PopAddedRemovedResponse();
            if (response) {
                std::memcpy(transfer->buffer, response.value().data(), 0x20);
                transfer->length = 32;
                responded = true;
            } else if (!m_queries.empty()) {
                std::memcpy(transfer->buffer, m_queries.front().data(), 0x20);
                transfer->length = 32;
                m_queries.pop();
                responded = true;
            }
        }
        break;
    }
    case 0x01: {
        // Write endpoint, similar structure of request to the Infinity Base with a command for byte
        // 3, sequence for byte 4, the payload after that, then a checksum for the final byte.

        const u8 command = transfer->buffer[2];
        const u8 sequence = transfer->buffer[3];

        std::array<u8, 32> q_result{};

        switch (command) {
        case 0xB0: // Wake
        {
            // Consistent device response to the wake command
            q_result = {0x55, 0x0e, 0x01, 0x28, 0x63, 0x29, 0x20, 0x4c, 0x45,
                        0x47, 0x4f, 0x20, 0x32, 0x30, 0x31, 0x34, 0x46};
            break;
        }
        case 0xB1: // Seed
        {
            // Initialise a random number generator using the seed provided
            m_dimensions_toypad->GenerateRandomNumber(&transfer->buffer[4], sequence, q_result);
            break;
        }
        case 0xB3: // Challenge
        {
            // Get the next number in the sequence based on the RNG from 0xB1 command
            m_dimensions_toypad->GetChallengeResponse(&transfer->buffer[4], sequence, q_result);
            break;
        }
        case 0xC0: // Color
        case 0xC1: // Get Pad Color
        case 0xC2: // Fade
        case 0xC3: // Flash
        case 0xC4: // Fade Random
        case 0xC6: // Fade All
        case 0xC7: // Flash All
        case 0xC8: // Color All
        {
            // Send a blank response to acknowledge color has been sent to toypad
            m_dimensions_toypad->GetBlankResponse(0x01, sequence, q_result);
            break;
        }
        case 0xD2: // Read
        {
            // Read 4 pages from the figure at index (buf[4]), starting with page buf[5]
            m_dimensions_toypad->QueryBlock(transfer->buffer[4], transfer->buffer[5], q_result,
                                            sequence);
            break;
        }
        case 0xD3: // Write
        {
            // Write 4 bytes to page buf[5] to the figure at index buf[4]
            m_dimensions_toypad->WriteBlock(transfer->buffer[4], transfer->buffer[5],
                                            &transfer->buffer[6], q_result, sequence);
            break;
        }
        case 0xD4: // Model
        {
            // Get the model id of the figure at index buf[4]
            m_dimensions_toypad->GetModel(&transfer->buffer[4], sequence, q_result);
            break;
        }
        case 0xD0: // Tag List
        case 0xE1: // PWD
        case 0xE5: // Active
        case 0xFF: // LEDS Query
        {
            // Further investigation required
            LOG_ERROR(Lib_Usbd, "Unimplemented LD Function: {:x}", command);
            break;
        }
        default: {
            LOG_ERROR(Lib_Usbd, "Unknown LD Function: {:x}", command);
            break;
        }
        }
        std::lock_guard lock(m_query_mutex);
        m_queries.push(q_result);
        break;
    }
    default:
        break;
    }
    return LIBUSB_TRANSFER_COMPLETED;
}

s32 DimensionsBackend::SubmitTransfer(libusb_transfer* transfer) {
    if (transfer->endpoint == 0x01) {
        std::thread write_thread([this, transfer] {
            HandleAsyncTransfer(transfer);

            const u8 flags = transfer->flags;
            transfer->status = LIBUSB_TRANSFER_COMPLETED;
            transfer->actual_length = transfer->length;
            if (transfer->callback) {
                transfer->callback(transfer);
            }
            if (flags & LIBUSB_TRANSFER_FREE_TRANSFER) {
                libusb_free_transfer(transfer);
            }
        });
        write_thread.detach();
        return LIBUSB_SUCCESS;
    }

    return UsbEmulatedBackend::SubmitTransfer(transfer);
}

void DimensionsFigure::Save() {
    if (!dimFile.IsOpen())
        return;

    dimFile.Seek(0);
    dimFile.Write(data);
}
} // namespace Libraries::Usbd