// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ngs2.h"

#include <stdarg.h> // va_list

namespace Libraries::Ngs2 {

class Ngs2Report;

struct OrbisNgs2ReportDataHeader {
    size_t size;
    OrbisNgs2Handle handle;
    u32 type;
    s32 result;
};

using OrbisNgs2ReportHandler = void PS4_SYSV_ABI (*)(const OrbisNgs2ReportDataHeader* data,
                                                     uintptr_t user_data);

struct OrbisNgs2ReportMessageData {
    OrbisNgs2ReportDataHeader header;
    const char* message;
};

struct OrbisNgs2ReportApiData {
    OrbisNgs2ReportDataHeader header;
    const char* functionName;
    const char* format;
    va_list argument;
};

struct OrbisNgs2ReportControlData {
    OrbisNgs2ReportDataHeader header;
    const OrbisNgs2VoiceParamHeader* param;
};

struct OrbisNgs2ReportOutputData {
    OrbisNgs2ReportDataHeader header;
    const OrbisNgs2RenderBufferInfo* bufferInfo;

    u32 bufferIndex;
    u32 sampleRate;
    u32 numGrainSamples;
    u32 reserved;
};

struct OrbisNgs2ReportCpuLoadData {
    OrbisNgs2ReportDataHeader header;
    float totalRatio;
    float flushRatio;
    float processRatio;
    float feedbackRatio;
};

struct OrbisNgs2ReportRenderStateData {
    OrbisNgs2ReportDataHeader header;
    u32 state;
    u32 reserved;
};

struct OrbisNgs2ReportVoiceWaveformData {
    OrbisNgs2ReportDataHeader header;
    u32 location;
    u32 waveformType;
    u32 numChannels;
    u32 sampleRate;
    u32 numGrainSamples;
    u32 reserved;
    void* const* aData;
};

s32 PS4_SYSV_ABI sceNgs2ReportRegisterHandler(u32 reportType, OrbisNgs2ReportHandler handler,
                                              uintptr_t userData, OrbisNgs2Handle* outHandle);

} // namespace Libraries::Ngs2
