// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/ngs2/ngs2_error.h"
#include "core/libraries/ngs2/ngs2_impl.h"
#include "core/libraries/ngs2/ngs2_mastering.h"
#include "core/libraries/ngs2/ngs2_reverb.h"
#include "core/libraries/ngs2/ngs2_sampler.h"
#include "core/libraries/ngs2/ngs2_submixer.h"

namespace Libraries::Ngs2::Impl {
namespace {

enum class HandleType { System, Rack, Voice };

enum class RackId : u32 {
    Sampler = 0x1000,
    Submixer = 0x2000,
    Reverb = 0x2001,
    Mastering = 0x3000,
    CustomSampler = 0x4001,
    CustomSubmixer = 0x4002,
    CustomMastering = 0x4003,
};

enum class RackType : u32 { Sampler = 0, Effect = 1, Mastering = 2 };

enum class RenderWaveformType : u32 {
    PcmI16LittleEndian = 0x12,
    PcmI16BigEndian = 0x13,
    PcmF32LittleEndian = 0x18,
    PcmF32BigEndian = 0x19,
};

constexpr u32 MinGrainSamples = 64;
constexpr u32 MaxGrainSamples = 1024;
constexpr u32 FirmwareRackOptionFlags = 0xc0000000;

constexpr u32 StateFlagInUse = 1;

struct Voice {
    OrbisNgs2Handle handle{};
    u32 state{};
};

struct RackConfig {
    OrbisNgs2RackOption option{};
    RackId id{};
    RackType type{};
    u32 maxChannels{8};
    u32 maxInputs{1};
    u32 maxChannelWorks{};
    std::array<u32, 9> extra{};
};

struct System {
    std::recursive_mutex mutex;
    bool alive{true};
    OrbisNgs2Handle handle{};
    OrbisNgs2ContextBufferInfo buffer{};
    OrbisNgs2BufferFreeHandler freeHandler{};
    OrbisNgs2SystemInfo* info{};
    uintptr_t userData{};
    std::vector<OrbisNgs2Handle> racks;
};

struct Rack {
    std::shared_ptr<System> system;
    bool alive{true};
    OrbisNgs2Handle handle{};
    OrbisNgs2ContextBufferInfo buffer{};
    OrbisNgs2BufferFreeHandler freeHandler{};
    RackConfig config{};
    OrbisNgs2RackInfo* info{};
    Voice* voices{};
    uintptr_t userData{};
};

struct HandleEntry {
    HandleType type{};
    std::shared_ptr<System> system;
    std::shared_ptr<Rack> rack;
    u32 voiceIndex{};
};

std::mutex registryMutex;
std::unordered_map<OrbisNgs2Handle, HandleEntry> handles;
OrbisNgs2Handle nextHandle{1};
u32 nextUid{1};

HandleEntry FindHandle(OrbisNgs2Handle handle, HandleType type) {
    std::scoped_lock lock{registryMutex};
    const auto it = handles.find(handle);
    return it != handles.end() && it->second.type == type ? it->second : HandleEntry{};
}

struct LockedHandle {
    HandleEntry entry;
    std::unique_lock<std::recursive_mutex> lock;

    LockedHandle(OrbisNgs2Handle handle, HandleType type) : entry{FindHandle(handle, type)} {
        if (entry.system) {
            lock = std::unique_lock{entry.system->mutex};
        }
    }

    explicit operator bool() const {
        return entry.system && (entry.rack ? entry.rack->alive : entry.system->alive);
    }
};

OrbisNgs2Handle InsertHandle(HandleEntry entry) {
    std::scoped_lock lock{registryMutex};
    const auto handle = nextHandle++;
    handles.emplace(handle, std::move(entry));
    return handle;
}

u32 AllocateUid() {
    std::scoped_lock lock{registryMutex};
    return nextUid++;
}

template <typename T>
size_t StorageSize(size_t count = 1) {
    return count ? sizeof(T) * count + alignof(T) - 1 : 0;
}

class BufferArena {
public:
    explicit BufferArena(const OrbisNgs2ContextBufferInfo& buffer)
        : current{buffer.hostBuffer}, remaining{buffer.hostBufferSize} {}

    template <typename T>
    T* Allocate(size_t count = 1) {
        if (!count) {
            return nullptr;
        }

        const size_t bytes = count * sizeof(T);
        auto* allocation = static_cast<T*>(std::align(alignof(T), bytes, current, remaining));
        if (!allocation) {
            return nullptr;
        }

        current = reinterpret_cast<u8*>(allocation) + bytes;
        remaining -= bytes;
        std::uninitialized_value_construct_n(allocation, count);
        return allocation;
    }

private:
    void* current;
    size_t remaining;
};

bool ValidGrain(u32 grain) {
    return grain >= MinGrainSamples && grain <= MaxGrainSamples && grain % MinGrainSamples == 0;
}

bool ValidSampleRate(u32 rate) {
    constexpr std::array rates{11025u, 12000u, 22050u, 24000u,  44100u,
                               48000u, 88200u, 96000u, 176400u, 192000u};
    return std::ranges::find(rates, rate) != rates.end();
}

s32 ReadSystemOption(const OrbisNgs2SystemOption* option, OrbisNgs2SystemOption& result) {
    result = {sizeof(result), "", 0, 512, 256, 48000, {}};

    if (option) {
        if (option->size != sizeof(*option)) {
            return ORBIS_NGS2_ERROR_INVALID_OPTION_SIZE;
        }

        result = *option;
    }

    if (!ValidGrain(result.maxGrainSamples)) {
        return ORBIS_NGS2_ERROR_INVALID_MAX_GRAIN_SAMPLES;
    }

    if (!ValidGrain(result.numGrainSamples)) {
        return ORBIS_NGS2_ERROR_INVALID_NUM_GRAIN_SAMPLES;
    }

    if (!ValidSampleRate(result.sampleRate)) {
        return ORBIS_NGS2_ERROR_INVALID_SAMPLE_RATE;
    }

    return ORBIS_OK;
}

s32 ReadRackOption(u32 rackId, const OrbisNgs2RackOption* option, RackConfig& result) {
    result = {};
    const auto id = static_cast<RackId>(rackId);
    result.id = id;

    switch (id) {
    case RackId::Sampler:
        result.type = RackType::Sampler;
        result.option = {sizeof(OrbisNgs2RackOption), "", 0, 512, 256, 0, 1, 8, {}};
        result.extra = {256, 0, 4, 4, 8, 256, 256, 0, 8};
        break;
    case RackId::Submixer:
        result.type = RackType::Effect;
        result.option = {sizeof(OrbisNgs2RackOption), "", 0, 512, 1, 1, 1, 8, {}};
        result.extra = {8, 4, 8, 1, 8};
        break;
    case RackId::Reverb:
        result.type = RackType::Effect;
        result.option = {sizeof(OrbisNgs2RackOption), "", 0, 512, 1, 0, 1, 8, {}};
        result.extra = {8, 1};
        break;
    case RackId::Mastering:
        result.type = RackType::Mastering;
        result.option = {sizeof(OrbisNgs2RackOption), "", 0, 512, 2, 1, 0, 0, {}};
        result.extra = {8, 8};
        break;
    case RackId::CustomSampler:
    case RackId::CustomSubmixer:
    case RackId::CustomMastering:
        LOG_ERROR(Lib_Ngs2, "Custom rack creation is not implemented (id={:#x})", rackId);
        return ORBIS_NGS2_ERROR_FAIL;
    default:
        return ORBIS_NGS2_ERROR_INVALID_RACK_ID;
    }

    if (option) {
        if (option->size < sizeof(*option)) {
            return ORBIS_NGS2_ERROR_INVALID_OPTION_SIZE;
        }

        if (option->flags & ~FirmwareRackOptionFlags) {
            return ORBIS_NGS2_ERROR_INVALID_OPTION_FLAG;
        }

        const size_t extendedSize = id == RackId::Sampler    ? sizeof(OrbisNgs2SamplerRackOption)
                                    : id == RackId::Submixer ? sizeof(OrbisNgs2SubmixerRackOption)
                                    : id == RackId::Reverb   ? sizeof(OrbisNgs2ReverbRackOption)
                                                             : sizeof(OrbisNgs2MasteringRackOption);
        if (option->size != extendedSize) {
            return ORBIS_NGS2_ERROR_INVALID_OPTION_SIZE;
        }

        result.option = *option;

        const size_t count = id == RackId::Sampler ? 9 : id == RackId::Submixer ? 5 : 2;
        std::memcpy(result.extra.data(), reinterpret_cast<const u8*>(option) + sizeof(*option),
                    count * sizeof(u32));
    }

    const auto& base = result.option;
    if (id == RackId::Sampler && !option) {
        auto& maxChannelWorks = result.extra[0];
        auto& maxAtrac9Decoders = result.extra[5];
        auto& maxAtrac9ChannelWorks = result.extra[6];

        maxChannelWorks = base.maxVoices;
        maxAtrac9Decoders = base.maxVoices;
        maxAtrac9ChannelWorks = base.maxVoices;
    }

    if (!ValidGrain(base.maxGrainSamples)) {
        return ORBIS_NGS2_ERROR_INVALID_MAX_GRAIN_SAMPLES;
    }

    const u32 maxVoices = id == RackId::Mastering ? 256 : id == RackId::Reverb ? 16 : 1024;
    if (!base.maxVoices || base.maxVoices > maxVoices) {
        return ORBIS_NGS2_ERROR_INVALID_MAX_VOICES;
    }

    if ((id == RackId::Mastering || id == RackId::Submixer)
            ? (base.maxInputDelayBlocks < 1 || base.maxInputDelayBlocks > 1024)
            : base.maxInputDelayBlocks != 0) {
        return ORBIS_NGS2_ERROR_INVALID_MAX_INPUT_DELAY_BLOCKS;
    }

    if (id == RackId::Mastering ? base.maxMatrices != 0
                                : (base.maxMatrices < 1 || base.maxMatrices > 16)) {
        return ORBIS_NGS2_ERROR_INVALID_MAX_MATRICES;
    }

    if (id == RackId::Mastering ? base.maxPorts != 0 : (base.maxPorts < 1 || base.maxPorts > 16)) {
        return ORBIS_NGS2_ERROR_INVALID_MAX_PORTS;
    }

    const auto range = [](u32 value, u32 min, u32 max, s32 error) {
        return value >= min && value <= max ? ORBIS_OK : error;
    };

    const auto& extra = result.extra;
    if (id == RackId::Sampler) {
        const auto maxChannelWorks = extra[0];
        const auto maxCodecCaches = extra[1];
        const auto maxWaveformBlocks = extra[2];
        const auto maxEnvelopePoints = extra[3];
        const auto maxFilters = extra[4];
        const auto maxAtrac9Decoders = extra[5];
        const auto maxAtrac9ChannelWorks = extra[6];
        const auto maxAjmAtrac9Decoders = extra[7];
        const auto numPeakMeterBlocks = extra[8];

        result.maxInputs = 0;
        result.maxChannelWorks = maxChannelWorks;

        for (auto error :
             {range(result.maxChannelWorks, base.maxVoices, 8 * base.maxVoices,
                    ORBIS_NGS2_ERROR_INVALID_MAX_CHANNEL_WORKS),
              range(maxWaveformBlocks, 1, 16, ORBIS_NGS2_ERROR_INVALID_MAX_WAVEFORM_BLOCKS),
              range(maxEnvelopePoints, 0, 16, ORBIS_NGS2_ERROR_INVALID_MAX_ENVELOPE_POINTS),
              range(maxFilters, 0, 16, ORBIS_NGS2_ERROR_INVALID_MAX_FILTERS),
              range(maxAtrac9Decoders, 0, base.maxVoices,
                    ORBIS_NGS2_ERROR_INVALID_MAX_ATRAC9_DECODERS),
              range(maxAjmAtrac9Decoders, 0, std::min(32u, base.maxVoices),
                    ORBIS_NGS2_ERROR_INVALID_MAX_ATRAC9_DECODERS),
              range(numPeakMeterBlocks, 1, 256, ORBIS_NGS2_ERROR_INVALID_NUM_PEAKMETER_BLOCKS)}) {
            if (error < 0) {
                return error;
            }
        }

        if (maxAtrac9Decoders &&
            range(maxAtrac9ChannelWorks, maxAtrac9Decoders, 8 * maxAtrac9Decoders,
                  ORBIS_NGS2_ERROR_INVALID_MAX_ATRAC9_CHANNEL_WORKS) < 0) {
            return ORBIS_NGS2_ERROR_INVALID_MAX_ATRAC9_CHANNEL_WORKS;
        }

        if (maxAjmAtrac9Decoders && range(maxCodecCaches, 10, 8 * maxAjmAtrac9Decoders,
                                          ORBIS_NGS2_ERROR_INVALID_MAX_CODEC_CACHES) < 0) {
            return ORBIS_NGS2_ERROR_INVALID_MAX_CODEC_CACHES;
        }
    } else {
        const auto maxChannels = extra[0];
        const auto maxInputs = id == RackId::Submixer ? extra[3] : 1;

        result.maxChannels = maxChannels;
        result.maxInputs = maxInputs;
        result.maxChannelWorks = base.maxVoices * maxChannels;

        if (maxChannels < 1 || maxChannels > 8) {
            return ORBIS_NGS2_ERROR_INVALID_MAX_CHANNELS;
        }

        if (id == RackId::Reverb) {
            const auto reverbSize = extra[1];
            if (reverbSize > 1) {
                return ORBIS_NGS2_ERROR_INVALID_REVERB_SIZE;
            }
        }

        if (id == RackId::Submixer) {
            const auto maxEnvelopePoints = extra[1];
            const auto maxFilters = extra[2];

            if (maxEnvelopePoints > 16) {
                return ORBIS_NGS2_ERROR_INVALID_MAX_ENVELOPE_POINTS;
            }

            if (maxFilters > 16) {
                return ORBIS_NGS2_ERROR_INVALID_MAX_FILTERS;
            }

            if (maxInputs < 1 || maxInputs > 2) {
                return ORBIS_NGS2_ERROR_INVALID_MAX_INPUTS;
            }
        }

        if (id != RackId::Reverb) {
            const auto numPeakMeterBlocks = id == RackId::Mastering ? extra[1] : extra[4];
            if (numPeakMeterBlocks < 1 || numPeakMeterBlocks > 256) {
                return ORBIS_NGS2_ERROR_INVALID_NUM_PEAKMETER_BLOCKS;
            }
        }
    }

    return ORBIS_OK;
}

size_t SystemBufferSize(const OrbisNgs2SystemOption& option) {
    return StorageSize<OrbisNgs2SystemInfo>() + StorageSize<float>(32 * option.maxGrainSamples);
}

size_t RackBufferSize(const RackConfig& config) {
    return StorageSize<OrbisNgs2RackInfo>() + StorageSize<Voice>(config.option.maxVoices);
}

s32 ValidateBuffer(const OrbisNgs2ContextBufferInfo& info, size_t required) {
    if (!info.hostBuffer) {
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_ADDRESS;
    }

    return info.hostBufferSize < required ? ORBIS_NGS2_ERROR_INVALID_BUFFER_SIZE : ORBIS_OK;
}

void QueryBuffer(OrbisNgs2ContextBufferInfo& buffer, size_t size) {
    buffer.hostBuffer = nullptr;
    buffer.hostBufferSize = size;
    std::ranges::fill(buffer.reserved, 0);
}

struct ReleasedBuffer {
    OrbisNgs2ContextBufferInfo buffer;
    OrbisNgs2BufferFreeHandler freeHandler;

    void Release(OrbisNgs2ContextBufferInfo* outInfo = nullptr) {
        if (freeHandler) {
            freeHandler(&buffer);
        }

        if (outInfo) {
            *outInfo = buffer;
        }
    }
};

ReleasedBuffer RemoveRack(const std::shared_ptr<Rack>& rack) {
    rack->alive = false;

    {
        std::scoped_lock lock{registryMutex};
        for (u32 i = 0; i < rack->config.option.maxVoices; ++i) {
            handles.erase(rack->voices[i].handle);
        }

        handles.erase(rack->handle);
    }

    std::erase(rack->system->racks, rack->handle);
    if (rack->system->alive) {
        rack->system->info->rackCount = static_cast<u32>(rack->system->racks.size());
    }

    return {rack->buffer, rack->freeHandler};
}

s32 CreateSystem(const OrbisNgs2SystemOption& option, const OrbisNgs2ContextBufferInfo& buffer,
                 OrbisNgs2BufferFreeHandler freeHandler, OrbisNgs2Handle* outHandle) try {
    const auto result = ValidateBuffer(buffer, SystemBufferSize(option));
    if (result < 0) {
        return result;
    }

    auto system = std::make_shared<System>();
    std::scoped_lock lock{system->mutex};

    BufferArena arena{buffer};
    system->info = arena.Allocate<OrbisNgs2SystemInfo>();
    arena.Allocate<float>(32 * option.maxGrainSamples);
    system->buffer = buffer;
    system->freeHandler = freeHandler;

    auto& info = *system->info;
    std::copy_n(option.name, sizeof(info.name), info.name);
    info.name[sizeof(info.name) - 1] = 0;
    info.bufferInfo = buffer;
    info.uid = AllocateUid();
    info.stateFlags = StateFlagInUse;
    info.minGrainSamples = MinGrainSamples;
    info.maxGrainSamples = option.maxGrainSamples;
    info.sampleRate = option.sampleRate;
    info.numGrainSamples = option.numGrainSamples;

    system->handle = InsertHandle({HandleType::System, system});
    info.systemHandle = system->handle;
    *outHandle = system->handle;
    return ORBIS_OK;
} catch (const std::bad_alloc&) {
    return ORBIS_NGS2_ERROR_EMPTY_BUFFER;
}

s32 CreateRack(const std::shared_ptr<System>& system, const RackConfig& config,
               const OrbisNgs2ContextBufferInfo& buffer, OrbisNgs2BufferFreeHandler freeHandler,
               OrbisNgs2Handle* outHandle) {
    if (!system->alive) {
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    const auto result = ValidateBuffer(buffer, RackBufferSize(config));
    if (result < 0) {
        return result;
    }

    std::shared_ptr<Rack> rack;
    try {
        rack = std::make_shared<Rack>();
        rack->system = system;
        rack->buffer = buffer;
        rack->config = config;
        rack->freeHandler = freeHandler;

        BufferArena arena{buffer};
        rack->info = arena.Allocate<OrbisNgs2RackInfo>();
        rack->voices = arena.Allocate<Voice>(config.option.maxVoices);

        auto& info = *rack->info;
        std::copy_n(config.option.name, sizeof(info.name), info.name);
        info.name[sizeof(info.name) - 1] = 0;
        info.bufferInfo = buffer;
        info.ownerSystemHandle = system->handle;
        info.type = static_cast<u32>(config.type);
        info.rackId = static_cast<u32>(config.id);
        info.uid = AllocateUid();
        info.stateFlags = StateFlagInUse;
        info.minGrainSamples = MinGrainSamples;
        info.maxGrainSamples = config.option.maxGrainSamples;
        info.maxVoices = config.option.maxVoices;
        info.maxChannelWorks = config.maxChannelWorks;
        info.maxInputs = config.maxInputs;
        info.maxMatrices = config.option.maxMatrices;
        info.maxPorts = config.option.maxPorts;

        rack->handle = InsertHandle({HandleType::Rack, system, rack});
        info.rackHandle = rack->handle;
        for (u32 i = 0; i < config.option.maxVoices; ++i) {
            rack->voices[i].handle = InsertHandle({HandleType::Voice, system, rack, i});
        }

        system->racks.push_back(rack->handle);
        system->info->rackCount = static_cast<u32>(system->racks.size());
        *outHandle = rack->handle;
        return ORBIS_OK;
    } catch (const std::bad_alloc&) {
        if (rack && rack->handle) {
            std::scoped_lock lock{registryMutex};
            for (u32 i = 0; i < config.option.maxVoices; ++i) {
                handles.erase(rack->voices[i].handle);
            }

            handles.erase(rack->handle);
        }

        return ORBIS_NGS2_ERROR_EMPTY_BUFFER;
    }
}

} // namespace

s32 SystemQueryBufferSize(const OrbisNgs2SystemOption* option,
                          OrbisNgs2ContextBufferInfo* outBufferInfo) {
    if (!outBufferInfo) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    OrbisNgs2SystemOption normalized;
    const auto result = ReadSystemOption(option, normalized);
    if (result >= 0) {
        QueryBuffer(*outBufferInfo, SystemBufferSize(normalized));
    }

    return result;
}

s32 SystemCreate(const OrbisNgs2SystemOption* option, const OrbisNgs2ContextBufferInfo* bufferInfo,
                 OrbisNgs2Handle* outHandle) {
    if (!bufferInfo) {
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_INFO;
    }

    if (!outHandle) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    OrbisNgs2SystemOption normalized;
    const auto result = ReadSystemOption(option, normalized);
    return result < 0 ? result : CreateSystem(normalized, *bufferInfo, nullptr, outHandle);
}

s32 SystemCreateWithAllocator(const OrbisNgs2SystemOption* option,
                              const OrbisNgs2BufferAllocator* allocator,
                              OrbisNgs2Handle* outHandle) {
    if (!allocator || !allocator->allocHandler) {
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_ALLOCATOR;
    }

    if (!outHandle) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    const auto callbacks = *allocator;
    OrbisNgs2SystemOption normalized;
    auto result = ReadSystemOption(option, normalized);
    if (result < 0) {
        return result;
    }

    OrbisNgs2ContextBufferInfo buffer{};
    QueryBuffer(buffer, SystemBufferSize(normalized));
    buffer.userData = callbacks.userData;
    result = callbacks.allocHandler(&buffer);
    if (result < 0) {
        return result;
    }

    result = ReadSystemOption(option, normalized);
    if (result >= 0) {
        result = CreateSystem(normalized, buffer, callbacks.freeHandler, outHandle);
    }

    if (result < 0 && callbacks.freeHandler) {
        callbacks.freeHandler(&buffer);
    }

    return result;
}

s32 SystemDestroy(OrbisNgs2Handle systemHandle, OrbisNgs2ContextBufferInfo* outBufferInfo) {
    auto object = LockedHandle{systemHandle, HandleType::System};
    if (!object) {
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    auto system = object.entry.system;

    // Firmware detaches racks here but their handles and buffers survive until
    // RackDestroy.
    for (const auto rackHandle : system->racks) {
        auto entry = FindHandle(rackHandle, HandleType::Rack);
        entry.rack->info->ownerSystemHandle = 0;
    }

    system->racks.clear();
    system->info->rackCount = 0;
    system->alive = false;

    {
        std::scoped_lock lock{registryMutex};
        handles.erase(systemHandle);
    }

    ReleasedBuffer released{system->buffer, system->freeHandler};
    object.lock.unlock();
    released.Release(outBufferInfo);
    return ORBIS_OK;
}

s32 RackQueryBufferSize(u32 rackId, const OrbisNgs2RackOption* option,
                        OrbisNgs2ContextBufferInfo* outBufferInfo) {
    RackConfig config;
    const auto result = ReadRackOption(rackId, option, config);
    if (result < 0) {
        return result;
    }

    if (!outBufferInfo) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    QueryBuffer(*outBufferInfo, RackBufferSize(config));
    return ORBIS_OK;
}

s32 RackCreate(OrbisNgs2Handle systemHandle, u32 rackId, const OrbisNgs2RackOption* option,
               const OrbisNgs2ContextBufferInfo* bufferInfo, OrbisNgs2Handle* outHandle) {
    RackConfig config;
    auto result = ReadRackOption(rackId, option, config);
    if (result < 0) {
        return result;
    }

    if (!bufferInfo) {
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_INFO;
    }

    if (!outHandle) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    auto system = LockedHandle{systemHandle, HandleType::System};
    if (!system) {
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    return CreateRack(system.entry.system, config, *bufferInfo, nullptr, outHandle);
}

s32 RackCreateWithAllocator(OrbisNgs2Handle systemHandle, u32 rackId,
                            const OrbisNgs2RackOption* option,
                            const OrbisNgs2BufferAllocator* allocator, OrbisNgs2Handle* outHandle) {
    RackConfig config;
    auto result = ReadRackOption(rackId, option, config);
    if (result < 0) {
        return result;
    }

    if (!allocator || !allocator->allocHandler) {
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_ALLOCATOR;
    }

    if (!outHandle) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    const auto callbacks = *allocator;
    OrbisNgs2ContextBufferInfo buffer{};
    QueryBuffer(buffer, RackBufferSize(config));
    buffer.userData = callbacks.userData;
    result = callbacks.allocHandler(&buffer);
    if (result < 0) {
        return result;
    }

    result = ReadRackOption(rackId, option, config);
    if (result >= 0) {
        auto system = LockedHandle{systemHandle, HandleType::System};
        result = system ? CreateRack(system.entry.system, config, buffer, callbacks.freeHandler,
                                     outHandle)
                        : ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    if (result < 0 && callbacks.freeHandler) {
        callbacks.freeHandler(&buffer);
    }

    return result;
}

s32 RackDestroy(OrbisNgs2Handle rackHandle, OrbisNgs2ContextBufferInfo* outBufferInfo) {
    auto rack = LockedHandle{rackHandle, HandleType::Rack};
    if (!rack) {
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    }

    auto released = RemoveRack(rack.entry.rack);
    rack.lock.unlock();
    released.Release(outBufferInfo);
    return ORBIS_OK;
}

s32 RackGetVoiceHandle(OrbisNgs2Handle rackHandle, u32 voiceIndex, OrbisNgs2Handle* outHandle) {
    if (!outHandle) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    auto rack = LockedHandle{rackHandle, HandleType::Rack};
    if (!rack) {
        *outHandle = 0;
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    }

    if (voiceIndex >= rack.entry.rack->config.option.maxVoices) {
        *outHandle = 0;
        return ORBIS_NGS2_ERROR_INVALID_VOICE_INDEX;
    }

    *outHandle = rack.entry.rack->voices[voiceIndex].handle;
    return ORBIS_OK;
}

s32 SystemEnumHandles(OrbisNgs2Handle* aOutHandle, u32 maxHandles) {
    if (!aOutHandle && maxHandles) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    std::scoped_lock lock{registryMutex};
    u32 count{};
    for (const auto& [handle, entry] : handles) {
        if (entry.type == HandleType::System) {
            if (count < maxHandles) {
                aOutHandle[count] = handle;
            }

            ++count;
        }
    }

    return count;
}

s32 SystemEnumRackHandles(OrbisNgs2Handle systemHandle, OrbisNgs2Handle* aOutHandle,
                          u32 maxHandles) {
    if (!aOutHandle && maxHandles) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    auto system = LockedHandle{systemHandle, HandleType::System};
    if (!system) {
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    const auto& racks = system.entry.system->racks;
    if (maxHandles) {
        std::copy_n(racks.begin(), std::min<size_t>(racks.size(), maxHandles), aOutHandle);
    }

    return static_cast<s32>(racks.size());
}

s32 SystemGetInfo(OrbisNgs2Handle systemHandle, OrbisNgs2SystemInfo* outInfo, size_t infoSize) {
    if (!outInfo) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    if (infoSize < sizeof(*outInfo)) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_SIZE;
    }

    auto system = LockedHandle{systemHandle, HandleType::System};
    if (!system) {
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    *outInfo = *system.entry.system->info;
    return ORBIS_OK;
}

s32 RackGetInfo(OrbisNgs2Handle rackHandle, OrbisNgs2RackInfo* outInfo, size_t infoSize) {
    if (!outInfo) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    if (infoSize < sizeof(*outInfo)) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_SIZE;
    }

    auto rack = LockedHandle{rackHandle, HandleType::Rack};
    if (!rack) {
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    }

    const auto& context = *rack.entry.rack;
    std::array<u8, sizeof(OrbisNgs2SamplerRackInfo)> info{};
    std::memcpy(info.data(), context.info, sizeof(*outInfo));

    auto extra = context.config.extra;
    size_t extraCount{};
    switch (context.config.id) {
    case RackId::Sampler:
        extra[0] = context.config.maxChannelWorks;
        extra[1] = extra[7] ? extra[1] : 0;
        extra[6] = extra[5] ? extra[6] : 0;
        extraCount = 8;
        break;
    case RackId::Submixer:
        extraCount = 4;
        break;
    case RackId::Reverb:
        extraCount = 2;
        break;
    case RackId::Mastering:
        extra[1] = 0;
        extraCount = 2;
        break;
    default:
        break;
    }

    std::memcpy(info.data() + sizeof(*outInfo), extra.data(), extraCount * sizeof(u32));
    std::memcpy(outInfo, info.data(),
                std::min(infoSize, sizeof(*outInfo) + extraCount * sizeof(u32)));
    return ORBIS_OK;
}

s32 SystemGetUserData(OrbisNgs2Handle systemHandle, uintptr_t* outUserData) {
    if (!outUserData) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    auto system = LockedHandle{systemHandle, HandleType::System};
    *outUserData = system ? system.entry.system->userData : 0;
    return system ? ORBIS_OK : ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
}

s32 RackGetUserData(OrbisNgs2Handle rackHandle, uintptr_t* outUserData) {
    if (!outUserData) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    auto rack = LockedHandle{rackHandle, HandleType::Rack};
    *outUserData = rack ? rack.entry.rack->userData : 0;
    return rack ? ORBIS_OK : ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
}

s32 SystemSetUserData(OrbisNgs2Handle systemHandle, uintptr_t userData) {
    auto system = LockedHandle{systemHandle, HandleType::System};
    if (!system) {
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    system.entry.system->userData = userData;
    return ORBIS_OK;
}

s32 RackSetUserData(OrbisNgs2Handle rackHandle, uintptr_t userData) {
    auto rack = LockedHandle{rackHandle, HandleType::Rack};
    if (!rack) {
        return ORBIS_NGS2_ERROR_INVALID_RACK_HANDLE;
    }

    rack.entry.rack->userData = userData;
    return ORBIS_OK;
}

s32 SystemSetGrainSamples(OrbisNgs2Handle systemHandle, u32 numSamples) {
    auto system = LockedHandle{systemHandle, HandleType::System};
    if (!system) {
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    if (!ValidGrain(numSamples) || numSamples > system.entry.system->info->maxGrainSamples) {
        return ORBIS_NGS2_ERROR_INVALID_NUM_GRAIN_SAMPLES;
    }

    system.entry.system->info->numGrainSamples = numSamples;
    return ORBIS_OK;
}

s32 SystemSetSampleRate(OrbisNgs2Handle systemHandle, u32 sampleRate) {
    auto system = LockedHandle{systemHandle, HandleType::System};
    if (!system) {
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    if (!ValidSampleRate(sampleRate)) {
        return ORBIS_NGS2_ERROR_INVALID_SAMPLE_RATE;
    }

    system.entry.system->info->sampleRate = sampleRate;
    return ORBIS_OK;
}

s32 VoiceGetOwner(OrbisNgs2Handle voiceHandle, OrbisNgs2Handle* outRackHandle, u32* outVoiceId) {
    auto voice = LockedHandle{voiceHandle, HandleType::Voice};
    if (!voice) {
        return ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
    }

    if (outRackHandle) {
        *outRackHandle = voice.entry.rack->handle;
    }

    if (outVoiceId) {
        *outVoiceId = voice.entry.voiceIndex;
    }

    return ORBIS_OK;
}

s32 VoiceGetStateFlags(OrbisNgs2Handle voiceHandle, u32* outStateFlags) {
    if (!outStateFlags) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    auto voice = LockedHandle{voiceHandle, HandleType::Voice};
    *outStateFlags = voice ? voice.entry.rack->voices[voice.entry.voiceIndex].state : 0;
    return voice ? ORBIS_OK : ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
}

s32 VoiceGetState(OrbisNgs2Handle voiceHandle, OrbisNgs2VoiceState* outState, size_t stateSize) {
    if (!outState) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_ADDRESS;
    }

    if (stateSize < sizeof(*outState)) {
        return ORBIS_NGS2_ERROR_INVALID_OUT_SIZE;
    }

    auto voice = LockedHandle{voiceHandle, HandleType::Voice};
    if (!voice) {
        return ORBIS_NGS2_ERROR_INVALID_VOICE_HANDLE;
    }

    const auto id = voice.entry.rack->config.id;
    const size_t fullSize = id == RackId::Sampler    ? sizeof(OrbisNgs2SamplerVoiceState)
                            : id == RackId::Submixer ? sizeof(OrbisNgs2SubmixerVoiceState)
                            : id == RackId::Reverb   ? sizeof(OrbisNgs2ReverbVoiceState)
                                                     : sizeof(OrbisNgs2MasteringVoiceState);

    // Firmware accepts intermediate sizes without writing any part of the state.
    if (stateSize == sizeof(*outState) || stateSize >= fullSize) {
        std::memset(outState, 0, stateSize == sizeof(*outState) ? sizeof(*outState) : fullSize);
        outState->stateFlags = voice.entry.rack->voices[voice.entry.voiceIndex].state;
    }

    return ORBIS_OK;
}

s32 SystemRender(OrbisNgs2Handle systemHandle, const OrbisNgs2RenderBufferInfo* aBufferInfo,
                 u32 numBufferInfo) {
    auto system = LockedHandle{systemHandle, HandleType::System};
    if (!system) {
        return ORBIS_NGS2_ERROR_INVALID_SYSTEM_HANDLE;
    }

    if (numBufferInfo > 16 || !numBufferInfo || !aBufferInfo) {
        return ORBIS_NGS2_ERROR_INVALID_BUFFER_ADDRESS;
    }

    const auto buffers = std::span{aBufferInfo, numBufferInfo};
    const size_t grain = system.entry.system->info->numGrainSamples;
    s32 result = ORBIS_OK;

    for (const auto& buffer : buffers) {
        if (buffer.numChannels != 1 && buffer.numChannels != 2 && buffer.numChannels != 6 &&
            buffer.numChannels != 8) {
            result = ORBIS_NGS2_ERROR_INVALID_NUM_CHANNELS;
            break;
        }

        const auto type = static_cast<RenderWaveformType>(buffer.waveformType);
        if (type != RenderWaveformType::PcmI16LittleEndian &&
            type != RenderWaveformType::PcmI16BigEndian &&
            type != RenderWaveformType::PcmF32LittleEndian &&
            type != RenderWaveformType::PcmF32BigEndian) {
            result = ORBIS_NGS2_ERROR_INVALID_WAVEFORM_TYPE;
            break;
        }

        if (!buffer.buffer) {
            result = ORBIS_NGS2_ERROR_INVALID_BUFFER_ADDRESS;
            break;
        }

        const size_t bytes = type == RenderWaveformType::PcmI16LittleEndian ||
                                     type == RenderWaveformType::PcmI16BigEndian
                                 ? 2
                                 : 4;
        const size_t required = bytes * buffer.numChannels * grain;
        if (buffer.bufferSize < required) {
            result = ORBIS_NGS2_ERROR_INVALID_BUFFER_SIZE;
            break;
        }
    }

    for (const auto& buffer : buffers) {
        if (buffer.buffer) {
            const auto type = static_cast<RenderWaveformType>(buffer.waveformType);
            const size_t bytes = type == RenderWaveformType::PcmI16LittleEndian ||
                                         type == RenderWaveformType::PcmI16BigEndian
                                     ? 2
                                     : 4;
            std::memset(buffer.buffer, 0,
                        result < 0 ? buffer.bufferSize : bytes * buffer.numChannels * grain);
        }
    }

    if (result >= 0) {
        ++system.entry.system->info->renderCount;
    }

    return result;
}

} // namespace Libraries::Ngs2::Impl
