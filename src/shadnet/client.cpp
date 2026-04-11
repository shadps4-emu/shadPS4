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

    LOG_INFO(ShadNet, "Login packet sent for '{}'", m_npid);
    // ConnectThread exits here. ReaderThread receives the reply.
}

// ReaderThread for blocking RecvN loop
void ShadNetClient::ReaderThread() {
    while (!m_terminate) {
        // Read header (15 bytes)
        u8 hdr[SHAD_HEADER_SIZE];
        if (!RecvN(hdr, SHAD_HEADER_SIZE)) {
            if (!m_terminate)
                LOG_WARNING(ShadNet, "Reader header recv failed, disconnecting");
            break;
        }

        const auto ptype = static_cast<PacketType>(hdr[0]);
        const u16 cmd_raw = GetLE16(hdr + 1);
        const u32 total_sz = GetLE32(hdr + 3);
        // hdr[7..14] = packet ID (not needed by dispatcher but available)

        if (total_sz < SHAD_HEADER_SIZE || total_sz > SHAD_MAX_PACKET_SIZE) {
            LOG_ERROR(ShadNet, "Corrupt packet (total_sz={})", total_sz);
            m_state = ShadNetState::FailureProtocol;
            break;
        }

        std::vector<u8> payload;
        const u32 payload_sz = total_sz - static_cast<u32>(SHAD_HEADER_SIZE);
        if (payload_sz > 0) {
            payload.resize(payload_sz);
            if (!RecvN(payload.data(), payload_sz)) {
                if (!m_terminate)
                    LOG_WARNING(ShadNet, "Reader payload recv failed");
                break;
            }
        }

        DispatchPacket(ptype, cmd_raw, payload);
    }

    // Socket is gone,make sure NpHandler wakes up if still waiting
    if (!m_authenticated) {
        if (m_state == ShadNetState::Ok)
            m_state = ShadNetState::FailureOther;
        m_sem_authenticated.release();
    }

    m_connected = false;
    m_authenticated = false;

    LOG_INFO(ShadNet, "ReaderThread exiting");
}

// WriterThread it drains m_send_queue
void ShadNetClient::WriterThread() {
    while (!m_terminate) {
        std::unique_lock lock(m_mutex_send_queue);
        m_cv_send_queue.wait(lock, [&] { return m_terminate.load() || !m_send_queue.empty(); });
        if (m_terminate)
            break;

        std::vector<std::vector<u8>> batch;
        std::swap(batch, m_send_queue);
        lock.unlock();

        for (auto& pkt : batch) {
            if (!m_connected)
                break;
            if (!SendAll(pkt)) {
                LOG_ERROR(ShadNet, "WriterThread send failed");
                return;
            }
        }
    }
}

// connected / disconnected methods
bool ShadNetClient::DoConnect() {
    struct addrinfo hints{}, *res_list = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (::getaddrinfo(m_host.c_str(), std::to_string(m_port).c_str(), &hints, &res_list) != 0 ||
        !res_list) {
        LOG_ERROR(ShadNet, "DNS resolution failed for '{}'", m_host);
        m_state = ShadNetState::FailureResolve;
        return false;
    }

    m_sock = ::socket(res_list->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == SHAD_INVALID_SOCK) {
        ::freeaddrinfo(res_list);
        m_state = ShadNetState::FailureConnect;
        return false;
    }

    if (::connect(m_sock, res_list->ai_addr, static_cast<int>(res_list->ai_addrlen)) < 0) {
        LOG_ERROR(ShadNet, "connect() failed for {}:{}", m_host, m_port);
        ::freeaddrinfo(res_list);
        SHAD_CLOSE(m_sock);
        m_sock = SHAD_INVALID_SOCK;
        m_state = ShadNetState::FailureConnect;
        return false;
    }
    ::freeaddrinfo(res_list);

    // Record local IP (NBO)
    struct sockaddr_in local{};
    socklen_t alen = sizeof(local);
    if (::getsockname(m_sock, reinterpret_cast<struct sockaddr*>(&local), &alen) == 0)
        m_addr_local.store(local.sin_addr.s_addr);

    LOG_INFO(ShadNet, "TCP connected to {}:{}", m_host, m_port);

    // ServerInfo handshake (blocking; socket is still blocking here)
    // The server sends a ServerInfo packet immediately on accept.
    u8 hdr[SHAD_HEADER_SIZE];
    if (!RecvN(hdr, SHAD_HEADER_SIZE)) {
        LOG_ERROR(ShadNet, "Timeout reading ServerInfo header");
        DoDisconnect();
        m_state = ShadNetState::FailureServerInfo;
        return false;
    }

    if (static_cast<PacketType>(hdr[0]) != PacketType::ServerInfo) {
        LOG_ERROR(ShadNet, "Expected ServerInfo, got packet type {:02x}", hdr[0]);
        DoDisconnect();
        m_state = ShadNetState::FailureServerInfo;
        return false;
    }

    const u32 total_sz = GetLE32(hdr + 3);
    const u32 payload_sz = (total_sz > SHAD_HEADER_SIZE) ? total_sz - SHAD_HEADER_SIZE : 0;

    std::vector<u8> si_payload(payload_sz);
    if (payload_sz > 0 && !RecvN(si_payload.data(), payload_sz)) {
        LOG_ERROR(ShadNet, "Timeout reading ServerInfo payload");
        DoDisconnect();
        m_state = ShadNetState::FailureServerInfo;
        return false;
    }

    if (payload_sz >= 4) {
        const u32 server_ver = GetLE32(si_payload.data());
        if (server_ver != SHAD_PROTOCOL_VERSION) {
            LOG_ERROR(ShadNet, "Protocol version mismatch server={} client={}", server_ver,
                      SHAD_PROTOCOL_VERSION);
            DoDisconnect();
            m_state = ShadNetState::FailureServerInfo;
            return false;
        }
    }

    LOG_INFO(ShadNet, "ServerInfo OK (protocol v{})", SHAD_PROTOCOL_VERSION);

    // Socket stays BLOCKING,ReaderThread uses RecvN(), no fcntl needed.
    m_connected = true;
    return true;
}

void ShadNetClient::DoDisconnect() {
    if (m_sock != SHAD_INVALID_SOCK) {
#ifdef _WIN32
        ::shutdown(m_sock, SD_BOTH);
#else
        ::shutdown(m_sock, SHUT_RDWR);
#endif
        SHAD_CLOSE(m_sock);
        m_sock = SHAD_INVALID_SOCK;
    }
}

// RecvN,blocking receive of exactly n bytes
bool ShadNetClient::RecvN(u8* buf, u32 n) {
    u32 received = 0;
    while (received < n) {
        if (m_terminate)
            return false;
        const int r = static_cast<int>(::recv(m_sock, reinterpret_cast<char*>(buf + received),
                                              static_cast<int>(n - received), 0));
        if (r <= 0)
            return false; // disconnect or error
        received += static_cast<u32>(r);
    }
    return true;
}

// SendAll,blocking send of all bytes
bool ShadNetClient::SendAll(const std::vector<u8>& data) {
    std::lock_guard lock(m_mutex_send_direct);
    int sent = 0;
    const int total = static_cast<int>(data.size());
    while (sent < total) {
        const int r = static_cast<int>(
            ::send(m_sock, reinterpret_cast<const char*>(data.data() + sent), total - sent, 0));
        if (r < 0) {
            LOG_ERROR(ShadNet, "send() failed");
            return false;
        }
        sent += r;
    }
    return true;
}

std::vector<u8> ShadNetClient::BuildPacket(CommandType cmd, u64 id,
                                           const std::vector<u8>& payload) const {
    const u32 total = static_cast<u32>(SHAD_HEADER_SIZE + payload.size());
    std::vector<u8> out(SHAD_HEADER_SIZE);
    out[0] = static_cast<u8>(PacketType::Request);
    PutLE16(out, 1, static_cast<u16>(cmd));
    PutLE32(out, 3, total);
    PutLE64(out, 7, id);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

void ShadNetClient::DispatchPacket(PacketType type, u16 cmd_raw, const std::vector<u8>& payload) {
    switch (type) {
    case PacketType::Reply:
        switch (static_cast<CommandType>(cmd_raw)) {
        case CommandType::Login:
            HandleLoginReply(payload);
            break;
        default:
            if (onAsyncReply)
                onAsyncReply(static_cast<CommandType>(cmd_raw), payload);
            else
                LOG_DEBUG(ShadNet, "Unhandled reply cmd={}", cmd_raw);
            break;
        }
        break;

    case PacketType::Notification:
        HandleNotification(cmd_raw, payload);
        break;

    case PacketType::ServerInfo:
        // Subsequent ServerInfo packets (keep-alive / IP updates)
        LOG_DEBUG(ShadNet, "ServerInfo update received");
        break;

    case PacketType::Request:
        LOG_WARNING(ShadNet, "Unexpected Request from server");
        break;
    }
}

void ShadNetClient::HandleLoginReply(const std::vector<u8>& payload) {
    LoginResult res;

    if (payload.empty()) {
        res.error = ErrorType::Malformed;
        LOG_ERROR(ShadNet, "Empty Login reply");
    } else {
        res.error = static_cast<ErrorType>(payload[0]);

        if (res.error == ErrorType::NoError) {
            int pos = 1;

            res.onlineName = ReadStr(payload, pos);
            res.avatarUrl = ReadStr(payload, pos);

            if (pos + 8 <= static_cast<int>(payload.size())) {
                res.userId = GetLE64(payload.data() + pos);
                pos += 8;
            }

            // Friends
            const u32 friend_count = ReadU32LE(payload, pos);
            for (u32 i = 0; i < friend_count && pos < static_cast<int>(payload.size()); ++i) {
                FriendEntry fe;
                fe.npid = ReadStr(payload, pos);
                fe.online = (pos < static_cast<int>(payload.size())) && (payload[pos++] != 0);
                SkipPresence(payload, pos);
                res.friends.push_back(std::move(fe));
            }

            // Friend requests sent
            const u32 sent_count = ReadU32LE(payload, pos);
            for (u32 i = 0; i < sent_count; ++i)
                res.requestsSent.push_back(ReadStr(payload, pos));

            // Friend requests received
            const u32 recv_count = ReadU32LE(payload, pos);
            for (u32 i = 0; i < recv_count; ++i)
                res.requestsReceived.push_back(ReadStr(payload, pos));

            // Blocked list
            const u32 block_count = ReadU32LE(payload, pos);
            for (u32 i = 0; i < block_count; ++i)
                res.blocked.push_back(ReadStr(payload, pos));

            // Store identity
            m_online_name = res.onlineName;
            m_avatar_url = res.avatarUrl;
            m_user_id = res.userId;
            {
                std::lock_guard lock(m_mutex_friends);
                m_friends = res.friends;
            }

            m_authenticated = true;
            m_sem_authenticated.release();

            LOG_INFO(ShadNet,
                     "Logged in npid='{}' onlineName='{}' "
                     "userId={} friends={}",
                     m_npid, m_online_name, m_user_id, m_friends.size());

        } else {
            switch (res.error) {
            case ErrorType::LoginAlreadyLoggedIn:
                m_state = ShadNetState::FailureAlreadyIn;
                break;
            case ErrorType::LoginInvalidUsername:
                m_state = ShadNetState::FailureUsername;
                break;
            case ErrorType::LoginInvalidPassword:
                m_state = ShadNetState::FailurePassword;
                break;
            case ErrorType::LoginInvalidToken:
                m_state = ShadNetState::FailureToken;
                break;
            default:
                m_state = ShadNetState::FailureAuth;
                break;
            }
            LOG_ERROR(ShadNet, "Login rejected error code {}", static_cast<u8>(res.error));
            m_sem_authenticated.release();
            DoDisconnect();
        }
    }

    if (onLoginResult)
        onLoginResult(res);
}

void ShadNetClient::HandleNotification(u16 cmd_raw, const std::vector<u8>& p) {
    int pos = 0;

    switch (static_cast<NotificationType>(cmd_raw)) {

    case NotificationType::FriendQuery: {
        NotifyFriendQuery n;
        n.fromNpid = ReadStr(p, pos);
        LOG_DEBUG(ShadNet, "FriendQuery from '{}'", n.fromNpid);
        if (onFriendQuery)
            onFriendQuery(n);
        break;
    }

    case NotificationType::FriendNew: {
        NotifyFriendNew n;
        n.online = (pos < static_cast<int>(p.size())) && (p[pos++] != 0);
        n.npid = ReadStr(p, pos);
        LOG_DEBUG(ShadNet, "FriendNew '{}' ({})", n.npid, n.online ? "online" : "offline");
        if (onFriendNew)
            onFriendNew(n);
        break;
    }

    case NotificationType::FriendLost: {
        NotifyFriendLost n;
        n.npid = ReadStr(p, pos);
        LOG_DEBUG(ShadNet, "FriendLost '{}'", n.npid);
        if (onFriendLost)
            onFriendLost(n);
        break;
    }

    case NotificationType::FriendStatus: {
        NotifyFriendStatus n;
        n.online = (pos < static_cast<int>(p.size())) && (p[pos++] != 0);
        n.timestamp = ReadU64LE(p, pos);
        n.npid = ReadStr(p, pos);
        LOG_DEBUG(ShadNet, "FriendStatus '{}' is {}", n.npid, n.online ? "online" : "offline");
        if (onFriendStatus)
            onFriendStatus(n);
        break;
    }

    default:
        LOG_DEBUG(ShadNet, "Unknown notification type {}", cmd_raw);
        break;
    }
}

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
