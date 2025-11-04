// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/types.h"

namespace AmdGpu {

enum class PrimitiveType : u32 {
    None = 0,
    PointList = 1,
    LineList = 2,
    LineStrip = 3,
    TriangleList = 4,
    TriangleFan = 5,
    TriangleStrip = 6,
    PatchPrimitive = 9,
    AdjLineList = 10,
    AdjLineStrip = 11,
    AdjTriangleList = 12,
    AdjTriangleStrip = 13,
    RectList = 17,
    LineLoop = 18,
    QuadList = 19,
    QuadStrip = 20,
    Polygon = 21,
};

struct IndexBufferBase {
    u32 base_addr_hi : 8;
    u32 base_addr_lo;

    template <typename T = VAddr>
    T Address() const {
        return std::bit_cast<T>((base_addr_lo & ~1U) | u64(base_addr_hi) << 32);
    }
};

enum class IndexType : u32 {
    Index16 = 0,
    Index32 = 1,
};

enum class IndexSwapMode : u32 {
    None = 0,
    Swap16 = 1,
    Swap32 = 2,
    SwapWord = 3,
};

union IndexBufferType {
    u32 raw;
    struct {
        IndexType index_type : 2;
        IndexSwapMode swap_mode : 2;
    };
};

struct VgtNumInstances {
    u32 num_instances;

    u32 NumInstances() const {
        return num_instances == 0 ? 1 : num_instances;
    }
};

struct PolygonOffset {
    float depth_bias;
    float front_scale;
    float front_offset;
    float back_scale;
    float back_offset;
};

struct Address {
    u32 address;

    VAddr GetAddress() const {
        return u64(address) << 8;
    }
};

union ShaderStageEnable {
    enum VgtStages : u32 {
        Vs = 0u, // always enabled
        EsGs = 0xB0u,
        LsHs = 0x45u,
    };

    VgtStages raw;
    struct {
        u32 ls_en : 2;
        u32 hs_en : 1;
        u32 es_en : 2;
        u32 gs_en : 1;
        u32 vs_en : 2;
        u32 dynamic_hs : 1;
    };

    bool IsStageEnabled(u32 stage) const {
        switch (stage) {
        case 0:
        case 1:
            return true;
        case 2:
            return gs_en;
        case 3:
            return es_en;
        case 4:
            return hs_en;
        case 5:
            return ls_en;
        default:
            UNREACHABLE();
        }
    }
};

union GsInstances {
    u32 raw;
    struct {
        u32 enable : 2;
        u32 count : 6;
    };

    bool IsEnabled() const {
        return enable && count > 0;
    }
};

enum class GsOutputPrimitiveType : u32 {
    PointList = 0,
    LineStrip = 1,
    TriangleStrip = 2,
};

union GsOutPrimitiveType {
    u32 raw;
    struct {
        GsOutputPrimitiveType outprim_type : 6;
        GsOutputPrimitiveType outprim_type1 : 6;
        GsOutputPrimitiveType outprim_type2 : 6;
        GsOutputPrimitiveType outprim_type3 : 6;
        u32 reserved : 3;
        u32 unique_type_per_stream : 1;
    };

    GsOutputPrimitiveType GetPrimitiveType(u32 stream) const {
        if (unique_type_per_stream == 0) {
            return outprim_type;
        }

        switch (stream) {
        case 0:
            return outprim_type;
        case 1:
            return outprim_type1;
        case 2:
            return outprim_type2;
        case 3:
            return outprim_type3;
        default:
            UNREACHABLE();
        }
    }
};

enum class GsScenario : u32 {
    Off = 0,
    ScenarioA = 1,
    ScenarioB = 2,
    ScenarioG = 3,
    ScenarioC = 4,
};

struct GsMode {
    GsScenario mode : 3;
    u32 cut_mode : 2;
    u32 : 17;
    u32 onchip : 2;
};

struct StreamOutControl {
    u32 offset_update_done : 1;
    u32 : 31;
};

union StreamOutConfig {
    u32 raw;
    struct {
        u32 streamout_0_en : 1;
        u32 streamout_1_en : 1;
        u32 streamout_2_en : 1;
        u32 streamout_3_en : 1;
        u32 rast_stream : 3;
        u32 : 1;
        u32 rast_stream_mask : 4;
        u32 : 19;
        u32 use_rast_stream_mask : 1;
    };
};

struct StreamOutBufferConfig {
    u32 stream_0_buf_en : 4;
    u32 stream_1_buf_en : 4;
    u32 stream_2_buf_en : 4;
    u32 stream_3_buf_en : 4;
};

struct LsHsConfig {
    u32 num_patches : 8;
    u32 hs_input_control_points : 6;
    u32 hs_output_control_points : 6;
};

enum class TessellationType : u32 {
    Isoline = 0,
    Triangle = 1,
    Quad = 2,
};

enum class TessellationPartitioning : u32 {
    Integer = 0,
    Pow2 = 1,
    FracOdd = 2,
    FracEven = 3,
};

enum class TessellationTopology : u32 {
    Point = 0,
    Line = 1,
    TriangleCw = 2,
    TriangleCcw = 3,
};

struct TessellationConfig {
    TessellationType type : 2;
    TessellationPartitioning partitioning : 3;
    TessellationTopology topology : 3;
};

struct TessFactorMemoryBase {
    u32 base;

    u64 MemoryBase() const {
        return static_cast<u64>(base) << 8;
    }
};

struct TessFactorClamp {
    float hs_max_tess;
    float hs_min_tess;
};

} // namespace AmdGpu
