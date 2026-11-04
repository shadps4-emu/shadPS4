// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Backends:
//  * Linux:          poll + eventfd, epoll
//  * macOS/FreeBSD:  poll + pipe, kqueue
//  * Windows:        WSAPoll + loopback UDP pair, wepoll

#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <span>

#include "common/types.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

namespace Core::Net::Host {

#ifdef _WIN32
using NativeSocket = SOCKET;
inline constexpr NativeSocket InvalidSocket = INVALID_SOCKET;
#else
using NativeSocket = int;
inline constexpr NativeSocket InvalidSocket = -1;
#endif

using Clock = std::chrono::steady_clock;
// nullopt = no deadline
using Deadline = std::optional<Clock::time_point>;

// BSD SO_RCVTIMEO semantics, 0 = wait forever.
Deadline DeadlineFromSocketTimeout(std::chrono::microseconds timeout);

enum class Error : s32 {
    Ok = 0,
    Perm = 1,
    NoEnt = 2,
    Intr = 4,
    Io = 5,
    BadF = 9,
    NoMem = 12,
    Acces = 13,
    Fault = 14,
    NotBlk = 15, // abort with nothing blocked
    Busy = 16,
    Exist = 17,
    NoDev = 19,
    Inval = 22,
    NFile = 23,
    MFile = 24,
    NoSpc = 28,
    Pipe = 32,
    WouldBlock = 35, // also EAGAIN
    InProgress = 36,
    Already = 37,
    NotSock = 38,
    DestAddrReq = 39,
    MsgSize = 40,
    ProtoType = 41,
    NoProtoOpt = 42,
    ProtoNoSupport = 43,
    SocktNoSupport = 44,
    OpNotSupp = 45,
    PfNoSupport = 46,
    AfNoSupport = 47,
    AddrInUse = 48,
    AddrNotAvail = 49,
    NetDown = 50,
    NetUnreach = 51,
    NetReset = 52,
    ConnAborted = 53,
    ConnReset = 54,
    NoBufs = 55,
    IsConn = 56,
    NotConn = 57,
    TooManyRefs = 59,
    TimedOut = 60,
    ConnRefused = 61,
    Loop = 62,
    NameTooLong = 63,
    HostDown = 64,
    HostUnreach = 65,
    NotEmpty = 66,
    Canceled = 85,
    Internal = 204, // host errors with no FreeBSD equivalent
};

Error TranslateNative(int native_code);
Error LastError();

struct IoResult {
    s64 value;
    Error error;
};

bool Initialize();
void Shutdown();
NativeSocket CreateSocket(int family, int type, int protocol, Error* error);
bool ConfigureSocket(NativeSocket s, int type);
void CloseSocket(NativeSocket s);
// Windows has no socketpair(), so it uses a loopback TCP connection (stream only).
Error CreateSocketPair(int family, int type, int protocol, NativeSocket out[2]);

Error PrepareBind(NativeSocket s, bool reuse_addr, bool reuse_port);
// SO_LINGER in seconds. Darwin's SO_LINGER counts clock ticks, SO_LINGER_SEC is the BSD one.
#ifdef __APPLE__
inline constexpr int LingerOption = SO_LINGER_SEC;
#else
inline constexpr int LingerOption = SO_LINGER;
#endif

// Call before closing a stream socket. Sorts out SO_LINGER for the guest's blocking mode.
void PrepareClose(NativeSocket s, bool guest_blocking);
Error Bind(NativeSocket s, const sockaddr* addr, socklen_t len);
Error Listen(NativeSocket s, int backlog);
Error ShutdownSocket(NativeSocket s, int how);
// Returns InProgress, never WouldBlock, while connecting.
Error Connect(NativeSocket s, const sockaddr* addr, socklen_t len);
// Reads and clears SO_ERROR.
Error PendingSocketError(NativeSocket s);
// Bytes ready to read (FIONREAD).
IoResult BytesReadable(NativeSocket s);
NativeSocket Accept(NativeSocket s, sockaddr* addr, socklen_t* len, Error* error);
// addr == nullptr uses send()/recv().
IoResult SendTo(NativeSocket s, const void* buf, size_t len, int flags, const sockaddr* addr,
                socklen_t addr_len);
IoResult RecvFrom(NativeSocket s, void* buf, size_t len, int flags, sockaddr* addr,
                  socklen_t* addr_len);

// Pollable object another thread can signal to interrupt a wait.
class WakeHandle {
public:
    WakeHandle();
    ~WakeHandle();
    WakeHandle(const WakeHandle&) = delete;
    WakeHandle& operator=(const WakeHandle&) = delete;

    bool Valid() const {
        return read_ != InvalidSocket;
    }
    // Thread-safe. Stays signalled until Drain().
    void Signal() const;
    void Drain() const;
    NativeSocket PollHandle() const {
        return read_;
    }

private:
    void Reset();

    NativeSocket read_ = InvalidSocket;
    NativeSocket write_ = InvalidSocket; // same as read_ for eventfd
};

// Wait on a single socket, for blocking guest calls.
enum Interest : u32 {
    Readable = 1u << 0,
    Writable = 1u << 1,
};

enum class WaitResult {
    Ready, // includes error/hangup, retry the operation
    TimedOut,
    Woken,  // abort or close
    Failed, // see LastError()
};

WaitResult WaitOne(NativeSocket s, u32 interest, const WakeHandle& wake, Deadline deadline);

// Host epoll/kqueue backing one guest epoll.
enum EventBits : u32 {
    EvIn = 1u << 0,
    EvOut = 1u << 1,
    EvErr = 1u << 2,
    EvHup = 1u << 3,
    // Guest only. A non-socket object such as a resolver has finished.
    EvDescId = 1u << 4,
};

// Always level-triggered. The PS4 has no edge-triggered mode and wepoll ignores EPOLLET.
enum EpollFlags : u32 {
    OneShot = 1u << 0, // re-arm with Modify()
};

// Used for the wake handle, not a valid caller tag.
inline constexpr u64 ReservedTag = ~0ull;

struct EpollEvent {
    u64 tag;
    u32 events; // EventBits
};

struct EpollWaitResult {
    s32 count;
    bool woken; // not drained, call DrainWake()
    Error error;
};

class HostEpoll {
public:
    HostEpoll();
    ~HostEpoll();
    HostEpoll(const HostEpoll&) = delete;
    HostEpoll& operator=(const HostEpoll&) = delete;

    bool Valid() const;

    Error Add(NativeSocket s, u64 tag, u32 events, u32 flags);
    Error Modify(NativeSocket s, u64 tag, u32 events, u32 flags);
    Error Remove(NativeSocket s);

    // At most 64 events per call.
    EpollWaitResult Wait(std::span<EpollEvent> out, Deadline deadline);
    // Thread-safe. Stays signalled until DrainWake().
    void Wake();
    void DrainWake();

private:
#ifdef _WIN32
    void* handle_ = nullptr; // wepoll HANDLE
#else
    int handle_ = -1;
#endif
    WakeHandle wake_;
};

} // namespace Core::Net::Host
