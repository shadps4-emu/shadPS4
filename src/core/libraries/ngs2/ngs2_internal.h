// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>
#include <memory>
#include <vector>
#include "ngs2.h"
#include "ngs2_impl.h"

namespace Libraries::Ngs2 {

// Forward declarations
struct RackInternal;
struct VoiceInternal;

// Waveform block for streaming support
struct WaveformBlockInternal {
    u32 dataOffset;     // Offset into the base data pointer
    u32 dataSize;       // Size of this block in bytes
    u32 numRepeats;     // Number of times to repeat this block (0 = once)
    u32 numSkipSamples; // Samples to skip at start
    u32 numSamples;     // Total samples in this block
    u32 currentRepeat;  // Current repeat iteration

    WaveformBlockInternal()
        : dataOffset(0), dataSize(0), numRepeats(0), numSkipSamples(0), numSamples(0),
          currentRepeat(0) {}
};

// Ring buffer slot for streaming
struct RingBufferSlot {
    const void* basePtr; // Original base pointer provided by game
    const void* data;    // Actual data pointer (with offset applied)
    u32 dataSize;
    u32 numSamples;
    bool valid;    // Whether this slot contains valid data
    bool consumed; // Whether this slot has been fully consumed

    RingBufferSlot()
        : basePtr(nullptr), data(nullptr), dataSize(0), numSamples(0), valid(false),
          consumed(false) {}

    void set(const void* base, const void* d, u32 ds, u32 ns) {
        basePtr = base;
        data = d;
        dataSize = ds;
        numSamples = ns;
        valid = true;
        consumed = false;
    }

    void markConsumed() {
        consumed = true;
    }

    void reset() {
        basePtr = nullptr;
        data = nullptr;
        dataSize = 0;
        numSamples = 0;
        valid = false;
        consumed = false;
    }
};

struct VoiceInternal {
    HandleInternal handle;
    RackInternal* ownerRack;
    u32 voiceIndex;
    u32 stateFlags;
    std::vector<OrbisNgs2VoicePortInfo> ports;
    std::vector<OrbisNgs2VoiceMatrixInfo> matrices;

    // Sampler-specific data
    OrbisNgs2WaveformFormat format;
    float pitchRatio;
    float portVolume; // Volume level for the voice (0.0 to 1.0+)
    bool isSetup;

    // Playback position (in samples, not bytes)
    u32 currentSamplePos; // Integer sample position
    float samplePosFloat; // Floating point position for resampling

    // Block-based streaming support
    std::vector<WaveformBlockInternal> waveformBlocks;
    u32 currentBlockIndex; // Which block we're reading from
    u32 flags;
    bool isStreaming;

    // Ring buffer for streaming audio
    // The game provides buffers in a circular fashion (A->B->C->A->B->C...)
    // We track which slots have data and which have been consumed
    // Game typically refills with 3 buffers at once, so ring must accommodate that
    static constexpr u32 MAX_RING_SLOTS = 3;       // Support game's 3-buffer refill pattern
    static constexpr u32 STARVATION_THRESHOLD = 0; // Signal when down to 1 buffer (game adds 3)
    RingBufferSlot ringBuffer[MAX_RING_SLOTS];
    u32 ringWriteIndex;                       // Next slot to write to (from game)
    u32 ringReadIndex;                        // Current slot being read (by renderer)
    u32 ringBufferCount;                      // Number of valid buffers in ring
    const void* lastConsumedBuffer = nullptr; // Last buffer we finished reading (for game to check)
    u64 totalDecodedSamples;                  // Total samples decoded so far
    const void* currentBufferPtr = nullptr;   // Add this

    VoiceInternal()
        : ownerRack(nullptr), voiceIndex(0), stateFlags(0), pitchRatio(1.0f), portVolume(1.0f),
          isSetup(false), currentSamplePos(0), samplePosFloat(0.0f), currentBlockIndex(0), flags(0),
          isStreaming(false), ringWriteIndex(0), ringReadIndex(0), ringBufferCount(0),
          lastConsumedBuffer(nullptr) {
        handle.selfPtr = &handle;
        handle.systemData = nullptr;
        handle.refCount = 1;
        handle.handleType = 3; // Voice
        handle.handleID = 0;
        std::memset(&format, 0, sizeof(format));
        for (u32 i = 0; i < MAX_RING_SLOTS; i++) {
            ringBuffer[i].reset();
        }
    }

    // Get the current buffer being read
    RingBufferSlot* getCurrentSlot() {
        // We use the consumed flag to determine if the current read head is valid data
        if (ringBuffer[ringReadIndex].valid && !ringBuffer[ringReadIndex].consumed) {
            return &ringBuffer[ringReadIndex];
        }
        return nullptr;
    }

    // Advance to the next buffer in the ring
    void advanceReadIndex() {
        if (!ringBuffer[ringReadIndex].valid)
            return;

        ringBuffer[ringReadIndex].consumed = true;
        lastConsumedBuffer = ringBuffer[ringReadIndex].basePtr;
        ringReadIndex = (ringReadIndex + 1) % MAX_RING_SLOTS;

        if (ringBufferCount > 0) {
            ringBufferCount--;
        }
    }

    // Add a buffer to the ring
    bool addToRing(const void* basePtr, const void* data, u32 dataSize, u32 numSamples) {
        if (ringBufferCount >= MAX_RING_SLOTS) {
            return false;
        }

        RingBufferSlot* slot = &ringBuffer[ringWriteIndex];
        if (slot->valid && !slot->consumed) {
            return false;
        }

        slot->set(basePtr, data, dataSize, numSamples);
        ringWriteIndex = (ringWriteIndex + 1) % MAX_RING_SLOTS;
        ringBufferCount++;

        return true;
    }

    // Reset the ring buffer state
    void resetRing() {
        for (u32 i = 0; i < MAX_RING_SLOTS; i++) {
            ringBuffer[i].reset();
        }
        ringWriteIndex = 0;
        ringReadIndex = 0;
        ringBufferCount = 0;
        lastConsumedBuffer = nullptr;
        currentBufferPtr = nullptr;
        totalDecodedSamples = 0;
    }

    // Get number of buffers ready to read
    u32 getReadyBufferCount() const {
        return ringBufferCount;
    }
};

struct RackInternal {
    HandleInternal handle;
    OrbisNgs2RackInfo info;
    SystemInternal* ownerSystem;
    std::vector<std::unique_ptr<VoiceInternal>> voices;
    u32 rackType;
    u32 rackId;

    RackInternal() : ownerSystem(nullptr), rackType(0), rackId(0) {
        handle.selfPtr = &handle;
        handle.systemData = nullptr;
        handle.refCount = 1;
        handle.handleType = 2; // Rack
        handle.handleID = 0;
        std::memset(&info, 0, sizeof(info));
    }
};

} // namespace Libraries::Ngs2
