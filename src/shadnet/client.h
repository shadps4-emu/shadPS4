// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <semaphore>
#include <string>
#include <thread>
#include <vector>
#include "common/types.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using ShadSocketHandle = SOCKET;
static constexpr ShadSocketHandle SHAD_INVALID_SOCK = INVALID_SOCKET;
#define SHAD_CLOSE(s) ::closesocket(s)
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using ShadSocketHandle = int;
static constexpr ShadSocketHandle SHAD_INVALID_SOCK = -1;
#define SHAD_CLOSE(s) ::close(s)
#endif

namespace ShadNet {

// Protocol constants
static constexpr u32 SHAD_HEADER_SIZE = 15;
static constexpr u32 SHAD_CONNECT_TIMEOUT_MS = 10000; // 10 second connect/handshake timeout
static constexpr u32 SHAD_PROTOCOL_VERSION = 1;
static constexpr u32 SHAD_MAX_PACKET_SIZE = 0x800000; // 8 MiB

// Protocol enumerations (must match shadnet server protocol.h)
enum class PacketType : u8 {
    Request = 0,
    Reply = 1,
    Notification = 2,
    ServerInfo = 3,
};

enum class CommandType : u16 {
    Login = 0,
    Terminate = 1,
    Create = 2,
    Delete = 3,
    SendToken = 4,
    SendResetToken = 5,
    ResetPassword = 6,
    ResetState = 7,
    AddFriend = 8,
    RemoveFriend = 9,
    AddBlock = 10,
    RemoveBlock = 11,
    // 12–29: room/lobby/ticket NOT implemented in shadNet
    GetBoardInfos = 30,
    RecordScore = 31,
    RecordScoreData = 32,
    GetScoreData = 33,
    GetScoreRange = 34,
    GetScoreFriends = 35,
    GetScoreNpid = 36,
};

enum class NotificationType : u16 {
    FriendQuery = 5,
    FriendNew = 6,
    FriendLost = 7,
    FriendStatus = 8,
};

enum class ErrorType : uint8_t {
    NoError = 0,
    Malformed = 1,
    Invalid = 2,
    InvalidInput = 3,
    TooSoon = 4,
    LoginError = 5,
    LoginAlreadyLoggedIn = 6,
    LoginInvalidUsername = 7,
    LoginInvalidPassword = 8,
    LoginInvalidToken = 9,
    CreationError = 10,
    CreationExistingUsername = 11,
    CreationBannedEmailProvider = 12,
    CreationExistingEmail = 13,
    RoomMissing = 14,
    RoomAlreadyJoined = 15,
    RoomFull = 16,
    RoomPasswordMismatch = 17,
    RoomPasswordMissing = 18,
    RoomGroupNoJoinLabel = 19,
    RoomGroupFull = 20,
    RoomGroupJoinLabelNotFound = 21,
    RoomGroupMaxSlotMismatch = 22,
    Unauthorized = 23,
    DbFail = 24,
    EmailFail = 25,
    NotFound = 26,
    Blocked = 27,
    AlreadyFriend = 28,
    ScoreNotBest = 29,
    ScoreInvalid = 30,
    ScoreHasData = 31,
    CondFail = 32,
    Unsupported = 33,
};

enum class ShadNetState {
    Ok,
    FailureInput,
    FailureResolve,
    FailureConnect,
    FailureServerInfo,
    FailureAuth,
    FailureAlreadyIn,
    FailureUsername,
    FailurePassword,
    FailureToken,
    FailureProtocol,
    FailureOther,
};

// Callback data structures

struct FriendEntry {
    std::string npid;
    bool online = false;
};

struct LoginResult {
    ErrorType error = ErrorType::Malformed;
    std::string avatarUrl;
    u64 userId = 0;
    std::vector<FriendEntry> friends;
    std::vector<std::string> requestsSent;
    std::vector<std::string> requestsReceived;
    std::vector<std::string> blocked;
};

struct NotifyFriendQuery {
    std::string fromNpid;
};
struct NotifyFriendNew {
    std::string npid;
    bool online = false;
};
struct NotifyFriendLost {
    std::string npid;
};
struct NotifyFriendStatus {
    std::string npid;
    bool online = false;
    u64 timestamp = 0;
};

// ShadNetClient

class ShadNetClient {
public:
    ShadNetClient();
    ~ShadNetClient();
    ShadNetClient(const ShadNetClient&) = delete;
    ShadNetClient& operator=(const ShadNetClient&) = delete;

    void Start(const std::string& host, u16 port, const std::string& npid,
               const std::string& password, const std::string& token = {});
    void Stop();

    ShadNetState WaitForConnection();
    ShadNetState WaitForAuthenticated();

    bool IsConnected() const;
    bool IsAuthenticated() const;
    ShadNetState GetState() const;

    const std::string& GetAvatarUrl() const;
    u64 GetUserId() const;
    u32 GetAddrLocal() const;
    u32 GetNumFriends() const;
    std::optional<std::string> GetFriendNpid(u32 index) const;

    // Callbacks
    std::function<void(const LoginResult&)> onLoginResult;
    std::function<void(const NotifyFriendQuery&)> onFriendQuery;
    std::function<void(const NotifyFriendNew&)> onFriendNew;
    std::function<void(const NotifyFriendLost&)> onFriendLost;
    std::function<void(const NotifyFriendStatus&)> onFriendStatus;
    std::function<void(CommandType, const std::vector<u8>&)> onAsyncReply;

private:
    void ConnectThread();
    void ReaderThread();
    void WriterThread();
    bool DoConnect();
    void DoDisconnect();
    bool RecvN(u8* buf, u32 n);
    bool SendAll(const std::vector<u8>& data);
    std::vector<u8> BuildPacket(CommandType cmd, u64 id, const std::vector<u8>& payload) const;
    void DispatchPacket(PacketType type, u16 cmd_raw, const std::vector<u8>& payload);
    void HandleLoginReply(const std::vector<u8>& payload);
    void HandleNotification(u16 cmd_raw, const std::vector<u8>& payload);

    // Helper: read a u32-LE-prefixed proto blob from a byte vector at pos.
    static std::string ExtractBlob(const std::vector<u8>& p, int pos);

    static void PutLE16(std::vector<u8>& b, size_t off, u16 v);
    static void PutLE32(std::vector<u8>& b, size_t off, u32 v);
    static void PutLE64(std::vector<u8>& b, size_t off, u64 v);
    static u16 GetLE16(const u8* p);
    static u32 GetLE32(const u8* p);
    static u64 GetLE64(const u8* p);

    ShadSocketHandle m_sock = SHAD_INVALID_SOCK;
    std::string m_host;
    u16 m_port = 31313;
    std::string m_npid;
    std::string m_password;
    std::string m_token;

    std::atomic<bool> m_terminate{false};
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_authenticated{false};
    std::atomic<ShadNetState> m_state{ShadNetState::Ok};

    std::binary_semaphore m_sem_connected{0};
    std::binary_semaphore m_sem_authenticated{0};
    std::mutex m_mutex_connected;
    std::mutex m_mutex_authenticated;

    std::thread m_thread_connect;
    std::thread m_thread_reader;
    std::thread m_thread_writer;

    std::mutex m_mutex_send_direct;
    std::mutex m_mutex_send_queue;
    std::condition_variable m_cv_send_queue;
    std::vector<std::vector<u8>> m_send_queue;

    std::string m_avatar_url;
    u64 m_user_id = 0;
    std::atomic<u32> m_addr_local{0};

    mutable std::mutex m_mutex_friends;
    std::vector<FriendEntry> m_friends;

    std::atomic<u64> m_pkt_counter{1};
};

} // namespace ShadNet
