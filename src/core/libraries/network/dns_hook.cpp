// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/libraries/network/dns_hook.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace Libraries::Net {

// Strip an optional "scheme://" prefix and ":port" suffix, returning just the host.
static std::string HostPart(std::string_view value) {
    if (const auto scheme_end = value.find("://"); scheme_end != std::string_view::npos) {
        value = value.substr(scheme_end + 3);
    }
    if (const auto colon = value.find(':'); colon != std::string_view::npos) {
        value = value.substr(0, colon);
    }
    return std::string(value);
}

// Resolve a host (IP literal or name) to a network-order IPv4 address.
static std::optional<u32> HostToIpv4(const std::string& host) {
    in_addr conv{};
    if (inet_pton(AF_INET, host.c_str(), &conv) == 1) {
        return conv.s_addr;
    }
    // Not a literal,try to resolve the name (e.g. "localhost").
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* info = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &info) == 0 && info && info->ai_addr) {
        const u32 addr = reinterpret_cast<sockaddr_in*>(info->ai_addr)->sin_addr.s_addr;
        freeaddrinfo(info);
        return addr;
    }
    if (info) {
        freeaddrinfo(info);
    }
    return std::nullopt;
}

DnsHook& DnsHook::Instance() {
    static DnsHook instance;
    return instance;
}

DnsHook::DnsHook() {
    in_addr loopback{};
    inet_pton(AF_INET, "127.0.0.1", &loopback);
    dns_server_addr = loopback.s_addr;
    LoadSwapList();
}

void DnsHook::LoadSwapList() {
    loaded = true;

    std::filesystem::path path =
        Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "dns_swap.json";

    std::string contents;
    std::ifstream f(path);
    if (!f.is_open()) {
        static constexpr std::string_view default_contents = "{\n"
                                                             "    \"*\": \"127.0.0.1\"\n"
                                                             "}\n";
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream out(path);
        if (out.is_open()) {
            out << default_contents;
            out.close();
            LOG_INFO(Lib_Net, "DNS swap: no dns_swap.json found; created default at {}",
                     path.string());
        } else {
            LOG_ERROR(Lib_Net, "DNS swap: no dns_swap.json and failed to create default at {}",
                      path.string());
        }
        contents = std::string(default_contents);
    } else {
        std::stringstream buf;
        buf << f.rdbuf();
        contents = buf.str();
    }

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(contents);
    } catch (const std::exception& e) {
        LOG_ERROR(Lib_Net, "DNS swap: JSON parse failed: {}", e.what());
        return;
    }
    if (!root.is_object()) {
        LOG_ERROR(Lib_Net, "DNS swap: JSON root must be an object");
        return;
    }

    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.key().empty() && it.key().front() == '_') {
            continue; // comment/meta key
        }
        if (!it.value().is_string()) {
            continue;
        }
        const std::string value = it.value().get<std::string>();
        if (value.empty()) {
            continue;
        }

        // Key to hostname pattern (strip any scheme/port a user might have added).
        const std::string pattern = HostPart(it.key());
        // Value to IPv4 for the A-record answer (IP literal or a resolvable name).
        const auto ip = HostToIpv4(HostPart(value));
        if (!ip) {
            LOG_ERROR(Lib_Net, "DNS swap: could not resolve target '{}' for key '{}'", value,
                      it.key());
            continue;
        }
        redirs.emplace_back(pattern, *ip);
    }

    LOG_INFO(Lib_Net, "DNS swap: loaded {} entries from {}", redirs.size(), path.string());
}

std::optional<u32> DnsHook::GetRedir(const std::string& hostname) {
    for (const auto& [pattern, ip] : redirs) {
        // Escape dots, turn '*' into '.*', match case-insensitively.
        std::string rx;
        rx.reserve(pattern.size() * 2);
        for (const char c : pattern) {
            if (c == '.') {
                rx += "\\.";
            } else if (c == '*') {
                rx += ".*";
            } else {
                rx += c;
            }
        }
        try {
            const std::regex re(rx, std::regex_constants::icase);
            if (std::regex_match(hostname, re)) {
                return ip;
            }
        } catch (const std::exception&) {
            // Bad pattern - skip.
        }
    }
    return std::nullopt;
}

void DnsHook::AddSpy(u64 sock) {
    std::lock_guard lock(mutex);
    spylist.try_emplace(sock);
}

void DnsHook::RemoveSpy(u64 sock) {
    std::lock_guard lock(mutex);
    spylist.erase(sock);
}

bool DnsHook::IsSpy(u64 sock) {
    std::lock_guard lock(mutex);
    return spylist.contains(sock);
}

bool DnsHook::HasQueued(u64 sock) {
    std::lock_guard lock(mutex);
    const auto it = spylist.find(sock);
    return it != spylist.end() && !it->second.empty();
}

std::vector<u8> DnsHook::PopPacket(u64 sock) {
    std::lock_guard lock(mutex);
    const auto it = spylist.find(sock);
    if (it == spylist.end() || it->second.empty()) {
        return {};
    }
    std::vector<u8> pkt = std::move(it->second.front());
    it->second.pop();
    return pkt;
}

s32 DnsHook::AnalyzeQuery(u64 sock, const u8* buf, u32 len) {
    std::lock_guard lock(mutex);

    // Minimum DNS header is 12 bytes.
    if (len < 12) {
        return -1;
    }

    // Header fields are big-endian.
    const u16 flags = static_cast<u16>((buf[2] << 8) | buf[3]);
    const u16 qdcount = static_cast<u16>((buf[4] << 8) | buf[5]);
    const u16 ancount = static_cast<u16>((buf[6] << 8) | buf[7]);
    const u16 nscount = static_cast<u16>((buf[8] << 8) | buf[9]);
    const u16 arcount = static_cast<u16>((buf[10] << 8) | buf[11]);

    const bool is_query = (flags & 0x8000) == 0;  // QR bit clear
    const bool truncated = (flags & 0x0200) != 0; // TC bit set
    // Only handle a single, untruncated question with no other records.
    if (!is_query || truncated || qdcount != 1 || ancount != 0 || nscount != 0 || arcount != 0) {
        return -1;
    }

    // Reconstruct the hostname from the question's labels.
    std::string host;
    u32 i = 12;
    u8 label_len = 0;
    for (; i < len && buf[i] != 0; ++i) {
        if (label_len == 0) {
            label_len = buf[i];
            if (i != 12) {
                host += '.';
            }
        } else {
            host += static_cast<char>(buf[i]);
            --label_len;
        }
    }

    if (host.empty()) {
        return -1;
    }

    const auto ip = GetRedir(host);
    if (!ip) {
        LOG_DEBUG(Lib_Net, "DNS swap: '{}' no match, passing through", host);
        return -1;
    }

    const u8* ip_octets = reinterpret_cast<const u8*>(&*ip);
    LOG_INFO(Lib_Net, "DNS swap: '{}' -> {}.{}.{}.{}", host, ip_octets[0], ip_octets[1],
             ip_octets[2], ip_octets[3]);

    // Build a fake response: copy the query, flip it to an answer, append one A record.
    std::vector<u8> fake(buf, buf + len);
    fake[2] |= 0x80; // QR = response
    fake[3] |= 0x80; // RA = recursion available
    fake[6] = 0x00;  // ancount = 1
    fake[7] = 0x01;

    static const u8 answer_head[] = {
        0xC0, 0x0C,             // name: pointer to the question
        0x00, 0x01,             // type A
        0x00, 0x01,             // class IN
        0x00, 0x00, 0x00, 0x3B, // TTL
        0x00, 0x04,             // rdlength = 4
    };
    fake.insert(fake.end(), std::begin(answer_head), std::end(answer_head));
    const u8* ip_bytes = reinterpret_cast<const u8*>(&*ip);
    fake.insert(fake.end(), ip_bytes, ip_bytes + 4);

    spylist[sock].push(std::move(fake));
    return static_cast<s32>(len);
}

} // namespace Libraries::Net
