// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#include <fmt/format.h>

#include "common/logging/log.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_log.h"

namespace Libraries::Net {

namespace {

struct ErrnoEntry {
    int value;
    const char* name;
};

// ORBIS_NET_ERROR_<name> is 0x80410100 | value.
constexpr ErrnoEntry ErrnoNames[] = {
    {1, "EPERM"},
    {2, "ENOENT"},
    {3, "ESRCH"},
    {4, "EINTR"},
    {5, "EIO"},
    {6, "ENXIO"},
    {7, "E2BIG"},
    {8, "ENOEXEC"},
    {9, "EBADF"},
    {10, "ECHILD"},
    {11, "EDEADLK"},
    {12, "ENOMEM"},
    {13, "EACCES"},
    {14, "EFAULT"},
    {15, "ENOTBLK"},
    {16, "EBUSY"},
    {17, "EEXIST"},
    {18, "EXDEV"},
    {19, "ENODEV"},
    {20, "ENOTDIR"},
    {21, "EISDIR"},
    {22, "EINVAL"},
    {23, "ENFILE"},
    {24, "EMFILE"},
    {25, "ENOTTY"},
    {26, "ETXTBSY"},
    {27, "EFBIG"},
    {28, "ENOSPC"},
    {29, "ESPIPE"},
    {30, "EROFS"},
    {31, "EMLINK"},
    {32, "EPIPE"},
    {33, "EDOM"},
    {34, "ERANGE"},
    {35, "EWOULDBLOCK"},
    {36, "EINPROGRESS"},
    {37, "EALREADY"},
    {38, "ENOTSOCK"},
    {39, "EDESTADDRREQ"},
    {40, "EMSGSIZE"},
    {41, "EPROTOTYPE"},
    {42, "ENOPROTOOPT"},
    {43, "EPROTONOSUPPORT"},
    {44, "ESOCKTNOSUPPORT"},
    {45, "EOPNOTSUPP"},
    {46, "EPFNOSUPPORT"},
    {47, "EAFNOSUPPORT"},
    {48, "EADDRINUSE"},
    {49, "EADDRNOTAVAIL"},
    {50, "ENETDOWN"},
    {51, "ENETUNREACH"},
    {52, "ENETRESET"},
    {53, "ECONNABORTED"},
    {54, "ECONNRESET"},
    {55, "ENOBUFS"},
    {56, "EISCONN"},
    {57, "ENOTCONN"},
    {58, "ESHUTDOWN"},
    {59, "ETOOMANYREFS"},
    {60, "ETIMEDOUT"},
    {61, "ECONNREFUSED"},
    {62, "ELOOP"},
    {63, "ENAMETOOLONG"},
    {64, "EHOSTDOWN"},
    {65, "EHOSTUNREACH"},
    {66, "ENOTEMPTY"},
    {67, "EPROCLIM"},
    {68, "EUSERS"},
    {69, "EDQUOT"},
    {70, "ESTALE"},
    {71, "EREMOTE"},
    {72, "EBADRPC"},
    {73, "ERPCMISMATCH"},
    {74, "EPROGUNAVAIL"},
    {75, "EPROGMISMATCH"},
    {76, "EPROCUNAVAIL"},
    {77, "ENOLCK"},
    {78, "ENOSYS"},
    {79, "EFTYPE"},
    {80, "EAUTH"},
    {81, "ENEEDAUTH"},
    {82, "EIDRM"},
    {83, "ENOMS"},
    {84, "EOVERFLOW"},
    {85, "ECANCELED"},
    {92, "EPROTO"},
    {160, "EADHOC"},
    {161, "ERESERVED161"},
    {162, "ERESERVED162"},
    {163, "EINACTIVEDISABLED"},
    {164, "ENODATA"},
    {165, "EDESC"},
    {166, "EDESCTIMEDOUT"},
    {167, "ENETINTR"},
    {200, "ENOTINIT"},
    {201, "ENOLIBMEM"},
    {202, "ERESERVED202"},
    {203, "ECALLBACK"},
    {204, "EINTERNAL"},
    {205, "ERETURN"},
    {206, "ENOALLOCMEM"},
    {220, "RESOLVER_EINTERNAL"},
    {221, "RESOLVER_EBUSY"},
    {222, "RESOLVER_ENOSPACE"},
    {223, "RESOLVER_EPACKET"},
    {224, "RESOLVER_ERESERVED224"},
    {225, "RESOLVER_ENODNS"},
    {226, "RESOLVER_ETIMEDOUT"},
    {227, "RESOLVER_ENOSUPPORT"},
    {228, "RESOLVER_EFORMAT"},
    {229, "RESOLVER_ESERVERFAILURE"},
    {230, "RESOLVER_ENOHOST"},
    {231, "RESOLVER_ENOTIMPLEMENTED"},
    {232, "RESOLVER_ESERVERREFUSED"},
    {233, "RESOLVER_ENORECORD"},
    {234, "RESOLVER_EALIGNMENT"},
    {235, "RESOLVER_ENOTFOUND"},
    {236, "RESOLVER_ENOTINIT"},
};

enum class Severity { Routine, Network, Suspicious };

Severity SeverityOf(int e) {
    switch (e) {
    case ORBIS_NET_EWOULDBLOCK:
    case ORBIS_NET_EINPROGRESS:
    case ORBIS_NET_EALREADY:
    case ORBIS_NET_EISCONN:
    case ORBIS_NET_EINTR:
    case ORBIS_NET_ENOTBLK:
        return Severity::Routine;
    case ORBIS_NET_ECONNREFUSED:
    case ORBIS_NET_ECONNRESET:
    case ORBIS_NET_ECONNABORTED:
    case ORBIS_NET_ENOTCONN:
    case ORBIS_NET_EPIPE:
    case ORBIS_NET_ETIMEDOUT:
    case ORBIS_NET_EHOSTUNREACH:
    case ORBIS_NET_EHOSTDOWN:
    case ORBIS_NET_ENETUNREACH:
    case ORBIS_NET_ENETDOWN:
    case ORBIS_NET_ENETRESET:
    case ORBIS_NET_EADDRINUSE:
    case ORBIS_NET_EADDRNOTAVAIL:
    case ORBIS_NET_ENOBUFS:
    case ORBIS_NET_EACCES:
    case ORBIS_NET_ESHUTDOWN:
        return Severity::Network;
    default:
        return e >= 0xdc && e <= 0xec ? Severity::Network : Severity::Suspicious;
    }
}

std::string_view ShortName(std::string_view signature) {
    const size_t paren = signature.find('(');
    std::string_view head = signature.substr(0, paren);
    const size_t start = head.find_last_of(": ");
    head = start == std::string_view::npos ? head : head.substr(start + 1);
    // Wrapped exports have their body in <name>Impl.
    if (head.ends_with("Impl")) {
        head.remove_suffix(4);
    }
    return head;
}

u64 CountFailure(std::string_view function, int e) {
    static std::mutex mutex;
    static std::map<std::pair<std::string, int>, u64> counts;
    std::scoped_lock lock{mutex};
    return ++counts[{std::string{function}, e}];
}

} // namespace

std::string ErrnoName(int orbis_errno) {
    const auto* it =
        std::find_if(std::begin(ErrnoNames), std::end(ErrnoNames),
                     [&](const ErrnoEntry& entry) { return entry.value == orbis_errno; });
    return it != std::end(ErrnoNames) ? it->name : fmt::format("E{}", orbis_errno);
}

std::string ErrorCodeName(s32 code) {
    const u32 u = static_cast<u32>(code);
    if ((u & 0xffffff00u) == static_cast<u32>(ORBIS_NET_ERROR_BASE)) {
        return ErrnoName(static_cast<int>(u & 0xff));
    }
    return fmt::format("{:#x}", u);
}

void LogFailure(int orbis_errno, const std::source_location& where) {
    const std::string_view function = ShortName(where.function_name());
    const Severity severity = SeverityOf(orbis_errno);
    if (severity == Severity::Routine) {
        LOG_TRACE(Lib_Net, "{} failed: {}", function, ErrnoName(orbis_errno));
        return;
    }
    const u64 count = CountFailure(function, orbis_errno);
    if (count > 16 && count % 1000 != 0) {
        return;
    }
    const std::string repeat = count > 16 ? fmt::format(", {} times so far", count) : "";
    std::string_view file = where.file_name();
    file.remove_prefix(std::min(file.size(), file.find_last_of("/\\") + 1));
    if (severity == Severity::Network) {
        LOG_DEBUG(Lib_Net, "{} failed: {} ({}:{}{})", function, ErrnoName(orbis_errno), file,
                  where.line(), repeat);
    } else {
        LOG_WARNING(Lib_Net, "{} failed: {} ({}:{}{})", function, ErrnoName(orbis_errno), file,
                    where.line(), repeat);
    }
}

namespace {

struct Traffic {
    u64 sent_bytes = 0;
    u64 sent_calls = 0;
    u64 received_bytes = 0;
    u64 received_calls = 0;
    std::set<std::string> peers;
};

std::mutex g_traffic_mutex;
std::map<OrbisNetId, Traffic> g_traffic; // guarded by g_traffic_mutex

} // namespace

void CountTraffic(OrbisNetId s, bool sent, s64 bytes) {
    std::scoped_lock lock{g_traffic_mutex};
    auto& t = g_traffic[s];
    if (sent) {
        t.sent_bytes += static_cast<u64>(bytes);
        ++t.sent_calls;
    } else {
        t.received_bytes += static_cast<u64>(bytes);
        ++t.received_calls;
    }
}

void NoteP2PPeer(OrbisNetId s, bool sent, const OrbisNetSockaddr* addr, u32 len) {
    if (addr == nullptr || addr->sa_family != ORBIS_NET_AF_INET ||
        len < sizeof(OrbisNetSockaddrIn)) {
        return;
    }
    OrbisNetSockaddrIn in;
    std::memcpy(&in, addr, sizeof(in));
    if (in.sin_vport == 0) {
        return; // not P2P
    }
    constexpr size_t MaxPeers = 16;
    const std::string text = FormatSockaddr(addr, len);
    {
        std::scoped_lock lock{g_traffic_mutex};
        auto& peers = g_traffic[s].peers;
        if (peers.size() >= MaxPeers || !peers.insert((sent ? "> " : "< ") + text).second) {
            return;
        }
    }
    LOG_INFO(Lib_Net, "socket {}: first P2P packet {} {}", s, sent ? "to" : "from", text);
}

void LogSocketClosed(OrbisNetId s) {
    Traffic t;
    {
        std::scoped_lock lock{g_traffic_mutex};
        const auto it = g_traffic.find(s);
        if (it != g_traffic.end()) {
            t = it->second;
            g_traffic.erase(it);
        }
    }
    LOG_DEBUG(Lib_Net, "socket {} closed: sent {} bytes in {} calls, received {} bytes in {} calls",
              s, t.sent_bytes, t.sent_calls, t.received_bytes, t.received_calls);
}

std::string FormatSockaddr(const OrbisNetSockaddr* addr, u32 len) {
    if (addr == nullptr) {
        return "null";
    }
    if (len < 2) {
        return fmt::format("(length {})", len);
    }
    if (addr->sa_family == ORBIS_NET_AF_INET && len >= sizeof(OrbisNetSockaddrIn)) {
        OrbisNetSockaddrIn in;
        std::memcpy(&in, addr, sizeof(in));
        const u32 ip = sceNetNtohl(in.sin_addr);
        std::string text = fmt::format("{}.{}.{}.{}:{}", ip >> 24, (ip >> 16) & 0xff,
                                       (ip >> 8) & 0xff, ip & 0xff, sceNetNtohs(in.sin_port));
        if (in.sin_vport != 0) {
            text += fmt::format(" vport {}", sceNetNtohs(in.sin_vport));
        }
        return text;
    }
    if (addr->sa_family == ORBIS_NET_AF_UNIX) {
        const char* path = reinterpret_cast<const char*>(addr) + 2;
        return fmt::format("unix:{}", std::string_view{path, strnlen(path, len - 2)});
    }
    return fmt::format("family {} (length {})", addr->sa_family, len);
}

std::string_view SocketTypeName(s32 type) {
    switch (type) {
    case ORBIS_NET_SOCK_STREAM:
        return "STREAM";
    case ORBIS_NET_SOCK_DGRAM:
        return "DGRAM";
    case ORBIS_NET_SOCK_RAW:
        return "RAW";
    case ORBIS_NET_SOCK_DGRAM_P2P:
        return "DGRAM_P2P";
    case ORBIS_NET_SOCK_STREAM_P2P:
        return "STREAM_P2P";
    default:
        return "unknown";
    }
}

std::string OptionName(s32 level, s32 name) {
    const char* option = nullptr;
    const char* level_name = nullptr;
    switch (level) {
    case ORBIS_NET_SOL_SOCKET:
        level_name = "SOL_SOCKET";
        switch (name) {
        case ORBIS_NET_SO_REUSEADDR:
            option = "SO_REUSEADDR";
            break;
        case ORBIS_NET_SO_KEEPALIVE:
            option = "SO_KEEPALIVE";
            break;
        case ORBIS_NET_SO_BROADCAST:
            option = "SO_BROADCAST";
            break;
        case ORBIS_NET_SO_LINGER:
            option = "SO_LINGER";
            break;
        case ORBIS_NET_SO_REUSEPORT:
            option = "SO_REUSEPORT";
            break;
        case ORBIS_NET_SO_ONESBCAST:
            option = "SO_ONESBCAST";
            break;
        case ORBIS_NET_SO_USECRYPTO:
            option = "SO_USECRYPTO";
            break;
        case ORBIS_NET_SO_USESIGNATURE:
            option = "SO_USESIGNATURE";
            break;
        case ORBIS_NET_SO_SNDBUF:
            option = "SO_SNDBUF";
            break;
        case ORBIS_NET_SO_RCVBUF:
            option = "SO_RCVBUF";
            break;
        case ORBIS_NET_SO_ERROR:
            option = "SO_ERROR";
            break;
        case ORBIS_NET_SO_TYPE:
            option = "SO_TYPE";
            break;
        case ORBIS_NET_SO_SNDTIMEO:
            option = "SO_SNDTIMEO";
            break;
        case ORBIS_NET_SO_RCVTIMEO:
            option = "SO_RCVTIMEO";
            break;
        case ORBIS_NET_SO_ERROR_EX:
            option = "SO_ERROR_EX";
            break;
        case ORBIS_NET_SO_ACCEPTTIMEO:
            option = "SO_ACCEPTTIMEO";
            break;
        case ORBIS_NET_SO_CONNECTTIMEO:
            option = "SO_CONNECTTIMEO";
            break;
        case ORBIS_NET_SO_NBIO:
            option = "SO_NBIO";
            break;
        case ORBIS_NET_SO_POLICY:
            option = "SO_POLICY";
            break;
        case ORBIS_NET_SO_NAME:
            option = "SO_NAME";
            break;
        case ORBIS_NET_SO_PRIORITY:
            option = "SO_PRIORITY";
            break;
        case 0x1005:
            option = "SO_SNDTIMEO (BSD)";
            break;
        case 0x1006:
            option = "SO_RCVTIMEO (BSD)";
            break;
        default:
            break;
        }
        break;
    case ORBIS_NET_IPPROTO_IP:
        level_name = "IPPROTO_IP";
        switch (name) {
        case ORBIS_NET_IP_HDRINCL:
            option = "IP_HDRINCL";
            break;
        case ORBIS_NET_IP_TOS:
            option = "IP_TOS";
            break;
        case ORBIS_NET_IP_TTL:
            option = "IP_TTL";
            break;
        case ORBIS_NET_IP_MULTICAST_IF:
            option = "IP_MULTICAST_IF";
            break;
        case ORBIS_NET_IP_MULTICAST_TTL:
            option = "IP_MULTICAST_TTL";
            break;
        case ORBIS_NET_IP_MULTICAST_LOOP:
            option = "IP_MULTICAST_LOOP";
            break;
        case ORBIS_NET_IP_ADD_MEMBERSHIP:
            option = "IP_ADD_MEMBERSHIP";
            break;
        case ORBIS_NET_IP_DROP_MEMBERSHIP:
            option = "IP_DROP_MEMBERSHIP";
            break;
        case ORBIS_NET_IP_TTLCHK:
            option = "IP_TTLCHK";
            break;
        case ORBIS_NET_IP_MAXTTL:
            option = "IP_MAXTTL";
            break;
        case ORBIS_NET_IP_DONTFRAG:
            option = "IP_DONTFRAG";
            break;
        default:
            break;
        }
        break;
    case ORBIS_NET_IPPROTO_TCP:
        level_name = "IPPROTO_TCP";
        switch (name) {
        case ORBIS_NET_TCP_NODELAY:
            option = "TCP_NODELAY";
            break;
        case ORBIS_NET_TCP_MAXSEG:
            option = "TCP_MAXSEG";
            break;
        case ORBIS_NET_TCP_MSS_TO_ADVERTISE:
            option = "TCP_MSS_TO_ADVERTISE";
            break;
        case ORBIS_NET_TCP_KEEPLISTEN:
            option = "TCP_KEEPLISTEN";
            break;
        default:
            break;
        }
        break;
    case ORBIS_NET_IPPROTO_UDP:
        level_name = "IPPROTO_UDP";
        if (name == ORBIS_NET_UDP_SND_ON_SUSPEND) {
            option = "UDP_SND_ON_SUSPEND";
        }
        break;
    default:
        break;
    }
    if (level_name == nullptr) {
        return fmt::format("level {:#x}/option {:#x}", level, name);
    }
    if (option == nullptr) {
        return fmt::format("{}/option {:#x}", level_name, name);
    }
    return fmt::format("{}/{}", level_name, option);
}

std::string EpollEventsName(u32 events) {
    static constexpr std::pair<u32, const char*> Bits[] = {
        {ORBIS_NET_EPOLLIN, "IN"},   {ORBIS_NET_EPOLLOUT, "OUT"},       {ORBIS_NET_EPOLLERR, "ERR"},
        {ORBIS_NET_EPOLLHUP, "HUP"}, {ORBIS_NET_EPOLLDESCID, "DESCID"},
    };
    std::string out;
    u32 rest = events;
    for (const auto& [bit, name] : Bits) {
        if (events & bit) {
            out += out.empty() ? "" : "|";
            out += name;
            rest &= ~bit;
        }
    }
    if (rest != 0) {
        out += fmt::format("{}{:#x}", out.empty() ? "" : "|", rest);
    }
    return out.empty() ? "0" : out;
}

} // namespace Libraries::Net
