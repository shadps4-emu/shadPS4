// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpWebApi2 {

s32 createLibraryContext(s32 http_ctx_id, u64 pool_size, const char* name);
s32 getMemoryPoolStats(s32 lib_ctx_id, OrbisNpWebApi2MemoryPoolStats* stats);
s32 createUserContext(s32 lib_ctx_id, Libraries::UserService::OrbisUserServiceUserId user_id);
}; // namespace Libraries::Np::NpWebApi2