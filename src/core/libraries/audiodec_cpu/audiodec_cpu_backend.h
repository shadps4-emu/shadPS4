// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include "core/libraries/audiodec_cpu/audiodec_cpu.h"

namespace Libraries::AudiodecCpu {

// LLE decoders export this object. Every callback uses the guest calling convention.
struct CodecOps {
    using QueryMemSizeFunc = PS4_SYSV_ABI int (*)(const OrbisAudiodecCpuQueryCtrl*,
                                                  OrbisAudiodecCpuMemoryDescriptor*);
    using InitDecoderFunc = PS4_SYSV_ABI int (*)(const OrbisAudiodecCpuInitCtrl*,
                                                 OrbisAudiodecCpuMemoryDescriptor*);
    using DecodeFunc = PS4_SYSV_ABI int (*)(const OrbisAudiodecCpuDecodeCtrl*,
                                            OrbisAudiodecCpuMemoryDescriptor*);
    using ClearContextFunc = PS4_SYSV_ABI int (*)(OrbisAudiodecCpuMemoryDescriptor*);

    QueryMemSizeFunc query_mem_size{};
    InitDecoderFunc init_decoder{};
    DecodeFunc decode{};
    ClearContextFunc clear_context{};
};
static_assert(sizeof(CodecOps) == 0x20);
static_assert(offsetof(CodecOps, query_mem_size) == 0x00);
static_assert(offsetof(CodecOps, init_decoder) == 0x08);
static_assert(offsetof(CodecOps, decode) == 0x10);
static_assert(offsetof(CodecOps, clear_context) == 0x18);

const CodecOps* ResolveLleCodecOps(const char* module, const char* nid);

} // namespace Libraries::AudiodecCpu
