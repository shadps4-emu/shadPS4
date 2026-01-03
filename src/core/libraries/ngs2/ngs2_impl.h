// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/kernel/threads/pthread.h"
#include <atomic>
#include <vector>

namespace Libraries::Ngs2 {

static const int ORBIS_NGS2_SYSTEM_NAME_LENGTH = 16;
static const int ORBIS_NGS2_RACK_NAME_LENGTH = 16;

typedef uintptr_t OrbisNgs2Handle;

struct OrbisNgs2ContextBufferInfo {
    void* hostBuffer;
    size_t hostBufferSize;
    uintptr_t reserved[5];
    uintptr_t userData;
};

struct OrbisNgs2SystemOption {
    size_t size;
    char name[ORBIS_NGS2_SYSTEM_NAME_LENGTH];

    u32 flags;
    u32 maxGrainSamples;
    u32 numGrainSamples;
    u32 sampleRate;
    u32 aReserved[6];
};

using OrbisNgs2BufferAllocHandler =
    s32 PS4_SYSV_ABI (*)(OrbisNgs2ContextBufferInfo* io_buffer_info);
using OrbisNgs2BufferFreeHandler = s32 PS4_SYSV_ABI (*)(OrbisNgs2ContextBufferInfo* io_buffer_info);

struct OrbisNgs2SystemInfo {
    char name[ORBIS_NGS2_SYSTEM_NAME_LENGTH]; // 0

    OrbisNgs2Handle systemHandle;          // 16
    OrbisNgs2ContextBufferInfo bufferInfo; // 24

    u32 uid;             // 88
    u32 minGrainSamples; // 92
    u32 maxGrainSamples; // 96

    u32 stateFlags;        // 100
    u32 rackCount;         // 104
    float lastRenderRatio; // 108
    s64 lastRenderTick;    // 112
    s64 renderCount;       // 120
    u32 sampleRate;        // 128
    u32 numGrainSamples;   // 132
};

struct OrbisNgs2RackInfo {
    char name[ORBIS_NGS2_RACK_NAME_LENGTH]; // 0

    OrbisNgs2Handle rackHandle;            // 16
    OrbisNgs2ContextBufferInfo bufferInfo; // 24

    OrbisNgs2Handle ownerSystemHandle; // 88

    u32 type;            // 96
    u32 rackId;          // 100
    u32 uid;             // 104
    u32 minGrainSamples; // 108
    u32 maxGrainSamples; // 112
    u32 maxVoices;       // 116
    u32 maxChannelWorks; // 120
    u32 maxInputs;       // 124
    u32 maxMatrices;     // 128
    u32 maxPorts;        // 132

    u32 stateFlags;             // 136
    float lastProcessRatio;     // 140
    u64 lastProcessTick;        // 144
    u64 renderCount;            // 152
    u32 activeVoiceCount;       // 160
    u32 activeChannelWorkCount; // 164
};

struct StackBuffer {
    void** top;
    void* base;
    size_t size;
    size_t currentOffset;
    size_t usedSize;
    size_t totalSize;
    size_t alignment;
    u8 flags;
    char padding[7];
};

// Forward declarations for types used before definition
struct RackInternal;
struct VoiceInternal;

struct SystemInternal {
    // setup init
    char name[ORBIS_NGS2_SYSTEM_NAME_LENGTH]; // 0
    OrbisNgs2ContextBufferInfo bufferInfo;    // 16
    OrbisNgs2BufferFreeHandler hostFree;      // 80
    OrbisNgs2Handle systemHandle;             // 88
    void* unknown1;                           // 96
    void* unknown2;                           // 104
    OrbisNgs2Handle rackHandle;               // 112
    uintptr_t* userData;                      // 120
    SystemInternal* systemList;               // 128
    StackBuffer* stackBuffer;                 // 136
    OrbisNgs2SystemInfo ownerSystemInfo;      // 144

    struct rackList {
        void* prev;
        void* next;
        void* unknown;
    };

    rackList rackListPreset; // 152
    rackList rackListNormal; // 176
    rackList rackListMaster; // 200

    void* unknown3;       // 208
    void* systemListPrev; // 216
    void* unknown4;       // 224
    void* systemListNext; // 232
    void* rackFunction;   // 240

    Kernel::PthreadMutex processLock; // 248
    u32 hasProcessMutex;              // 256
    u32 unknown5;                     // 260
    Kernel::PthreadMutex flushLock;   // 264
    u32 hasFlushMutex;                // 272
    u32 unknown6;                     // 276

    // info
    u64 lastRenderTick;         // 280
    u64 renderCount;            // 288
    u32 isActive;               // 296
    std::atomic<int> lockCount; // 300
    u32 uid;                    // 304
    u32 systemType;             // 308

    struct {
        u8 isBufferValid : 1;
        u8 isRendering : 1;
        u8 isSorted : 1;
        u8 isFlushReady : 1;
    } flags; // 312

    u16 currentMaxGrainSamples; // 316
    u16 minGrainSamples;        // 318
    u16 maxGrainSamples;        // 320
    u16 numGrainSamples;        // 322
    u32 currentNumGrainSamples; // 324
    u32 sampleRate;             // 328
    u32 currentSampleRate;      // 332
    u32 rackCount;              // 336
    float lastRenderRatio;      // 340
    float cpuLoad;              // 344
    
    // Rack management
    std::vector<RackInternal*> racks;
};

struct HandleInternal {
    HandleInternal* selfPtr;    // 0
    SystemInternal* systemData; // 8
    std::atomic<int> refCount;  // 16
    u32 handleType;             // 24
    u32 handleID;               // 28
};

s32 StackBufferClose(StackBuffer* stackBuffer, size_t* outTotalSize);
s32 StackBufferOpen(StackBuffer* stackBuffer, void* buffer, size_t bufferSize, void** outBuffer,
                    u8 flags);
s32 SystemSetupCore(StackBuffer* stackBuffer, const OrbisNgs2SystemOption* option,
                    SystemInternal* outSystem);

s32 HandleReportInvalid(OrbisNgs2Handle handle, u32 handleType);
void* MemoryClear(void* buffer, size_t size);
s32 SystemCleanup(OrbisNgs2Handle systemHandle, OrbisNgs2ContextBufferInfo* outInfo);
s32 SystemSetup(const OrbisNgs2SystemOption* option, OrbisNgs2ContextBufferInfo* hostBufferInfo,
                OrbisNgs2BufferFreeHandler hostFree, OrbisNgs2Handle* outHandle);

// Forward declarations for internal types
struct RackInternal;
struct SystemInternal;
struct OrbisNgs2RackOption;

u32 RackIdToIndex(u32 rackId);
s32 RackCreate(SystemInternal* system, u32 rackId, const OrbisNgs2RackOption* option,
               const OrbisNgs2ContextBufferInfo* bufferInfo, OrbisNgs2Handle* outHandle);
s32 RackDestroy(RackInternal* rack, OrbisNgs2ContextBufferInfo* outBufferInfo);

} // namespace Libraries::Ngs2
