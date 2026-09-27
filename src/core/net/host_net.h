// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
    WouldBlock = 35,
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
    Internal = 204, // PS4 ORBIS_NET_EINTERNAL
};

bool Initialize();
void Shutdown();

/// Non-blocking, close-on-exec, on Windows also SIO_UDP_CONNRESET off for UDP.
NativeSocket CreateSocket(int family, int type, int protocol, Error* error);
/// Applies the same configuration to sockets that come from accept().
bool ConfigureSocket(NativeSocket s, int type);
void CloseSocket(NativeSocket s);

/// Returns InProgress (never WouldBlock) for a started non-blocking connect.
Error Connect(NativeSocket s, const sockaddr* addr, socklen_t len);

} // namespace Core::Net::Host
