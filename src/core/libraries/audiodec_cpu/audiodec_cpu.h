// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::AudiodecCpu {

enum class OrbisAudiodecCpuCodec : s32 {
    M4Aac = 0x1001,
    Dts = 0x1002,
    LpcmBd2 = 0x1003,
    LpcmDvd2 = 0x1004,
    DtsHdMa = 0x1005,
    DtsHdLbr = 0x1006,
    Ddp = 0x1007,
    Hevag = 0x1008,
    Alac2 = 0x1009,
    Flac2 = 0x100A,
};

constexpr u32 ORBIS_AUDIODECCPU_DESCRIPTOR_SIZE = 0x18;

// Resource, AU info and PCM item descriptors share this layout.
struct OrbisAudiodecCpuMemoryDescriptor {
    u32 this_size{};
    u32 reserved{};
    void* data{};
    // Query writes the work memory size. Decode replaces AU/PCM capacities with the byte counts.
    u32 data_size{};
    u32 reserved2{};
};
static_assert(sizeof(OrbisAudiodecCpuMemoryDescriptor) == ORBIS_AUDIODECCPU_DESCRIPTOR_SIZE);
static_assert(offsetof(OrbisAudiodecCpuMemoryDescriptor, data) == 0x08);
static_assert(offsetof(OrbisAudiodecCpuMemoryDescriptor, data_size) == 0x10);

struct OrbisAudiodecCpuQueryCtrl {
    const void* param{};
};

struct OrbisAudiodecCpuInitCtrl {
    const void* param{};
    void* bsi_info{};
};

struct OrbisAudiodecCpuDecodeCtrl {
    const void* param{};
    void* bsi_info{};
    OrbisAudiodecCpuMemoryDescriptor* au_info{};
    OrbisAudiodecCpuMemoryDescriptor* pcm_item{};
};
static_assert(sizeof(OrbisAudiodecCpuQueryCtrl) == 0x08);
static_assert(sizeof(OrbisAudiodecCpuInitCtrl) == 0x10);
static_assert(sizeof(OrbisAudiodecCpuDecodeCtrl) == 0x20);

int PS4_SYSV_ABI sceAudiodecCpuQueryMemSize(const OrbisAudiodecCpuQueryCtrl* ctrl,
                                            OrbisAudiodecCpuMemoryDescriptor* resource,
                                            const s32 codec_id);

int PS4_SYSV_ABI sceAudiodecCpuInitDecoder(const OrbisAudiodecCpuInitCtrl* ctrl,
                                           OrbisAudiodecCpuMemoryDescriptor* resource,
                                           const s32 codec_id);

int PS4_SYSV_ABI sceAudiodecCpuDecode(const OrbisAudiodecCpuDecodeCtrl* ctrl,
                                      OrbisAudiodecCpuMemoryDescriptor* resource,
                                      const s32 codec_id);

int PS4_SYSV_ABI sceAudiodecCpuClearContext(OrbisAudiodecCpuMemoryDescriptor* resource,
                                            const s32 codec_id);

int PS4_SYSV_ABI sceAudiodecCpuInternalQueryMemSize(const OrbisAudiodecCpuQueryCtrl* ctrl,
                                                    OrbisAudiodecCpuMemoryDescriptor* resource,
                                                    const s32 codec_id);
int PS4_SYSV_ABI sceAudiodecCpuInternalInitDecoder(const OrbisAudiodecCpuInitCtrl* ctrl,
                                                   OrbisAudiodecCpuMemoryDescriptor* resource,
                                                   const s32 codec_id);
int PS4_SYSV_ABI sceAudiodecCpuInternalDecode(const OrbisAudiodecCpuDecodeCtrl* ctrl,
                                              OrbisAudiodecCpuMemoryDescriptor* resource,
                                              const s32 codec_id);
int PS4_SYSV_ABI sceAudiodecCpuInternalClearContext(OrbisAudiodecCpuMemoryDescriptor* resource,
                                                    const s32 codec_id);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::AudiodecCpu
