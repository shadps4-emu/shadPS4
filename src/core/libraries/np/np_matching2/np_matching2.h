// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types2.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpMatching2 {

int PS4_SYSV_ABI sceNpMatching2Initialize(OrbisNpMatching2InitializeParameter* param);
int PS4_SYSV_ABI sceNpMatching2Terminate();
int PS4_SYSV_ABI sceNpMatching2CreateContext(const OrbisNpMatching2CreateContextParameter* param,
                                             OrbisNpMatching2ContextId* ctxId);
int PS4_SYSV_ABI sceNpMatching2CreateContextA(const OrbisNpMatching2CreateContextParameterA* param,
                                              OrbisNpMatching2ContextId* ctxId);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpMatching2
