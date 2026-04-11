#include "client.h"
// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

ShadNetClient::ShadNetClient() {}

ShadNetClient::~ShadNetClient() {}

void ShadNetClient::Start(const std::string& host, u16 port, const std::string& npid,
                          const std::string& password, const std::string& token) {}

void ShadNetClient::Stop() {}

ShadNetState ShadNetClient::WaitForConnection() {
    return ShadNetState::Ok;
}

ShadNetState ShadNetClient::WaitForAuthenticated() {
    return ShadNetState::Ok;
}

bool ShadNetClient::IsConnected() const {
    return false;
}

bool ShadNetClient::IsAuthenticated() const {
    return false;
}

ShadNetState ShadNetClient::GetState() const {
    return ShadNetState::Ok;
}

const std::string& ShadNetClient::GetOnlineName() const {
    static std::string empty;
    return empty;
}

const std::string& ShadNetClient::GetAvatarUrl() const {
    static std::string empty;
    return empty;
}

u64 ShadNetClient::GetUserId() const {
    return 0;
}

u32 ShadNetClient::GetAddrLocal() const {
    return 0;
}

u32 ShadNetClient::GetNumFriends() const {
    return 0;
}

std::optional<std::string> ShadNetClient::GetFriendNpid(u32 index) const {
    return std::nullopt;
}

void ShadNetClient::ConnectThread() {}

void ShadNetClient::ReaderThread() {}

void ShadNetClient::WriterThread() {}

bool ShadNetClient::DoConnect() {
    return false;
}

void ShadNetClient::DoDisconnect() {}

bool ShadNetClient::RecvN(u8* buf, u32 n) {
    return false;
}

bool ShadNetClient::SendAll(const std::vector<u8>& data) {
    return false;
}

std::vector<u8> ShadNetClient::BuildPacket(CommandType cmd, u64 id,
                                           const std::vector<u8>& payload) const {
    return {};
}

void ShadNetClient::DispatchPacket(PacketType type, u16 cmd_raw, const std::vector<u8>& payload) {}

void ShadNetClient::HandleLoginReply(const std::vector<u8>& payload) {}

// helper functions for little-endian encoding/decoding and string reading
std::string ShadNetClient::ReadStr(const std::vector<u8>& p, int& pos) {
    std::string s;
    while (pos < static_cast<int>(p.size()) && p[pos] != 0)
        s += static_cast<char>(p[pos++]);
    if (pos < static_cast<int>(p.size()))
        ++pos; // consume '\0'
    return s;
}

void ShadNetClient::PutLE16(std::vector<u8>& b, size_t off, u16 v) {
    b[off] = static_cast<u8>(v);
    b[off + 1] = static_cast<u8>(v >> 8);
}
void ShadNetClient::PutLE32(std::vector<u8>& b, size_t off, u32 v) {
    b[off] = static_cast<u8>(v);
    b[off + 1] = static_cast<u8>(v >> 8);
    b[off + 2] = static_cast<u8>(v >> 16);
    b[off + 3] = static_cast<u8>(v >> 24);
}
void ShadNetClient::PutLE64(std::vector<u8>& b, size_t off, u64 v) {
    for (int i = 0; i < 8; ++i)
        b[off + i] = static_cast<u8>(v >> (8 * i));
}
u16 ShadNetClient::GetLE16(const u8* p) {
    return static_cast<u16>(p[0]) | (static_cast<u16>(p[1]) << 8);
}
u32 ShadNetClient::GetLE32(const u8* p) {
    return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) | (static_cast<u32>(p[2]) << 16) |
           (static_cast<u32>(p[3]) << 24);
}
u64 ShadNetClient::GetLE64(const u8* p) {
    u64 v = 0;
    for (int i = 0; i < 8; ++i)
        v |= static_cast<u64>(p[i]) << (8 * i);
    return v;
}

// skip presence , rpcs3 has it i dunno if we need it so although there is a place for this we don't
// support it yet so skip it.
bool ShadNetClient::SkipPresence(const std::vector<u8>& p, int& pos) {
    if (pos + 12 > static_cast<int>(p.size()))
        return false;
    pos += 12;

    for (int i = 0; i < 3; ++i) {
        while (pos < static_cast<int>(p.size()) && p[pos] != 0)
            ++pos;
        if (pos >= static_cast<int>(p.size()))
            return false;
        ++pos; // consume '\0'
    }

    if (pos + 4 > static_cast<int>(p.size()))
        return false;
    const u32 dlen = GetLE32(p.data() + pos);
    pos += 4 + static_cast<int>(dlen);
    return pos <= static_cast<int>(p.size());
}

u32 ShadNetClient::ReadU32LE(const std::vector<u8>& p, int& pos) {
    if (pos + 4 > static_cast<int>(p.size()))
        return 0;
    const u32 v = GetLE32(p.data() + pos);
    pos += 4;
    return v;
}

u64 ShadNetClient::ReadU64LE(const std::vector<u8>& p, int& pos) {
    if (pos + 8 > static_cast<int>(p.size()))
        return 0;
    const u64 v = GetLE64(p.data() + pos);
    pos += 8;
    return v;
}
