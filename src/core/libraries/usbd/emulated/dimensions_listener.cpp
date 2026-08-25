//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "dimensions_listener.h"

#include <array>
#include <chrono>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/libraries/usbd/emulated/dimensions.h"

namespace Libraries::Usbd {

namespace {

#ifdef _WIN32
using socket_t = SOCKET;
constexpr socket_t invalid_socket = INVALID_SOCKET;
void CloseSocket(socket_t s) {
    ::closesocket(s);
}
#else
using socket_t = int;
constexpr socket_t invalid_socket = -1;
void CloseSocket(socket_t s) {
    ::close(s);
}
#endif

constexpr u8 kLoadCommand = 0x01;
constexpr u8 kRemoveCommand = 0x02;
constexpr u8 kMoveCommand = 0x03;
constexpr u8 kGetLedCommand = 0x04;

constexpr size_t kHeaderSize = 5;
constexpr size_t kFigureDataSize = DIMENSIONS_FIGURE_SIZE;
// { 'L', serial, 3, then 3 regions x 9 bytes }.
constexpr size_t kLedResponseSize = 3 + 3 * 9;

// A figure landing on a slot that still holds one has to be announced as two
// separate events, and the game needs to see the removal before the arrival.
// Issuing both back to back queues two 0x56 responses for the same slot in one
// go, which the game reads as a single change and ignores - the swap silently
// does nothing. Letting the removal be picked up first keeps them distinct.
constexpr auto kRemoveThenLoadDelay = std::chrono::milliseconds(100);

bool RecvAll(socket_t s, u8* data, size_t len) {
    while (len != 0) {
        const auto got = ::recv(s, reinterpret_cast<char*>(data), static_cast<int>(len), 0);
        if (got <= 0) {
            return false;
        }
        data += got;
        len -= static_cast<size_t>(got);
    }
    return true;
}

bool SendAll(socket_t s, const u8* data, size_t len) {
    while (len != 0) {
        const auto sent = ::send(s, reinterpret_cast<const char*>(data), static_cast<int>(len), 0);
        if (sent <= 0) {
            return false;
        }
        data += sent;
        len -= static_cast<size_t>(sent);
    }
    return true;
}

} // namespace

DimensionsListener::DimensionsListener(std::shared_ptr<DimensionsToypad> toypad)
    : m_toypad(std::move(toypad)) {}

DimensionsListener::~DimensionsListener() {
    Stop();
}

void DimensionsListener::Start(u16 port) {
    if (port == 0 || m_running.exchange(true)) {
        return;
    }
    m_thread = std::thread([this, port] { Run(port); });
}

void DimensionsListener::Stop() {
    if (!m_running.exchange(false)) {
        return;
    }
    // Closing the listening socket is what unblocks accept(); there is no
    // portable way to interrupt it otherwise.
    const auto listen_socket = m_listen_socket.exchange(~0ULL);
    if (listen_socket != ~0ULL) {
        CloseSocket(static_cast<socket_t>(listen_socket));
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void DimensionsListener::Run(u16 port) {
    Common::SetCurrentThreadName("shadPS4:DimensionsListener");

#ifdef _WIN32
    WSADATA wsa_data;
    // Reference counted, so this cooperates with any other winsock user in the
    // process rather than fighting over initialisation.
    if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        LOG_ERROR(Lib_Usbd, "Dimensions listener: WSAStartup failed");
        m_running = false;
        return;
    }
#endif

    const socket_t server = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == invalid_socket) {
        LOG_ERROR(Lib_Usbd, "Dimensions listener: could not create socket");
#ifdef _WIN32
        ::WSACleanup();
#endif
        m_running = false;
        return;
    }

    int reuse = 1;
    ::setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse),
                 sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = ::htons(port);
    address.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);

    if (::bind(server, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0 ||
        ::listen(server, 1) != 0) {
        LOG_ERROR(Lib_Usbd, "Dimensions listener: could not bind 127.0.0.1:{}", port);
        CloseSocket(server);
#ifdef _WIN32
        ::WSACleanup();
#endif
        m_running = false;
        return;
    }

    m_listen_socket = static_cast<u64>(server);
    LOG_INFO(Lib_Usbd, "Dimensions listener active on 127.0.0.1:{}", port);

    while (m_running) {
        const socket_t client = ::accept(server, nullptr, nullptr);
        if (client == invalid_socket) {
            break; // Stop() closed the listening socket, or the socket died.
        }
        HandleClient(static_cast<u64>(client));
        CloseSocket(client);
    }

    // Stop() may already have closed and cleared this; only close if it did not.
    const auto remaining = m_listen_socket.exchange(~0ULL);
    if (remaining != ~0ULL) {
        CloseSocket(static_cast<socket_t>(remaining));
    }
#ifdef _WIN32
    ::WSACleanup();
#endif
    LOG_INFO(Lib_Usbd, "Dimensions listener stopped");
}

void DimensionsListener::HandleClient(u64 client_handle) {
    const auto client = static_cast<socket_t>(client_handle);

    // A connection may carry several messages back to back; each command reads
    // exactly the bytes it needs, so the next message boundary stays known.
    while (m_running) {
        std::array<u8, kHeaderSize> header{};
        if (!RecvAll(client, header.data(), header.size())) {
            return;
        }

        const u8 command = header[0];
        const u8 pad = header[1];
        const u8 index = header[2];

        // GET_LED addresses no slot, so its zeroed pad/index must not be
        // validated as one.
        if (command != kGetLedCommand && (pad < 1 || pad > 3 || index >= MAX_DIMENSIONS_FIGURES)) {
            LOG_ERROR(Lib_Usbd, "Dimensions listener: rejected cmd 0x{:02x} pad {} index {}",
                      command, pad, index);
            return;
        }

        switch (command) {
        case kLoadCommand: {
            std::array<u8, kFigureDataSize> tag{};
            if (!RecvAll(client, tag.data(), tag.size())) {
                return;
            }

            std::array<u8, 2> length_bytes{};
            if (!RecvAll(client, length_bytes.data(), length_bytes.size())) {
                return;
            }
            const u16 path_length = static_cast<u16>(length_bytes[0] | (length_bytes[1] << 8));

            std::string path;
            if (path_length != 0) {
                std::vector<u8> path_bytes(path_length);
                if (!RecvAll(client, path_bytes.data(), path_bytes.size())) {
                    return;
                }
                path.assign(path_bytes.begin(), path_bytes.end());
            }

            // Loading onto an occupied slot replaces what is there, matching the
            // contract the Cemu and RPCS3 listeners already established.
            m_toypad->RemoveFigure(pad, index, true);
            std::this_thread::sleep_for(kRemoveThenLoadDelay);

            if (path.empty()) {
                // No file behind this figure: the tag lives in memory for the
                // session. An unopened IOFile makes DimensionsFigure::Save() a
                // no-op, which is exactly the behaviour that wants.
                m_toypad->LoadDimensionsFigure(tag, Common::FS::IOFile{}, pad, index);
            } else {
                m_toypad->LoadFigure(path, pad, index);
            }
            LOG_INFO(Lib_Usbd, "Dimensions listener: LOAD pad {} index {} ({})", pad, index,
                     path.empty() ? "in-memory tag" : path);
            break;
        }
        case kRemoveCommand: {
            m_toypad->RemoveFigure(pad, index, true);
            LOG_INFO(Lib_Usbd, "Dimensions listener: REMOVE pad {} index {}", pad, index);
            break;
        }
        case kMoveCommand: {
            const u8 source_pad = header[3];
            const u8 source_index = header[4];
            if (source_pad < 1 || source_pad > 3 || source_index >= MAX_DIMENSIONS_FIGURES) {
                LOG_ERROR(Lib_Usbd, "Dimensions listener: rejected MOVE source pad {} index {}",
                          source_pad, source_index);
                return;
            }
            m_toypad->MoveFigure(pad, index, source_pad, source_index);
            LOG_INFO(Lib_Usbd, "Dimensions listener: MOVE {}/{} -> {}/{}", source_pad, source_index,
                     pad, index);
            break;
        }
        case kGetLedCommand: {
            const auto states = m_toypad->GetLedStates();
            std::array<u8, kLedResponseSize> response{};
            response[0] = 0x4C; // 'L' magic
            response[1] = m_toypad->GetLedSerial();
            response[2] = 0x03; // region count
            for (size_t i = 0; i < states.size(); ++i) {
                const size_t offset = 3 + i * 9;
                response[offset + 0] = states[i].pad;
                response[offset + 1] = states[i].mode;
                response[offset + 2] = states[i].r;
                response[offset + 3] = states[i].g;
                response[offset + 4] = states[i].b;
                response[offset + 5] = states[i].on_ms;
                response[offset + 6] = states[i].off_ms;
                response[offset + 7] = states[i].count;
                response[offset + 8] = states[i].speed_ms;
            }
            if (!SendAll(client, response.data(), response.size())) {
                return;
            }
            break;
        }
        default:
            // The next message boundary is no longer knowable, so the connection
            // cannot be reused after an unknown command.
            LOG_ERROR(Lib_Usbd, "Dimensions listener: unknown command 0x{:02x}", command);
            return;
        }
    }
}

} // namespace Libraries::Usbd
