// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "client.h"
#include "common/logging/log.h"
#include "shadnet.pb.h"

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
static void PlatformInit() {
    static bool done = false;
    if (!done) {
        WSADATA wd;
        WSAStartup(MAKEWORD(2, 2), &wd);
        done = true;
    }
}
#else
static void PlatformInit() {}
#endif

namespace ShadNet {

// Build a u32-LE-prefixed proto blob payload for a request packet.
template <typename T>
static std::vector<u8> MakeProtoPayload(const T& msg) {
    const std::string serialised = msg.SerializeAsString();
    const u32 len = static_cast<u32>(serialised.size());
    std::vector<u8> out(4);
    out[0] = static_cast<u8>(len);
    out[1] = static_cast<u8>(len >> 8);
    out[2] = static_cast<u8>(len >> 16);
    out[3] = static_cast<u8>(len >> 24);
    out.insert(out.end(), serialised.begin(), serialised.end());
    return out;
}

// Read a u32-LE-prefixed proto blob starting at pos in p.
// Returns the raw bytes ready for ParseFromString.
std::string ShadNetClient::ExtractBlob(const std::vector<u8>& p, int pos) {
    if (pos + 4 > static_cast<int>(p.size()))
        return {};
    const u32 len = GetLE32(p.data() + pos);
    pos += 4;
    if (pos + static_cast<int>(len) > static_cast<int>(p.size()))
        return {};
    return std::string(reinterpret_cast<const char*>(p.data() + pos), len);
}

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
    try {
        m_sem_connected.release();
    } catch (...) {
    }
    try {
        m_sem_authenticated.release();
    } catch (...) {
    }
    {
        std::lock_guard lock(m_mutex_send_queue);
        m_cv_send_queue.notify_all();
    }
    DoDisconnect();
    if (m_thread_connect.joinable())
        m_thread_connect.join();
    if (m_thread_reader.joinable())
        m_thread_reader.join();
    if (m_thread_writer.joinable())
        m_thread_writer.join();
}

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

bool ShadNetClient::IsConnected() const {
    return m_connected.load();
}
bool ShadNetClient::IsAuthenticated() const {
    return m_authenticated.load();
}
ShadNetState ShadNetClient::GetState() const {
    return m_state.load();
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

// Threading

void ShadNetClient::ConnectThread() {
    if (!DoConnect()) {
        m_sem_connected.release();
        m_sem_authenticated.release();
        return;
    }

    m_thread_reader = std::thread(&ShadNetClient::ReaderThread, this);
    m_thread_writer = std::thread(&ShadNetClient::WriterThread, this);
    m_sem_connected.release();

    // Build Login request as protobuf
    shadnet::LoginRequest req;
    req.set_npid(m_npid);
    req.set_password(m_password);
    if (!m_token.empty())
        req.set_token(m_token);

    const u64 id = m_pkt_counter.fetch_add(1);
    if (!SendAll(BuildPacket(CommandType::Login, id, MakeProtoPayload(req)))) {
        LOG_ERROR(ShadNet, "ShadNet: Failed to send Login packet");
        m_state = ShadNetState::FailureOther;
        return;
    }
    LOG_INFO(ShadNet, "Login packet sent for '{}'", m_npid);
}

void ShadNetClient::ReaderThread() {
    while (!m_terminate) {
        u8 hdr[SHAD_HEADER_SIZE];
        if (!RecvN(hdr, SHAD_HEADER_SIZE)) {
            if (!m_terminate)
                LOG_WARNING(ShadNet, "Reader header recv failed, disconnecting");
            break;
        }
        const auto ptype = static_cast<PacketType>(hdr[0]);
        const u16 cmd_raw = GetLE16(hdr + 1);
        const u32 total_sz = GetLE32(hdr + 3);

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

    if (!m_authenticated) {
        if (m_state == ShadNetState::Ok)
            m_state = ShadNetState::FailureOther;
        m_sem_authenticated.release();
    }
    m_connected = false;
    m_authenticated = false;
    LOG_INFO(ShadNet, "ReaderThread exiting");
}

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

// Connect / Disconnect

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

    struct sockaddr_in local{};
    socklen_t alen = sizeof(local);
    if (::getsockname(m_sock, reinterpret_cast<struct sockaddr*>(&local), &alen) == 0)
        m_addr_local.store(local.sin_addr.s_addr);

    LOG_INFO(ShadNet, "TCP connected to {}:{}", m_host, m_port);

    // ServerInfo handshake
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

bool ShadNetClient::RecvN(u8* buf, u32 n) {
    u32 received = 0;
    while (received < n) {
        if (m_terminate)
            return false;
        const int r = static_cast<int>(::recv(m_sock, reinterpret_cast<char*>(buf + received),
                                              static_cast<int>(n - received), 0));
        if (r <= 0)
            return false;
        received += static_cast<u32>(r);
    }
    return true;
}

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

// Packet dispatch

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
        LOG_DEBUG(ShadNet, "ServerInfo update received");
        break;
    case PacketType::Request:
        LOG_WARNING(ShadNet, "Unexpected Request from server");
        break;
    }
}

// Login reply

void ShadNetClient::HandleLoginReply(const std::vector<u8>& payload) {
    LoginResult res;

    if (payload.empty()) {
        res.error = ErrorType::Malformed;
        LOG_ERROR(ShadNet, "Empty Login reply");
    } else {
        res.error = static_cast<ErrorType>(payload[0]);

        if (res.error == ErrorType::NoError) {
            // payload[0]   = ErrorType byte
            // payload[1..] = u32 LE blob size + LoginReply proto bytes
            shadnet::LoginReply pb;
            const std::string blob = ExtractBlob(payload, 1);
            if (!blob.empty() && pb.ParseFromString(blob)) {
                res.avatarUrl = pb.avatar_url();
                res.userId = pb.user_id();
                for (const auto& f : pb.friends()) {
                    FriendEntry fe;
                    fe.npid = f.npid();
                    fe.online = f.online();
                    res.friends.push_back(std::move(fe));
                }
                for (const auto& n : pb.friend_requests_sent())
                    res.requestsSent.push_back(n);
                for (const auto& n : pb.friend_requests_received())
                    res.requestsReceived.push_back(n);
                for (const auto& n : pb.blocked())
                    res.blocked.push_back(n);

                m_avatar_url = res.avatarUrl;
                m_user_id = res.userId;
                {
                    std::lock_guard lock(m_mutex_friends);
                    m_friends = res.friends;
                }
                m_authenticated = true;
                m_sem_authenticated.release();
                LOG_INFO(ShadNet, "Logged in npid='{}' userId={} friends={}", m_npid, m_user_id,
                         m_friends.size());
            } else {
                res.error = ErrorType::Malformed;
                LOG_ERROR(ShadNet, "Failed to parse LoginReply proto");
                m_state = ShadNetState::FailureProtocol;
                m_sem_authenticated.release();
                DoDisconnect();
            }
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

// Notifications

void ShadNetClient::HandleNotification(u16 cmd_raw, const std::vector<u8>& payload) {
    // Notification payload = u32 LE blob size + proto bytes
    const std::string blob = ExtractBlob(payload, 0);
    if (blob.empty()) {
        LOG_WARNING(ShadNet, "Empty notification payload type={}", cmd_raw);
        return;
    }

    switch (static_cast<NotificationType>(cmd_raw)) {
    case NotificationType::FriendQuery: {
        shadnet::NotifyFriendQuery pb;
        if (!pb.ParseFromString(blob)) {
            LOG_WARNING(ShadNet, "FriendQuery parse error");
            break;
        }
        NotifyFriendQuery n;
        n.fromNpid = pb.from_npid();
        LOG_DEBUG(ShadNet, "FriendQuery from '{}'", n.fromNpid);
        if (onFriendQuery)
            onFriendQuery(n);
        break;
    }
    case NotificationType::FriendNew: {
        shadnet::NotifyFriendNew pb;
        if (!pb.ParseFromString(blob)) {
            LOG_WARNING(ShadNet, "FriendNew parse error");
            break;
        }
        NotifyFriendNew n;
        n.npid = pb.npid();
        n.online = pb.online();
        LOG_DEBUG(ShadNet, "FriendNew '{}' ({})", n.npid, n.online ? "online" : "offline");
        if (onFriendNew)
            onFriendNew(n);
        break;
    }
    case NotificationType::FriendLost: {
        shadnet::NotifyFriendLost pb;
        if (!pb.ParseFromString(blob)) {
            LOG_WARNING(ShadNet, "FriendLost parse error");
            break;
        }
        NotifyFriendLost n;
        n.npid = pb.npid();
        LOG_DEBUG(ShadNet, "FriendLost '{}'", n.npid);
        if (onFriendLost)
            onFriendLost(n);
        break;
    }
    case NotificationType::FriendStatus: {
        shadnet::NotifyFriendStatus pb;
        if (!pb.ParseFromString(blob)) {
            LOG_WARNING(ShadNet, "FriendStatus parse error");
            break;
        }
        NotifyFriendStatus n;
        n.npid = pb.npid();
        n.online = pb.online();
        n.timestamp = pb.timestamp();
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

// encode / decode

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

} // namespace ShadNet
