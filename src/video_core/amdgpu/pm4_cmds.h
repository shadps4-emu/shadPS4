// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstring>
#include "common/bit_field.h"
#include "common/rdtsc.h"
#include "common/types.h"
#include "core/platform.h"
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
    static constexpr u32 TYPE = 3;

    constexpr PM4Type3Header(PM4ItOpcode code, u32 num_words_min_one,
                             PM4ShaderType stype = PM4ShaderType::ShaderGraphics,
                             PM4Predicate pred = PM4Predicate::PredDisable) {
        raw = 0;
        predicate.Assign(pred);
        shader_type.Assign(stype);
        opcode.Assign(code);
        count.Assign(num_words_min_one);
        type.Assign(3);
    }

    u32 NumWords() const {
        return count + 1;
    }

    u32 raw;
    BitField<0, 1, PM4Predicate> predicate;    ///< Predicated version of packet when set
    BitField<1, 1, PM4ShaderType> shader_type; ///< 0: Graphics, 1: Compute Shader
    BitField<8, 8, PM4ItOpcode> opcode;        ///< IT opcode
    BitField<16, 14, u32> count;               ///< Number of DWORDs - 1 in the information body.
    BitField<30, 2, u32> type; ///< Packet identifier. It should be 3 for type 3 packets
};

union PM4Header {
    u32 raw;
    PM4Type0Header type0;
    PM4Type3Header type3;
    BitField<30, 2, u32> type;
};

// Write the PM4 header
template <PM4ItOpcode opcode>
constexpr u32* WriteHeader(u32* cmdbuf, u32 size,
                           PM4ShaderType type = PM4ShaderType::ShaderGraphics,
                           PM4Predicate predicate = PM4Predicate::PredDisable) {
    PM4Type3Header header{opcode, size - 1, type, predicate};
    std::memcpy(cmdbuf, &header, sizeof(header));
    return ++cmdbuf;
}

// Write arguments
template <typename... Args>
constexpr u32* WriteBody(u32* cmdbuf, Args... data) {
    const std::array<u32, sizeof...(Args)> args{data...};
    std::memcpy(cmdbuf, args.data(), sizeof(args));
    cmdbuf += args.size();
    return cmdbuf;
}

template <PM4ItOpcode opcode, typename... Args>
constexpr u32* WritePacket(u32* cmdbuf, PM4ShaderType type, Args... data) {
    cmdbuf = WriteHeader<opcode>(cmdbuf, sizeof...(Args), type);
    cmdbuf = WriteBody(cmdbuf, data...);
    return cmdbuf;
}

union ContextControlEnable {
    u32 raw;
    BitField<0, 1, u32> enable_single_cntx_config_reg; ///< single context config reg
    BitField<1, 1, u32> enable_multi_cntx_render_reg;  ///< multi context render state reg
    BitField<15, 1, u32> enable_user_config_reg__CI;   ///< User Config Reg on CI(reserved for SI)
    BitField<16, 1, u32> enable_gfx_sh_reg;            ///< Gfx SH Registers
    BitField<24, 1, u32> enable_cs_sh_reg;             ///< CS SH Registers
    BitField<31, 1, u32> enable_dw;                    ///< DW enable
};

struct PM4CmdContextControl {
    PM4Type3Header header;
    ContextControlEnable load_control;  ///< Enable bits for loading
    ContextControlEnable shadow_enable; ///< Enable bits for shadowing
};

union LoadAddressHigh {
    u32 raw;
    BitField<0, 16, u32>
        addr_hi; ///< bits for the block in Memory from where the CP will fetch the state
    BitField<31, 1, u32>
        wait_idle; ///< if set the CP will wait for the graphics pipe to be idle by writing
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
    u32 addr_lo; ///< low 32 address bits for the block in memory from where the CP will fetch the
                 ///< state
    LoadAddressHigh addr_hi;
    u32 reg_offset; ///< offset in DWords from the register base address
    u32 num_dwords; ///< number of DWords that the CP will fetch and write into the chip. A value of
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
    BitField<2, 30, u32> addr_lo; ///< bits for the block in Memory from where the CP will fetch the
                                  ///< state. DWORD aligned
};

/**
 * PM4CMDLOADDATAINDEX can be used with the following opcodes (VI+)
 * - IT_LOAD_CONTEXT_REG_INDEX
 * - IT_LOAD_SH_REG_INDEX
 */
struct PM4CmdLoadDataIndex {
    PM4Type3Header header;
    LoadAddressLow addr_lo; ///< low 32 address bits for the block in memory from where the CP will
                            ///< fetch the state
    u32 addr_offset;        ///< addrLo.index = 1 Indexed mode
    union {
        BitField<0, 16, u32> reg_offset; ///< offset in DWords from the register base address
        BitField<31, 1, LoadDataFormat> data_format;
        u32 raw;
    };
    u32 num_dwords; ///< Number of DWords that the CP will fetch and write
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
        BitField<0, 16, u32> reg_offset; ///< Offset in DWords from the register base address
        BitField<28, 4, u32> index;      ///< Index for UCONFIG/CONTEXT on CI+
                                         ///< Program to zero for other opcodes and on SI
    };
    u32 data[0];

    [[nodiscard]] u32 Size() const {
        return header.count << 2u;
    }

    template <PM4ShaderType type = PM4ShaderType::ShaderGraphics, typename... Args>
    static constexpr u32* SetContextReg(u32* cmdbuf, Args... data) {
        return WritePacket<PM4ItOpcode::SetContextReg>(cmdbuf, type, data...);
    }

    template <PM4ShaderType type = PM4ShaderType::ShaderGraphics, typename... Args>
    static constexpr u32* SetShReg(u32* cmdbuf, Args... data) {
        return WritePacket<PM4ItOpcode::SetShReg>(cmdbuf, type, data...);
    }
};

struct PM4CmdNop {
    PM4Type3Header header;
    u32 data_block[0];

    enum PayloadType : u32 {
        DebugMarkerPush = 0x68750001u,      ///< Begin of GPU event scope
        DebugMarkerPop = 0x68750002u,       ///< End of GPU event scope
        DebugSetMarker = 0x68750003u,       ///< Set GPU event marker
        SetVsharpInUdata = 0x68750004u,     ///< Indicates that V# will be set in the next packet
        SetTsharpInUdata = 0x68750005u,     ///< Indicates that T# will be set in the next packet
        SetSsharpInUdata = 0x68750006u,     ///< Indicates that S# will be set in the next packet
        DebugColorMarkerPush = 0x6875000eu, ///< Begin of GPU event scope with color
        PatchedFlip = 0x68750776u,          ///< Patched flip marker
        PrepareFlip = 0x68750777u,          ///< Flip marker
        PrepareFlipLabel = 0x68750778u,     ///< Flip marker with label address
        PrepareFlipInterrupt = 0x68750780u, ///< Flip marker with interrupt
        PrepareFlipInterruptLabel = 0x68750781u, ///< Flip marker with interrupt and label
    };
};

struct PM4CmdDrawIndexOffset2 {
    PM4Type3Header header;
    u32 max_size;       ///< Maximum number of indices
    u32 index_offset;   ///< Zero based starting index number in the index buffer
    u32 index_count;    ///< number of indices in the Index Buffer
    u32 draw_initiator; ///< draw Initiator Register
};

struct PM4CmdDrawIndex2 {
    PM4Type3Header header;
    u32 max_size;       ///< maximum number of indices
    u32 index_base_lo;  ///< base Address Lo [31:1] of Index Buffer
                        ///< (Word-Aligned). Written to the VGT_DMA_BASE register.
    u32 index_base_hi;  ///< base Address Hi [39:32] of Index Buffer.
                        ///< Written to the VGT_DMA_BASE_HI register
    u32 index_count;    ///< number of indices in the Index Buffer.
                        ///< Written to the VGT_NUM_INDICES register.
    u32 draw_initiator; ///< written to the VGT_DRAW_INITIATOR register
};

struct PM4CmdDrawIndexType {
    PM4Type3Header header;
    union {
        u32 raw;
        BitField<0, 2, u32> index_type; ///< Select 16 Vs 32bit index
        BitField<2, 2, u32> swap_mode;  ///< DMA swap mode
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
    IrqUndocumented = 3,
};

static u64 GetGpuClock64() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    return static_cast<u64>(ticks);
}

struct PM4CmdEventWriteEop {
    PM4Type3Header header;
    union {
        u32 event_control;
        BitField<0, 6, u32> event_type;  ///< Event type written to VGT_EVENT_INITIATOR
        BitField<8, 4, u32> event_index; ///< Event index
    };
    u32 address_lo;
    union {
        u32 data_control;
        BitField<0, 16, u32> address_hi;          ///< High bits of address
        BitField<24, 2, InterruptSelect> int_sel; ///< Selects interrupt action for end-of-pipe
        BitField<29, 3, DataSelect> data_sel;     ///< Selects source of data
    };
    u32 data_lo; ///< Value that will be written to memory when event occurs
    u32 data_hi; ///< Value that will be written to memory when event occurs

    template <typename T>
    T* Address() const {
        return reinterpret_cast<T*>(address_lo | u64(address_hi) << 32);
    }

    u32 DataDWord() const {
        return data_lo;
    }

    u64 DataQWord() const {
        return data_lo | u64(data_hi) << 32;
    }

    void SignalFence(auto&& write_mem) const {
        u32* address = Address<u32>();
        switch (data_sel.Value()) {
        case DataSelect::None: {
            break;
        }
        case DataSelect::Data32Low: {
            write_mem(address, DataDWord(), sizeof(u32));
            break;
        }
        case DataSelect::Data64: {
            write_mem(address, DataQWord(), sizeof(u64));
            break;
        }
        case DataSelect::GpuClock64: {
            write_mem(address, GetGpuClock64(), sizeof(u64));
            break;
        }
        case DataSelect::PerfCounter: {
            write_mem(address, Common::FencedRDTSC(), sizeof(u64));
            break;
        }
        default: {
            UNREACHABLE();
        }
        }

        switch (int_sel.Value()) {
        case InterruptSelect::None: {
            // No interrupt
            break;
        }
        case InterruptSelect::IrqOnly:
            ASSERT(data_sel == DataSelect::None);
            [[fallthrough]];
        case InterruptSelect::IrqWhenWriteConfirm: {
            Platform::IrqC::Instance()->Signal(Platform::InterruptId::GfxEop);
            break;
        }
        default: {
            UNREACHABLE();
        }
        }
    }
};

struct PM4CmdAcquireMem {
    PM4Type3Header header;
    u32 cp_coher_cntl;
    u32 cp_coher_size_lo;
    u32 cp_coher_size_hi;
    u32 cp_coher_base_lo;
    u32 cp_coher_base_hi;
    u32 poll_interval;
};

enum class DmaDataDst : u32 {
    Memory = 0,
    Gds = 1,
};

enum class DmaDataSrc : u32 {
    Memory = 0,
    Gds = 1,
    Data = 2,
};

struct PM4DmaData {
    PM4Type3Header header;
    union {
        BitField<0, 1, u32> engine;
        BitField<12, 1, u32> src_atc;
        BitField<13, 2, u32> src_cache_policy;
        BitField<15, 1, u32> src_volatile;
        BitField<20, 2, DmaDataDst> dst_sel;
        BitField<24, 1, u32> dst_atc;
        BitField<25, 2, u32> dst_cache_policy;
        BitField<27, 1, u32> dst_volatile;
        BitField<29, 2, DmaDataSrc> src_sel;
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

    template <typename T>
    T SrcAddress() const {
        return std::bit_cast<T>(src_addr_lo | u64(src_addr_hi) << 32);
    }

    template <typename T>
    T DstAddress() const {
        return std::bit_cast<T>(dst_addr_lo | u64(dst_addr_hi) << 32);
    }

    u32 NumBytes() const noexcept {
        return command & 0x1fffff;
    }
};

struct PM4CmdWaitRegMem {
    enum class Engine : u32 { Me = 0u, Pfp = 1u };
    enum class MemSpace : u32 { Register = 0u, Memory = 1u };
    enum class Function : u32 {
        Always = 0u,
        LessThan = 1u,
        LessThanEqual = 2u,
        Equal = 3u,
        NotEqual = 4u,
        GreaterThanEqual = 5u,
        GreaterThan = 6u,
        Reserved = 7u
    };

    PM4Type3Header header;
    union {
        BitField<0, 3, Function> function;
        BitField<4, 1, MemSpace> mem_space;
        BitField<8, 1, Engine> engine;
        u32 raw;
    };
    u32 poll_addr_lo;
    u32 poll_addr_hi;
    u32 ref;
    u32 mask;
    u32 poll_interval;

    template <typename T = u32*>
    T Address() const {
        return std::bit_cast<T>((uintptr_t(poll_addr_hi) << 32) | poll_addr_lo);
    }

    bool Test() const {
        switch (function.Value()) {
        case Function::Always: {
            return true;
        }
        case Function::LessThan: {
            return (*Address() & mask) < ref;
        }
        case Function::LessThanEqual: {
            return (*Address() & mask) <= ref;
        }
        case Function::Equal: {
            return (*Address() & mask) == ref;
        }
        case Function::NotEqual: {
            return (*Address() & mask) != ref;
        }
        case Function::GreaterThanEqual: {
            return (*Address() & mask) >= ref;
        }
        case Function::GreaterThan: {
            return (*Address() & mask) > ref;
        }
        case Function::Reserved:
            [[fallthrough]];
        default: {
            UNREACHABLE();
        }
        }
    }
};

struct PM4CmdWriteData {
    PM4Type3Header header;
    union {
        BitField<8, 11, u32> dst_sel;
        BitField<16, 1, u32> wr_one_addr;
        BitField<20, 1, u32> wr_confirm;
        BitField<30, 1, u32> engine_sel;
        u32 raw;
    };
    union {
        struct {
            u32 dst_addr_lo;
            u32 dst_addr_hi;
        };
        u64 addr64;
    };
    u32 data[0];

    u32 Size() const {
        return (header.count.Value() - 2) * 4;
    }

    template <typename T>
    void Address(T addr) {
        addr64 = static_cast<u64>(addr);
    }

    template <typename T>
    T Address() const {
        return reinterpret_cast<T>(addr64);
    }
};

struct PM4CmdEventWriteEos {
    enum class Command : u32 {
        GdsStore = 1u,
        SignalFence = 2u,
    };

    PM4Type3Header header;
    union {
        u32 event_control;
        BitField<0, 6, u32> event_type;  ///< Event type written to VGT_EVENT_INITIATOR
        BitField<8, 4, u32> event_index; ///< Event index
    };
    u32 address_lo;
    union {
        u32 cmd_info;
        BitField<0, 16, u32> address_hi;  ///< High bits of address
        BitField<29, 3, Command> command; ///< Command
    };
    union {
        u32 data; ///< Fence value that will be written to memory when event occurs
        BitField<0, 16, u32>
            gds_index; ///< Indexed offset from the start of the segment within the partition
        BitField<16, 16, u32> size; ///< Number of DWs to read from the GDS
    };

    template <typename T = u32*>
    T Address() const {
        return reinterpret_cast<T>(address_lo | u64(address_hi) << 32);
    }

    u32 DataDWord() const {
        return this->data;
    }

    void SignalFence(auto&& write_mem) const {
        const auto cmd = command.Value();
        switch (cmd) {
        case Command::SignalFence: {
            write_mem(Address(), DataDWord(), sizeof(u32));
            break;
        }
        case Command::GdsStore: {
            break;
        }
        default: {
            UNREACHABLE_MSG("Unknown command {}", u32(cmd));
        }
        }
    }
};

struct PM4WriteConstRam {
    PM4Type3Header header;
    union {
        BitField<0, 16, u32> offset; ///< Starting DW granularity offset into the constant RAM.
                                     ///< Thus, bits[1:0] are zero.
        u32 dw1;
    };
    u32 data[0];

    [[nodiscard]] u32 Offset() const {
        return offset.Value();
    }

    [[nodiscard]] u32 Size() const {
        return header.count << 2u;
    }
};

struct PM4DumpConstRam {
    PM4Type3Header header;
    union {
        BitField<0, 16, u32> offset; ///< Starting byte offset into the Constant RAM. The minimum
                                     ///< granularity is 4 bytes
        u32 dw1;
    };
    union {
        BitField<0, 15, u32>
            num_dw; ///< Number of DWs to read from the constant RAM. The minimum granularity is DWs
        u32 dw2;
    };
    u32 addr_lo;
    u32 addr_hi;

    template <typename T>
    T Address() const {
        return reinterpret_cast<T>((u64(addr_hi) << 32u) | addr_lo);
    }

    [[nodiscard]] u32 Offset() const {
        return offset.Value();
    }

    [[nodiscard]] u32 Size() const {
        return num_dw.Value() << 2u;
    }
};

struct PM4CmdDispatchDirect {
    PM4Type3Header header;
    u32 dim_x;              ///< X dimensions of the array of thread groups to be dispatched
    u32 dim_y;              ///< Y dimensions of the array of thread groups to be dispatched
    u32 dim_z;              ///< Z dimensions of the array of thread groups to be dispatched
    u32 dispatch_initiator; ///< Dispatch Initiator Register
};

struct PM4CmdDrawNumInstances {
    PM4Type3Header header;
    u32 num_instances;
};

struct PM4CmdDrawIndexBase {
    PM4Type3Header header;
    u32 addr_lo;
    u32 addr_hi;
};

struct PM4CmdDrawIndexBufferSize {
    PM4Type3Header header;
    u32 num_indices;
};

struct PM4CmdIndirectBuffer {
    PM4Type3Header header;
    u32 ibase_lo; ///< Indirect buffer base address, must be 4 byte aligned
    union {
        BitField<0, 16, u32> ibase_hi; ///< Indirect buffer base address
        u32 dw1;
    };
    union {
        BitField<0, 20, u32> ib_size; ///< Indirect buffer size
        BitField<20, 1, u32> chain;   ///< set to chain to IB allocations
        BitField<24, 8, u32> vmid;    ///< Virtual memory domain ID for command buffer
        u32 dw2;
    };

    template <typename T>
    T* Address() const {
        return reinterpret_cast<T*>((u64(ibase_hi) << 32u) | ibase_lo);
    }
};

struct PM4CmdReleaseMem {
    PM4Type3Header header;
    union {
        BitField<0, 6, u32> event_type;  ///< Event type written to VGT_EVENT_INITIATOR
        BitField<8, 4, u32> event_index; ///< Event index
        BitField<12, 1, u32> tcl1_vol_action_ena;
        BitField<13, 1, u32> tc_vol_action_ena;
        BitField<15, 1, u32> tc_wb_action_ena;
        BitField<16, 1, u32> tcl1__action_ena;
        BitField<17, 1, u32> tc_action_ena;
        BitField<25, 2, u32> cache_policy; ///< Cache Policy setting used for writing fences and
                                           ///< timestamps to the TCL2
        u32 dw1;
    };
    union {
        BitField<16, 2, u32> dst_sel;             ///< destination select
        BitField<24, 3, InterruptSelect> int_sel; ///< selects interrupt action for end-of-pipe
        BitField<29, 3, DataSelect> data_sel;     ///< selects source of data
        u32 dw2;
    };
    u32 address_lo; ///< low bits of address
    u32 address_hi; ///< high bits of address
    union {
        struct {
            u16 gds_index; ///< Byte offset into GDS to copy from
            u16 num_dw;    ///< Number of DWORDS of GDS to copy
        };
        u32 data_lo; ///< value that will be written to memory when event occurs
    };
    u32 data_hi;

    template <typename T>
    T* Address() const {
        return reinterpret_cast<T*>(address_lo | u64(address_hi) << 32);
    }

    u32 DataDWord() const {
        return data_lo;
    }

    u64 DataQWord() const {
        return data_lo | u64(data_hi) << 32;
    }

    void SignalFence(Platform::InterruptId irq_id) const {
        switch (data_sel.Value()) {
        case DataSelect::Data32Low: {
            *Address<u32>() = DataDWord();
            break;
        }
        case DataSelect::Data64: {
            *Address<u64>() = DataQWord();
            break;
        }
        case DataSelect::GpuClock64: {
            *Address<u64>() = GetGpuClock64();
            break;
        }
        case DataSelect::PerfCounter: {
            *Address<u64>() = Common::FencedRDTSC();
            break;
        }
        default: {
            UNREACHABLE();
        }
        }

        switch (int_sel.Value()) {
        case InterruptSelect::None: {
            // No interrupt
            break;
        }
        case InterruptSelect::IrqUndocumented:
            [[fallthrough]];
        case InterruptSelect::IrqWhenWriteConfirm: {
            Platform::IrqC::Instance()->Signal(irq_id);
            break;
        }
        default: {
            UNREACHABLE();
        }
        }
    }
};

struct PM4CmdSetBase {
    enum class BaseIndex : u32 {
        DisplayListPatchTable = 0b0000,
        DrawIndexIndirPatchTable = 0b0001,
        GdsPartition = 0b0010,
        CePartition = 0b0011,
    };

    PM4Type3Header header;
    union {
        BitField<0, 4, BaseIndex> base_index;
        u32 dw1;
    };
    u32 address0;
    u32 address1;

    template <typename T>
    T Address() const {
        ASSERT(base_index == BaseIndex::DisplayListPatchTable ||
               base_index == BaseIndex::DrawIndexIndirPatchTable);
        return reinterpret_cast<T>(address0 | (u64(address1 & 0xffff) << 32u));
    }
};

struct PM4CmdDispatchIndirect {
    struct GroupDimensions {
        u32 dim_x;
        u32 dim_y;
        u32 dim_z;
    };

    PM4Type3Header header;
    u32 data_offset;        ///< Byte aligned offset where the required data structure starts
    u32 dispatch_initiator; ///< Dispatch Initiator Register
};

struct PM4CmdDrawIndirect {
    struct DrawInstancedArgs {
        u32 vertex_count_per_instance;
        u32 instance_count;
        u32 start_vertex_location;
        u32 start_instance_location;
    };

    PM4Type3Header header; ///< header
    u32 data_offset;       ///< Byte aligned offset where the required data structure starts
    union {
        u32 dw2;
        BitField<0, 16, u32> base_vtx_loc; ///< Offset where the CP will write the
                                           ///< BaseVertexLocation it fetched from memory
    };
    union {
        u32 dw3;
        BitField<0, 16, u32> start_inst_loc; ///< Offset where the CP will write the
                                             ///< StartInstanceLocation it fetched from memory
    };
    u32 draw_initiator; ///< Draw Initiator Register
};

struct PM4CmdDrawIndexIndirect {
    struct DrawIndexInstancedArgs {
        u32 index_count_per_instance;
        u32 instance_count;
        u32 start_index_location;
        u32 base_vertex_location;
        u32 start_instance_location;
    };

    PM4Type3Header header; ///< header
    u32 data_offset;       ///< Byte aligned offset where the required data structure starts
    union {
        u32 dw2;
        BitField<0, 16, u32> base_vtx_loc; ///< Offset where the CP will write the
                                           ///< BaseVertexLocation it fetched from memory
    };
    union { // NOTE: this one is undocumented in AMD spec, but Gnm driver writes this field
        u32 dw3;
        BitField<0, 16, u32> start_inst_loc; ///< Offset where the CP will write the
                                             ///< StartInstanceLocation it fetched from memory
    };
    u32 draw_initiator; ///< Draw Initiator Register
};

} // namespace AmdGpu
