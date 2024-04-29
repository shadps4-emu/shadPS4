// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstring>
#include "common/bit_field.h"
#include "common/types.h"
#include "video_core/amdgpu/pm4_opcodes.h"

namespace AmdGpu {

/// This enum defines the Shader types supported in PM4 type 3 header
enum class PM4ShaderType : u32 {
    ShaderGraphics = 0, ///< Graphics shader
    ShaderCompute = 1   ///< Compute shader
};

/// This enum defines the predicate value supported in PM4 type 3 header
enum class PM4Predicate : u32 {
    PredDisable = 0, ///< Predicate disabled
    PredEnable = 1   ///< Predicate enabled
};

union PM4Type0Header {
    u32 raw;
    BitField<0, 16, u32> base;   ///< DWORD Memory-mapped address
    BitField<16, 14, u32> count; ///< Count of DWORDs in the *information* body (N - 1 for N dwords)
    BitField<30, 2, u32> type;   ///< Packet identifier. It should be 0 for type 0 packets.

    u32 NumWords() const {
        return count + 1;
    }
};

union PM4Type3Header {
    constexpr PM4Type3Header(PM4ItOpcode code, u32 num_words_min_one,
                             PM4ShaderType stype = PM4ShaderType::ShaderGraphics,
                             PM4Predicate pred = PM4Predicate::PredDisable) {
        raw = 0;
        predicate.Assign(pred);
        shaderType.Assign(stype);
        opcode.Assign(code);
        count.Assign(num_words_min_one);
        type.Assign(3);
    }

    u32 NumWords() const {
        return count + 1;
    }

    u32 raw;
    BitField<0, 1, PM4Predicate> predicate;   ///< Predicated version of packet when set
    BitField<1, 1, PM4ShaderType> shaderType; ///< 0: Graphics, 1: Compute Shader
    BitField<8, 8, PM4ItOpcode> opcode;       ///< IT opcode
    BitField<16, 14, u32> count;              ///< Number of DWORDs - 1 in the information body.
    BitField<30, 2, u32> type; ///< Packet identifier. It should be 3 for type 3 packets
};

union PM4Header {
    u32 raw;
    PM4Type0Header type0;
    PM4Type3Header type3;
    BitField<30, 2, u32> type;
};

template <PM4ItOpcode opcode, typename... Args>
constexpr u32* Write(u32* cmdbuf, PM4ShaderType type, Args... data) {
    // Write the PM4 header.
    PM4Type3Header header{opcode, sizeof...(Args) - 1, type};
    std::memcpy(cmdbuf, &header, sizeof(header));

    // Write arguments
    const std::array<u32, sizeof...(Args)> args{data...};
    std::memcpy(++cmdbuf, args.data(), sizeof(args));
    cmdbuf += args.size();
    return cmdbuf;
}

union ContextControlEnable {
    u32 raw;
    BitField<0, 1, u32> enableSingleCntxConfigReg; ///< single context config reg
    BitField<1, 1, u32> enableMultiCntxRenderReg;  ///< multi context render state reg
    BitField<15, 1, u32> enableUserConfigReg__CI;  ///< User Config Reg on CI(reserved for SI)
    BitField<16, 1, u32> enableGfxSHReg;           ///< Gfx SH Registers
    BitField<24, 1, u32> enableCSSHReg;            ///< CS SH Registers
    BitField<31, 1, u32> enableDw;                 ///< DW enable
};

struct PM4CmdContextControl {
    PM4Type3Header header;
    ContextControlEnable loadControl;  ///< Enable bits for loading
    ContextControlEnable shadowEnable; ///< Enable bits for shadowing
};

union LoadAddressHigh {
    u32 raw;
    BitField<0, 16, u32>
        addrHi; ///< bits for the block in Memory from where the CP will fetch the state
    BitField<31, 1, u32>
        waitIdle; ///< if set the CP will wait for the graphics pipe to be idle by writing
                  ///< to the GRBM Wait Until register with "Wait for 3D idle"
};

/**
 * PM4CMDLOADDATA can be used with the following opcodes
 * - IT_LOAD_CONFIG_REG
 * - IT_LOAD_CONTEXT_REG
 * - IT_LOAD_SH_REG
 */
struct PM4CmdLoadData {
    PM4Type3Header header;
    u32 addrLo; ///< low 32 address bits for the block in memory from where the CP will fetch the
                ///< state
    LoadAddressHigh addrHi;
    u32 regOffset; ///< offset in DWords from the register base address
    u32 numDwords; ///< number of DWords that the CP will fetch and write into the chip. A value of
                   ///< zero will fetch nothing
};

enum class LoadDataIndex : u32 {
    DirectAddress = 0, /// ADDR_LO is direct address
    Offset = 1,        /// ARRD_LO is ignored and memory offset is in addrOffset
};

enum class LoadDataFormat : u32 {
    OffsetAndSize = 0, /// Data is consecutive DWORDs
    OffsetAndData = 1, /// Register offset and data is interleaved
};

union LoadAddressLow {
    u32 raw;
    BitField<0, 1, LoadDataIndex> index;
    BitField<2, 30, u32> addrLo; ///< bits for the block in Memory from where the CP will fetch the
                                 ///< state. DWORD aligned
};

/**
 * PM4CMDLOADDATAINDEX can be used with the following opcodes (VI+)
 * - IT_LOAD_CONTEXT_REG_INDEX
 * - IT_LOAD_SH_REG_INDEX
 */
struct PM4CmdLoadDataIndex {
    PM4Type3Header header;
    LoadAddressLow addrLo; ///< low 32 address bits for the block in memory from where the CP will
                           ///< fetch the state
    u32 addrOffset;        ///< addrLo.index = 1 Indexed mode
    union {
        BitField<0, 16, u32> regOffset; ///< offset in DWords from the register base address
        BitField<31, 1, LoadDataFormat> dataFormat;
        u32 raw;
    };
    u32 numDwords; ///< Number of DWords that the CP will fetch and write
                   ///< into the chip. A value of zero will fetch nothing
};

/**
 * PM4CMDSETDATA can be used with the following opcodes:
 *
 * - IT_SET_CONFIG_REG
 * - IT_SET_CONTEXT_REG
 * - IT_SET_CONTEXT_REG_INDIRECT
 * - IT_SET_SH_REG
 * - IT_SET_SH_REG_INDEX
 * - IT_SET_UCONFIG_REG
 */
struct PM4CmdSetData {
    PM4Type3Header header;
    union {
        u32 raw;
        BitField<0, 16, u32> regOffset; ///< Offset in DWords from the register base address
        BitField<28, 4, u32> index;     ///< Index for UCONFIG/CONTEXT on CI+
                                        ///< Program to zero for other opcodes and on SI
    };

    template <PM4ShaderType type = PM4ShaderType::ShaderGraphics, typename... Args>
    static constexpr u32* SetContextReg(u32* cmdbuf, Args... data) {
        return Write<PM4ItOpcode::SetContextReg>(cmdbuf, type, data...);
    }

    template <PM4ShaderType type = PM4ShaderType::ShaderGraphics, typename... Args>
    static constexpr u32* SetShReg(u32* cmdbuf, Args... data) {
        return Write<PM4ItOpcode::SetShReg>(cmdbuf, type, data...);
    }
};

struct PM4CmdNop {
    PM4Type3Header header;
};

struct PM4CmdDrawIndexOffset2 {
    PM4Type3Header header;
    u32 maxSize;       ///< Maximum number of indices
    u32 indexOffset;   ///< Zero based starting index number in the index buffer
    u32 indexCount;    ///< number of indices in the Index Buffer
    u32 drawInitiator; ///< draw Initiator Register
};

struct PM4CmdDrawIndex2 {
    PM4Type3Header header;
    u32 maxSize;       ///< maximum number of indices
    u32 indexBaseLo;   ///< base Address Lo [31:1] of Index Buffer
                       ///< (Word-Aligned). Written to the VGT_DMA_BASE register.
    u32 indexBaseHi;   ///< base Address Hi [39:32] of Index Buffer.
                       ///< Written to the VGT_DMA_BASE_HI register
    u32 indexCount;    ///< number of indices in the Index Buffer.
                       ///< Written to the VGT_NUM_INDICES register.
    u32 drawInitiator; ///< written to the VGT_DRAW_INITIATOR register
};

struct PM4CmdDrawIndexType {
    PM4Type3Header header;
    union {
        u32 raw;
        BitField<0, 2, u32> indexType; ///< Select 16 Vs 32bit index
        BitField<2, 2, u32> swapMode;  ///< DMA swap mode
    };
};

struct PM4CmdDrawIndexAuto {
    PM4Type3Header header;
    u32 index_count;
    u32 draw_initiator;
};

enum class DataSelect : u32 {
    None = 0,
    Data32Low = 1,
    Data64 = 2,
    GpuClock64 = 3,
    PerfCounter = 4,
};

enum class InterruptSelect : u32 {
    None = 0,
    IrqOnly = 1,
    IrqWhenWriteConfirm = 2,
};

struct PM4CmdEventWriteEop {
    PM4Type3Header header;
    union {
        u32 event_control;
        BitField<0, 6, u32> eventType;  ///< Event type written to VGT_EVENT_INITIATOR
        BitField<8, 4, u32> eventIndex; ///< Event index
    };
    u32 addressLo;
    union {
        u32 data_control;
        BitField<0, 16, u32> addressHi;          ///< High bits of address
        BitField<24, 2, InterruptSelect> intSel; ///< Selects interrupt action for end-of-pipe
        BitField<29, 3, DataSelect> dataSel;     ///< Selects source of data
    };
    u32 dataLo; ///< Value that will be written to memory when event occurs
    u32 dataHi; ///< Value that will be written to memory when event occurs

    u64* Address() const {
        return reinterpret_cast<u64*>(addressLo | u64(addressHi) << 32);
    }

    u64 DataQWord() const {
        return dataLo | u64(dataHi) << 32;
    }
};

struct PM4DmaData {
    PM4Type3Header header;
    union {
        BitField<0, 1, u32> engine;
        BitField<12, 1, u32> src_atc;
        BitField<13, 2, u32> src_cache_policy;
        BitField<15, 1, u32> src_volatile;
        BitField<20, 2, u32> dst_sel;
        BitField<24, 1, u32> dst_atc;
        BitField<25, 2, u32> dst_cache_policy;
        BitField<27, 1, u32> dst_volatile;
        BitField<29, 2, u32> src_sel;
        BitField<31, 1, u32> cp_sync;
    };
    union {
        u32 src_addr_lo;
        u32 data;
    };
    u32 src_addr_hi;
    u32 dst_addr_lo;
    u32 dst_addr_hi;
    u32 command;
};

} // namespace AmdGpu
