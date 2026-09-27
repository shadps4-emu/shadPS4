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
/// std::nullopt means "no deadline".
using Deadline = std::optional<Clock::time_point>;

/// BSD socket-timeout semantics (SO_RCVTIMEO/SO_SNDTIMEO): 0 means "wait forever".
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
    NotBlk = 15, // abort with nothing waiting
    Busy = 16,
    Exist = 17,
    NoDev = 19,
    Inval = 22,
    NFile = 23,
    MFile = 24,
    NoSpc = 28,
    Pipe = 32,
    WouldBlock = 35, // EAGAIN == EWOULDBLOCK on FreeBSD
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
    Internal = 204, // ORBIS_NET_EINTERNAL: every host error without a FreeBSD equivalent
};

Error TranslateNative(int native_code);
/// errno or WSAGetLastError() of the calling thread, translated.
Error LastError();

struct IoResult {
    s64 value;
    Error error;
};

bool Initialize();
void Shutdown();

// Sockets. All wrappers are non-blocking and return WouldBlock instead of waiting.

// Non-blocking, close-on-exec, on Windows also SIO_UDP_CONNRESET off for UDP.
NativeSocket CreateSocket(int family, int type, int protocol, Error* error);
// Applies the same configuration to sockets that come from accept().
bool ConfigureSocket(NativeSocket s, int type);
void CloseSocket(NativeSocket s);
// A connected pair, configured like CreateSocket. POSIX socketpair(). Windows has none, so
// there a loopback TCP connection stands in (stream sockets only).
Error CreateSocketPair(int family, int type, int protocol, NativeSocket out[2]);

// Call right before bind. Maps SO_REUSEADDR semantics onto the host.
Error PrepareBind(NativeSocket s, bool reuse_addr);
Error Bind(NativeSocket s, const sockaddr* addr, socklen_t len);
Error Listen(NativeSocket s, int backlog);
// how: 0 = receive, 1 = send, 2 = both (same values as SHUT_* and SD_*).
Error ShutdownSocket(NativeSocket s, int how);
// Returns InProgress (never WouldBlock) for a started non-blocking connect.
Error Connect(NativeSocket s, const sockaddr* addr, socklen_t len);
// Reads and clears SO_ERROR
Error PendingSocketError(NativeSocket s);
NativeSocket Accept(NativeSocket s, sockaddr* addr, socklen_t* len, Error* error);
// addr == nullptr means send()/recv() on a connected socket.
IoResult SendTo(NativeSocket s, const void* buf, size_t len, int flags, const sockaddr* addr,
                socklen_t addr_len);
IoResult RecvFrom(NativeSocket s, void* buf, size_t len, int flags, sockaddr* addr,
                  socklen_t* addr_len);

// Wake handle: a pollable object another thread can signal to interrupt a wait.
class WakeHandle {
public:
    WakeHandle();
    ~WakeHandle();
    WakeHandle(const WakeHandle&) = delete;
    WakeHandle& operator=(const WakeHandle&) = delete;

    bool Valid() const {
        return read_ != InvalidSocket;
    }
    /// Thread-safe. Level-triggered: stays signalled until Drain().
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

// Primitive 1: wait for one socket (used by blocking guest calls).
enum Interest : u32 {
    Readable = 1u << 0,
    Writable = 1u << 1,
};

enum class WaitResult {
    Ready,    // socket has an event (including error/hangup): retry the operation
    TimedOut, // deadline passed
    Woken,    // wake handle signalled (abort/close)
    Failed,   // poll itself failed, see LastError()
};

WaitResult WaitOne(NativeSocket s, u32 interest, const WakeHandle& wake, Deadline deadline);

// Primitive 2: a real host epoll/kqueue instance backing one guest sceNetEpoll.
enum EventBits : u32 {
    EvIn = 1u << 0,
    EvOut = 1u << 1,
    EvErr = 1u << 2,
    EvHup = 1u << 3,
    /// Guest level only, never passed to the host: a non-socket net object (a resolver) has
    /// finished (ORBIS_NET_EPOLLDESCID).
    EvDescId = 1u << 4,
};

/// Registrations are always level-triggered: the PS4's epoll has no edge-triggered mode, and wepoll
/// would ignore EPOLLET anyway.
enum EpollFlags : u32 {
    OneShot = 1u << 0, // disabled after one report until Modify() re-arms it
};

/// Reserved internally for the wake handle; callers must never use it as a tag.
inline constexpr u64 ReservedTag = ~0ull;

struct EpollEvent {
    u64 tag;
    u32 events; // EventBits
};

struct EpollWaitResult {
    s32 count;  // events written to out
    bool woken; // Wake() was called (the wake is level-triggered and not drained here)
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

    /// Returns at most min(out.size(), 64) events per call.
    EpollWaitResult Wait(std::span<EpollEvent> out, Deadline deadline);
    /// Thread-safe. Level-triggered: stays signalled until DrainWake().
    void Wake();
    void DrainWake();

private:
#ifdef _WIN32
    void* handle_ = nullptr; // wepoll HANDLE
#else
    int handle_ = -1; // epoll or kqueue fd
#endif
    WakeHandle wake_;
};

} // namespace Core::Net::Host
