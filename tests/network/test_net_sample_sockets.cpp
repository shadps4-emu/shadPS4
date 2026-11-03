// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "core/libraries/error_codes.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_error.h"

using namespace Libraries::Net;
using namespace std::chrono_literals;

namespace {

// The homebrew's ports.
constexpr u16 TestPort = 8080;
constexpr u16 LogPort = 8181;
constexpr s32 MaxClients = 10;

OrbisNetSockaddr* Sa(OrbisNetSockaddrIn* in) {
    return reinterpret_cast<OrbisNetSockaddr*>(in);
}

void Usleep(u32 micros) {
    std::this_thread::sleep_for(std::chrono::microseconds{micros});
}

std::string Format(const char* fmt, u64 value) {
    char buf[256];
    std::snprintf(buf, sizeof(buf), fmt, static_cast<unsigned>(value));
    return buf;
}

class Logger {
public:
    virtual ~Logger() = default;
    virtual void LogMessage(const char* fmt, u64 value) = 0;

    std::vector<std::string> Sent() {
        std::scoped_lock lock{mutex_};
        return sent_;
    }
    std::vector<std::string> Received() {
        std::scoped_lock lock{mutex_};
        return received_;
    }
    std::vector<std::string> Errors() {
        std::scoped_lock lock{mutex_};
        return errors_;
    }
    /// Waits (up to 10 s) until every sent line has been received.
    bool WaitForDelivery() {
        for (int i = 0; i < 1000; ++i) {
            {
                std::scoped_lock lock{mutex_};
                if (received_.size() >= sent_.size()) {
                    return true;
                }
            }
            Usleep(10000);
        }
        return false;
    }

protected:
    void NoteSent(std::string line) {
        std::scoped_lock lock{mutex_};
        sent_.push_back(std::move(line));
    }
    void NoteReceived(std::string text) {
        std::scoped_lock lock{mutex_};
        received_.push_back(std::move(text));
    }
    void Error(const char* fmt, s32 value) {
        std::scoped_lock lock{mutex_};
        errors_.push_back(Format(fmt, static_cast<u32>(value)));
    }

private:
    std::mutex mutex_;
    std::vector<std::string> sent_;
    std::vector<std::string> received_;
    std::vector<std::string> errors_;
};

/// Blocking stream sockets: a server thread accepting one connection per message.
class BIOStreamLogger final : public Logger {
public:
    BIOStreamLogger() {
        server_ = std::thread([this] { ServerThread(); });
        while (!ready_) {
            Usleep(10000);
        }
    }
    ~BIOStreamLogger() override {
        {
            std::scoped_lock lock{log_mutex_};
            ready_ = false;
        }
        // Frees the server from its blocking accept; not expected to arrive.
        Send("Closing BIOLoggerServer%lx\n", 0);
        server_.join();
    }

    void LogMessage(const char* fmt, u64 value) override {
        std::scoped_lock lock{log_mutex_};
        NoteSent(Format(fmt, value));
        std::thread client([&] { Send(fmt, value); });
        client.join();
    }

private:
    void ServerThread() {
        const s32 stream_sock = sceNetSocket("BIOStreamLoggerServerSocket", ORBIS_NET_AF_INET,
                                             ORBIS_NET_SOCK_STREAM, 0);
        if (stream_sock <= 0) {
            return Error("socket creation failed with 0x%08x", stream_sock);
        }
        s32 opt = 1;
        EXPECT_EQ(sceNetSetsockopt(stream_sock, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEADDR, &opt,
                                   sizeof(opt)),
                  ORBIS_OK);
        EXPECT_EQ(sceNetSetsockopt(stream_sock, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEPORT, &opt,
                                   sizeof(opt)),
                  ORBIS_OK);
        OrbisNetSockaddrIn in_addr{};
        in_addr.sin_family = ORBIS_NET_AF_INET;
        in_addr.sin_addr = ORBIS_NET_INADDR_ANY;
        in_addr.sin_port = sceNetHtons(LogPort);
        s32 result = sceNetBind(stream_sock, Sa(&in_addr), sizeof(in_addr));
        if (result < 0) {
            sceNetSocketClose(stream_sock);
            return Error("bind failed with 0x%08x", result);
        }
        result = sceNetListen(stream_sock, 1);
        if (result < 0) {
            sceNetSocketClose(stream_sock);
            return Error("listen failed with 0x%08x", result);
        }
        ready_ = true;

        while (ready_) {
            u32 addr_len = sizeof(in_addr);
            const s32 log_sock = sceNetAccept(stream_sock, Sa(&in_addr), &addr_len);
            if (!ready_) {
                if (log_sock > 0) {
                    sceNetSocketClose(log_sock);
                }
                break;
            }
            if (log_sock <= 0) {
                Error("Unexpected error 0x%08x while waiting for client", log_sock);
                continue;
            }
            char log_buf[0x1000]{};
            result = sceNetRecv(log_sock, log_buf, sizeof(log_buf), 0);
            if (result <= 0) {
                Error("Unexpected error 0x%08x while reading from socket", result);
            } else {
                NoteReceived(log_buf);
            }
            if (sceNetSocketClose(log_sock) < 0) {
                Error("Failed to close connection, error 0x%08x", result);
            }
        }
        EXPECT_EQ(sceNetSocketClose(stream_sock), ORBIS_OK);
    }

    void Send(const char* fmt, u64 value) {
        const s32 stream_sock = sceNetSocket("BIOStreamLoggerClientSocket", ORBIS_NET_AF_INET,
                                             ORBIS_NET_SOCK_STREAM, 0);
        if (stream_sock <= 0) {
            return Error("Failed to create client socket, error 0x%08x", stream_sock);
        }
        OrbisNetSockaddrIn in_addr{};
        in_addr.sin_family = ORBIS_NET_AF_INET;
        in_addr.sin_port = sceNetHtons(LogPort);
        EXPECT_EQ(sceNetInetPton(ORBIS_NET_AF_INET, "127.0.0.1", &in_addr.sin_addr), 1);
        const s32 result = sceNetConnect(stream_sock, Sa(&in_addr), sizeof(in_addr));
        if (!ready_) {
            sceNetSocketClose(stream_sock);
            return;
        }
        if (result < 0) {
            sceNetSocketClose(stream_sock);
            return Error("Unexpected error while trying to connect to server, error 0x%08x",
                         result);
        }
        const std::string text = Format(fmt, value);
        if (sceNetSend(stream_sock, text.data(), text.size(), 0) <= 0) {
            Error("Unexpected error while sending data to logger server, error 0x%08x",
                  *sceNetErrnoLoc());
        }
        EXPECT_EQ(sceNetSocketClose(stream_sock), ORBIS_OK);
    }

    std::mutex log_mutex_;
    std::atomic<bool> ready_{false};
    std::thread server_;
};

/// Non-blocking stream sockets and epolls; with `async`, up to MaxClients messages in flight.
class NBIOStreamLogger final : public Logger {
public:
    explicit NBIOStreamLogger(bool async) : async_(async) {
        server_ = std::thread([this] { ServerThread(); });
        while (!ready_) {
            Usleep(10000);
        }
    }
    ~NBIOStreamLogger() override {
        std::scoped_lock lock{log_mutex_};
        for (auto& t : clients_) {
            t.join();
        }
        clients_.clear();
        ready_ = false;
        server_.join();
    }

    void LogMessage(const char* fmt, u64 value) override {
        std::scoped_lock lock{log_mutex_};
        NoteSent(Format(fmt, value));
        if (async_ && clients_.size() == static_cast<size_t>(MaxClients)) {
            for (auto& t : clients_) {
                t.join();
            }
            clients_.clear();
        }
        // The thread copies what it needs; nothing here outlives this call.
        std::thread client([this, text = Format(fmt, value)] { Send(text); });
        if (async_) {
            clients_.push_back(std::move(client));
        } else {
            client.join();
        }
    }

private:
    void ServerThread() {
        const s32 stream_sock = sceNetSocket("NBIOStreamLoggerServerSocket", ORBIS_NET_AF_INET,
                                             ORBIS_NET_SOCK_STREAM, 0);
        if (stream_sock <= 0) {
            return Error("socket creation failed with 0x%08x", stream_sock);
        }
        s32 opt = 1;
        EXPECT_EQ(sceNetSetsockopt(stream_sock, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEADDR, &opt,
                                   sizeof(opt)),
                  ORBIS_OK);
        EXPECT_EQ(sceNetSetsockopt(stream_sock, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEPORT, &opt,
                                   sizeof(opt)),
                  ORBIS_OK);
        OrbisNetSockaddrIn in_addr{};
        in_addr.sin_family = ORBIS_NET_AF_INET;
        in_addr.sin_addr = ORBIS_NET_INADDR_ANY;
        in_addr.sin_port = sceNetHtons(LogPort);
        s32 result = sceNetBind(stream_sock, Sa(&in_addr), sizeof(in_addr));
        if (result < 0) {
            sceNetSocketClose(stream_sock);
            return Error("bind failed with 0x%08x", result);
        }
        EXPECT_EQ(sceNetSetsockopt(stream_sock, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO, &opt,
                                   sizeof(opt)),
                  ORBIS_OK);
        result = sceNetListen(stream_sock, MaxClients);
        if (result < 0) {
            sceNetSocketClose(stream_sock);
            return Error("listen failed with 0x%08x", result);
        }
        ready_ = true;

        while (ready_) {
            u32 addr_len = sizeof(in_addr);
            const s32 log_sock = sceNetAccept(stream_sock, Sa(&in_addr), &addr_len);
            if (!ready_) {
                if (log_sock > 0) {
                    sceNetSocketClose(log_sock);
                }
                break;
            }
            if (log_sock == ORBIS_NET_ERROR_EAGAIN) {
                Usleep(10000); // nobody waiting yet
                continue;
            }
            if (log_sock <= 0) {
                Error("Unexpected error 0x%08x while waiting for client", log_sock);
                continue;
            }

            const s32 log_epoll = sceNetEpollCreate("NBIOStreamLoggerServerEpoll", 0);
            if (log_epoll <= 0) {
                Error("Unexpected error 0x%08x while creating epoll", log_epoll);
                sceNetSocketClose(log_sock);
                continue;
            }
            OrbisNetEpollEvent epoll_event{};
            epoll_event.events = ORBIS_NET_EPOLLIN;
            epoll_event.data.fd = log_sock;
            result = sceNetEpollControl(log_epoll, ORBIS_NET_EPOLL_CTL_ADD, log_sock, &epoll_event);
            if (result < 0) {
                Error("Unexpected error 0x%08x while preparing epoll", result);
                sceNetEpollDestroy(log_epoll);
                sceNetSocketClose(log_sock);
                continue;
            }
            OrbisNetEpollEvent epoll_ev_out{};
            result = sceNetEpollWait(log_epoll, &epoll_ev_out, 1, -1);
            if (result <= 0) {
                Error("Unexpected result 0x%08x while waiting for socket", result);
                sceNetEpollDestroy(log_epoll);
                sceNetSocketClose(log_sock);
                continue;
            }
            EXPECT_EQ(epoll_ev_out.data.fd, log_sock);

            std::string text;
            char log_buf[0x1000];
            for (;;) {
                std::memset(log_buf, 0, sizeof(log_buf));
                result = sceNetRecv(log_sock, log_buf, sizeof(log_buf), 0);
                if (result == ORBIS_NET_ERROR_EWOULDBLOCK || result == 0) {
                    break;
                }
                if (result < 0) {
                    Error("Unexpected error 0x%08x while reading from socket", result);
                    break;
                }
                text.append(log_buf, static_cast<size_t>(result));
            }
            if (!text.empty()) {
                NoteReceived(std::move(text));
            }
            if (sceNetEpollDestroy(log_epoll) < 0) {
                Error("Failed to destroy epoll, error 0x%08x", *sceNetErrnoLoc());
            }
            if (sceNetSocketClose(log_sock) < 0) {
                Error("Failed to close connection, error 0x%08x", *sceNetErrnoLoc());
            }
        }
        EXPECT_EQ(sceNetSocketClose(stream_sock), ORBIS_OK);
    }

    void Send(const std::string& text) {
        const s32 stream_sock = sceNetSocket("NBIOStreamLoggerClientSocket", ORBIS_NET_AF_INET,
                                             ORBIS_NET_SOCK_STREAM, 0);
        if (stream_sock <= 0) {
            return Error("Failed to create client socket, error 0x%08x", stream_sock);
        }
        OrbisNetSockaddrIn in_addr{};
        in_addr.sin_family = ORBIS_NET_AF_INET;
        in_addr.sin_port = sceNetHtons(LogPort);
        EXPECT_EQ(sceNetInetPton(ORBIS_NET_AF_INET, "127.0.0.1", &in_addr.sin_addr), 1);
        s32 opt = 1;
        EXPECT_EQ(sceNetSetsockopt(stream_sock, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_NBIO, &opt,
                                   sizeof(opt)),
                  ORBIS_OK);

        // A non-blocking connect: EINPROGRESS, then retried until EISCONN. The homebrew treats
        // anything else as an error (no EALREADY on the PS4 over loopback).
        s32 result = sceNetConnect(stream_sock, Sa(&in_addr), sizeof(in_addr));
        while (result != 0) {
            if (!ready_) {
                sceNetSocketClose(stream_sock);
                return;
            }
            if (result == ORBIS_NET_ERROR_EINPROGRESS) {
                Usleep(100000);
                result = sceNetConnect(stream_sock, Sa(&in_addr), sizeof(in_addr));
                continue;
            }
            if (result == ORBIS_NET_ERROR_EISCONN) {
                break;
            }
            Error("Unexpected error while trying to connect to server, error 0x%08x", result);
            sceNetSocketClose(stream_sock);
            return;
        }

        const s32 log_epoll = sceNetEpollCreate("NBIOStreamLoggerClientEpoll", 0);
        if (log_epoll <= 0) {
            Error("Unexpected error while creating epoll, error 0x%08x", log_epoll);
            sceNetSocketClose(stream_sock);
            return;
        }
        OrbisNetEpollEvent epoll_event{};
        epoll_event.events = ORBIS_NET_EPOLLOUT;
        epoll_event.data.fd = stream_sock;
        result = sceNetEpollControl(log_epoll, ORBIS_NET_EPOLL_CTL_ADD, stream_sock, &epoll_event);
        if (result < 0) {
            Error("Unexpected error while preparing epoll, error 0x%08x", result);
            sceNetEpollDestroy(log_epoll);
            sceNetSocketClose(stream_sock);
            return;
        }
        OrbisNetEpollEvent epoll_ev_out{};
        result = sceNetEpollWait(log_epoll, &epoll_ev_out, 1, -1);
        if (result <= 0) {
            Error("Unexpected result while waiting for socket, 0x%08x", result);
            sceNetEpollDestroy(log_epoll);
            sceNetSocketClose(stream_sock);
            return;
        }
        EXPECT_TRUE(epoll_ev_out.events & ORBIS_NET_EPOLLOUT);

        size_t sent = 0;
        while (sent < text.size()) {
            result = sceNetSend(stream_sock, text.data() + sent, text.size() - sent, 0);
            if (result < 0) {
                Error("Unexpected error while sending data to logger server, error 0x%08x", result);
                break;
            }
            sent += static_cast<size_t>(result);
        }
        EXPECT_EQ(sceNetEpollDestroy(log_epoll), ORBIS_OK);
        EXPECT_EQ(sceNetSocketClose(stream_sock), ORBIS_OK);
    }

    const bool async_;
    std::mutex log_mutex_;
    std::atomic<bool> ready_{false};
    std::thread server_;
    std::vector<std::thread> clients_;
};

std::atomic<bool> g_server_ready{false};
std::atomic<bool> g_cond{false};
Logger* g_active_logger = nullptr;

void LogMessage(const char* msg, u64 value) {
    if (g_active_logger != nullptr) {
        g_active_logger->LogMessage(msg, value);
    }
}

constexpr const char* ServerMessage = "This is a test message coming from the server";
constexpr const char* ClientMessage = "This is a test message coming from the client";
constexpr u32 MessageLength = 0x2d; // both messages, as the PS4 log shows

void ServerThread() {
    const s32 stream_sock =
        sceNetSocket("TestStreamServer", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    LogMessage("Server: socket(AF_INET, SOCK_STREAM, 0) returns 0x%08x\n", stream_sock);
    ASSERT_GT(stream_sock, 0);

    s32 opt = 1;
    s32 result = sceNetSetsockopt(stream_sock, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEADDR, &opt,
                                  sizeof(opt));
    LogMessage("Server: setsockopt(SOL_SOCKET, SO_REUSEADDR) returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);
    result = sceNetSetsockopt(stream_sock, ORBIS_NET_SOL_SOCKET, ORBIS_NET_SO_REUSEPORT, &opt,
                              sizeof(opt));
    LogMessage("Server: setsockopt(SOL_SOCKET, SO_REUSEPORT) returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);

    OrbisNetSockaddrIn in_addr{};
    in_addr.sin_family = ORBIS_NET_AF_INET;
    in_addr.sin_addr = ORBIS_NET_INADDR_ANY;
    in_addr.sin_port = sceNetHtons(TestPort);
    result = sceNetBind(stream_sock, Sa(&in_addr), sizeof(in_addr));
    LogMessage("Server: bind returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);

    result = sceNetListen(stream_sock, 3);
    LogMessage("Server: listen returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);

    g_server_ready = true;

    u32 addr_len = sizeof(in_addr);
    const s32 connected_sock = sceNetAccept(stream_sock, Sa(&in_addr), &addr_len);
    LogMessage("Server: accept returns 0x%08x\n", connected_sock);
    ASSERT_GT(connected_sock, 0);

    result = sceNetSend(connected_sock, ServerMessage, std::strlen(ServerMessage), 0);
    LogMessage("Server: send returns 0x%08x\n", result);
    EXPECT_EQ(static_cast<u32>(result), MessageLength);

    char buffer[1024]{};
    result = sceNetRecv(connected_sock, buffer, sizeof(buffer), 0);
    LogMessage("Server: recv returns 0x%08x\n", result);
    EXPECT_EQ(static_cast<u32>(result), MessageLength);
    EXPECT_STREQ(buffer, ClientMessage);

    const s32 epoll = sceNetEpollCreate("ServerEpoll", 0);
    LogMessage("Server: epoll_create returns 0x%08x\n", epoll);
    EXPECT_GT(epoll, 0);

    OrbisNetEpollEvent epoll_event{};
    epoll_event.events = ORBIS_NET_EPOLLIN;
    epoll_event.data.fd = connected_sock;
    result = sceNetEpollControl(epoll, ORBIS_NET_EPOLL_CTL_ADD, connected_sock, &epoll_event);
    LogMessage("Server: epoll_ctl returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);

    // Nothing sent yet: a zero timeout finds nothing.
    OrbisNetEpollEvent epoll_ev_out{};
    result = sceNetEpollWait(epoll, &epoll_ev_out, 1, 0);
    LogMessage("Server: epoll_wait returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);

    g_cond = true;

    // No timeout: returns once the client's second message is there.
    result = sceNetEpollWait(epoll, &epoll_ev_out, 1, -1);
    LogMessage("Server: epoll_wait returns 0x%08x\n", result);
    EXPECT_EQ(result, 1);
    EXPECT_EQ(epoll_ev_out.data.fd, connected_sock);
    EXPECT_EQ(epoll_ev_out.ident, static_cast<u64>(connected_sock));
    EXPECT_TRUE(epoll_ev_out.events & ORBIS_NET_EPOLLIN);

    std::memset(buffer, 0, sizeof(buffer));
    result = sceNetRecv(connected_sock, buffer, sizeof(buffer), 0);
    LogMessage("Server: recv returns 0x%08x\n", result);
    EXPECT_EQ(static_cast<u32>(result), MessageLength);
    EXPECT_STREQ(buffer, ClientMessage);

    result = sceNetEpollDestroy(epoll);
    LogMessage("Server: epoll_destroy returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);

    result = sceNetSocketClose(connected_sock);
    LogMessage("Server: socket_close returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);
    result = sceNetSocketClose(stream_sock);
    LogMessage("Server: socket_close returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);

    g_server_ready = false;
    g_cond = false;
}

void ClientThread() {
    while (!g_server_ready) {
        Usleep(10000);
    }
    const s32 stream_sock =
        sceNetSocket("TestStreamSocket", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    LogMessage("Client: socket(AF_INET, SOCK_STREAM, 0) returns 0x%08x\n", stream_sock);
    ASSERT_GT(stream_sock, 0);

    OrbisNetSockaddrIn in_addr{};
    in_addr.sin_family = ORBIS_NET_AF_INET;
    in_addr.sin_port = sceNetHtons(TestPort);
    s32 result = sceNetInetPton(ORBIS_NET_AF_INET, "127.0.0.1", &in_addr.sin_addr);
    LogMessage("Client: inet_pton(ORBIS_NET_AF_INET) returns 0x%08x\n", result);
    EXPECT_EQ(result, 1);
    result = sceNetConnect(stream_sock, Sa(&in_addr), sizeof(in_addr));
    LogMessage("Client: connect returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);

    char buffer[1024]{};
    result = sceNetRecv(stream_sock, buffer, sizeof(buffer), 0);
    LogMessage("Client: recv returns 0x%08x\n", result);
    EXPECT_EQ(static_cast<u32>(result), MessageLength);
    EXPECT_STREQ(buffer, ServerMessage);

    result = sceNetSend(stream_sock, ClientMessage, std::strlen(ClientMessage), 0);
    LogMessage("Client: send returns 0x%08x\n", result);
    EXPECT_EQ(static_cast<u32>(result), MessageLength);

    while (!g_cond) {
        Usleep(10000);
    }
    g_cond = false;

    result = sceNetSend(stream_sock, ClientMessage, std::strlen(ClientMessage), 0);
    LogMessage("Client: send returns 0x%08x\n", result);
    EXPECT_EQ(static_cast<u32>(result), MessageLength);

    result = sceNetSocketClose(stream_sock);
    LogMessage("Client: socket_close returns 0x%08x\n", result);
    EXPECT_EQ(result, 0);
}

void RunRound() {
    std::thread server(ServerThread);
    std::thread client(ClientThread);
    server.join();
    client.join();
}

constexpr const char* Ps4Round[] = {
    "Server: socket(AF_INET, SOCK_STREAM, 0) returns <id>\n",
    "Server: setsockopt(SOL_SOCKET, SO_REUSEADDR) returns 0x00000000\n",
    "Server: setsockopt(SOL_SOCKET, SO_REUSEPORT) returns 0x00000000\n",
    "Server: bind returns 0x00000000\n",
    "Server: listen returns 0x00000000\n",
    "Client: socket(AF_INET, SOCK_STREAM, 0) returns <id>\n",
    "Client: inet_pton(ORBIS_NET_AF_INET) returns 0x00000001\n",
    "Client: connect returns 0x00000000\n",
    "Server: accept returns <id>\n",
    "Server: send returns 0x0000002d\n",
    "Client: recv returns 0x0000002d\n",
    "Client: send returns 0x0000002d\n",
    "Server: recv returns 0x0000002d\n",
    "Server: epoll_create returns <id>\n",
    "Server: epoll_ctl returns 0x00000000\n",
    "Server: epoll_wait returns 0x00000000\n",
    "Client: send returns 0x0000002d\n",
    "Client: socket_close returns 0x00000000\n",
    "Server: epoll_wait returns 0x00000001\n",
    "Server: recv returns 0x0000002d\n",
    "Server: epoll_destroy returns 0x00000000\n",
    "Server: socket_close returns 0x00000000\n",
    "Server: socket_close returns 0x00000000\n",
};

std::string MaskId(const std::string& line) {
    static constexpr const char* IdLines[] = {"socket(AF_INET", "accept returns",
                                              "epoll_create returns"};
    const bool is_id = std::any_of(std::begin(IdLines), std::end(IdLines), [&](const char* k) {
        return line.find(k) != std::string::npos;
    });
    const size_t at = line.rfind("0x");
    if (!is_id || at == std::string::npos) {
        return line;
    }
    const auto value = static_cast<s32>(std::stoul(line.substr(at + 2), nullptr, 16));
    EXPECT_GT(value, 0) << line;
    return line.substr(0, at) + "<id>\n";
}

/// The lines of one thread ("Server:" or "Client:"), in order.
std::vector<std::string> Thread(const std::vector<std::string>& lines, const char* prefix) {
    std::vector<std::string> out;
    for (const auto& line : lines) {
        if (line.starts_with(prefix)) {
            out.push_back(line);
        }
    }
    return out;
}

void CheckLogger(Logger& logger, const char* name, bool ordered) {
    SCOPED_TRACE(name);
    ASSERT_TRUE(logger.WaitForDelivery()) << "not every message reached the logger server";
    EXPECT_TRUE(logger.Errors().empty()) << "logger error: " << logger.Errors().front();

    std::vector<std::string> sent, received;
    for (const auto& line : logger.Sent()) {
        sent.push_back(MaskId(line));
    }
    for (const auto& text : logger.Received()) {
        received.push_back(MaskId(text));
    }
    const std::vector<std::string> ps4(std::begin(Ps4Round), std::end(Ps4Round));

    for (const char* prefix : {"Server:", "Client:"}) {
        EXPECT_EQ(Thread(sent, prefix), Thread(ps4, prefix)) << prefix;
        if (ordered) {
            EXPECT_EQ(Thread(received, prefix), Thread(ps4, prefix)) << prefix;
        }
    }
    std::sort(received.begin(), received.end());
    auto expected = ps4;
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(received, expected);
}

class NetSampleSockets : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        ASSERT_EQ(sceNetInit(), ORBIS_OK);
    }
    static void TearDownTestSuite() {
        EXPECT_EQ(sceNetTerm(), ORBIS_OK);
    }
};

} // namespace

TEST_F(NetSampleSockets, IntegrationTestsPr14) {
    // First round: no logging; basic blocking stream socket behaviour on its own.
    RunRound();

    // Second round: a logger built on blocking stream sockets.
    {
        BIOStreamLogger logger;
        g_active_logger = &logger;
        RunRound();
        g_active_logger = nullptr;
        CheckLogger(logger, "BIOStreamLogger", true);
    }

    // Third round: non-blocking stream sockets, one message at a time.
    {
        NBIOStreamLogger logger(false);
        g_active_logger = &logger;
        RunRound();
        g_active_logger = nullptr;
        CheckLogger(logger, "NBIOStreamLogger (sync)", true);
    }

    // Fourth round: non-blocking stream sockets, several messages in flight.
    {
        NBIOStreamLogger logger(true);
        g_active_logger = &logger;
        RunRound();
        g_active_logger = nullptr;
        CheckLogger(logger, "NBIOStreamLogger (async)", false);
    }
}
