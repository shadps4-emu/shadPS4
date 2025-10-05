// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpAuth {

struct OrbisNpAuthCreateAsyncRequestParameter {
	u64 size;
	u64 cpu_affinity_mask;
	s32 thread_priority;
	u8 padding[4];
};

struct OrbisNpAuthGetAuthorizationCodeParameterA {
	u64 size;
	s32 user_id;
	u8 padding[4];
	const OrbisNpClientId* client_id;
	const char* scope;
};

struct OrbisNpAuthGetIdTokenParameterA {
	u64 size;
	s32 user_id;
	u8 padding[4];
	const OrbisNpClientId* client_id;
	const OrbisNpClientSecret* client_secret;
	const char* scope;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpAuth