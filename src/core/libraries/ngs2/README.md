# NGS2 HLE Implementation

## Overview

**libSceNgs2** (Next Generation Sound 2) is the PlayStation 4's high-level audio library, responsible for audio synthesis, mixing, and effects processing. This HLE (High-Level Emulation) implementation provides the core functionality needed to render audio in PS4 games.

### Architecture

The NGS2 system follows a hierarchical structure:

```
System (OrbisNgs2Handle)
  └── Racks (OrbisNgs2Handle)
        └── Voices (OrbisNgs2Handle)
```

- **System**: The top-level audio context that manages sample rate, grain size, and all child racks
- **Rack**: A processing unit of a specific type (Sampler, Submixer, Mastering, etc.) containing multiple voices
- **Voice**: An individual audio channel that can be configured, played, paused, and stopped

### Audio Flow

```
Sampler Racks → Submixer Racks → Mastering Rack → Output Buffers
```

---

## Implementation Status

### ✅ Fully Implemented

| Feature | Description |
|---------|-------------|
| System Create/Destroy | System lifecycle with buffer allocation |
| System Query Buffer Size | Calculate required buffer sizes |
| Rack Create/Destroy | Sampler rack creation with voice allocation |
| Rack Query Buffer Size | Calculate rack buffer requirements |
| Voice Handle Retrieval | Get voice handles from rack |
| Voice State Management | State flags, play/pause/stop/kill/resume |
| PCM16 Playback | Decode and render 16-bit PCM audio |
| Streaming Audio | Ring buffer-based streaming with 3-slot circular buffer |
| One-Shot Playback | Single-buffer audio playback |
| Pitch Control | Variable playback speed via pitch ratio |
| Port Volume | Per-voice volume control |
| Sample Rate Conversion | Resampling with linear interpolation |
| Multi-channel Support | Mono to 8-channel audio |
| Pan Volume Matrix | Basic stereo panning calculations |

### 🚧 Partially Implemented (Stubbed)

| Feature | Status | Notes |
|---------|--------|-------|
| Mastering Rack | Stubbed | Params accepted but not processed |
| Submixer Rack | Stubbed | Created but no mixing logic |
| Matrix Levels | Stubbed | Returns identity matrix |
| Port Matrix | Stubbed | Param accepted, no effect |
| Port Delay | Stubbed | Param accepted, no effect |
| Voice Patch | Stubbed | Routing not implemented |
| Voice Callback | Stubbed | Callbacks not invoked |
| Envelope | Stubbed | Always returns height 1.0 |
| Peak Meter | Stubbed | Always returns peak 1.0 |

### ❌ Not Implemented

| Feature | Notes |
|---------|-------|
| ATRAC9 Decoding | Compressed audio codec (0x40) |
| Reverb Rack (0x4001) | Effects processing |
| Equalizer Rack (0x4002) | Frequency band adjustment |
| Custom Rack (0x4003) | User-defined processing modules |
| Filter Processing | Biquad, lowpass, etc. |
| Compressor/Limiter | Dynamics processing |
| Distortion | Audio distortion effect |
| Chorus/Delay | Time-based effects |
| LFE (Low Frequency Effects) | Subwoofer channel handling |
| 3D Geometry (sceNgs2Geom*) | Spatial audio positioning |
| FFT Functions | Frequency analysis |
| Stream API | sceNgs2Stream* functions |
| Job Scheduler | Multi-threaded processing |
| Report Handlers | Debug/profiling callbacks |

---

## Rack IDs
```
0x1000 = Sampler (index 0)
0x2000 = Submixer (index 2)
0x2001 = Submixer alt (index 3)
0x3000 = Mastering (index 1)
0x4001 = Reverb (index 4)
0x4002 = Equalizer (index 5)
0x4003 = Custom (index 6)
```

## Sample Rates (valid values)
```
11025, 12000, 22050, 24000, 44100, 48000, 88200, 96000, 176400, 192000
```

## Waveform Types
Valid if: `(type & 0xFFFFFFF8) == 0x80 || type == 0x40 || (type - 0x10) < 0xD`
- `0x40` = ATRAC9
- `0x10-0x1C` = PCM variants
- `0x80-0x87` = PCM variants

Render buffer types: `0x12, 0x13, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D`

## Voice Control Params (ID → size)
```
1 → 0x18  MatrixLevels
2 → 0x10  PortVolume
3 → 0x10  PortMatrix
4 → 0x10  PortDelay
5 → 0x18  Patch
6 → 0x0C  Event
7 → 0x20  Callback
0xC001 → 0x20  Custom
```

## Voice Events (param ID 6)
```
0 = Reset       (clears state)
1 = Pause       (sets 0x200)
2 = Stop        (sets 0x400)
3 = Kill        (sets 0x800)
4 = Resume A    (sets 0x1000)
5 = Resume B    (sets 0x2000)
```

## State Flags (bitfield)
```
0x01     = Playing
0x200    = Paused
0x400    = Stopped
0x800    = Killed
0x1000   = Resume A
0x2000   = Resume B
```
Init sets: `flags = (flags & 0xDF000000) | 0x20000101`
GetStateFlags returns: `flags & 0xFF`

## Module IDs (Custom Racks)
```
0x10=Envelope  0x11=Compressor  0x12=Distortion  0x13=Filter
0x14=Chorus    0x15=Delay       0x16=Reverb      0x17=Sampler
0x18=PitchShift 0x19=Unknown    0x1A=Limiter     0x1B=UserFx
0x1C=Mixer     0x1D=Generator   0x1E=EQ          0x70=Passthrough
```

## Callback Flags
Valid: `0, 1, 2, 3`

## Key Errors
```
0x804a0230 = Invalid system handle
0x804a0260 = Invalid rack ID
0x804a0300 = Invalid voice handle
0x804a0303 = Invalid voice event
0x804a0308 = Invalid voice control ID
0x804a030a = Invalid voice control size
0x804a0402 = Invalid waveform type
```

---

## Implementation

### Existing Structures (DO NOT REDEFINE)
The following are already defined in ngs2 headers:
- `OrbisNgs2Handle` → ngs2_impl.h
- `OrbisNgs2RackDEBUG` → ngs2_impl.h
- `OrbisNgs2RackOption` → ngs2.h
- `OrbisNgs2SystemDEBUG` → ngs2_impl.h
- `OrbisNgs2SystemOption` → ngs2_impl.h
- `OrbisNgs2VoiceState` → ngs2.h
- `OrbisNgs2VoicePortDEBUG` → ngs2.h
- `OrbisNgs2VoiceMatrixDEBUG` → ngs2.h
- `OrbisNgs2VoiceParamHeader` → ngs2.h
- `OrbisNgs2Sampler*` → ngs2_sampler.h
- `OrbisNgs2Submixer*` → ngs2_submixer.h
- `OrbisNgs2Mastering*` → ngs2_mastering.h
- `OrbisNgs2Reverb*` → ngs2_reverb.h
- `OrbisNgs2Custom*` → ngs2_custom.h
- `OrbisNgs2Eq*` → ngs2_eq.h
- `HandleInternal` → ngs2_impl.h
- `SystemInternal` → ngs2_impl.h
- `StackBuffer` → ngs2_impl.h

### New Internal Structures (add to ngs2_impl.h)
```cpp
struct RackInternal {
    HandleInternal handle;
    OrbisNgs2RackDEBUG DEBUG;       // use existing struct
    SystemInternal* ownerSystem;
    std::vector<std::unique_ptr<VoiceInternal>> voices;
    u32 rackType;
};

struct VoiceInternal {
    HandleInternal handle;
    RackInternal* ownerRack;
    u32 voiceIndex;
    u32 stateFlags;
    std::vector<OrbisNgs2VoicePortDEBUG> ports;  // use existing struct
    std::vector<OrbisNgs2VoiceMatrixDEBUG> matrices;  // use existing struct
};

// Add field to existing SystemInternal:
std::vector<RackInternal*> racks;
```

### Phase 1: Handles
- `sceNgs2SystemCreate`: Allocate `SystemInternal`, return as handle
- `sceNgs2SystemDestroy`: Free system and all racks

### Phase 2: Racks
- `sceNgs2RackQueryBufferSize`: Return size based on rack type
- `sceNgs2RackCreate`: Allocate `RackInternal`, map rackId→index, add to system
- `sceNgs2RackGetVoiceHandle`: Return `rack->voices[voiceIndex]`

### Phase 3: Voices
- `sceNgs2VoiceControl`: Parse linked list by param ID, apply per switch above
- `sceNgs2VoiceGetState`: Copy voice state struct
- `sceNgs2VoiceGetStateFlags`: Return `stateFlags & 0xFF`

### Phase 4: Render
```cpp
s32 sceNgs2SystemRender(handle, bufferDEBUG, numBufferDEBUG) {
    // Validate: numBufferDEBUG in 1-16, buffers non-null, types valid
    // Process racks: Samplers → Submixers → Mastering
    // Write output: size = grainSamples * channels * bytesPerSample
    return ORBIS_OK;
}
```

### Minimal Stub
```cpp
// SystemCreate
auto* sys = new SystemInternal();
*outHandle = reinterpret_cast<OrbisNgs2Handle>(sys);

// RackCreate
auto* rack = new RackInternal();
rack->voices.resize(option->maxVoices);
*outHandle = reinterpret_cast<OrbisNgs2Handle>(rack);

// VoiceGetStateFlags
*out = 0;  // Not playing

// SystemRender
for (u32 i = 0; i < numBufferDEBUG; i++)
    memset(bufferDEBUG[i].buffer, 0, bufferDEBUG[i].bufferSize);
```

## Files
| File | Purpose |
|------|---------|
| ngs2.cpp | Main API entry points and render loop |
| ngs2.h | Public API structures and types |
| ngs2_impl.cpp | System and rack lifecycle management |
| ngs2_impl.h | Core types (SystemInternal, HandleInternal) |
| ngs2_internal.h | Internal structures (VoiceInternal, RackInternal, RingBufferSlot) |
| ngs2_sampler.h | Sampler rack structures |
| ngs2_mastering.h | Mastering rack structures |
| ngs2_submixer.h | Submixer rack structures |
| ngs2_reverb.h | Reverb rack structures |
| ngs2_custom.h | Custom rack and module structures |
| ngs2_eq.h | Equalizer structures |
| ngs2_pan.h | Pan work and param structures |
| ngs2_geom.h | 3D geometry/spatial audio structures |
| ngs2_report.h | Report handler structures |
| ngs2_error.h | Error code definitions |

---

## Areas for Improvement

### High Priority
1. **ATRAC9 Decoding**: Many games use ATRAC9 compressed audio. Integration with an ATRAC9 decoder is critical for broader game compatibility.
2. **Mastering Rack Processing**: Currently stubbed - should apply gain, limiting, and LFE filtering to final output.
3. **Voice Routing (Patch)**: Voices should be able to route to submixers instead of directly to output.
4. **Streaming Buffer Gap**: Reduce/eliminate the audio gap between buffer consumption and refill. Currently there can be a brief silence when the ring buffer empties before the game refills it. Consider pre-buffering or predictive starvation signaling.

### Medium Priority
1. **Submixer Processing**: Implement actual mixing of multiple input voices with envelope and effects.
2. **Filter Implementation**: Biquad filters for EQ, lowpass, highpass processing.
3. **Envelope Processing**: ADSR envelopes for volume shaping.
4. **Better Resampling**: Current linear interpolation could be upgraded to sinc or polyphase for higher quality.

### Low Priority
1. **Reverb/Delay Effects**: Time-based audio effects for spatial depth.
2. **3D Geometry API**: Spatial positioning with distance attenuation and panning.
3. **Compressor/Limiter**: Dynamics processing for mastering.
4. **Peak Metering**: Accurate level measurement for debugging.
5. **Multi-threaded Rendering**: Job scheduler for parallel voice processing.

### Code Quality
1. **Thread Safety**: Add proper mutex locking for multi-threaded game access.
2. **Memory Management**: Consider using the game-provided buffer allocator instead of `new`.
3. **Waveform Block Repeats**: Loop handling for blocks with `numRepeats > 0`.
4. **Callback Invocation**: Actually invoke registered callbacks on buffer events.

---

## Audio Data Management

In the context of the NGS2 HLE implementation, audio data is processed primarily through the **Sampler Rack (0x1000)**. The system differentiates between short, memory-resident sounds (One-Shots) and longer, buffered content (Streaming) based on how the waveform data is supplied and managed during the `sceNgs2SystemRender` cycle.

#### One-Shot Playback
One-shots are typically used for UI sounds, sound effects, or short musical stings.

* **Memory Layout:** The entire audio payload (whether raw PCM or a complete compressed ATRAC9 block) is loaded into a contiguous block of RAM by the application before playback begins.
* **Voice Handling:** When the voice is triggered, the `VoiceInternal` structure maintains a read cursor relative to the start of this static memory region.
* **Rendering:** During the render pass, the system decodes or copies data starting from the current cursor position. If the sound is looped, the cursor simply jumps back to a defined loop-start offset upon reaching the end. Since the data is static, no synchronization with game-side file I/O is required.

#### Streamed Playback
Streaming is used for background music (BGM) or long speech tracks to conserve memory.

* **Ring Buffer Mechanism:** Instead of a linear buffer, the voice utilizes a fixed-size circular buffer (3 slots). The application (Producer) and the NGS2 renderer (Consumer) operate concurrently on this buffer.
    * **The Game's Role:** Periodically fills sections of the buffer that have already been played, ensuring the "Write Head" stays ahead of the "Read Head."
    * **NGS2's Role:** The `SystemRender` function consumes data from the "Read Head" position and signals starvation when buffers run low.
* **Starvation Handling:** When the ring buffer count drops to the threshold, the stateFlags `0x80` bit is set to signal the game to provide more data.
* **Decoding State:** For compressed formats like ATRAC9 (`0x40`), the decoder context must be preserved seamlessly across buffer transitions to prevent audio artifacts.