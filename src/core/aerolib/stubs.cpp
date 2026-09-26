// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <xbyak/xbyak.h>

#include "common/logging/log.h"
#include "core/aerolib/aerolib.h"
#include "core/aerolib/stubs.h"

namespace Core::AeroLib {

// Helper to provide stub implementations for missing functions
//
// This works by constructing a minimal trampoline for each new stub which then jumps to a common
// handler with its index provided as a parameter so nid info can be found

struct StubEntry {
    const NidEntry* nid = nullptr;
    std::string nid_unknown;
    std::unique_ptr<Xbyak::CodeGenerator> code;
};

static std::vector<StubEntry> g_stub_entries;
static std::mutex g_stub_mutex;

static u64 PS4_SYSV_ABI CommonStub(u64 index) {
    const auto& e = g_stub_entries[index];
    if (e.nid) {
        LOG_ERROR(Core, "Stub: {} (nid: {}) called, returning zero to {}", e.nid->name, e.nid->nid,
                  __builtin_return_address(0));
    } else {
        LOG_ERROR(Core, "Stub: Unknown (nid: {}) called, returning zero to {}", e.nid_unknown,
                  __builtin_return_address(0));
    }
    return 0;
}

u64 GetStub(const char* nid) {
    std::scoped_lock lock{g_stub_mutex};

    if (g_stub_entries.empty()) {
        g_stub_entries.reserve(500);
    }

    const u64 index = g_stub_entries.size();

    StubEntry e;
    if (const auto* entry = FindByNid(nid)) {
        if (auto const& it = std::ranges::find_if(
                g_stub_entries, [entry](StubEntry const& en) { return en.nid && en.nid == entry; });
            it != g_stub_entries.end()) {
            return reinterpret_cast<u64>(it->code->getCode());
        }
        e.nid = entry;
    } else {
        e.nid_unknown = nid;
    }

    e.code = std::make_unique<Xbyak::CodeGenerator>(32, Xbyak::AutoGrow);
    e.code->mov(e.code->rdi, index);
    e.code->mov(e.code->rax, reinterpret_cast<u64>(&CommonStub));
    e.code->jmp(e.code->rax);
    e.code->ready();

    g_stub_entries.push_back(std::move(e));
    return reinterpret_cast<u64>(g_stub_entries.back().code->getCode());
}

} // namespace Core::AeroLib
