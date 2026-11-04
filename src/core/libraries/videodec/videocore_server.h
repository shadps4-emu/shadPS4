// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::VideoCoreServer {

// libSceVideoCoreServerInterface definitions
struct OrbisVideoCoreAcquireDecoderResourceParam {
    u32 resource_type;
    u32 max_width;
    u32 max_height;
    u32 stream_spec;
};
static_assert(sizeof(OrbisVideoCoreAcquireDecoderResourceParam) == 0x10);

using OrbisVideoCoreCallback = void(PS4_SYSV_ABI*)(u32 res_id, u64 user_data);

s32 PS4_SYSV_ABI
sceVideoCoreAcquireDecoderResource(const OrbisVideoCoreAcquireDecoderResourceParam* param,
                                   OrbisVideoCoreCallback cb, u32* out_resource_id);

s32 PS4_SYSV_ABI sceVideoCoreReleaseDecoderResource(u32 resource_id);

s32 PS4_SYSV_ABI sceVideoCoreAcquireDecoderResourceForBeWrapper(
    const OrbisVideoCoreAcquireDecoderResourceParam* param, OrbisVideoCoreCallback cb,
    u32* out_resource_id);

s32 PS4_SYSV_ABI sceVideoCoreReleaseDecoderResourceForBeWrapper(u32 resource_id);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::VideoCoreServer
