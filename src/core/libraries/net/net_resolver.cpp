// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "common/logging/log.h"
#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_log.h"
#include "core/libraries/net/net_resolver.h"
#include "core/net/guest_net.h"
#include "core/net/host_net.h"

#ifndef _WIN32
#include <netdb.h>
#endif

namespace Libraries::Net {

namespace {

using Core::Net::Host::Error;

// Each kind has its own preserved abort.
enum class LookupKind { Ntoa = 0, Aton = 1 };

struct Resolver {
    std::string name;

    std::mutex mutex; // guards everything below
    bool running = false;
    bool destroyed = false;
    u64 lookup = 0; // bumped to orphan a running async lookup
    s32 status = 0;
    std::array<bool, 2> pending_abort{}; // indexed by LookupKind
};

std::mutex g_mutex;
std::unordered_map<s32, std::shared_ptr<Resolver>> g_resolvers;
std::function<bool()> g_is_online;

std::shared_ptr<Resolver> Find(s32 id) {
    std::scoped_lock lock{g_mutex};
    const auto it = g_resolvers.find(id);
    return it != g_resolvers.end() ? it->second : nullptr;
}

bool IsOnline() {
    std::function<bool()> check;
    {
        std::scoped_lock lock{g_mutex};
        check = g_is_online;
    }
    return !check || check();
}

// IPv4 only, at most 10 records.
s32 Lookup(std::string hostname, bool allow_literal, std::vector<u32>* addrs) {
    if (!hostname.empty() && hostname.back() == '.') {
        hostname.pop_back();
    }
    if (hostname.compare(0, 9, "localhost") == 0) {
        addrs->push_back(htonl(INADDR_LOOPBACK));
        return 0;
    }
    if (allow_literal) {
        in_addr literal{};
        if (inet_pton(AF_INET, hostname.c_str(), &literal) == 1) {
            addrs->push_back(literal.s_addr);
            return 0;
        }
    }
    if (!IsOnline()) {
        return ORBIS_NET_ERROR_RESOLVER_ENODNS;
    }
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // one entry per address
    addrinfo* result = nullptr;
    const int gai = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (gai != 0) {
        LOG_WARNING(Lib_Net, "lookup of {} failed: {}", hostname, gai);
        switch (gai) {
        case EAI_NONAME:
#if defined(EAI_NODATA) && EAI_NODATA != EAI_NONAME
        case EAI_NODATA:
#endif
            return ORBIS_NET_ERROR_RESOLVER_ENOHOST;
        case EAI_AGAIN:
            return ORBIS_NET_ERROR_RESOLVER_ETIMEDOUT;
        case EAI_FAIL:
            return ORBIS_NET_ERROR_RESOLVER_ESERVERFAILURE;
        default:
            return ORBIS_NET_ERROR_RESOLVER_EINTERNAL;
        }
    }
    for (const addrinfo* ai = result; ai != nullptr && addrs->size() < 10; ai = ai->ai_next) {
        if (ai->ai_family != AF_INET || ai->ai_addr == nullptr) {
            continue;
        }
        const u32 addr = reinterpret_cast<const sockaddr_in*>(ai->ai_addr)->sin_addr.s_addr;
        if (std::find(addrs->begin(), addrs->end(), addr) == addrs->end()) {
            addrs->push_back(addr);
        }
    }
    freeaddrinfo(result);
    return addrs->empty() ? ORBIS_NET_ERROR_RESOLVER_ENORECORD : 0;
}

void Deliver(const ResolverOutput& output, const std::vector<u32>& addrs) {
    if (output.addr != nullptr) {
        output.addr->inaddr_addr = addrs.front();
    }
    if (output.info != nullptr) {
        std::memset(output.info, 0, sizeof(*output.info));
        for (size_t i = 0; i < addrs.size(); ++i) {
            output.info->addrs[i].u.addr.inaddr_addr = addrs[i];
            output.info->addrs[i].af = ORBIS_NET_AF_INET;
        }
        output.info->records = static_cast<u32>(addrs.size());
        output.info->recordsv4 = static_cast<u32>(addrs.size());
    }
}

} // namespace

s32 CreateResolver(std::string name) {
    const auto r = Core::Net::ReserveExternalId();
    if (r.error != Error::Ok) {
        return -static_cast<s32>(r.error);
    }
    const auto id = static_cast<s32>(r.value);
    auto resolver = std::make_shared<Resolver>();
    resolver->name = std::move(name);
    std::scoped_lock lock{g_mutex};
    g_resolvers[id] = std::move(resolver);
    return id;
}

int DestroyResolver(s32 id) {
    std::shared_ptr<Resolver> resolver;
    {
        std::scoped_lock lock{g_mutex};
        const auto it = g_resolvers.find(id);
        if (it == g_resolvers.end()) {
            return ORBIS_NET_EBADF;
        }
        resolver = std::move(it->second);
        g_resolvers.erase(it);
    }
    {
        std::scoped_lock lock{resolver->mutex};
        resolver->destroyed = true; // keeps a running lookup from writing guest memory
    }
    Core::Net::ReleaseExternalId(id);
    return 0;
}

int AbortResolver(s32 id, u32 flags) {
    const auto resolver = Find(id);
    if (!resolver) {
        return ORBIS_NET_EBADF;
    }
    std::scoped_lock lock{resolver->mutex};
    if (resolver->running) {
        ++resolver->lookup; // orphan the running thread
        resolver->running = false;
        resolver->status = ORBIS_NET_ERROR_EINTR;
        Core::Net::SignalExternal(id, true);
        LOG_DEBUG(Lib_Net, "resolver {}: running lookup aborted", id);
        return 0;
    }
    LOG_DEBUG(Lib_Net, "resolver {}: abort with nothing running, flags {:#x}", id, flags);
    if (flags & ORBIS_NET_RESOLVER_ABORT_FLAG_NTOA_PRESERVATION) {
        resolver->pending_abort[static_cast<size_t>(LookupKind::Ntoa)] = true;
    }
    if (flags & ORBIS_NET_RESOLVER_ABORT_FLAG_ATON_PRESERVATION) {
        resolver->pending_abort[static_cast<size_t>(LookupKind::Aton)] = true;
    }
    return 0;
}

namespace {

template <typename Work, typename Deliver>
s32 RunLookup(s32 id, LookupKind kind, bool async, Work work, Deliver deliver) {
    const auto resolver = Find(id);
    if (!resolver) {
        return ORBIS_NET_ERROR_EBADF;
    }
    u64 lookup = 0;
    {
        std::scoped_lock lock{resolver->mutex};
        if (resolver->running) {
            return ORBIS_NET_ERROR_RESOLVER_EBUSY;
        }
        if (std::exchange(resolver->pending_abort[static_cast<size_t>(kind)], false)) {
            LOG_DEBUG(Lib_Net, "resolver {}: lookup fails with a preserved abort", id);
            resolver->status = ORBIS_NET_ERROR_EINTR;
            return ORBIS_NET_ERROR_EINTR;
        }
        if (async) {
            resolver->running = true;
            resolver->status = 0;
            lookup = ++resolver->lookup;
        }
    }
    if (!async) {
        auto [status, result] = work();
        std::scoped_lock lock{resolver->mutex};
        if (status == 0) {
            status = deliver(result);
        }
        resolver->status = status;
        return status;
    }
    std::thread([resolver, id, lookup, work = std::move(work), deliver = std::move(deliver)] {
        auto [status, result] = work();
        std::scoped_lock lock{resolver->mutex};
        if (resolver->destroyed || resolver->lookup != lookup) {
            return; // stale, the id may have been reused
        }
        if (status == 0) {
            status = deliver(result);
        }
        resolver->status = status;
        resolver->running = false;
        Core::Net::SignalExternal(id);
    }).detach();
    return 0;
}

s32 ReverseLookup(u32 addr, std::string* name) {
    if (addr == htonl(INADDR_LOOPBACK)) {
        *name = "localhost";
        return 0;
    }
    if (!IsOnline()) {
        return ORBIS_NET_ERROR_RESOLVER_ENODNS;
    }
    sockaddr_in in{};
    in.sin_family = AF_INET;
    in.sin_addr.s_addr = addr;
    char host[1025]{}; // NI_MAXHOST
    const int gai = getnameinfo(reinterpret_cast<const sockaddr*>(&in), sizeof(in), host,
                                sizeof(host), nullptr, 0, NI_NAMEREQD);
    switch (gai) {
    case 0:
        *name = host;
        return 0;
    case EAI_NONAME:
        return ORBIS_NET_ERROR_RESOLVER_ENOHOST;
    case EAI_AGAIN:
        return ORBIS_NET_ERROR_RESOLVER_ETIMEDOUT;
    case EAI_FAIL:
        return ORBIS_NET_ERROR_RESOLVER_ESERVERFAILURE;
    default:
        return ORBIS_NET_ERROR_RESOLVER_EINTERNAL;
    }
}

} // namespace

s32 ResolverStartNtoa(s32 id, const char* hostname, ResolverOutput output, bool async,
                      bool disable_ipaddress) {
    if (!Find(id)) {
        return ORBIS_NET_ERROR_EBADF;
    }
    if (hostname == nullptr || (output.addr == nullptr && output.info == nullptr)) {
        return ORBIS_NET_ERROR_EINVAL;
    }
    const size_t length = strnlen(hostname, ORBIS_NET_RESOLVER_HOSTNAME_LEN_MAX + 1);
    if (length == 0 || length > static_cast<size_t>(ORBIS_NET_RESOLVER_HOSTNAME_LEN_MAX)) {
        return ORBIS_NET_ERROR_EINVAL;
    }
    return RunLookup(
        id, LookupKind::Ntoa, async,
        [host = std::string{hostname}, allow_literal = !disable_ipaddress] {
            std::vector<u32> addrs;
            const s32 status = Lookup(host, allow_literal, &addrs);
            if (status == 0) {
                std::string list;
                for (const u32 a : addrs) {
                    const u32 ip = ntohl(a);
                    list += fmt::format("{}{}.{}.{}.{}", list.empty() ? "" : ", ", ip >> 24,
                                        (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
                }
                LOG_INFO(Lib_Net, "{} resolved to {}", host, list);
            } else {
                LOG_INFO(Lib_Net, "{} not resolved: {}", host, ErrorCodeName(status));
            }
            return std::pair{status, std::move(addrs)};
        },
        [output](const std::vector<u32>& addrs) {
            Deliver(output, addrs);
            return s32{0};
        });
}

s32 ResolverStartAton(s32 id, const OrbisNetInAddr* addr, char* hostname, s32 len, bool async) {
    if (!Find(id)) {
        return ORBIS_NET_ERROR_EBADF;
    }
    if (addr == nullptr || hostname == nullptr || len <= 0) {
        return ORBIS_NET_ERROR_EINVAL;
    }
    return RunLookup(
        id, LookupKind::Aton, async,
        [address = addr->inaddr_addr] {
            std::string name;
            const s32 status = ReverseLookup(address, &name);
            const u32 ip = ntohl(address);
            LOG_INFO(Lib_Net, "reverse lookup of {}.{}.{}.{}: {}", ip >> 24, (ip >> 16) & 0xff,
                     (ip >> 8) & 0xff, ip & 0xff, status == 0 ? name : ErrorCodeName(status));
            return std::pair{status, std::move(name)};
        },
        [hostname, len](const std::string& name) {
            if (name.size() + 1 > static_cast<size_t>(len)) {
                return s32{ORBIS_NET_ERROR_RESOLVER_ENOSPACE};
            }
            std::memcpy(hostname, name.c_str(), name.size() + 1);
            return s32{0};
        });
}

int ResolverGetError(s32 id, s32* status) {
    const auto resolver = Find(id);
    if (!resolver) {
        return ORBIS_NET_EBADF;
    }
    std::scoped_lock lock{resolver->mutex};
    *status = resolver->status;
    return 0;
}

void SetResolverOnlineCheck(std::function<bool()> is_online) {
    std::scoped_lock lock{g_mutex};
    g_is_online = std::move(is_online);
}

} // namespace Libraries::Net
