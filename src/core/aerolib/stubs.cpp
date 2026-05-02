// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/aerolib/aerolib.h"
#include "core/aerolib/stubs.h"
#include <mutex>
#include <string>
#include <unordered_map>

namespace Core::AeroLib {

// Helper to provide stub implementations for missing functions
//
// This works by pre-compiling generic stub functions ("slots"), and then
// on lookup, setting up the nid_entry they are matched with
//
// If it runs out of stubs with name information, it will return
// a default implementation without function name details

// Up to 512, larger values lead to more resolve stub slots
// and to longer compile / CI times
//
// Must match STUBS_LIST define
constexpr u32 MAX_STUBS = 4096;

u64 UnresolvedStub() {
    LOG_ERROR(Core, "Returning zero to {}", __builtin_return_address(0));
    return 0;
}

static u64 UnknownStub() {
    LOG_ERROR(Core, "Returning zero to {}", __builtin_return_address(0));
    return 0;
}

static const NidEntry* stub_nids[MAX_STUBS];
static std::string stub_nids_unknown[MAX_STUBS];
static std::unordered_map<std::string, u64> nid_stub_cache{};
static std::mutex stub_mutex{};

template <int stub_index>
static u64 CommonStub() {
    auto entry = stub_nids[stub_index];
    if (entry) {
        LOG_ERROR(Core, "Stub: {} (nid: {}) called, returning zero to {}", entry->name, entry->nid,
                  __builtin_return_address(0));
    } else {
        LOG_ERROR(Core, "Stub: Unknown (nid: {}) called, returning zero to {}",
                  stub_nids_unknown[stub_index], __builtin_return_address(0));
    }
    return 0;
}

static u32 UsedStubEntries;

#define XREP_1(x) &CommonStub<x>,

#define XREP_2(x) XREP_1(x) XREP_1(x + 1)
#define XREP_4(x) XREP_2(x) XREP_2(x + 2)
#define XREP_8(x) XREP_4(x) XREP_4(x + 4)
#define XREP_16(x) XREP_8(x) XREP_8(x + 8)
#define XREP_32(x) XREP_16(x) XREP_16(x + 16)
#define XREP_64(x) XREP_32(x) XREP_32(x + 32)
#define XREP_128(x) XREP_64(x) XREP_64(x + 64)
#define XREP_256(x) XREP_128(x) XREP_128(x + 128)
#define XREP_512(x) XREP_256(x) XREP_256(x + 256)
#define XREP_1024(x) XREP_512(x) XREP_512(x + 512)
#define XREP_2048(x) XREP_1024(x) XREP_1024(x + 1024)
#define XREP_4096(x) XREP_2048(x) XREP_2048(x + 2048)

#define STUBS_LIST XREP_4096(0)

static u64 (*stub_handlers[MAX_STUBS])() = {STUBS_LIST};

u64 GetStub(const char* nid) {
    if (nid == nullptr || nid[0] == '\0') {
        return (u64)&UnknownStub;
    }
    const std::string nid_key{nid};
    std::scoped_lock lk{stub_mutex};
    if (const auto it = nid_stub_cache.find(nid_key); it != nid_stub_cache.end()) {
        return it->second;
    }

    if (UsedStubEntries >= MAX_STUBS) {
        LOG_ERROR(Core, "Stub slot exhausted for nid {} (max={})", nid_key, MAX_STUBS);
        return (u64)&UnknownStub;
    }

    const auto entry = FindByNid(nid);
    if (!entry) {
        stub_nids_unknown[UsedStubEntries] = nid;
    } else {
        stub_nids[UsedStubEntries] = entry;
    }

    const u64 stub_addr = (u64)stub_handlers[UsedStubEntries++];
    nid_stub_cache.emplace(nid_key, stub_addr);
    return stub_addr;
}

} // namespace Core::AeroLib
