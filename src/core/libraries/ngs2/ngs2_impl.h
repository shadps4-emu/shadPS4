// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/ngs2/ngs2.h"

namespace Libraries::Ngs2::Impl {

s32 RackCreate(OrbisNgs2Handle systemHandle, u32 rackId, const OrbisNgs2RackOption* option,
               const OrbisNgs2ContextBufferInfo* bufferInfo, OrbisNgs2Handle* outHandle);
s32 RackCreateWithAllocator(OrbisNgs2Handle systemHandle, u32 rackId,
                            const OrbisNgs2RackOption* option,
                            const OrbisNgs2BufferAllocator* allocator, OrbisNgs2Handle* outHandle);
s32 RackDestroy(OrbisNgs2Handle rackHandle, OrbisNgs2ContextBufferInfo* outBufferInfo);
s32 RackGetInfo(OrbisNgs2Handle rackHandle, OrbisNgs2RackInfo* outInfo, size_t infoSize);
s32 RackGetUserData(OrbisNgs2Handle rackHandle, uintptr_t* outUserData);
s32 RackGetVoiceHandle(OrbisNgs2Handle rackHandle, u32 voiceIndex, OrbisNgs2Handle* outHandle);
s32 RackQueryBufferSize(u32 rackId, const OrbisNgs2RackOption* option,
                        OrbisNgs2ContextBufferInfo* outBufferInfo);
s32 RackSetUserData(OrbisNgs2Handle rackHandle, uintptr_t userData);
s32 SystemCreate(const OrbisNgs2SystemOption* option, const OrbisNgs2ContextBufferInfo* bufferInfo,
                 OrbisNgs2Handle* outHandle);
s32 SystemCreateWithAllocator(const OrbisNgs2SystemOption* option,
                              const OrbisNgs2BufferAllocator* allocator,
                              OrbisNgs2Handle* outHandle);
s32 SystemDestroy(OrbisNgs2Handle systemHandle, OrbisNgs2ContextBufferInfo* outBufferInfo);
s32 SystemEnumHandles(OrbisNgs2Handle* aOutHandle, u32 maxHandles);
s32 SystemEnumRackHandles(OrbisNgs2Handle systemHandle, OrbisNgs2Handle* aOutHandle,
                          u32 maxHandles);
s32 SystemGetInfo(OrbisNgs2Handle systemHandle, OrbisNgs2SystemInfo* outInfo, size_t infoSize);
s32 SystemGetUserData(OrbisNgs2Handle systemHandle, uintptr_t* outUserData);
s32 SystemQueryBufferSize(const OrbisNgs2SystemOption* option,
                          OrbisNgs2ContextBufferInfo* outBufferInfo);
s32 SystemSetGrainSamples(OrbisNgs2Handle systemHandle, u32 numSamples);
s32 SystemSetSampleRate(OrbisNgs2Handle systemHandle, u32 sampleRate);
s32 SystemSetUserData(OrbisNgs2Handle systemHandle, uintptr_t userData);
s32 VoiceGetOwner(OrbisNgs2Handle voiceHandle, OrbisNgs2Handle* outRackHandle, u32* outVoiceId);
s32 VoiceGetState(OrbisNgs2Handle voiceHandle, OrbisNgs2VoiceState* outState, size_t stateSize);
s32 VoiceGetStateFlags(OrbisNgs2Handle voiceHandle, u32* outStateFlags);
s32 SystemRender(OrbisNgs2Handle systemHandle, const OrbisNgs2RenderBufferInfo* aBufferInfo,
                 u32 numBufferInfo);

} // namespace Libraries::Ngs2::Impl
