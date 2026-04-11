#include <common/logging/log.h>
#include "client.h"
// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
static void PlatformInit() {
    static bool done = false;
    if (!done) {
        WSADATA wd;
        WSAStartup(
            MAKEWORD(2, 2),
            &wd); // might already be called from somwewhere else in shadps4 but make it sure...
        done = true;
    }
}
#else
static void PlatformInit() {}
#endif

ShadNetClient::ShadNetClient() {
    PlatformInit();
}

ShadNetClient::~ShadNetClient() {
    Stop();
}

void ShadNetClient::Start(const std::string& host, u16 port, const std::string& npid,
                          const std::string& password, const std::string& token) {
    m_host = host;
    m_port = port;
    m_npid = npid;
    m_password = password;
    m_token = token;
    m_terminate = false;

    m_thread_connect = std::thread(&ShadNetClient::ConnectThread, this);
}

void ShadNetClient::Stop() {
    m_terminate = true;

    // Unblock any WaitFor* callers that haven't fired yet
    try {
        m_sem_connected.release();
    } catch (...) {
    }
    try {
        m_sem_authenticated.release();
    } catch (...) {
    }

    // Wake the writer thread
    {
        std::lock_guard lock(m_mutex_send_queue);
        m_cv_send_queue.notify_all();
    }

    // Closing the socket causes any blocking RecvN() to return immediately
    DoDisconnect();

    if (m_thread_connect.joinable())
        m_thread_connect.join();
    if (m_thread_reader.joinable())
        m_thread_reader.join();
    if (m_thread_writer.joinable())
        m_thread_writer.join();
}
// blocking calls
ShadNetState ShadNetClient::WaitForConnection() {
    {
        std::lock_guard lock(m_mutex_connected);
        if (m_connected)
            return ShadNetState::Ok;
    }
    m_sem_connected.acquire();
    return m_connected ? ShadNetState::Ok : m_state.load();
}

ShadNetState ShadNetClient::WaitForAuthenticated() {
    {
        std::lock_guard lock(m_mutex_authenticated);
        if (m_authenticated)
            return ShadNetState::Ok;
    }
    m_sem_authenticated.acquire();
    return m_authenticated ? ShadNetState::Ok : m_state.load();
}
// getters/setters
bool ShadNetClient::IsConnected() const {
    return m_connected.load();
}
bool ShadNetClient::IsAuthenticated() const {
    return m_authenticated.load();
}
ShadNetState ShadNetClient::GetState() const {
    return m_state.load();
}

const std::string& ShadNetClient::GetOnlineName() const {
    return m_online_name;
}
const std::string& ShadNetClient::GetAvatarUrl() const {
    return m_avatar_url;
}
u64 ShadNetClient::GetUserId() const {
    return m_user_id;
}
u32 ShadNetClient::GetAddrLocal() const {
    return m_addr_local.load();
}

u32 ShadNetClient::GetNumFriends() const {
    std::lock_guard lock(m_mutex_friends);
    return static_cast<u32>(m_friends.size());
}
std::optional<std::string> ShadNetClient::GetFriendNpid(u32 index) const {
    std::lock_guard lock(m_mutex_friends);
    if (index >= m_friends.size())
        return std::nullopt;
    return m_friends[index].npid;
}

// threading
void ShadNetClient::ConnectThread() {
    if (!DoConnect()) {
        m_sem_connected.release();
        m_sem_authenticated.release();
        return;
    }

    // Start the reader and writer threads now that the socket is open
    m_thread_reader = std::thread(&ShadNetClient::ReaderThread, this);
    m_thread_writer = std::thread(&ShadNetClient::WriterThread, this);

    m_sem_connected.release();

    // Build and send Login payload: npid\0  password\0  token\0
    std::vector<u8> payload;
    auto addStr = [&](const std::string& s) {
        payload.insert(payload.end(), s.begin(), s.end());
        payload.push_back(0);
    };
    addStr(m_npid);
    addStr(m_password);
    addStr(m_token);

    const u64 id = m_pkt_counter.fetch_add(1);
    if (!SendAll(BuildPacket(CommandType::Login, id, payload))) {
        LOG_ERROR(ShadNet, "ShadNet: Failed to send Login packet");
        m_state = ShadNetState::FailureOther;
        // ReaderThread will detect the dead socket and release sem_authenticated
        return;
    }

    LOG_INFO(ShadNet, "ShadNet: Login packet sent for '{}'", m_npid);
    // ConnectThread exits here. ReaderThread receives the reply.
}

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
