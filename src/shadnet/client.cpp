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

std::string ShadNetClient::ReadStr(const std::vector<u8>& p, int& pos) {
    return {};
}

bool ShadNetClient::SkipPresence(const std::vector<u8>& p, int& pos) {
    return false;
}

u32 ShadNetClient::ReadU32LE(const std::vector<u8>& p, int& pos) {
    return 0;
}

u64 ShadNetClient::ReadU64LE(const std::vector<u8>& p, int& pos) {
    return 0;
}

void ShadNetClient::PutLE16(std::vector<u8>& b, size_t off, u16 v) {}

void ShadNetClient::PutLE32(std::vector<u8>& b, size_t off, u32 v) {}

void ShadNetClient::PutLE64(std::vector<u8>& b, size_t off, u64 v) {}

u16 ShadNetClient::GetLE16(const u8* p) {
    return 0;
}

u32 ShadNetClient::GetLE32(const u8* p) {
    return 0;
}

u64 ShadNetClient::GetLE64(const u8* p) {
    return 0;
}
