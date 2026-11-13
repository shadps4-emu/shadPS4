// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Piglet {

// Registration of each part of the library. Split by area so every part can be reviewed
// and extended independently.
void RegisterEgl(Core::Loader::SymbolsResolver* sym);
void RegisterGles(Core::Loader::SymbolsResolver* sym);
void RegisterPigletApi(Core::Loader::SymbolsResolver* sym);

// Logs once per function that an entry point has no data to report yet, so a call made every
// frame does not flood the log.
void LogStubOnce(const char* name);

// The piglet platform layer is brought up together with the first EGL display. Configuration set
// afterwards is reported as an error, like on hardware.
void MarkPigletInitialized();

// Whether an EGL display currently exists.
bool IsDisplayCreated();

// Name to address table used by eglGetProcAddress.
void RegisterProcAddress(const char* name, void* address);
void* LookupProcAddress(const char* name);

} // namespace Libraries::Piglet

// Registers an exported function and makes it reachable through eglGetProcAddress.
#define PIGLET_EXPORT(nid, name)                                                                   \
    LIB_FUNCTION(nid, "libScePigletv2VSH", 1, "libScePigletv2VSH", name);                          \
    RegisterProcAddress(#name, reinterpret_cast<void*>(&name))
