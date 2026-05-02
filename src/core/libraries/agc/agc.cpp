// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/types.h"
#include "core/libraries/agc/agc.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/gnmdriver/gnmdriver.h"
#include "core/libraries/libs.h"

#include <gcn/si_ci_vi_merged_offset.h>

#include <array>
#include <cstddef>
#include <cstring>

namespace Libraries::Agc {

namespace {

using namespace Pal::Gfx6;

constexpr u32 Cx(u32 reg) {
    return reg - 0xA000u;
}

constexpr u32 Sh(u32 reg) {
    return reg - 0x2C00u;
}

constexpr u32 Uc(u32 reg) {
    return reg - 0xC000u;
}

struct ShaderRegister {
    u32 offset;
    u32 value;
};

struct RegisterDefaultInfo {
    u32 type;
    ShaderRegister reg[16];
};

struct RegisterDefaults {
    ShaderRegister** tbl0 = nullptr;
    ShaderRegister** tbl1 = nullptr;
    ShaderRegister** tbl2 = nullptr;
    ShaderRegister** tbl3 = nullptr;
    u64 unknown[2] = {};
    u32* types = nullptr;
    u32 count = 0;
};

struct CommandBuffer {
    using Callback = bool(PS4_SYSV_ABI*)(CommandBuffer*, u32, void*);

    u32* bottom;
    u32* top;
    u32* cursor_up;
    u32* cursor_down;
    Callback callback;
    void* user_data;
    u32 reserved_dw;

    [[nodiscard]] u32 GetAvailableSizeDW() const {
        const auto available = static_cast<u32>(cursor_down - cursor_up);
        return available > reserved_dw ? available - reserved_dw : 0;
    }

    bool ReserveDW(u32 num_dw) {
        if (num_dw <= GetAvailableSizeDW()) {
            return true;
        }
        return callback != nullptr && callback(this, num_dw + reserved_dw, user_data) &&
               num_dw <= GetAvailableSizeDW();
    }

    u32* AllocateDW(u32 num_dw) {
        if (num_dw == 0 || !ReserveDW(num_dw)) {
            return nullptr;
        }
        auto* result = cursor_up;
        cursor_up += num_dw;
        return result;
    }
};

struct Packet {
    u32* addr;
    u32 dw_num;
    u8 pad[4];
};

struct ShaderSharp {
    u16 offset_dw : 15;
    u16 size : 1;
};

struct ShaderUserData {
    u16* direct_resource_offset;
    ShaderSharp* sharp_resource_offset[4];
    u16 eud_size_dw;
    u16 srt_size_dw;
    u16 direct_resource_count;
    u16 sharp_resource_count[4];
};

struct ShaderRegisterRange {
    u16 start;
    u16 end;
};

struct ShaderDrawModifier {
    u32 enbl_start_vertex_offset : 1;
    u32 enbl_start_index_offset : 1;
    u32 enbl_start_instance_offset : 1;
    u32 enbl_draw_index : 1;
    u32 enbl_user_vgprs : 1;
    u32 render_target_slice_offset : 3;
    u32 fuse_draws : 1;
    u32 compiler_flags : 23;
    u32 is_default : 1;
    u32 reserved : 31;
};

struct ShaderSpecialRegs {
    ShaderRegister ge_cntl;
    ShaderRegister vgt_shader_stages_en;
    u32 dispatch_modifier;
    ShaderRegisterRange user_data_range;
    ShaderDrawModifier draw_modifier;
    ShaderRegister vgt_gs_out_prim_type;
    ShaderRegister ge_user_vgpr_en;
};

struct ShaderSemantic {
    u32 semantic : 8;
    u32 hardware_mapping : 8;
    u32 size_in_elements : 4;
    u32 is_f16 : 2;
    u32 is_flat_shaded : 1;
    u32 is_linear : 1;
    u32 is_custom : 1;
    u32 static_vb_index : 1;
    u32 static_attribute : 1;
    u32 reserved : 1;
    u32 default_value : 2;
    u32 default_value_hi : 2;
};

struct Shader {
    u32 file_header;
    u32 version;
    ShaderUserData* user_data;
    const volatile void* code;
    ShaderRegister* cx_registers;
    ShaderRegister* sh_registers;
    ShaderSpecialRegs* specials;
    ShaderSemantic* input_semantics;
    ShaderSemantic* output_semantics;
    u32 header_size;
    u32 shader_size;
    u32 embedded_constant_buffer_size_dqw;
    u32 target;
    u32 num_input_semantics;
    u16 scratch_size_dw_per_thread;
    u16 num_output_semantics;
    u16 special_sizes_bytes;
    u8 type;
    u8 num_cx_registers;
    u8 num_sh_registers;
};

static_assert(offsetof(RegisterDefaults, count) == 0x38);
static_assert(sizeof(ShaderRegister) == 0x8);
static_assert(sizeof(ShaderUserData) == 0x38);
static_assert(sizeof(ShaderDrawModifier) == sizeof(u64));

#include "core/libraries/agc/agc_register_defaults.inc"

template <std::size_t CxCount, std::size_t ShCount, std::size_t UcCount>
struct DefaultRegisterTables {
    static constexpr u32 TableCx = 0;
    static constexpr u32 TableSh = 1;
    static constexpr u32 TableUc = 2;
    static constexpr std::size_t TotalCount = CxCount + ShCount + UcCount;

    std::array<ShaderRegister*, CxCount> cx_regs{};
    std::array<ShaderRegister*, ShCount> sh_regs{};
    std::array<ShaderRegister*, UcCount> uc_regs{};
    std::array<u32, TotalCount * 3> index{};
    RegisterDefaults defaults{};

    DefaultRegisterTables(RegisterDefaultInfo (&cx_info)[CxCount],
                          RegisterDefaultInfo (&sh_info)[ShCount],
                          RegisterDefaultInfo (&uc_info)[UcCount]) {
        std::size_t dst = 0;
        FillTable(cx_regs, cx_info, dst, TableCx);
        FillTable(sh_regs, sh_info, dst, TableSh);
        FillTable(uc_regs, uc_info, dst, TableUc);

        defaults.tbl0 = cx_regs.data();
        defaults.tbl1 = sh_regs.data();
        defaults.tbl2 = uc_regs.data();
        defaults.types = index.data();
        defaults.count = static_cast<u32>(TotalCount);
    }

    template <std::size_t Count>
    void FillTable(std::array<ShaderRegister*, Count>& dst, RegisterDefaultInfo (&src)[Count],
                   std::size_t& index_dst, u32 table_id) {
        for (std::size_t i = 0; i < Count; ++i) {
            dst[i] = src[i].reg;
            index[index_dst++] = src[i].type;
            index[index_dst++] = static_cast<u32>(i) * 4u + table_id;
            index[index_dst++] = 0;
        }
    }
};

DefaultRegisterTables g_reg_defaults2{g_cx_reg_info1, g_sh_reg_info1, g_uc_reg_info1};
DefaultRegisterTables g_reg_defaults2_internal{g_cx_reg_info2, g_sh_reg_info2, g_uc_reg_info2};

constexpr u32 Pm4Packet(u32 len, u32 op, u32 custom_r = 0) {
    return 0xC0000000u | (((len - 2u) & 0x3fffu) << 16u) | ((op & 0xffu) << 8u) |
           ((custom_r & 0x3fu) << 2u);
}

constexpr u32 Pm4ItNop = 0x10;
constexpr u32 Pm4ItIndexBase = 0x26;
constexpr u32 Pm4ItIndexType = 0x2A;
constexpr u32 Pm4ItSetShReg = 0x76;
constexpr u32 Pm4CustomZero = 0x00;
constexpr u32 Pm4CustomDrawReset = 0x05;
constexpr u32 Pm4CustomShRegsIndirect = 0x11;
constexpr u32 Pm4CustomCxRegsIndirect = 0x12;
constexpr u32 Pm4CustomUcRegsIndirect = 0x13;
constexpr u32 Pm4CustomFlip = 0x17;
constexpr u32 RegGeCntl = 0x25B;
constexpr u32 RegGeUserVgprEn = 0x262;

RegisterDefaults* GetRegisterDefaults(u32 version, bool internal) {
    if (version != 8) {
        LOG_ERROR(Lib_Agc, "sceAgcGetRegisterDefaults2{} unsupported version {}",
                  internal ? "Internal" : "", version);
        return nullptr;
    }

    return internal ? &g_reg_defaults2_internal.defaults : &g_reg_defaults2.defaults;
}

template <typename T>
void RelocateRelativePointer(T*& ptr) {
    if (ptr != nullptr) {
        ptr = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(&ptr) + reinterpret_cast<uintptr_t>(ptr));
    }
}

} // namespace

s32 PS4_SYSV_ABI sceAgcInit2Maybe(u32* state, u32 version) {
    LOG_INFO(Lib_Agc, "sceAgcInit2Maybe(state = {}, version = {})", fmt::ptr(state), version);

    if (state == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcInit2Maybe called with null state");
        return ORBIS_OK;
    }

    if (version != 8) {
        LOG_ERROR(Lib_Agc, "sceAgcInit2Maybe unsupported version {}", version);
    }

    return ORBIS_OK;
}

void* PS4_SYSV_ABI sceAgcGetRegisterDefaults2(u32 version) {
    LOG_INFO(Lib_Agc, "sceAgcGetRegisterDefaults2(version = {})", version);
    return GetRegisterDefaults(version, false);
}

void* PS4_SYSV_ABI sceAgcGetRegisterDefaults2Internal(u32 version) {
    LOG_INFO(Lib_Agc, "sceAgcGetRegisterDefaults2Internal(version = {})", version);
    return GetRegisterDefaults(version, true);
}

s32 PS4_SYSV_ABI sceAgcCreateShader(Shader** dst, void* header, const volatile void* code) {
    LOG_INFO(Lib_Agc, "sceAgcCreateShader(dst = {}, header = {}, code = {})", fmt::ptr(dst),
             fmt::ptr(header), fmt::ptr(code));

    if (dst == nullptr || header == nullptr || code == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcCreateShader called with null argument");
        return ORBIS_OK;
    }

    auto* shader = static_cast<Shader*>(header);

    RelocateRelativePointer(shader->cx_registers);
    RelocateRelativePointer(shader->sh_registers);
    RelocateRelativePointer(shader->user_data);
    RelocateRelativePointer(shader->specials);
    RelocateRelativePointer(shader->input_semantics);
    RelocateRelativePointer(shader->output_semantics);

    if (shader->user_data != nullptr) {
        RelocateRelativePointer(shader->user_data->direct_resource_offset);
        for (auto& offset : shader->user_data->sharp_resource_offset) {
            RelocateRelativePointer(offset);
        }
    }

    shader->code = code;

    if (shader->file_header != 0x34333231 || shader->version != 0x18) {
        LOG_ERROR(Lib_Agc, "Unsupported shader header {:#x}, version {:#x}", shader->file_header,
                  shader->version);
        *dst = shader;
        return ORBIS_OK;
    }

    const u64 base = reinterpret_cast<u64>(code);
    if ((base & 0xFFFF0000000000FFull) != 0) {
        LOG_ERROR(Lib_Agc, "Unsupported shader code address {:#x}", base);
        *dst = shader;
        return ORBIS_OK;
    }

    if (shader->num_sh_registers >= 2 && shader->sh_registers != nullptr) {
        const u32 lo = static_cast<u32>((base >> 8u) & 0xffffffffu);
        const u32 hi = static_cast<u32>((base >> 40u) & 0xffu);

        if (shader->type == 2 && shader->sh_registers[0].offset == Sh(mmSPI_SHADER_PGM_LO_ES) &&
            shader->sh_registers[1].offset == Sh(mmSPI_SHADER_PGM_HI_ES)) {
            shader->sh_registers[0].value = lo;
            shader->sh_registers[1].value = hi;
        } else if (shader->type == 1 &&
                   shader->sh_registers[0].offset == Sh(mmSPI_SHADER_PGM_LO_PS) &&
                   shader->sh_registers[1].offset == Sh(mmSPI_SHADER_PGM_HI_PS)) {
            shader->sh_registers[0].value = lo;
            shader->sh_registers[1].value = hi;
        } else {
            LOG_ERROR(Lib_Agc, "Unsupported shader type {} or register layout", shader->type);
        }
    }

    *dst = shader;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAgcCreateInterpolantMapping(ShaderRegister* regs, const Shader* gs,
                                                const Shader* ps) {
    LOG_INFO(Lib_Agc, "sceAgcCreateInterpolantMapping(regs = {}, gs = {}, ps = {})",
             fmt::ptr(regs), fmt::ptr(gs), fmt::ptr(ps));

    if (regs == nullptr || gs == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcCreateInterpolantMapping called with null argument");
        return ORBIS_OK;
    }

    if (gs->type != 2 || (ps != nullptr && ps->type != 1)) {
        LOG_ERROR(Lib_Agc, "Unsupported shader types gs = {}, ps = {}", gs->type,
                  ps != nullptr ? ps->type : 0xff);
    }
    if (ps != nullptr && gs->num_output_semantics != ps->num_input_semantics) {
        LOG_ERROR(Lib_Agc, "GS output semantic count {} does not match PS input semantic count {}",
                  gs->num_output_semantics, ps->num_input_semantics);
    }
    if (ps != nullptr && ps->num_output_semantics != 0) {
        LOG_ERROR(Lib_Agc, "Unexpected PS output semantic count {}", ps->num_output_semantics);
    }

    for (u32 i = 0; i < 32; ++i) {
        regs[i].offset = Cx(mmSPI_PS_INPUT_CNTL_0) + i;
        regs[i].value = 0;

        if (i >= gs->num_output_semantics) {
            continue;
        }

        bool flat = false;
        if (ps != nullptr && i < ps->num_input_semantics) {
            flat = ps->input_semantics[i].is_flat_shaded != 0;
        }

        regs[i].value = i | (flat ? 0x400u : 0u);
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAgcGetDataPacketPayloadAddress(u32** addr, u32* cmd, s32 type) {
    LOG_INFO(Lib_Agc, "sceAgcGetDataPacketPayloadAddress(addr = {}, cmd = {}, type = {})",
             fmt::ptr(addr), fmt::ptr(cmd), type);

    if (addr == nullptr || cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcGetDataPacketPayloadAddress called with null argument");
        return ORBIS_OK;
    }
    if (type != 1) {
        LOG_ERROR(Lib_Agc, "Unsupported data packet payload type {}", type);
        *addr = nullptr;
        return ORBIS_OK;
    }

    *addr = cmd + 2;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAgcSetCxRegIndirectPatchSetAddress(u32* cmd,
                                                       const volatile ShaderRegister* regs) {
    LOG_INFO(Lib_Agc, "sceAgcSetCxRegIndirectPatchSetAddress(cmd = {}, regs = {})",
             fmt::ptr(cmd), fmt::ptr(const_cast<const ShaderRegister*>(regs)));

    if (cmd == nullptr || regs == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcSetCxRegIndirectPatchSetAddress called with null argument");
        return ORBIS_OK;
    }

    const auto addr = reinterpret_cast<u64>(regs);
    cmd[2] = static_cast<u32>(addr);
    cmd[3] = static_cast<u32>(addr >> 32u);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAgcSetCxRegIndirectPatchAddRegisters(u32* cmd, u32 numRegs) {
    LOG_INFO(Lib_Agc, "sceAgcSetCxRegIndirectPatchAddRegisters(cmd = {}, numRegs = {})",
             fmt::ptr(cmd), numRegs);

    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcSetCxRegIndirectPatchAddRegisters called with null command");
        return ORBIS_OK;
    }

    cmd[1] += numRegs;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAgcSetShRegIndirectPatchSetAddress(u32* cmd,
                                                       const volatile ShaderRegister* regs) {
    LOG_INFO(Lib_Agc, "sceAgcSetShRegIndirectPatchSetAddress(cmd = {}, regs = {})",
             fmt::ptr(cmd), fmt::ptr(const_cast<const ShaderRegister*>(regs)));

    if (cmd == nullptr || regs == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcSetShRegIndirectPatchSetAddress called with null argument");
        return ORBIS_OK;
    }

    const auto addr = reinterpret_cast<u64>(regs);
    cmd[2] = static_cast<u32>(addr);
    cmd[3] = static_cast<u32>(addr >> 32u);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAgcSetShRegIndirectPatchAddRegisters(u32* cmd, u32 numRegs) {
    LOG_INFO(Lib_Agc, "sceAgcSetShRegIndirectPatchAddRegisters(cmd = {}, numRegs = {})",
             fmt::ptr(cmd), numRegs);

    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcSetShRegIndirectPatchAddRegisters called with null command");
        return ORBIS_OK;
    }

    cmd[1] += numRegs;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAgcSetUcRegIndirectPatchSetAddress(u32* cmd,
                                                       const volatile ShaderRegister* regs) {
    LOG_INFO(Lib_Agc, "sceAgcSetUcRegIndirectPatchSetAddress(cmd = {}, regs = {})",
             fmt::ptr(cmd), fmt::ptr(const_cast<const ShaderRegister*>(regs)));

    if (cmd == nullptr || regs == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcSetUcRegIndirectPatchSetAddress called with null argument");
        return ORBIS_OK;
    }

    const auto addr = reinterpret_cast<u64>(regs);
    cmd[2] = static_cast<u32>(addr);
    cmd[3] = static_cast<u32>(addr >> 32u);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAgcSetUcRegIndirectPatchAddRegisters(u32* cmd, u32 numRegs) {
    LOG_INFO(Lib_Agc, "sceAgcSetUcRegIndirectPatchAddRegisters(cmd = {}, numRegs = {})",
             fmt::ptr(cmd), numRegs);

    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcSetUcRegIndirectPatchAddRegisters called with null command");
        return ORBIS_OK;
    }

    cmd[1] += numRegs;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceAgcSuspendPoint() {
    LOG_INFO(Lib_Agc, "sceAgcSuspendPoint()");
    return Libraries::GnmDriver::sceGnmSubmitDone();
}

s32 PS4_SYSV_ABI sceAgcCreatePrimState(ShaderRegister* cxRegs, ShaderRegister* ucRegs,
                                       const Shader* hs, const Shader* gs, u32 primType) {
    LOG_INFO(Lib_Agc, "sceAgcCreatePrimState(cxRegs = {}, ucRegs = {}, hs = {}, gs = {}, "
                      "primType = {})",
             fmt::ptr(cxRegs), fmt::ptr(ucRegs), fmt::ptr(hs), fmt::ptr(gs), primType);

    if (cxRegs == nullptr || ucRegs == nullptr || gs == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcCreatePrimState called with null argument");
        return ORBIS_OK;
    }
    if (hs != nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcCreatePrimState does not support HS yet");
    }
    if (gs->type != 2 || gs->specials == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcCreatePrimState unsupported GS shader");
    }

    cxRegs[0] = gs->specials->vgt_shader_stages_en;
    cxRegs[1] = gs->specials->vgt_gs_out_prim_type;

    ucRegs[0] = gs->specials->ge_cntl;
    ucRegs[1] = gs->specials->ge_user_vgpr_en;
    ucRegs[2].offset = Uc(mmVGT_PRIMITIVE_TYPE__CI__VI);
    ucRegs[2].value = primType;

    return ORBIS_OK;
}

u32* PS4_SYSV_ABI sceAgcCbSetShRegisterRangeDirect(CommandBuffer* buf, u32 offset,
                                                   const u32* values, u32 numValues) {
    LOG_INFO(Lib_Agc,
             "sceAgcCbSetShRegisterRangeDirect(buf = {}, offset = {:#x}, values = {}, "
             "numValues = {})",
             fmt::ptr(buf), offset, fmt::ptr(values), numValues);

    if (buf == nullptr || numValues == 0 || offset == 0 || offset > 0x3ffu) {
        LOG_ERROR(Lib_Agc, "sceAgcCbSetShRegisterRangeDirect invalid arguments");
        return nullptr;
    }

    auto* marker = buf->AllocateDW(2);
    if (marker == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcCbSetShRegisterRangeDirect failed to allocate marker");
        return nullptr;
    }
    marker[0] = Pm4Packet(2, Pm4ItNop, Pm4CustomZero);
    marker[1] = 0x6875000d;

    auto* cmd = buf->AllocateDW(numValues + 2);
    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcCbSetShRegisterRangeDirect failed to allocate command");
        return nullptr;
    }

    cmd[0] = Pm4Packet(numValues + 2, Pm4ItSetShReg);
    cmd[1] = offset;
    if (values == nullptr) {
        std::memset(cmd + 2, 0, static_cast<std::size_t>(numValues) * sizeof(u32));
    } else {
        std::memcpy(cmd + 2, values, static_cast<std::size_t>(numValues) * sizeof(u32));
    }

    return cmd;
}

u32* PS4_SYSV_ABI sceAgcDcbResetQueue(CommandBuffer* buf, u32 op, u32 state) {
    LOG_INFO(Lib_Agc, "sceAgcDcbResetQueue(buf = {}, op = {:#x}, state = {:#x})",
             fmt::ptr(buf), op, state);

    if (buf == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbResetQueue called with null command buffer");
        return nullptr;
    }
    if (op != 0x3ff || state != 0) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbResetQueue unsupported op/state");
    }

    auto* cmd = buf->AllocateDW(2);
    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbResetQueue failed to allocate command");
        return nullptr;
    }

    cmd[0] = Pm4Packet(2, Pm4ItNop, Pm4CustomDrawReset);
    cmd[1] = 0;
    return cmd;
}

u32* PS4_SYSV_ABI sceAgcDcbSetCxRegistersIndirect(CommandBuffer* buf,
                                                  const volatile ShaderRegister* regs,
                                                  u32 numRegs) {
    LOG_INFO(Lib_Agc, "sceAgcDcbSetCxRegistersIndirect(buf = {}, regs = {}, numRegs = {})",
             fmt::ptr(buf), fmt::ptr(const_cast<const ShaderRegister*>(regs)), numRegs);

    if (buf == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetCxRegistersIndirect called with null command buffer");
        return nullptr;
    }

    auto* cmd = buf->AllocateDW(4);
    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetCxRegistersIndirect failed to allocate command");
        return nullptr;
    }

    const auto addr = reinterpret_cast<u64>(regs);
    cmd[0] = Pm4Packet(4, Pm4ItNop, Pm4CustomCxRegsIndirect);
    cmd[1] = numRegs;
    cmd[2] = static_cast<u32>(addr);
    cmd[3] = static_cast<u32>(addr >> 32u);

    return cmd;
}

u32* PS4_SYSV_ABI sceAgcDcbSetShRegistersIndirect(CommandBuffer* buf,
                                                  const volatile ShaderRegister* regs,
                                                  u32 numRegs) {
    LOG_INFO(Lib_Agc, "sceAgcDcbSetShRegistersIndirect(buf = {}, regs = {}, numRegs = {})",
             fmt::ptr(buf), fmt::ptr(const_cast<const ShaderRegister*>(regs)), numRegs);

    if (buf == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetShRegistersIndirect called with null command buffer");
        return nullptr;
    }

    auto* cmd = buf->AllocateDW(4);
    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetShRegistersIndirect failed to allocate command");
        return nullptr;
    }

    const auto addr = reinterpret_cast<u64>(regs);
    cmd[0] = Pm4Packet(4, Pm4ItNop, Pm4CustomShRegsIndirect);
    cmd[1] = numRegs;
    cmd[2] = static_cast<u32>(addr);
    cmd[3] = static_cast<u32>(addr >> 32u);

    return cmd;
}

u32* PS4_SYSV_ABI sceAgcDcbSetUcRegistersIndirect(CommandBuffer* buf,
                                                   const volatile ShaderRegister* regs,
                                                   u32 numRegs) {
    LOG_INFO(Lib_Agc, "sceAgcDcbSetUcRegistersIndirect(buf = {}, regs = {}, numRegs = {})",
             fmt::ptr(buf), fmt::ptr(const_cast<const ShaderRegister*>(regs)), numRegs);

    if (buf == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetUcRegistersIndirect called with null command buffer");
        return nullptr;
    }

    auto* cmd = buf->AllocateDW(4);
    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetUcRegistersIndirect failed to allocate command");
        return nullptr;
    }

    const auto addr = reinterpret_cast<u64>(regs);
    cmd[0] = Pm4Packet(4, Pm4ItNop, Pm4CustomUcRegsIndirect);
    cmd[1] = numRegs;
    cmd[2] = static_cast<u32>(addr);
    cmd[3] = static_cast<u32>(addr >> 32u);

    return cmd;
}

u32* PS4_SYSV_ABI sceAgcDcbSetIndexSize(CommandBuffer* buf, u8 indexSize, u8 cachePolicy) {
    LOG_INFO(Lib_Agc, "sceAgcDcbSetIndexSize(buf = {}, indexSize = {}, cachePolicy = {})",
             fmt::ptr(buf), static_cast<u32>(indexSize), static_cast<u32>(cachePolicy));

    if (buf == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetIndexSize called with null command buffer");
        return nullptr;
    }
    if (cachePolicy != 0) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetIndexSize unsupported cache policy {}",
                  static_cast<u32>(cachePolicy));
    }
    if (indexSize > 1) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetIndexSize unsupported index size {}",
                  static_cast<u32>(indexSize));
    }

    auto* cmd = buf->AllocateDW(2);
    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetIndexSize failed to allocate command");
        return nullptr;
    }

    cmd[0] = Pm4Packet(2, Pm4ItIndexType);
    cmd[1] = indexSize;
    return cmd;
}

u32* PS4_SYSV_ABI sceAgcDcbSetIndexBuffer(CommandBuffer* buf, uintptr_t indexAddr) {
    LOG_INFO(Lib_Agc, "sceAgcDcbSetIndexBuffer(buf = {}, indexAddr = {:#x})", fmt::ptr(buf),
             indexAddr);

    if (buf == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetIndexBuffer called with null command buffer");
        return nullptr;
    }
    if ((indexAddr & 1u) != 0) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetIndexBuffer called with unaligned address {:#x}",
                  indexAddr);
    }

    auto* cmd = buf->AllocateDW(3);
    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetIndexBuffer failed to allocate command");
        return nullptr;
    }

    cmd[0] = Pm4Packet(3, Pm4ItIndexBase);
    cmd[1] = static_cast<u32>(indexAddr);
    cmd[2] = static_cast<u32>(indexAddr >> 32u);
    return cmd;
}

u32 PS4_SYSV_ABI sceAgcDcbSetIndexBufferGetSize() {
    LOG_INFO(Lib_Agc, "sceAgcDcbSetIndexBufferGetSize()");
    return 3;
}

u32* PS4_SYSV_ABI sceAgcDcbDrawIndexOffset(CommandBuffer* buf, u32 indexOffset, u32 indexCount,
                                           u64 modifier) {
    ShaderDrawModifier drawModifier{};
    std::memcpy(&drawModifier, &modifier, sizeof(drawModifier));

    LOG_INFO(Lib_Agc,
             "sceAgcDcbDrawIndexOffset(buf = {}, indexOffset = {}, indexCount = {}, "
             "modifier = {:#018x})",
             fmt::ptr(buf), indexOffset, indexCount, modifier);
    LOG_INFO(Lib_Agc,
             "sceAgcDcbDrawIndexOffset modifier: enbl_start_vertex_offset = {}, "
             "enbl_start_index_offset = {}, enbl_start_instance_offset = {}, "
             "enbl_draw_index = {}, enbl_user_vgprs = {}, render_target_slice_offset = {}, "
             "fuse_draws = {}, compiler_flags = {:#x}, is_default = {}, reserved = {:#x}",
             static_cast<u32>(drawModifier.enbl_start_vertex_offset),
             static_cast<u32>(drawModifier.enbl_start_index_offset),
             static_cast<u32>(drawModifier.enbl_start_instance_offset),
             static_cast<u32>(drawModifier.enbl_draw_index),
             static_cast<u32>(drawModifier.enbl_user_vgprs),
             static_cast<u32>(drawModifier.render_target_slice_offset),
             static_cast<u32>(drawModifier.fuse_draws),
             static_cast<u32>(drawModifier.compiler_flags),
             static_cast<u32>(drawModifier.is_default), static_cast<u32>(drawModifier.reserved));

    if (buf == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbDrawIndexOffset called with null command buffer");
        return nullptr;
    }
    if ((modifier >> 32u) != 0) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbDrawIndexOffset ignores high modifier bits {:#x}",
                  static_cast<u32>(modifier >> 32u));
    }

    constexpr u32 PacketSizeDw = 9;
    auto* cmd = buf->AllocateDW(PacketSizeDw);
    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbDrawIndexOffset failed to allocate command");
        return nullptr;
    }

    const auto ret = Libraries::GnmDriver::sceGnmDrawIndexOffset(
        cmd, PacketSizeDw, indexOffset, indexCount, static_cast<u32>(modifier));
    if (ret != ORBIS_OK) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbDrawIndexOffset failed to write packet: {}", ret);
        return cmd;
    }

    LOG_INFO(Lib_Agc,
             "sceAgcDcbDrawIndexOffset packet: [{:#010x}, {:#010x}, {:#010x}, {:#010x}, "
             "{:#010x}, {:#010x}, {:#010x}, {:#010x}, {:#010x}]",
             cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8]);
    return cmd;
}

u32* PS4_SYSV_ABI sceAgcDcbSetFlip(CommandBuffer* buf, u32 videoOutHandle,
                                   s32 displayBufferIndex, u32 flipMode, s64 flipArg) {
    LOG_INFO(Lib_Agc,
             "sceAgcDcbSetFlip(buf = {}, videoOutHandle = {}, displayBufferIndex = {}, "
             "flipMode = {}, flipArg = {})",
             fmt::ptr(buf), videoOutHandle, displayBufferIndex, flipMode, flipArg);

    if (buf == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetFlip called with null command buffer");
        return nullptr;
    }

    auto* cmd = buf->AllocateDW(6);
    if (cmd == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDcbSetFlip failed to allocate command");
        return nullptr;
    }

    const auto arg = static_cast<u64>(flipArg);
    cmd[0] = Pm4Packet(6, Pm4ItNop, Pm4CustomFlip);
    cmd[1] = videoOutHandle;
    cmd[2] = static_cast<u32>(displayBufferIndex);
    cmd[3] = flipMode;
    cmd[4] = static_cast<u32>(arg);
    cmd[5] = static_cast<u32>(arg >> 32u);

    return cmd;
}

s32 PS4_SYSV_ABI sceAgcDriverSubmitDcb(const Packet* packet) {
    LOG_INFO(Lib_Agc, "sceAgcDriverSubmitDcb(packet = {})", fmt::ptr(packet));

    if (packet == nullptr || packet->addr == nullptr) {
        LOG_ERROR(Lib_Agc, "sceAgcDriverSubmitDcb called with null packet");
        return ORBIS_OK;
    }
    if (packet->pad[0] != 0) {
        LOG_ERROR(Lib_Agc, "sceAgcDriverSubmitDcb unexpected packet padding");
    }

    const u32* dcbAddrs[] = {packet->addr};
    u32 dcbSizes[] = {static_cast<u32>(packet->dw_num * sizeof(u32))};
    return Libraries::GnmDriver::sceGnmSubmitCommandBuffers(1, dcbAddrs, dcbSizes, nullptr,
                                                            nullptr);
}

#define AGC_FUNCTIONS                                                                          \
    X("KT-hTp-Ch14", sceAgcAcbAcquireMem)                                                     \
    X("ewobAQeMo5k", sceAgcAcbAcquireMemGetSize)                                              \
    X("cduV1f0dcGQ", sceAgcAcbAtomicGds)                                                      \
    X("hcIxS8pmXF4", sceAgcAcbAtomicGdsGetSize)                                               \
    X("XKKuA6VkSRc", sceAgcAcbAtomicMem)                                                      \
    X("da1Sm8-QDoU", sceAgcAcbAtomicMemGetSize)                                               \
    X("qyM2bxYFPAk", sceAgcAcbCondExec)                                                       \
    X("ozKzBP4aki4", sceAgcAcbCondExecGetSize)                                                \
    X("qzMN2XKGA4k", sceAgcAcbCopyData)                                                       \
    X("CbQh3DKMSno", sceAgcAcbCopyDataGetSize)                                                \
    X("j3EtxFkSIhQ", sceAgcAcbDispatchIndirect)                                               \
    X("PxKWV2fVAps", sceAgcAcbDispatchIndirectGetSize)                                        \
    X("-RnpfpxIhec", sceAgcAcbDmaData)                                                        \
    X("M0ttm8h7SKA", sceAgcAcbDmaDataGetSize)                                                 \
    X("cFazmnXpJOE", sceAgcAcbEventWrite)                                                     \
    X("Y-5vneiBtzk", sceAgcAcbEventWriteGetSize)                                              \
    X("e1DFTg+Sd8U", sceAgcAcbJump)                                                           \
    X("b-oySn+G2tE", sceAgcAcbJumpGetSize)                                                    \
    X("q4VuU-QsLOE", sceAgcAcbMemSemaphore)                                                   \
    X("6mFxkVqdmbQ", sceAgcAcbPopMarker)                                                      \
    X("szG7hz2yEhA", sceAgcAcbPrimeUtcl2)                                                     \
    X("eCjKaqeeQ5s", sceAgcAcbPrimeUtcl2GetSize)                                              \
    X("cpCILPya5Zk", sceAgcAcbPushMarker)                                                     \
    X("F8NLhWvFemI", sceAgcAcbQueueEndOfShaderActionGetSize)                                  \
    X("JrtiDtKeS38", sceAgcAcbResetQueue)                                                     \
    X("DwICrVxerkY", sceAgcAcbRewind)                                                         \
    X("0ZOG0jc9nRg", sceAgcAcbRewindGetSize)                                                  \
    X("ebixW91gpPw", sceAgcAcbSetFlip)                                                        \
    X("xAeBOa0A3kk", sceAgcAcbSetMarker)                                                      \
    X("opR1JeJZCBU", sceAgcAcbSetWorkloadComplete)                                            \
    X("rVOmPz2RBlg", sceAgcAcbSetWorkloadsActive)                                             \
    X("idlaArvdXEs", sceAgcAcbWaitOnAddressGetSize)                                           \
    X("htn36gPnBk4", sceAgcAcbWaitRegMem)                                                     \
    X("GPbUp9jXQa8", sceAgcAcbWaitUntilSafeForRendering)                                      \
    X("eZ4+17OQz4Q", sceAgcAcbWriteData)                                                      \
    X("3ZWa3AoyWZQ", sceAgcAsyncCondExecPatchSetCommandAddress)                               \
    X("k-JpyR2dYAM", sceAgcAsyncCondExecPatchSetEnd)                                          \
    X("eWaWyFegzgQ", sceAgcAsyncRewindPatchSetRewindState)                                    \
    X("GXBlM-ekzrI", sceAgcBranchPatchSetCompareAddress)                                      \
    X("QmfvaYpsOcI", sceAgcBranchPatchSetElseTarget)                                          \
    X("xb8VgcXQhvI", sceAgcBranchPatchSetThenTarget)                                          \
    X("w1KFAHVqpaU", sceAgcCbBranch)                                                          \
    X("uZW-mqsxkrM", sceAgcCbBranchGetSize)                                                   \
    X("7toV+elXqNM", sceAgcCbCondWrite)                                                       \
    X("FuVbkyKlf+s", sceAgcCbCondWriteGetSize)                                                \
    X("k3GhuSNmBLU", sceAgcCbDispatch)                                                        \
    X("Abendgtz+3o", sceAgcCbDispatchGetSize)                                                 \
    X("vHX9guneRBY", sceAgcCbMemSemaphore)                                                    \
    X("LtTouSCZjHM", sceAgcCbNop)                                                             \
    X("t7PlZ9nt5Lc", sceAgcCbNopGetSize)                                                      \
    X("hL7C0IRpWZI", sceAgcCbQueueEndOfPipeActionGetSize)                                     \
    X("wr23dPKyWc0", sceAgcCbReleaseMem)                                                      \
    X("bxGoVxpdSPQ", sceAgcCbSetShRegisterRangeDirectGetSize)                                 \
    X("UZbQjYAwwXM", sceAgcCbSetShRegistersDirect)                                            \
    X("yUBESvCCJ4I", sceAgcCbSetShRegistersDirectGetSize)                                     \
    X("MDLD5Ly94Xk", sceAgcCbSetUcRegisterRangeDirect)                                        \
    X("JOWmDrl+j20", sceAgcCbSetUcRegisterRangeDirectGetSize)                                 \
    X("03RZmELWWzw", sceAgcCbSetUcRegistersDirect)                                            \
    X("TGEZzUWLbrc", sceAgcCbSetUcRegistersDirectGetSize)                                     \
    X("YWTKOju587o", sceAgcCondExecPatchSetCommandAddress)                                    \
    X("ORWsxIbk4TE", sceAgcCondExecPatchSetEnd)                                               \
    X("57labkp+rSQ", sceAgcDcbAcquireMem)                                                     \
    X("-vnlTPPXPrw", sceAgcDcbAcquireMemGetSize)                                              \
    X("pH3-dfRpfA0", sceAgcDcbAtomicGds)                                                      \
    X("1tB0xkLNjcw", sceAgcDcbAtomicGdsGetSize)                                               \
    X("1-gUn1PI4Sw", sceAgcDcbAtomicMem)                                                      \
    X("oz6zQq1JwCE", sceAgcDcbAtomicMemGetSize)                                               \
    X("ms1xVoZ-Vwc", sceAgcDcbBeginOcclusionQueryGetSize)                                     \
    X("PxEFhy0d5v8", sceAgcDcbClearState)                                                     \
    X("BIPexNBSGog", sceAgcDcbCondExec)                                                       \
    X("ou16V5hh5sg", sceAgcDcbCondExecGetSize)                                                \
    X("HabmgqPwPw0", sceAgcDcbContextStateOp)                                                 \
    X("H6vHS5cidSA", sceAgcDcbContextStateOpGetSize)                                          \
    X("1rZSWUv1IRc", sceAgcDcbCopyData)                                                       \
    X("b5u0Jzm8TF8", sceAgcDcbCopyDataGetSize)                                                \
    X("CtB+A9-VxO0", sceAgcDcbDispatchIndirect)                                               \
    X("w8HVkEeXPv8", sceAgcDcbDispatchIndirectGetSize)                                        \
    X("WmAc2MEj6Io", sceAgcDcbDmaData)                                                        \
    X("2ccJz9LQI+w", sceAgcDcbDmaDataGetSize)                                                 \
    X("q88lQ+GP5Yk", sceAgcDcbDrawIndex)                                                      \
    X("Yw0jKSqop+E", sceAgcDcbDrawIndexAuto)                                                  \
    X("WrdP9Zxx3lQ", sceAgcDcbDrawIndexAutoGetSize)                                           \
    X("6ee9Hd3EWXQ", sceAgcDcbDrawIndexGetSize)                                               \
    X("t1vNu082-jM", sceAgcDcbDrawIndexIndirect)                                              \
    X("mStuvI0zOtc", sceAgcDcbDrawIndexIndirectGetSize)                                       \
    X("ypVBz4uPKcQ", sceAgcDcbDrawIndexIndirectMulti)                                         \
    X("r98I08t+LOg", sceAgcDcbDrawIndexIndirectMultiGetSize)                                  \
    X("Rlx+bykm0r0", sceAgcDcbDrawIndexMultiInstanced)                                        \
    X("mR9j7+SfM34", sceAgcDcbDrawIndexMultiInstancedGetSize)                                 \
    X("qMlfB1ZhMDc", sceAgcDcbDrawIndexOffsetGetSize)                                         \
    X("1q1titRBL6o", sceAgcDcbDrawIndirect)                                                   \
    X("cxPZ4Wgvdj8", sceAgcDcbDrawIndirectGetSize)                                            \
    X("kUlvghKs-mA", sceAgcDcbDrawIndirectMulti)                                              \
    X("pYoKs3lPy88", sceAgcDcbDrawIndirectMultiGetSize)                                       \
    X("P1CugZ99Uzc", sceAgcDcbEndOcclusionQueryGetSize)                                       \
    X("aJf+j5yntiU", sceAgcDcbEventWrite)                                                     \
    X("C4l9fB17t8w", sceAgcDcbEventWriteGetSize)                                              \
    X("vuSXe69VILM", sceAgcDcbGetLodStats)                                                    \
    X("rUuVjyR+Rd4", sceAgcDcbGetLodStatsGetSize)                                             \
    X("xSAR0LTcRKM", sceAgcDcbJump)                                                           \
    X("VEGu4dixjUg", sceAgcDcbJumpGetSize)                                                    \
    X("G0jrLdvEqDw", sceAgcDcbMemSemaphore)                                                   \
    X("H7uZqCoNuWk", sceAgcDcbPopMarker)                                                      \
    X("jt3pl7EN17o", sceAgcDcbPrimeUtcl2)                                                     \
    X("KjPeVduz6jU", sceAgcDcbPrimeUtcl2GetSize)                                              \
    X("+kSrjIVxKFE", sceAgcDcbPushMarker)                                                     \
    X("zg6u-N6Otxs", sceAgcDcbQueueEndOfShaderActionGetSize)                                  \
    X("zfcxg-ewMK8", sceAgcDcbRewind)                                                         \
    X("QIXCsbipds0", sceAgcDcbRewindGetSize)                                                  \
    X("9S4noWrUI0s", sceAgcDcbSetBaseDispatchIndirectArgsGetSize)                             \
    X("MMlmJAL7N5w", sceAgcDcbSetBaseDrawIndirectArgsGetSize)                                 \
    X("RmaJwLtc8rY", sceAgcDcbSetBaseIndirectArgs)                                            \
    X("yheJGN-ay+A", sceAgcDcbSetBoolPredicationEnableGetSize)                                \
    X("73ZZdojLIgs", sceAgcDcbSetCfRegisterDirect)                                            \
    X("BVFg3CWU6Eo", sceAgcDcbSetCfRegisterRangeDirect)                                       \
    X("LHFXRrlTPD8", sceAgcDcbSetCxRegisterDirect)                                            \
    X("1DeUNpRIDDA", sceAgcDcbSetCxRegisterDirectGetSize)                                     \
    X("GBCh3zCihoU", sceAgcDcbSetCxRegistersIndirectGetSize)                                  \
    X("8N2tmT3jmC8", sceAgcDcbSetIndexCount)                                                  \
    X("mljzuGDZRQ4", sceAgcDcbSetIndexCountGetSize)                                           \
    X("0o3VDdtA6nM", sceAgcDcbSetIndexIndirectArgs)                                           \
    X("AFIh8SQkYlQ", sceAgcDcbSetIndexIndirectArgsGetSize)                                    \
    X("ca4KPvp0qLQ", sceAgcDcbSetIndexSizeGetSize)                                            \
    X("QhCbS4X9Rl8", sceAgcDcbSetMarker)                                                      \
    X("tSBxhAPyytQ", sceAgcDcbSetNumInstances)                                                \
    X("6DFuRKT4C9w", sceAgcDcbSetNumInstancesGetSize)                                         \
    X("bbFueFP+J4k", sceAgcDcbSetPredication)                                                 \
    X("vLrBL8DQiz8", sceAgcDcbSetPredicationDisableGetSize)                                   \
    X("pFLArOT53+w", sceAgcDcbSetShRegisterDirect)                                            \
    X("QhPDD513V0w", sceAgcDcbSetShRegisterDirectGetSize)                                     \
    X("nNlUtdDDvZ0", sceAgcDcbSetShRegistersIndirectGetSize)                                  \
    X("w4-d0n60hdo", sceAgcDcbSetUcRegisterDirect)                                            \
    X("aP1Ki9G3++4", sceAgcDcbSetUcRegisterDirectGetSize)                                     \
    X("UQGTw4xRlcM", sceAgcDcbSetUcRegistersIndirectGetSize)                                  \
    X("hEK26Wdny6s", sceAgcDcbSetWorkloadComplete)                                            \
    X("LFSPFmGc9Hg", sceAgcDcbSetWorkloadsActive)                                             \
    X("XN+Iuu7XsM8", sceAgcDcbSetZPassPredicationEnableGetSize)                               \
    X("u2T2DiA5hRI", sceAgcDcbStallCommandBufferParser)                                       \
    X("+u6dKSLWM2o", sceAgcDcbStallCommandBufferParserGetSize)                                \
    X("43WJ08sSugE", sceAgcDcbWaitOnAddressGetSize)                                           \
    X("VmW0Tdpy420", sceAgcDcbWaitRegMem)                                                     \
    X("MWiElSNE8j8", sceAgcDcbWaitUntilSafeForRendering)                                      \
    X("i1jyy49AjXU", sceAgcDcbWriteData)                                                      \
    X("p9tI+yTvx68", sceAgcDcbWriteDataGetSize)                                               \
    X("IxYiarKlXxM", sceAgcDmaDataPatchSetDstAddressOrOffset)                                 \
    X("cdDRpqcFGbU", sceAgcDmaDataPatchSetSrcAddressOrOffsetOrImmediate)                      \
    X("nApJjpKNBl4", sceAgcFuseShaderHalves)                                                  \
    X("nQT5kYLv0cg", sceAgcGetFusedShaderSize)                                                \
    X("Lkf86B98qPc", sceAgcGetPacketSize)                                                     \
    X("Wi82ArQtAwg", sceAgcGetRegisterDefaults)                                               \
    X("uIwxsqDlHRc", sceAgcGetRegisterDefaultsInternal)                                       \
    X("kW3GLb7QfPg", sceAgcInit)                                                              \
    X("2BS4EtAaF28", sceAgcJumpPatchSetTarget)                                                \
    X("MqAdbRMdNz4", sceAgcLinkShaders)                                                       \
    X("0fWWK5uG9rQ", sceAgcQueueEndOfPipeActionPatchAddress)                                  \
    X("MlEw1feXcjg", sceAgcQueueEndOfPipeActionPatchData)                                     \
    X("J8YCgfKAMQs", sceAgcQueueEndOfPipeActionPatchGcrCntl)                                  \
    X("T9fjQIINoeE", sceAgcQueueEndOfPipeActionPatchType)                                     \
    X("ziVA3whp3p4", sceAgcRewindPatchSetRewindState)                                         \
    X("whb1RL7K4Ss", sceAgcSetCxRegIndirectPatchSetNumRegisters)                              \
    X("K2mciNVxUCE", sceAgcSetNop)                                                            \
    X("w6Dj1VJt5qY", sceAgcSetPacketPredication)                                              \
    X("n8vgpaQg6dA", sceAgcSetRangePredication)                                               \
    X("nCUgItdN2ms", sceAgcSetShRegIndirectPatchSetNumRegisters)                              \
    X("-DtvmQ-tgEA", sceAgcSetSubmitMode)                                                     \
    X("fRG-JOH5+sI", sceAgcSetUcRegIndirectPatchSetNumRegisters)                              \
    X("b+fis+WZ3Ig", sceAgcSuspendPointAndCheckStatus)                                        \
    X("SbuY2jN+axQ", sceAgcUpdateInterpolantMapping)                                          \
    X("Y3ymLfZ1384", sceAgcUpdatePrimState)                                                    \
    X("3KDcnM3lrcU", sceAgcWaitRegMemPatchAddress)                                            \
    X("n485EBnIWmk", sceAgcWaitRegMemPatchCompareFunction)                                    \
    X("hXAnLgDHCoI", sceAgcWaitRegMemPatchMask)                                               \
    X("7nOoijNPvEU", sceAgcWaitRegMemPatchReference)

#define AGC_DRIVER_FUNCTIONS                                                                   \
    X("MetMOQVd8HY", sceAgcDriverAcquireRazorACQ)                                             \
    X("w2rJhmD+dsE", sceAgcDriverAddEqEvent)                                                  \
    X("AhGvpITrf4M", sceAgcDriverAgrSubmitDcb)                                                \
    X("+T8Xo6LtFJI", sceAgcDriverAgrSubmitMultiDcbs)                                          \
    X("zP4ZNlXLBVg", sceAgcDriverCreateQueue)                                                 \
    X("1DXIHxWHZAQ", sceAgcDriverCwsrResumeAcq)                                               \
    X("SOAMmdlyaIc", sceAgcDriverCwsrSuspendAcq)                                              \
    X("1BUTwixUG5Y", sceAgcDriverDebugHardwareStatus)                                         \
    X("DL2RXaXOy88", sceAgcDriverDeleteEqEvent)                                               \
    X("XNbrdwCsZ9A", sceAgcDriverDestroyQueue)                                                \
    X("5l3IfCFJxBs", sceAgcDriverFindResourcesPublic)                                        \
    X("Zw7uUVPulbw", sceAgcDriverGetEqContextId)                                              \
    X("5CdQTZIQPxM", sceAgcDriverGetEqEventType)                                              \
    X("g68eYcZS7PY", sceAgcDriverGetGpuRefClks)                                               \
    X("r28hEh6cNH0", sceAgcDriverGetHsOffchipParam)                                          \
    X("LepGrgk77sM", sceAgcDriverGetOwnerName)                                                \
    X("Pqxglq1oKec", sceAgcDriverGetPaDebugInterfaceVersion)                                  \
    X("CP-kVAMmWVw", sceAgcDriverGetRegShadowInfo)                                           \
    X("ME1eUot7+Qw", sceAgcDriverGetRegShadowInfoAgr)                                        \
    X("Um-jkyDy9rI", sceAgcDriverGetReservedDmemForAgc)                                      \
    X("NghWEUXp1qM", sceAgcDriverGetResourceBaseAddressAndSizeInBytes)                        \
    X("M9yBzRKkjPc", sceAgcDriverGetResourceName)                                             \
    X("mXn+K9E-wOA", sceAgcDriverGetResourceShaderGuid)                                      \
    X("rI9lNAXPMIw", sceAgcDriverGetResourceType)                                            \
    X("ls4jfY576lw", sceAgcDriverGetResourceUserData)                                        \
    X("2PrsbRYyZi4", sceAgcDriverGetSetFlipPacketSizeInDwords)                                \
    X("WNyjOWq8-Vk", sceAgcDriverGetSetWorkloadCompletePacketSize)                            \
    X("gyVTZWyySpM", sceAgcDriverGetSetWorkloadsActivePacketSize)                             \
    X("e-YMQ+2tj9M", sceAgcDriverGetTFRing)                                                   \
    X("xDPdCurOujQ", sceAgcDriverGetTraceInitiator)                                           \
    X("0MtUJ3BpGhE", sceAgcDriverGetWaitRenderingPacketSizeInDwords)                          \
    X("t8PLXbBCiRA", sceAgcDriverGetWorkloadStreamInfo)                                       \
    X("C2yjkNdzbW4", sceAgcDriverIDHSSubmit)                                                  \
    X("F0Y42t-3e18", sceAgcDriverInitResourceRegistration)                                    \
    X("Ddwk4gLT5j0", sceAgcDriverIsCaptureInProgress)                                        \
    X("qspAL8bgcBY", sceAgcDriverIsSubmitValidationEnabled)                                   \
    X("+TN0oRTBxJQ", sceAgcDriverIsTraceInProgress)                                           \
    X("04JRU1Uf8Ms", sceAgcDriverModuleRegistration)                                          \
    X("nR6xhiFsOoc", sceAgcDriverNotifyDefaultStates)                                         \
    X("aCfbPzyjU90", sceAgcDriverPassInfoDownward)                                            \
    X("lYz7vbL4W4A", sceAgcDriverPatchClearState)                                             \
    X("AOLcoIkQDgM", sceAgcDriverQueryResourceRegistrationUserMemoryRequirements)             \
    X("emP3ckeS2uo", sceAgcDriverRegisterGdsResource)                                         \
    X("X-Nm5KLREeg", sceAgcDriverRegisterOwner)                                               \
    X("W5z4eZrjEas", sceAgcDriverRegisterResource)                                            \
    X("3AyTaWcF-H8", sceAgcDriverRegisterWorkloadStream)                                      \
    X("SAfhzJPcjuk", sceAgcDriverRequestCaptureStart)                                         \
    X("FOwvmNlFLjM", sceAgcDriverRequestCaptureStop)                                          \
    X("cwbxjPSJ7WQ", sceAgcDriverSetFlip)                                                     \
    X("MM4IZSEYytQ", sceAgcDriverSetHsOffchipParam)                                           \
    X("DPcAnsOlTQs", sceAgcDriverSetHsOffchipParamDirect)                                     \
    X("VOMSpd9+vxU", sceAgcDriverSetResourceUserData)                                         \
    X("XlNp7jzGiPo", sceAgcDriverSetTFRing)                                                   \
    X("i6bfTi13ApA", sceAgcDriverSetWorkloadComplete)                                         \
    X("UM9b9NunSrE", sceAgcDriverSetWorkloadsActive)                                          \
    X("Vlaj1gwmIFA", sceAgcDriverSetupAsyncGraphics)                                          \
    X("Hj4eWnDektQ", sceAgcDriverSetupRegisterShadow)                                         \
    X("gSRnr79F8tQ", sceAgcDriverSubmitAcb)                                                   \
    X("b4fpgH5ZXxQ", sceAgcDriverSubmitCommandBuffer)                                         \
    X("HF3YllT3mXU", sceAgcDriverSubmitMultiAcbs)                                             \
    X("Fj7r9EHzF38", sceAgcDriverSubmitMultiCommandBuffers)                                   \
    X("xmWi73o1BR0", sceAgcDriverSubmitMultiCommandBuffersDirect)                             \
    X("6UzEidRZwkg", sceAgcDriverSubmitMultiDcbs)                                             \
    X("lOYHtoUcJD4", sceAgcDriverSubmitToHDRScopesACQ)                                        \
    X("jJyVJyhi5h8", sceAgcDriverSubmitToRazorACQ)                                            \
    X("QcmHLO2n7mk", sceAgcDriverSuspendPointSubmit)                                          \
    X("ZV04pRl7cWU", sceAgcDriverSuspendPointSubmitDirect)                                    \
    X("x3K61sY5m8Q", sceAgcDriverSysEnableSubmitDone45Exception)                              \
    X("kuE1uTiWfuk", sceAgcDriverSysGetClientNumber)                                          \
    X("ftf-xlfBQpo", sceAgcDriverSysIsGameClosed)                                             \
    X("BbI8si4o-fA", sceAgcDriverSysSubmitFlipHandleProxy)                                    \
    X("UM8rn9hRWrY", sceAgcDriverTmpInitIdhs)                                                 \
    X("Xq5WmbwPTnQ", sceAgcDriverTriggerCapture)                                              \
    X("SCoAN5fYlUM", sceAgcDriverUnregisterAllResourcesForOwner)                              \
    X("ZLJk9r2+2Aw", sceAgcDriverUnregisterOwnerAndResources)                                 \
    X("pWLG7WOpVcw", sceAgcDriverUnregisterResource)                                          \
    X("n5ElQVYsU1A", sceAgcDriverUnregisterWorkloadStream)                                    \
    X("VhLnEiTuuWo", sceAgcDriverUserDataGetPacketSize)                                       \
    X("t30LG1ibIJE", sceAgcDriverUserDataImmediateWrite)                                      \
    X("-vc-xL+G8u0", sceAgcDriverUserDataWritePacket)                                         \
    X("LEnn-4ARRJM", sceAgcDriverUserDataWritePopMarker)                                      \
    X("+b34-CLWc0s", sceAgcDriverUserDataWritePushMarker)                                     \
    X("zmw2uVSEj94", sceAgcDriverUserDataWriteSetMarker)                                      \
    X("u8BkdHb1+Po", sceAgcDriverWaitUntilSafeForRendering)                                   \
    X("OYBiWgeGpPo", sceAgcSdmaClose)                                                         \
    X("CmlLiND79W8", sceAgcSdmaConstFill)                                                     \
    X("7HmaD1MaWQs", sceAgcSdmaCopyLinear)                                                    \
    X("DBZMYLxXRYA", sceAgcSdmaCopyTiledBC)                                                   \
    X("hBsEOmOR3qQ", sceAgcSdmaCopyTiledGen2)                                                 \
    X("oWyZFLUVjcI", sceAgcSdmaCopyWindowBC)                                                  \
    X("f4aTpX9UhD0", sceAgcSdmaCopyWindowGen2)                                                \
    X("GOP1R6vumsg", sceAgcSdmaFlush)                                                         \
    X("aYySApdZmtE", sceAgcSdmaOpen)                                                          \
    X("scA1QSh8cfE", sceAgcSdmaSetTileModesBC)

#define X(nid, function)                                                                        \
    s32 PS4_SYSV_ABI function() {                                                               \
        LOG_ERROR(Lib_Agc, "{} (STUBBED) called", #function);                                  \
        return ORBIS_OK;                                                                        \
    }
AGC_FUNCTIONS
AGC_DRIVER_FUNCTIONS
#undef X

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
#define X(nid, function) LIB_FUNCTION(nid, "libSceAgc", 1, "libSceAgc", function);
    AGC_FUNCTIONS
#undef X
    LIB_FUNCTION("2JtWUUiYBXs", "libSceAgc", 1, "libSceAgc", sceAgcGetRegisterDefaults2);
    LIB_FUNCTION("wRbq6ZjNop4", "libSceAgc", 1, "libSceAgc", sceAgcGetRegisterDefaults2Internal);
    LIB_FUNCTION("23LRUSvYu1M", "libSceAgc", 1, "libSceAgc", sceAgcInit2Maybe);
    LIB_FUNCTION("f3dg2CSgRKY", "libSceAgc", 1, "libSceAgc", sceAgcCreateShader);
    LIB_FUNCTION("pdEV7bI6COI", "libSceAgc", 1, "libSceAgc", sceAgcCreateInterpolantMapping);
    LIB_FUNCTION("HV4j+E0MBHE", "libSceAgc", 1, "libSceAgc", sceAgcCreateInterpolantMapping);
    LIB_FUNCTION("CQsSq6l6+kA", "libSceAgc", 1, "libSceAgc", sceAgcGetDataPacketPayloadAddress);
    LIB_FUNCTION("V++UgBtQhn0", "libSceAgc", 1, "libSceAgc", sceAgcGetDataPacketPayloadAddress);
    LIB_FUNCTION("ZvwO9euwYzc", "libSceAgc", 1, "libSceAgc", sceAgcDcbSetCxRegistersIndirect);
    LIB_FUNCTION("vcmNN+AAXnY", "libSceAgc", 1, "libSceAgc",
                 sceAgcSetCxRegIndirectPatchSetAddress);
    LIB_FUNCTION("d-6uF9sZDIU", "libSceAgc", 1, "libSceAgc",
                 sceAgcSetCxRegIndirectPatchAddRegisters);
    LIB_FUNCTION("D9sr1xGUriE", "libSceAgc", 1, "libSceAgc", sceAgcCreatePrimState);
    LIB_FUNCTION("n2fD4A+pb+g", "libSceAgc", 1, "libSceAgc",
                 sceAgcCbSetShRegisterRangeDirect);
    LIB_FUNCTION("TRO721eVt4g", "libSceAgc", 1, "libSceAgc", sceAgcDcbResetQueue);
    LIB_FUNCTION("-HOOCn0JY48", "libSceAgc", 1, "libSceAgc",
                 sceAgcDcbSetShRegistersIndirect);
    LIB_FUNCTION("Qrj4c+61z4A", "libSceAgc", 1, "libSceAgc",
                 sceAgcSetShRegIndirectPatchSetAddress);
    LIB_FUNCTION("z2duB-hHQSM", "libSceAgc", 1, "libSceAgc",
                 sceAgcSetShRegIndirectPatchAddRegisters);
    LIB_FUNCTION("YUeqkyT7mEQ", "libSceAgc", 1, "libSceAgc", sceAgcDcbSetFlip);
    LIB_FUNCTION("hvUfkUIQcOE", "libSceAgc", 1, "libSceAgc",
                 sceAgcDcbSetUcRegistersIndirect);
    LIB_FUNCTION("6lNcCp+fxi4", "libSceAgc", 1, "libSceAgc",
                 sceAgcSetUcRegIndirectPatchSetAddress);
    LIB_FUNCTION("vRoArM9zaIk", "libSceAgc", 1, "libSceAgc",
                 sceAgcSetUcRegIndirectPatchAddRegisters);
    LIB_FUNCTION("h9z6+0hEydk", "libSceAgc", 1, "libSceAgc", sceAgcSuspendPoint);
    LIB_FUNCTION("B+aG9DUnTKA", "libSceAgc", 1, "libSceAgc", sceAgcDcbDrawIndexOffset);
    LIB_FUNCTION("l4fM9K-Lyks", "libSceAgc", 1, "libSceAgc", sceAgcDcbSetIndexBuffer);
    LIB_FUNCTION("j4emHHndCPY", "libSceAgc", 1, "libSceAgc", sceAgcDcbSetIndexBufferGetSize);
    LIB_FUNCTION("GIIW2J37e70", "libSceAgc", 1, "libSceAgc", sceAgcDcbSetIndexSize);
#define X(nid, function) LIB_FUNCTION(nid, "libSceAgcDriver", 1, "libSceAgcDriver", function);
    AGC_DRIVER_FUNCTIONS
#undef X
    LIB_FUNCTION("UglJIZjGssM", "libSceAgcDriver", 1, "libSceAgcDriver",
                 sceAgcDriverSubmitDcb);
}

#undef AGC_FUNCTIONS
#undef AGC_DRIVER_FUNCTIONS

} // namespace Libraries::Agc
