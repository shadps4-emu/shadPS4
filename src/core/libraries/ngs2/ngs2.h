// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Ngs2 {

class Ngs2;

using SceNgs2Handle = Ngs2*;

enum class SceNgs2HandleType : u32 {
    System = 0,
};

struct Ngs2Handle {
    void* selfPointer;
    void* dataPointer;
    std::atomic<u32>* atomicPtr;
    u32 handleType;
    u32 flags_unk;

    u32 uid;
    u16 maxGrainSamples;
    u16 minGrainSamples;
    u16 currentGrainSamples;
    u16 numGrainSamples;
    u16 unknown2;
    u32 sampleRate;
    u32 unknown3;

    void* flushMutex;
    u32 flushMutexInitialized;
    void* processMutex;
    u32 processMutexInitialized;

    // Linked list pointers for system list
    Ngs2Handle* prev;
    Ngs2Handle* next;
};

struct SystemOptions {
    char padding[6];
    s32 maxGrainSamples;
    s32 numGrainSamples;
    s32 sampleRate;
};

struct SystemState {
    // TODO
};

struct StackBuffer {
    void** top;
    void* base;
    void* curr;
    size_t usedSize;
    size_t totalSize;
    size_t alignment;
    char isVerifyEnabled;
    char padding[7];
};

void RegisterlibSceNgs2(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Ngs2
