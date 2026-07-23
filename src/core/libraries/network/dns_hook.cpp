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
    // Not a literal - try to resolve the name (e.g. "localhost").
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
            LOG_INFO(Lib_Net, "DNS swap: no dns_swap.json found.Created default at {}",
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

        const std::string pattern = HostPart(it.key());
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

static bool IsNonPublicLiteral(const std::string& host) {
    in_addr conv{};
    if (inet_pton(AF_INET, host.c_str(), &conv) != 1) {
        return false; // not a literal - it's a name, let the rule apply
    }
    const u32 a = ntohl(conv.s_addr);
    const u8 b1 = static_cast<u8>(a >> 24);
    const u8 b2 = static_cast<u8>((a >> 16) & 0xFF);
    if (b1 == 0 || b1 == 127) {
        return true; // 0.0.0.0/8, loopback
    }
    if (b1 == 10) {
        return true; // 10.0.0.0/8
    }
    if (b1 == 172 && b2 >= 16 && b2 <= 31) {
        return true; // 172.16.0.0/12
    }
    if (b1 == 192 && b2 == 168) {
        return true; // 192.168.0.0/16
    }
    if (b1 == 169 && b2 == 254) {
        return true; // 169.254.0.0/16 link-local
    }
    if (b1 >= 224) {
        return true; // multicast + reserved + 255.255.255.255
    }
    return false;
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
            if (!std::regex_match(hostname, re)) {
                continue;
            }
            // A wildcard rule must not hijack local/LAN literals
            if (pattern.find('*') != std::string::npos && IsNonPublicLiteral(hostname)) {
                LOG_DEBUG(Lib_Net, "DNS swap: '{}' is local/LAN, wildcard rule skipped", hostname);
                continue;
            }
            return ip;
        } catch (const std::exception&) {
            // Bad pattern - skip.
        }
    }
    return std::nullopt;
}

std::optional<u32> DnsHook::Lookup(const std::string& hostname) {
    std::lock_guard lock(mutex);
    return GetRedir(hostname);
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

u32 DnsHook::ConsumeQueued(u64 sock, u8* dst, u32 len, bool is_stream) {
    std::lock_guard lock(mutex);
    const auto it = spylist.find(sock);
    if (it == spylist.end() || it->second.empty()) {
        return 0;
    }
    auto& front = it->second.front();
    const u32 n = std::min<u32>(len, static_cast<u32>(front.size()));
    std::memcpy(dst, front.data(), n);
    if (is_stream && n < front.size()) {
        front.erase(front.begin(), front.begin() + n);
    } else {
        it->second.pop();
    }
    return n;
}

s32 DnsHook::AnalyzeQuery(u64 sock, const u8* buf, u32 len, bool is_stream) {
    std::lock_guard lock(mutex);

    const u8* dns = buf;
    u32 dns_len = len;
    if (is_stream) {
        if (len < 2) {
            return -1;
        }
        const u16 declared = static_cast<u16>((buf[0] << 8) | buf[1]);
        dns = buf + 2;
        dns_len = len - 2;
        if (declared != dns_len) {
            return -1;
        }
    }

    // Minimum DNS header is 12 bytes.
    if (dns_len < 12) {
        return -1;
    }

    // Header fields are big-endian.
    const u16 flags = static_cast<u16>((dns[2] << 8) | dns[3]);
    const u16 qdcount = static_cast<u16>((dns[4] << 8) | dns[5]);
    const u16 ancount = static_cast<u16>((dns[6] << 8) | dns[7]);
    const u16 nscount = static_cast<u16>((dns[8] << 8) | dns[9]);
    const u16 arcount = static_cast<u16>((dns[10] << 8) | dns[11]);

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
    for (; i < dns_len && dns[i] != 0; ++i) {
        if (label_len == 0) {
            label_len = dns[i];
            if (i != 12) {
                host += '.';
            }
        } else {
            host += static_cast<char>(dns[i]);
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
    LOG_INFO(Lib_Net, "DNS swap: '{}' -> {}.{}.{}.{} ({})", host, ip_octets[0], ip_octets[1],
             ip_octets[2], ip_octets[3], is_stream ? "tcp" : "udp");

    // Build the DNS response message: copy the query, flip it to an answer,
    // append one A record.
    std::vector<u8> msg(dns, dns + dns_len);
    msg[2] |= 0x80; // QR = response
    msg[3] |= 0x80; // RA = recursion available
    msg[6] = 0x00;  // ancount = 1
    msg[7] = 0x01;

    static const u8 answer_head[] = {
        0xC0, 0x0C,             // name: pointer to the question
        0x00, 0x01,             // type A
        0x00, 0x01,             // class IN
        0x00, 0x00, 0x00, 0x3B, // TTL
        0x00, 0x04,             // rdlength = 4
    };
    msg.insert(msg.end(), std::begin(answer_head), std::end(answer_head));
    const u8* ip_bytes = reinterpret_cast<const u8*>(&*ip);
    msg.insert(msg.end(), ip_bytes, ip_bytes + 4);

    // Frame the response: TCP gets a 2-byte big-endian length prefix, UDP doesn't.
    std::vector<u8> fake;
    if (is_stream) {
        const u16 msg_len = static_cast<u16>(msg.size());
        fake.push_back(static_cast<u8>((msg_len >> 8) & 0xFF));
        fake.push_back(static_cast<u8>(msg_len & 0xFF));
    }
    fake.insert(fake.end(), msg.begin(), msg.end());

    spylist[sock].push(std::move(fake));
    return static_cast<s32>(len);
}

} // namespace Libraries::Net
