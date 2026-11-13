// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <mutex>
#include <set>
#include <string>

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/piglet/piglet.h"
#include "core/libraries/piglet/piglet_internal.h"

namespace Libraries::Piglet {

namespace {

std::mutex g_proc_mutex;
std::map<std::string, void*> g_proc_addresses;
std::set<std::string> g_logged_stubs;

} // namespace

void RegisterProcAddress(const char* name, void* address) {
    std::scoped_lock lk{g_proc_mutex};
    g_proc_addresses[name] = address;
}

void* LookupProcAddress(const char* name) {
    std::scoped_lock lk{g_proc_mutex};
    const auto it = g_proc_addresses.find(name);
    return it != g_proc_addresses.end() ? it->second : nullptr;
}

void LogStubOnce(const char* name) {
    std::scoped_lock lk{g_proc_mutex};
    if (g_logged_stubs.insert(name).second) {
        LOG_ERROR(Lib_Piglet, "(STUBBED) {} called", name);
    }
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    RegisterEgl(sym);
    RegisterGles(sym);
    RegisterPigletApi(sym);
}

} // namespace Libraries::Piglet
