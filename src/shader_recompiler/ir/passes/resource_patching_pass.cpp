// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <limits>
#include <unordered_map>
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/dominance.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/operand_helper.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/ir/reinterpret.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/resource.h"
#include "video_core/amdgpu/resource.h"

namespace Shader::Optimization {
namespace {

IR::Type BufferDataType(const IR::Inst& inst, const Profile& profile) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadBufferU8:
    case IR::Opcode::StoreBufferU8:
        return IR::Type::U8;
    case IR::Opcode::LoadBufferU16:
    case IR::Opcode::StoreBufferU16:
        return IR::Type::U16;
    case IR::Opcode::LoadBufferU64:
    case IR::Opcode::StoreBufferU64:
    case IR::Opcode::BufferAtomicIAdd64:
    case IR::Opcode::BufferAtomicSMax64:
    case IR::Opcode::BufferAtomicSMin64:
    case IR::Opcode::BufferAtomicUMax64:
    case IR::Opcode::BufferAtomicUMin64:
        return IR::Type::U64;
    case IR::Opcode::BufferAtomicFMax32:
    case IR::Opcode::BufferAtomicFMin32:
        return profile.supports_buffer_fp32_atomic_min_max ? IR::Type::F32 : IR::Type::U32;
    case IR::Opcode::LoadBufferFormatF32:
    case IR::Opcode::StoreBufferFormatF32:
        // Formatted buffer loads can use a variety of types.
        return IR::Type::U32 | IR::Type::F32 | IR::Type::U16 | IR::Type::U8;
    default:
        return IR::Type::U32;
    }
}

u32 BufferAddressShift(const IR::Inst& inst, AmdGpu::DataFormat data_format) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadBufferU8:
    case IR::Opcode::StoreBufferU8:
        return 0;
    case IR::Opcode::LoadBufferU16:
    case IR::Opcode::StoreBufferU16:
        return 1;
    case IR::Opcode::LoadBufferU64:
    case IR::Opcode::StoreBufferU64:
    case IR::Opcode::BufferAtomicIAdd64:
    case IR::Opcode::BufferAtomicSMax64:
    case IR::Opcode::BufferAtomicSMin64:
    case IR::Opcode::BufferAtomicUMax64:
    case IR::Opcode::BufferAtomicUMin64:
        return 3;
    case IR::Opcode::LoadBufferFormatF32:
    case IR::Opcode::StoreBufferFormatF32: {
        switch (data_format) {
        case AmdGpu::DataFormat::Format8:
            return 0;
        case AmdGpu::DataFormat::Format8_8:
        case AmdGpu::DataFormat::Format16:
            return 1;
        case AmdGpu::DataFormat::Format8_8_8_8:
        case AmdGpu::DataFormat::Format16_16:
        case AmdGpu::DataFormat::Format10_11_11:
        case AmdGpu::DataFormat::Format2_10_10_10:
        case AmdGpu::DataFormat::Format16_16_16_16:
        case AmdGpu::DataFormat::Format32:
        case AmdGpu::DataFormat::Format32_32:
        case AmdGpu::DataFormat::Format32_32_32:
        case AmdGpu::DataFormat::Format32_32_32_32:
            return 2;
        default:
            return 0;
        }
        break;
    }
    case IR::Opcode::ReadConstBuffer:
        // Provided address is already in dwords
        return 0;
    default:
        return 2;
    }
}

class Descriptors {
public:
    explicit Descriptors(Info& info_)
        : info{info_}, buffer_resources{info_.buffers}, image_resources{info_.images},
          sampler_resources{info_.samplers}, fmask_resources(info_.fmasks) {}

    u32 Add(const BufferResource& desc) {
        const u32 index{Add(buffer_resources, desc, [&desc](const auto& existing) {
            return desc.sharp_fetch == existing.sharp_fetch && desc.post_op == existing.post_op &&
                   desc.post_op_dw1_mask == existing.post_op_dw1_mask &&
                   desc.buffer_type == existing.buffer_type;
        })};
        auto& buffer = buffer_resources[index];
        buffer.used_types |= desc.used_types;
        buffer.is_written |= desc.is_written;
        buffer.is_formatted |= desc.is_formatted;
        return index;
    }

    u32 Add(const ImageResource& desc) {
        const u32 index{Add(image_resources, desc, [&desc](const auto& existing) {
            return desc.sharp_fetch == existing.sharp_fetch && desc.is_array == existing.is_array &&
                   desc.mip_fallback_mode == existing.mip_fallback_mode &&
                   desc.constant_mip_index == existing.constant_mip_index &&
                   desc.post_op == existing.post_op;
        })};
        auto& image = image_resources[index];
        image.is_atomic |= desc.is_atomic;
        image.is_written |= desc.is_written;
        return index;
    }

    u32 Add(const SamplerResource& desc) {
        const u32 index{Add(sampler_resources, desc, [this, &desc](const auto& existing) {
            return desc.sharp_fetch == existing.sharp_fetch && desc.post_op == existing.post_op &&
                   desc.post_op_tsharp_dw3_off == existing.post_op_tsharp_dw3_off &&
                   desc.is_depth == existing.is_depth;
        })};
        return index;
    }

    u32 Add(const FMaskResource& desc) {
        u32 index = Add(fmask_resources, desc, [&desc](const auto& existing) {
            return desc.sharp_idx == existing.sharp_idx;
        });
        return index;
    }

private:
    template <typename Descriptors, typename Descriptor, typename Func>
    static u32 Add(Descriptors& descriptors, const Descriptor& desc, Func&& pred) {
        const auto it{std::ranges::find_if(descriptors, pred)};
        if (it != descriptors.end()) {
            return static_cast<u32>(std::distance(descriptors.begin(), it));
        }
        descriptors.push_back(desc);
        return static_cast<u32>(descriptors.size()) - 1;
    }

    const Info& info;
    BufferResourceList& buffer_resources;
    ImageResourceList& image_resources;
    SamplerResourceList& sampler_resources;
    FMaskResourceList& fmask_resources;
};

} // Anonymous namespace

using SharpSources = boost::container::small_vector<const IR::Inst*, 4>;

SharpLocation SharpLocationFromSource(const IR::Inst* inst, bool warn = true) {
    SharpLocation location{};
    if (inst->GetOpcode() == IR::Opcode::GetUserData) {
        return static_cast<SharpLocation>(inst->Arg(0).ScalarReg());
    } else if (inst->GetOpcode() == IR::Opcode::ReadConstBuffer) {
        location = inst->Flags<IR::BufferInstInfo>().flatbuf_off_dw;
    } else {
        location = inst->Flags<SharpLocation>();
    }
    if (location == 0) {
        if (warn) {
            LOG_WARNING(Render_Recompiler, "Sharp source was not flatenned");
        }
        return UNKNOWN_LOCATION;
    }
    return location;
}

template <typename T>
SharpFetch<T> ConstructSharpFetch(const SharpReference& sharp, bool warn = true) {
    using Summary = SharpFetch<T>::Summary;
    SharpFetch<T> sharp_fetch{};
    for (u32 i = 0; i < sharp.num_dwords; i++) {
        auto dword = sharp.dwords[i];
        if (dword.IsImmediate()) {
            sharp_fetch.immediates[i] = dword.U32();
        } else {
            sharp_fetch.offsets[i] = SharpLocationFromSource(dword.Inst(), warn);
            sharp_fetch.load_mask |= (1 << i);
            if (sharp_fetch.offsets[i] == UNKNOWN_LOCATION) {
                sharp_fetch.summary = Summary::Invalid;
            }
        }
    }
    if (sharp_fetch.summary != Summary::Invalid) {
        const u32 base = sharp_fetch.offsets[0];
        for (u32 i = 1; i < sharp.num_dwords; ++i) {
            if (sharp_fetch.offsets[i] - base != i) {
                return sharp_fetch;
            }
        }
        sharp_fetch.summary = Summary::SingleLoad;
    }
    return sharp_fetch;
}

void PatchBufferSharp(const ResourceDiscovery& resource, Info& info, Descriptors& descriptors,
                      const Profile& profile) {
    IR::Inst& inst = *resource.user;

    const u32 buffer_binding = descriptors.Add(BufferResource{
        .sharp_fetch = ConstructSharpFetch<AmdGpu::Buffer>(resource.sharps[0]),
        .used_types = BufferDataType(inst, profile),
        .buffer_type = BufferType::Guest,
        .is_written = IsBufferStore(inst),
        .is_formatted = inst.GetOpcode() == IR::Opcode::LoadBufferFormatF32 ||
                        inst.GetOpcode() == IR::Opcode::StoreBufferFormatF32,
        .post_op = resource.sharps[0].post_op,
        .post_op_dw1_mask = resource.sharps[0].post_op_data.dw1_mask,
    });

    // Replace handle with binding index in buffer resource list.
    IR::IREmitter ir{*inst.GetParent(), IR::Block::InstructionList::s_iterator_to(inst)};
    inst.SetArg(0, ir.Imm32(buffer_binding));
}

void PatchImageSharp(const ResourceDiscovery& resource, const std::array<u8, 2>& guards, Info& info,
                     Descriptors& descriptors, const Profile& profile) {
    IR::Inst& inst = *resource.user;

    // Read image sharp.
    const auto inst_info = inst.Flags<IR::TextureInstInfo>();
    const bool is_atomic = IsImageAtomicInstruction(inst);
    const bool is_written = inst.GetOpcode() == IR::Opcode::ImageWrite || is_atomic;
    // ImageRead with !is_written gets emitted as OpImageFetch with LOD operand, doesn't
    // need fallback (TODO is this 100% true?)
    const bool needs_mip_storage_fallback =
        inst_info.has_lod && is_written && !profile.supports_image_load_store_lod;
    ImageResource image_res = {
        .sharp_fetch = ConstructSharpFetch<AmdGpu::Image>(resource.sharps[0]),
        .is_depth = bool(inst_info.is_depth),
        .is_atomic = is_atomic,
        .is_array = bool(inst_info.is_array),
        .is_written = is_written,
        .is_r128 = bool(inst_info.is_r128),
        .post_op = resource.sharps[0].post_op,
        .guard = guards[0],
    };

    auto image = image_res.GetSharp(info);
    ASSERT(image.GetType() != AmdGpu::ImageType::Invalid);

    if (needs_mip_storage_fallback) {
        // If the mip level to IMAGE_(LOAD/STORE)_MIP is a constant, set up ImageResource
        // so that we will only bind a single level.
        // If index is dynamic, we will bind levels as an array
        const auto view_type = image.GetViewType(image_res.is_array);

        IR::Inst* body = inst.Arg(1).Inst();
        const auto lod_arg = [&] -> IR::Value {
            switch (view_type) {
            case AmdGpu::ImageType::Color1D: // x, [lod]
                return body->Arg(1);
            case AmdGpu::ImageType::Color1DArray: // x, slice, [lod]
            case AmdGpu::ImageType::Color2D:      // x, y, [lod]
                return body->Arg(2);
            case AmdGpu::ImageType::Color2DArray: // x, y, slice, [lod]
            case AmdGpu::ImageType::Cube:         // x, y, face, [lod]
            case AmdGpu::ImageType::Color3D:      // x, y, z, [lod]
                return body->Arg(3);
            case AmdGpu::ImageType::Color2DMsaa:
            case AmdGpu::ImageType::Color2DMsaaArray:
            default:
                UNREACHABLE_MSG("Invalid image type {}", view_type);
            }
        }();

        if (lod_arg.IsImmediate()) {
            image_res.mip_fallback_mode = MipStorageFallbackMode::ConstantIndex;
            image_res.constant_mip_index = lod_arg.U32();
        } else {
            image_res.mip_fallback_mode = MipStorageFallbackMode::DynamicIndex;
        }
    }

    // Patch image instruction if image is FMask.
    if (AmdGpu::IsFmask(image.GetDataFmt())) {
        ASSERT_MSG(!is_written, "FMask storage instructions are not supported");

        IR::IREmitter ir{*inst.GetParent(), IR::Block::InstructionList::s_iterator_to(inst)};
        switch (inst.GetOpcode()) {
        case IR::Opcode::ImageRead:
        case IR::Opcode::ImageSampleRaw: {
            IR::F32 fmaskx = ir.BitCast<IR::F32>(ir.Imm32(0x76543210));
            IR::F32 fmasky = ir.BitCast<IR::F32>(ir.Imm32(0xfedcba98));
            inst.ReplaceUsesWith(ir.CompositeConstruct(fmaskx, fmasky));
            return;
        }
        case IR::Opcode::ImageQueryLod:
            inst.ReplaceUsesWith(ir.Imm32(1));
            return;
        case IR::Opcode::ImageQueryDimensions: {
            IR::Value dims = ir.CompositeConstruct(ir.Imm32(static_cast<u32>(image.width)),  // x
                                                   ir.Imm32(static_cast<u32>(image.height)), // y
                                                   ir.Imm32(1), ir.Imm32(1)); // depth, mip
            inst.ReplaceUsesWith(dims);

            // Track FMask resource to do specialization.
            descriptors.Add(FMaskResource{
                .sharp_idx = SharpLocationFromSource(resource.sharps[0].dwords[0].Inst()),
                .guard = guards[0],
            });
            return;
        }
        default:
            UNREACHABLE_MSG("Can't patch fmask instruction {}", inst.GetOpcode());
        }
    }

    u32 image_binding = descriptors.Add(image_res);

    IR::IREmitter ir{*inst.GetParent(), IR::Block::InstructionList::s_iterator_to(inst)};

    if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
        auto& lod_prod = resource.sharps[1].post_op_data.lod_prod;
        const u32 sampler_binding = descriptors.Add(SamplerResource{
            .sharp_fetch = ConstructSharpFetch<AmdGpu::Sampler>(resource.sharps[1]),
            .post_op = resource.sharps[1].post_op,
            .post_op_tsharp_dw3_off =
                lod_prod.IsEmpty() ? UNKNOWN_LOCATION : SharpLocationFromSource(lod_prod.Inst()),
            .is_depth = bool(inst_info.is_depth), // true for the _C (compare) opcodes
            .guard = guards[1],
        });
        inst.SetArg(0, ir.Imm32(image_binding | sampler_binding << 16));
    } else {
        inst.SetArg(0, ir.Imm32(image_binding));
    }
}

static IR::Value IsMbcntWithExec(IR::Value value, bool is_hi, IR::U1 exec) {
    if (value.IsImmediate()) {
        return {};
    }
    const IR::Inst* inst = value.Inst();
    if (inst->GetOpcode() != IR::Opcode::MaskedBitCount32 || inst->Arg(0).IsImmediate() ||
        inst->Arg(2).U1() != is_hi) {
        return {};
    }
    IR::Inst* prod = inst->Arg(0).Inst();
    if (prod->GetOpcode() != IR::Opcode::CompositeExtractU32x2 || prod->Arg(0).IsImmediate() ||
        prod->Arg(1).U32() != is_hi) {
        return {};
    }
    prod = prod->Arg(0).Inst();
    if (prod->GetOpcode() != IR::Opcode::UnpackUint2x32 || prod->Arg(0).IsImmediate()) {
        return {};
    }
    prod = prod->Arg(0).Inst();
    if (prod->GetOpcode() != IR::Opcode::Ballot || (prod->Arg(0) != exec && !is_hi)) {
        return {};
    }
    return inst->Arg(1);
}

static IR::Use FindUniqueUser(IR::Inst* inst, auto&& pred) {
    IR::Use picked{};
    for (auto& use : inst->Uses()) {
        if (pred(use.user)) {
            ASSERT(!picked.user);
            picked = use;
        }
    }
    ASSERT(picked.user);
    return picked;
}

static void RemoveAppendBufferLaneOffset(IR::Inst& vx) {
    // Attempt to detect either of the following patterns and
    // remove the lane id addition/subtraction to ds instruction.
    //
    // ds_append       vX gds
    // v_mbcnt_hi_u32_b32 vY, exec_hi, vX
    // v_mbcnt_lo_u32_b32 idx, exec_lo, vY
    //
    // v_mbcnt_hi_u32_b32 vY, exec_hi, 0
    // v_mbcnt_lo_u32_b32 vZ, exec_lo, vY
    // ds_append       vX gds
    // v_add_i32       idx, vcc, vX, vZ
    //
    // ds_consume      vX gds
    // v_mbcnt_hi_u32_b32 vY, exec_hi, 0
    // v_mbcnt_lo_u32_b32 vZ, exec_lo, vY
    // v_sub_i32       vK, vcc, vX, vZ
    // v_subrev_i32    idx, vcc, 1, vK / v_add_i32    idx, vcc, -1, vK
    //
    // ds_consume      vX gds
    // v_mbcnt_hi_u32_b32 vY, exec_hi, 0
    // v_mbcnt_lo_u32_b32 vZ, exec_lo, vY
    // v_add_i32       vK, vcc, -1, vX
    // v_sub_i32       idx, vcc, vK, vZ

    IR::U1 exec{vx.Arg(1)};
    const auto uses = vx.Uses();
    for (auto use : uses) {
        const auto& [user, operand] = use;
        if (user->GetOpcode() == IR::Opcode::MaskedBitCount32) {
            // First pattern
            ASSERT(vx.GetOpcode() == IR::Opcode::DataAppend);
            IR::Inst* vy = user;
            auto vx_2 = IsMbcntWithExec(IR::Value{vy}, true, exec);
            ASSERT(!vx_2.IsEmpty() && vx_2 == IR::Value{&vx});
            const auto [idx, operand] = FindUniqueUser(vy, [](const IR::Inst* inst) {
                return inst->GetOpcode() == IR::Opcode::MaskedBitCount32;
            });
            auto vy_2 = IsMbcntWithExec(IR::Value{idx}, false, exec);
            ASSERT(!vy_2.IsEmpty() && vy_2 == IR::Value{vy});
            idx->ReplaceUsesWithAndRemove(IR::Value{&vx});
            continue;
        }
        if (user->GetOpcode() != IR::Opcode::IAdd32 && user->GetOpcode() != IR::Opcode::ISub32) {
            continue;
        }
        const auto other = user->Arg(1 - operand);
        if (other.IsImmediate()) {
            ASSERT_MSG(other.U32() == 1u || other.U32() == std::numeric_limits<u32>::max() &&
                                                vx.GetOpcode() == IR::Opcode::DataConsume,
                       "Unexpected constant offset {} to DataConsume result", other.U32());
            use = FindUniqueUser(user, [](const IR::Inst* inst) {
                return inst->GetOpcode() == IR::Opcode::IAdd32 ||
                       inst->GetOpcode() == IR::Opcode::ISub32;
            });
        }

        // Other patterns
        auto vz = user->Arg(1 - operand);
        auto vy = IsMbcntWithExec(vz, false, exec);
        ASSERT(!vy.IsEmpty() && !vy.IsImmediate());
        auto zero_const = IsMbcntWithExec(vy, true, exec);
        ASSERT(!zero_const.IsEmpty() && zero_const.IsImmediate() && zero_const.U32() == 0u);
        user->SetArg(1 - operand, IR::Value{u32{0u}});
    }
}

void PatchGlobalDataShareAccess(IR::Inst& inst, Info& info, Descriptors& descriptors,
                                const Profile& profile) {
    const u32 binding = descriptors.Add(BufferResource{
        .used_types = IR::Type::U32,
        .buffer_type = BufferType::GdsBuffer,
        .is_written = true,
    });

    IR::IREmitter ir{*inst.GetParent(), IR::Block::InstructionList::s_iterator_to(inst)};

    if (inst.GetOpcode() == IR::Opcode::DataAppend || inst.GetOpcode() == IR::Opcode::DataConsume) {
        RemoveAppendBufferLaneOffset(inst);
        const IR::Value replacement =
            inst.GetOpcode() == IR::Opcode::DataAppend
                ? ir.BufferAtomicIAdd(ir.Imm32(binding), inst.Arg(0), ir.Imm32(1u), {})
                : ir.BufferAtomicISub(ir.Imm32(binding), inst.Arg(0), ir.Imm32(1u), {});
        inst.ReplaceUsesWithAndRemove(replacement);
        return;
    }

    // Convert shared memory opcode to storage buffer atomic to GDS buffer.
    auto& buffer = info.buffers[binding];
    const IR::U32 offset = IR::U32{inst.Arg(0)};
    const IR::U32 address_words = ir.ShiftRightLogical(offset, ir.Imm32(1));
    const IR::U32 address_dwords = ir.ShiftRightLogical(offset, ir.Imm32(2));
    const IR::U32 address_qwords = ir.ShiftRightLogical(offset, ir.Imm32(3));
    const IR::U32 handle = ir.Imm32(binding);
    switch (inst.GetOpcode()) {
    case IR::Opcode::SharedAtomicIAdd32:
        inst.ReplaceUsesWith(ir.BufferAtomicIAdd(handle, address_dwords, inst.Arg(1), {}));
        break;
    case IR::Opcode::SharedAtomicIAdd64:
        inst.ReplaceUsesWith(ir.BufferAtomicIAdd(handle, address_qwords, IR::U64{inst.Arg(1)}, {}));
        break;
    case IR::Opcode::SharedAtomicISub32:
        inst.ReplaceUsesWith(ir.BufferAtomicISub(handle, address_dwords, inst.Arg(1), {}));
        break;
    case IR::Opcode::SharedAtomicSMin32:
    case IR::Opcode::SharedAtomicUMin32: {
        const bool is_signed = inst.GetOpcode() == IR::Opcode::SharedAtomicSMin32;
        inst.ReplaceUsesWith(
            ir.BufferAtomicIMin(handle, address_dwords, inst.Arg(1), is_signed, {}));
        break;
    }
    case IR::Opcode::SharedAtomicSMax32:
    case IR::Opcode::SharedAtomicUMax32: {
        const bool is_signed = inst.GetOpcode() == IR::Opcode::SharedAtomicSMax32;
        inst.ReplaceUsesWith(
            ir.BufferAtomicIMax(handle, address_dwords, inst.Arg(1), is_signed, {}));
        break;
    }
    case IR::Opcode::SharedAtomicInc32:
        inst.ReplaceUsesWith(ir.BufferAtomicInc(handle, address_dwords, {}));
        break;
    case IR::Opcode::SharedAtomicDec32:
        inst.ReplaceUsesWith(ir.BufferAtomicDec(handle, address_dwords, {}));
        break;
    case IR::Opcode::SharedAtomicAnd32:
        inst.ReplaceUsesWith(ir.BufferAtomicAnd(handle, address_dwords, inst.Arg(1), {}));
        break;
    case IR::Opcode::SharedAtomicOr32:
        inst.ReplaceUsesWith(ir.BufferAtomicOr(handle, address_dwords, inst.Arg(1), {}));
        break;
    case IR::Opcode::SharedAtomicXor32:
        inst.ReplaceUsesWith(ir.BufferAtomicXor(handle, address_dwords, inst.Arg(1), {}));
        break;
    case IR::Opcode::SharedAtomicCmpSwap32:
        // Args are (address, value, cmp_value)
        inst.ReplaceUsesWith(
            ir.BufferAtomicCmpSwap(handle, address_dwords, inst.Arg(1), inst.Arg(2), {}));
        break;
    case IR::Opcode::LoadSharedU16: {
        inst.ReplaceUsesWith(ir.LoadBufferU16(handle, address_words, {}));
        buffer.used_types |= IR::Type::U16;
        break;
    }
    case IR::Opcode::LoadSharedU32:
        inst.ReplaceUsesWith(ir.LoadBufferU32(1, handle, address_dwords, {}));
        break;
    case IR::Opcode::LoadSharedU64: {
        inst.ReplaceUsesWith(ir.LoadBufferU64(handle, address_qwords, {}));
        buffer.used_types |= IR::Type::U64;
        break;
    }
    case IR::Opcode::WriteSharedU16: {
        ir.StoreBufferU16(handle, address_words, IR::U16{inst.Arg(1)}, {});
        inst.Invalidate();
        buffer.used_types |= IR::Type::U16;
        break;
    }
    case IR::Opcode::WriteSharedU32:
        ir.StoreBufferU32(1, handle, address_dwords, inst.Arg(1), {});
        inst.Invalidate();
        break;
    case IR::Opcode::WriteSharedU64: {
        ir.StoreBufferU64(handle, address_qwords, IR::U64{inst.Arg(1)}, {});
        inst.Invalidate();
        buffer.used_types |= IR::Type::U64;
        break;
    }
    default:
        UNREACHABLE_MSG("Unexpected opcode {}", inst.GetOpcode());
    }
}

IR::U32 CalculateBufferAddress(IR::IREmitter& ir, const IR::Inst& inst, const Info& info,
                               const AmdGpu::Buffer& buffer, u32 stride) {
    const auto inst_info = inst.Flags<IR::BufferInstInfo>();
    const u32 inst_offset = inst_info.inst_offset.Value();
    const auto is_inst_typed = inst_info.inst_data_fmt != AmdGpu::DataFormat::FormatInvalid;
    const auto data_format = is_inst_typed
                                 ? AmdGpu::RemapDataFormat(inst_info.inst_data_fmt.Value())
                                 : buffer.GetDataFmt();
    const u32 shift = BufferAddressShift(inst, data_format);
    const u32 mask = (1 << shift) - 1;
    const IR::U32 soffset = IR::GetBufferSOffsetArg(&inst);

    // If address calculation is of the form "index * const_stride + offset" with offset constant
    // and both const_stride and offset are divisible with the element size, apply shift directly.
    if (inst_info.index_enable && !inst_info.voffset_enable && soffset.IsImmediate() &&
        !buffer.swizzle_enable && !buffer.add_tid_enable && (stride & mask) == 0) {
        const u32 total_offset = soffset.U32() + inst_offset;
        if ((total_offset & mask) == 0) {
            // buffer_offset = index * (const_stride >> shift) + (offset >> shift)
            const IR::U32 index = IR::GetBufferIndexArg(&inst);
            return ir.IAdd(ir.IMul(index, ir.Imm32(stride >> shift)),
                           ir.Imm32(total_offset >> shift));
        }
    }

    // index = (inst_idxen ? vgpr_index : 0) + (const_add_tid_enable ? thread_id[5:0] : 0)
    IR::U32 index = ir.Imm32(0U);
    if (inst_info.index_enable) {
        const IR::U32 vgpr_index = IR::GetBufferIndexArg(&inst);
        index = ir.IAdd(index, vgpr_index);
    }
    if (buffer.add_tid_enable) {
        ASSERT_MSG(info.sw_stage == SwStage::Compute,
                   "Thread ID buffer addressing is not supported outside of compute.");
        const IR::U32 thread_id{ir.LaneId()};
        index = ir.IAdd(index, thread_id);
    }
    // offset = (inst_offen ? vgpr_offset : 0) + inst_offset
    IR::U32 offset = ir.Imm32(inst_offset);
    offset = ir.IAdd(offset, soffset);
    if (inst_info.voffset_enable) {
        const IR::U32 voffset = IR::GetBufferVOffsetArg(&inst);
        offset = ir.IAdd(offset, voffset);
    }
    const IR::U32 const_stride = ir.Imm32(stride);
    IR::U32 buffer_offset;
    if (buffer.swizzle_enable) {
        const IR::U32 const_index_stride = ir.Imm32(buffer.GetIndexStride());
        const IR::U32 const_element_size = ir.Imm32(buffer.GetElementSize());
        // index_msb = index / const_index_stride
        const IR::U32 index_msb{ir.IDiv(index, const_index_stride)};
        // index_lsb = index % const_index_stride
        const IR::U32 index_lsb{ir.IMod(index, const_index_stride)};
        // offset_msb = offset / const_element_size
        const IR::U32 offset_msb{ir.IDiv(offset, const_element_size)};
        // offset_lsb = offset % const_element_size
        const IR::U32 offset_lsb{ir.IMod(offset, const_element_size)};
        // buffer_offset =
        //     (index_msb * const_stride + offset_msb * const_element_size) * const_index_stride
        //     + index_lsb * const_element_size + offset_lsb
        const IR::U32 buffer_offset_msb = ir.IMul(
            ir.IAdd(ir.IMul(index_msb, const_stride), ir.IMul(offset_msb, const_element_size)),
            const_index_stride);
        const IR::U32 buffer_offset_lsb =
            ir.IAdd(ir.IMul(index_lsb, const_element_size), offset_lsb);
        buffer_offset = ir.IAdd(buffer_offset_msb, buffer_offset_lsb);
    } else {
        // buffer_offset = index * const_stride + offset
        buffer_offset = ir.IAdd(ir.IMul(index, const_stride), offset);
    }
    if (shift != 0) {
        buffer_offset = ir.ShiftRightLogical(buffer_offset, ir.Imm32(shift));
    }
    return buffer_offset;
}

void PatchBufferArgs(IR::Inst& inst, Info& info) {
    const auto handle = inst.Arg(0);
    const auto buffer_res = info.buffers[handle.U32()];
    const auto buffer = buffer_res.GetSharp(info);

    // Address of constant buffer reads can be calculated at IR emission time.
    if (inst.GetOpcode() == IR::Opcode::ReadConstBuffer) {
        return;
    }

    IR::IREmitter ir{*inst.GetParent(), IR::Block::InstructionList::s_iterator_to(inst)};
    inst.SetArg(IR::LoadBufferArgs::Address,
                CalculateBufferAddress(ir, inst, info, buffer, buffer.stride));
}

void PatchImageSampleArgs(IR::Inst& inst, Info& info, const ImageResource& image_res,
                          const AmdGpu::Image& image) {
    const auto handle = inst.Arg(0);
    const auto& sampler_res = info.samplers[(handle.U32() >> 16) & 0xFFFF];
    const auto sampler = sampler_res.GetSharp(info);

    IR::IREmitter ir{*inst.GetParent(), IR::Block::InstructionList::s_iterator_to(inst)};
    const auto inst_info = inst.Flags<IR::TextureInstInfo>();
    const auto view_type = image.GetViewType(image_res.is_array);

    IR::Inst* body1 = inst.Arg(2).Inst();
    IR::Inst* body2 = inst.Arg(3).Inst();
    IR::Inst* body3 = inst.Arg(4).Inst();
    IR::F32 body4 = IR::F32{inst.Arg(5)};
    const auto get_addr_reg = [&](u32 index) -> IR::F32 {
        if (index <= 3) {
            return IR::F32{body1->Arg(index)};
        }
        if (index >= 4 && index <= 7) {
            return IR::F32{body2->Arg(index - 4)};
        }
        if (index >= 8 && index <= 11) {
            return IR::F32{body3->Arg(index - 8)};
        }
        if (index == 12) {
            return body4;
        }
        UNREACHABLE();
    };
    u32 addr_reg = 0;

    // Load first address components as denoted in 8.2.4 VGPR Usage Sea Islands Series Instruction
    // Set Architecture
    const IR::Value offset = [&] -> IR::Value {
        if (!inst_info.has_offset) {
            return IR::U32{};
        }

        // The offsets are six-bit signed integers: X=[5:0], Y=[13:8], and Z=[21:16].
        IR::Value arg = get_addr_reg(addr_reg++);
        if (const IR::Inst* offset_inst = arg.TryInst()) {
            ASSERT(offset_inst->GetOpcode() == IR::Opcode::BitCastF32U32);
            arg = offset_inst->Arg(0);
        }

        const auto read = [&](u32 off) -> IR::U32 {
            if (arg.IsImmediate()) {
                const u32 imm =
                    arg.Type() == IR::Type::F32 ? std::bit_cast<u32>(arg.F32()) : arg.U32();
                const u16 comp = (imm >> off) & 0x3F;
                return ir.Imm32(s32(comp << 26) >> 26);
            }
            return ir.BitFieldExtract(IR::U32{arg}, ir.Imm32(off), ir.Imm32(6), true);
        };

        switch (view_type) {
        case AmdGpu::ImageType::Color1D:
        case AmdGpu::ImageType::Color1DArray:
            return read(0);
        case AmdGpu::ImageType::Color2D:
        case AmdGpu::ImageType::Color2DMsaa:
        case AmdGpu::ImageType::Color2DArray:
        case AmdGpu::ImageType::Cube:
            return ir.CompositeConstruct(read(0), read(8));
        case AmdGpu::ImageType::Color3D:
            return ir.CompositeConstruct(read(0), read(8), read(16));
        default:
            UNREACHABLE();
        }
    }();
    const IR::F32 bias = inst_info.has_bias ? get_addr_reg(addr_reg++) : IR::F32{};
    const IR::F32 dref = inst_info.is_depth ? get_addr_reg(addr_reg++) : IR::F32{};
    const auto [derivatives_dx, derivatives_dy] = [&] -> std::pair<IR::Value, IR::Value> {
        if (!inst_info.has_derivatives) {
            return {};
        }
        switch (view_type) {
        case AmdGpu::ImageType::Color1D:
        case AmdGpu::ImageType::Color1DArray:
            // du/dx, du/dy
            addr_reg = addr_reg + 2;
            return {get_addr_reg(addr_reg - 2), get_addr_reg(addr_reg - 1)};
        case AmdGpu::ImageType::Color2D:
        case AmdGpu::ImageType::Color2DMsaa:
        case AmdGpu::ImageType::Color2DArray:
        case AmdGpu::ImageType::Cube:
            // (du/dx, dv/dx), (du/dy, dv/dy)
            addr_reg = addr_reg + 4;
            return {ir.CompositeConstruct(get_addr_reg(addr_reg - 4), get_addr_reg(addr_reg - 3)),
                    ir.CompositeConstruct(get_addr_reg(addr_reg - 2), get_addr_reg(addr_reg - 1))};
        case AmdGpu::ImageType::Color3D:
            // (du/dx, dv/dx, dw/dx), (du/dy, dv/dy, dw/dy)
            addr_reg = addr_reg + 6;
            return {ir.CompositeConstruct(get_addr_reg(addr_reg - 6), get_addr_reg(addr_reg - 5),
                                          get_addr_reg(addr_reg - 4)),
                    ir.CompositeConstruct(get_addr_reg(addr_reg - 3), get_addr_reg(addr_reg - 2),
                                          get_addr_reg(addr_reg - 1))};
        default:
            UNREACHABLE();
        }
    }();

    const bool is_msaa = view_type == AmdGpu::ImageType::Color2DMsaa ||
                         view_type == AmdGpu::ImageType::Color2DMsaaArray;
    const bool unnormalized = sampler.force_unnormalized || inst_info.is_unnormalized;
    const bool needs_dimentions = (!is_msaa && unnormalized) || (is_msaa && !unnormalized);
    const auto dimensions =
        needs_dimentions ? ir.ImageQueryDimension(handle, ir.Imm32(0u), ir.Imm1(false), inst_info)
                         : IR::Value{};
    const auto get_coord = [&](u32 coord_idx, u32 dim_idx) -> IR::Value {
        const auto coord = get_addr_reg(coord_idx);
        if (is_msaa) {
            // For MSAA images preserve the unnormalized coord or manually unnormalize it
            if (unnormalized) {
                return ir.ConvertFToU(32, coord);
            } else {
                const auto dim =
                    ir.ConvertUToF(32, 32, IR::U32{ir.CompositeExtract(dimensions, dim_idx)});
                return ir.ConvertFToU(32, ir.FPMul(coord, dim));
            }
        }
        if (unnormalized) {
            // Normalize the coordinate for sampling, dividing by its corresponding dimension.
            const auto dim =
                ir.ConvertUToF(32, 32, IR::U32{ir.CompositeExtract(dimensions, dim_idx)});
            return ir.FPDiv(coord, dim);
        }
        return coord;
    };

    // Now we can load body components as noted in Table 8.9 Image Opcodes with Sampler
    const IR::Value coords = [&] -> IR::Value {
        switch (view_type) {
        case AmdGpu::ImageType::Color1D: // x
            addr_reg = addr_reg + 1;
            return get_coord(addr_reg - 1, 0);
        case AmdGpu::ImageType::Color1DArray: // x, slice
        case AmdGpu::ImageType::Color2D:      // x, y
        case AmdGpu::ImageType::Color2DMsaa:  // x, y
            addr_reg = addr_reg + 2;
            return ir.CompositeConstruct(get_coord(addr_reg - 2, 0), get_coord(addr_reg - 1, 1));
        case AmdGpu::ImageType::Color2DArray: // x, y, slice
        case AmdGpu::ImageType::Cube:         // x, y, face
        case AmdGpu::ImageType::Color3D:      // x, y, z
            addr_reg = addr_reg + 3;
            return ir.CompositeConstruct(get_coord(addr_reg - 3, 0), get_coord(addr_reg - 2, 1),
                                         get_coord(addr_reg - 1, 2));
        default:
            UNREACHABLE();
        }
    }();

    ASSERT(!inst_info.has_lod || !inst_info.has_lod_clamp);
    const bool explicit_lod = inst_info.has_lod || inst_info.force_level0;
    const IR::F32 lod = inst_info.has_lod        ? get_addr_reg(addr_reg++)
                        : inst_info.force_level0 ? ir.Imm32(0.0f)
                                                 : IR::F32{};
    const IR::F32 lod_clamp = inst_info.has_lod_clamp ? get_addr_reg(addr_reg++) : IR::F32{};

    auto texel = [&] -> IR::Value {
        if (is_msaa) {
            return ir.ImageRead(handle, coords, {}, ir.Imm32(0U), inst_info);
        }
        if (inst_info.is_gather) {
            if (inst_info.is_depth) {
                return ir.ImageGatherDref(handle, coords, offset, dref, inst_info);
            }
            return ir.ImageGather(handle, coords, offset, inst_info);
        }
        if (inst_info.has_derivatives) {
            return ir.ImageGradient(handle, coords, derivatives_dx, derivatives_dy, offset,
                                    lod_clamp, inst_info);
        }
        if (inst_info.is_depth) {
            if (explicit_lod) {
                return ir.ImageSampleDrefExplicitLod(handle, coords, dref, lod, offset, inst_info);
            }
            return ir.ImageSampleDrefImplicitLod(handle, coords, dref, bias, offset, inst_info);
        }
        if (explicit_lod) {
            return ir.ImageSampleExplicitLod(handle, coords, lod, offset, inst_info);
        }
        return ir.ImageSampleImplicitLod(handle, coords, bias, offset, inst_info);
    }();

    auto converted = ApplyReadNumberConversionVec4(ir, texel, image.GetNumberConversion());
    if (sampler.force_degamma && image.GetNumberFmt() != AmdGpu::NumberFormat::Srgb) {
        converted = ApplyForceDegamma(ir, texel);
    }
    inst.ReplaceUsesWith(converted);
}

void PatchImageArgs(IR::Inst& inst, Info& info) {
    // Nothing to patch for dimension query.
    if (inst.GetOpcode() == IR::Opcode::ImageQueryDimensions) {
        return;
    }

    const auto image_handle = inst.Arg(0);
    const auto binding_index = image_handle.U32() & 0xFFFF;
    const auto& image_res = info.images[binding_index];
    auto image = image_res.GetSharp(info);

    // Sample instructions must be handled separately using address register data.
    if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
        return PatchImageSampleArgs(inst, info, image_res, image);
    }

    IR::IREmitter ir{*inst.GetParent(), IR::Block::InstructionList::s_iterator_to(inst)};
    auto inst_info = inst.Flags<IR::TextureInstInfo>();
    const auto view_type = image.GetViewType(image_res.is_array);

    // Now that we know the image type, adjust texture coordinate vector.
    IR::Inst* body = inst.Arg(1).Inst();
    const auto [coords, arg] = [&] -> std::pair<IR::Value, IR::Value> {
        switch (view_type) {
        case AmdGpu::ImageType::Color1D: // x, [lod]
            return {body->Arg(0), body->Arg(1)};
        case AmdGpu::ImageType::Color1DArray: // x, slice, [lod]
        case AmdGpu::ImageType::Color2D:      // x, y, [lod]
            return {ir.CompositeConstruct(body->Arg(0), body->Arg(1)), body->Arg(2)};
        case AmdGpu::ImageType::Color2DMsaa: // x, y. (sample is passed on different argument)
        {
            auto skip_lod = false;
            if (inst_info.has_lod) {
                const auto mipid = body->Arg(2);
                if (mipid.IsImmediate() && mipid.U32() == 0) {
                    // if image_x_mip refers to a MSAA image, and mipid is 0, it is safe to be
                    // skipped and fragid is taken from the next arg
                    LOG_WARNING(Render_Recompiler, "Encountered a _mip instruction with MSAA "
                                                   "image, and mipid is 0, skipping LoD");
                    inst_info.has_lod.Assign(false);
                    skip_lod = true;
                } else {
                    UNREACHABLE_MSG(
                        "Encountered a _mip instruction with MSAA image, and mipid is non-zero");
                }
            }
            return {ir.CompositeConstruct(body->Arg(0), body->Arg(1)), body->Arg(skip_lod ? 3 : 2)};
        }
        case AmdGpu::ImageType::Color2DArray:     // x, y, slice, [lod]
        case AmdGpu::ImageType::Color2DMsaaArray: // x, y, slice. (sample is passed on different
                                                  // argument)
        case AmdGpu::ImageType::Cube:             // x, y, face, [lod]
        case AmdGpu::ImageType::Color3D:          // x, y, z, [lod]
            return {ir.CompositeConstruct(body->Arg(0), body->Arg(1), body->Arg(2)), body->Arg(3)};
        default:
            UNREACHABLE_MSG("Unknown image type {}", view_type);
        }
    }();

    const auto has_ms = view_type == AmdGpu::ImageType::Color2DMsaa ||
                        view_type == AmdGpu::ImageType::Color2DMsaaArray;
    ASSERT(!inst_info.has_lod || !has_ms);
    // If we are binding a single mip level as fallback, drop the argument
    const auto lod =
        (inst_info.has_lod && image_res.mip_fallback_mode != MipStorageFallbackMode::ConstantIndex)
            ? IR::U32{arg}
            : IR::U32{};
    const auto ms = has_ms ? IR::U32{arg} : IR::U32{};

    const auto is_storage = image_res.is_written;
    if (inst.GetOpcode() == IR::Opcode::ImageRead) {
        auto texel = ir.ImageRead(image_handle, coords, lod, ms, inst_info);
        if (is_storage) {
            // Storage image requires shader swizzle.
            texel = ApplySwizzle(ir, texel, image.DstSelect());
        }
        const auto converted =
            ApplyReadNumberConversionVec4(ir, texel, image.GetNumberConversion());
        inst.ReplaceUsesWith(converted);
    } else {
        inst.SetArg(1, coords);
        if (inst.GetOpcode() == IR::Opcode::ImageWrite) {
            inst.SetArg(2, lod);
            inst.SetArg(3, ms);

            auto texel = inst.Arg(4);
            if (is_storage) {
                // Storage image requires shader swizzle.
                texel = ApplySwizzle(ir, texel, image.DstSelect().Inverse());
            }
            const auto converted =
                ApplyWriteNumberConversionVec4(ir, texel, image.GetNumberConversion());
            inst.SetArg(4, converted);
        }
    }
}

namespace {

class GuardBuilder {
public:
    explicit GuardBuilder(ResourceGuards& table_) : table{table_} {
        table = {};
        table.nodes[NodeFalse].op = GuardOp::False;
        table.nodes[NodeTrue].op = GuardOp::True;
        table.num_nodes = static_cast<u8>(NodeTrue + 1);
    }

    u16 Condition(const IR::Value& value, bool polarity) {
        const u8 cond = Bool(value, 0);
        const u8 root = polarity ? cond : Not(cond);
        if (root <= NodeTrue || !has_leaf[root]) {
            return 0;
        }
        for (u32 i = 0; i < table.num_conds; ++i) {
            if (table.conds[i] == root) {
                return static_cast<u16>(1U << i);
            }
        }
        if (table.num_conds == ResourceGuards::MaxConds) {
            return 0;
        }
        table.conds[table.num_conds] = root;
        return static_cast<u16>(1U << table.num_conds++);
    }

private:
    enum View : u32 {
        ViewBool,
        ViewWord,
        ViewFloat,
        ViewLanes,
        NumViews,
    };

    static constexpr u8 NodeUnknown = 0;
    static constexpr u8 NodeFalse = 1;
    static constexpr u8 NodeTrue = 2;
    static constexpr u8 NotBuilt = 0xFF;
    static constexpr u32 MaxDepth = 32;

    u8 Make(GuardOp op, u8 a = NodeUnknown, u8 b = NodeUnknown, u8 c = NodeUnknown, u32 imm = 0) {
        const GuardNode node{.op = op, .a = a, .b = b, .c = c, .imm = imm};
        for (u32 i = 0; i < table.num_nodes; ++i) {
            if (table.nodes[i] == node) {
                return static_cast<u8>(i);
            }
        }
        if (table.num_nodes == ResourceGuards::MaxNodes) {
            return NodeUnknown;
        }
        const u8 index = table.num_nodes++;
        table.nodes[index] = node;
        has_leaf[index] = op == GuardOp::Flatbuf || has_leaf[a] || has_leaf[b] || has_leaf[c];
        return index;
    }

    u8 Leaf(GuardOp op, u32 imm) {
        return Make(op, NodeUnknown, NodeUnknown, NodeUnknown, imm);
    }

    u8 Strict(GuardOp op, u8 a) {
        return a == NodeUnknown ? NodeUnknown : Make(op, a);
    }

    u8 Strict(GuardOp op, u8 a, u8 b) {
        return a == NodeUnknown || b == NodeUnknown ? NodeUnknown : Make(op, a, b);
    }

    u8 Strict(GuardOp op, u8 a, u8 b, u8 c) {
        if (a == NodeUnknown || b == NodeUnknown || c == NodeUnknown) {
            return NodeUnknown;
        }
        return Make(op, a, b, c);
    }

    u8 Not(u8 a) {
        if (a == NodeFalse) {
            return NodeTrue;
        }
        if (a == NodeTrue) {
            return NodeFalse;
        }
        if (table.nodes[a].op == GuardOp::Not) {
            return table.nodes[a].a;
        }
        return Strict(GuardOp::Not, a);
    }

    u8 And(u8 a, u8 b) {
        if (a == NodeFalse || b == NodeFalse) {
            return NodeFalse;
        }
        if (a == NodeTrue || a == b) {
            return b;
        }
        if (b == NodeTrue) {
            return a;
        }
        return Make(GuardOp::And, a, b);
    }

    u8 Or(u8 a, u8 b) {
        if (a == NodeTrue || b == NodeTrue) {
            return NodeTrue;
        }
        if (a == NodeFalse || a == b) {
            return b;
        }
        if (b == NodeFalse) {
            return a;
        }
        return Make(GuardOp::Or, a, b);
    }

    u8 Xor(u8 a, u8 b) {
        if (a == NodeFalse) {
            return b;
        }
        if (b == NodeFalse) {
            return a;
        }
        if (a == NodeTrue) {
            return Not(b);
        }
        if (b == NodeTrue) {
            return Not(a);
        }
        return Strict(GuardOp::Xor, a, b);
    }

    u8 Select(u8 cond, u8 a, u8 b) {
        if (cond == NodeTrue || a == b) {
            return a;
        }
        if (cond == NodeFalse) {
            return b;
        }
        if (cond == NodeUnknown && (a == NodeUnknown || b == NodeUnknown)) {
            return NodeUnknown;
        }
        return Make(GuardOp::Select, cond, a, b);
    }

    template <typename Func>
    u8 Memo(const IR::Inst* inst, View view, u32 depth, Func&& func) {
        auto [it, inserted] = memo.try_emplace(inst);
        std::array<u8, NumViews>& entry = it->second;
        if (inserted) {
            entry.fill(NotBuilt);
        }
        if (entry[view] == NotBuilt) {
            entry[view] = NodeUnknown;
            entry[view] = depth < MaxDepth ? func(depth + 1) : NodeUnknown;
        }
        return entry[view];
    }

    u8 Compare(GuardOp op, const IR::Inst& inst, u32 depth, bool swap) {
        const u8 lhs = Word(inst.Arg(0), depth);
        const u8 rhs = Word(inst.Arg(1), depth);
        return swap ? Strict(op, rhs, lhs) : Strict(op, lhs, rhs);
    }

    u8 FloatCompare(GuardOp op, const IR::Inst& inst, u32 depth, bool swap) {
        const u8 lhs = Float(inst.Arg(0), depth);
        const u8 rhs = Float(inst.Arg(1), depth);
        return swap ? Strict(op, rhs, lhs) : Strict(op, lhs, rhs);
    }

    u8 WordBinary(GuardOp op, const IR::Inst& inst, u32 depth) {
        const u8 lhs = Word(inst.Arg(0), depth);
        const u8 rhs = Word(inst.Arg(1), depth);
        return Strict(op, lhs, rhs);
    }

    u8 FloatBinary(GuardOp op, const IR::Inst& inst, u32 depth) {
        const u8 lhs = Float(inst.Arg(0), depth);
        const u8 rhs = Float(inst.Arg(1), depth);
        return Strict(op, lhs, rhs);
    }

    u8 FloatTernary(GuardOp op, const IR::Inst& inst, u32 depth) {
        const u8 x = Float(inst.Arg(0), depth);
        const u8 y = Float(inst.Arg(1), depth);
        const u8 z = Float(inst.Arg(2), depth);
        return Strict(op, x, y, z);
    }

    u8 Bool(const IR::Value& value, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() != IR::Type::U1) {
                return NodeUnknown;
            }
            return value.U1() ? NodeTrue : NodeFalse;
        }
        const IR::Inst* const inst = value.Inst();
        return Memo(inst, ViewBool, depth, [&](u32 d) -> u8 {
            switch (inst->GetOpcode()) {
            case IR::Opcode::ConditionRef:
                return Bool(inst->Arg(0), d);
            case IR::Opcode::LogicalNot:
                return Not(Bool(inst->Arg(0), d));
            case IR::Opcode::LogicalAnd: {
                const u8 lhs = Bool(inst->Arg(0), d);
                const u8 rhs = Bool(inst->Arg(1), d);
                return And(lhs, rhs);
            }
            case IR::Opcode::LogicalOr: {
                const u8 lhs = Bool(inst->Arg(0), d);
                const u8 rhs = Bool(inst->Arg(1), d);
                return Or(lhs, rhs);
            }
            case IR::Opcode::LogicalXor: {
                const u8 lhs = Bool(inst->Arg(0), d);
                const u8 rhs = Bool(inst->Arg(1), d);
                return Xor(lhs, rhs);
            }
            case IR::Opcode::SelectU1: {
                const u8 cond = Bool(inst->Arg(0), d);
                const u8 lhs = Bool(inst->Arg(1), d);
                const u8 rhs = Bool(inst->Arg(2), d);
                return Select(cond, lhs, rhs);
            }
            case IR::Opcode::InverseBallot:
                return Lanes(inst->Arg(0), d);
            case IR::Opcode::IEqual32:
                return Compare(GuardOp::IEq, *inst, d, false);
            case IR::Opcode::INotEqual32:
                return Compare(GuardOp::INe, *inst, d, false);
            case IR::Opcode::SLessThan32:
                return Compare(GuardOp::SLt, *inst, d, false);
            case IR::Opcode::SLessThanEqual32:
                return Compare(GuardOp::SLe, *inst, d, false);
            case IR::Opcode::SGreaterThan32:
                return Compare(GuardOp::SLt, *inst, d, true);
            case IR::Opcode::SGreaterThanEqual32:
                return Compare(GuardOp::SLe, *inst, d, true);
            case IR::Opcode::ULessThan32:
                return Compare(GuardOp::ULt, *inst, d, false);
            case IR::Opcode::ULessThanEqual32:
                return Compare(GuardOp::ULe, *inst, d, false);
            case IR::Opcode::UGreaterThan32:
                return Compare(GuardOp::ULt, *inst, d, true);
            case IR::Opcode::UGreaterThanEqual32:
                return Compare(GuardOp::ULe, *inst, d, true);
            case IR::Opcode::FPOrdEqual32:
            case IR::Opcode::FPUnordEqual32:
                return FloatCompare(GuardOp::FEq, *inst, d, false);
            case IR::Opcode::FPOrdNotEqual32:
            case IR::Opcode::FPUnordNotEqual32:
                return FloatCompare(GuardOp::FNe, *inst, d, false);
            case IR::Opcode::FPOrdLessThan32:
            case IR::Opcode::FPUnordLessThan32:
                return FloatCompare(GuardOp::FLt, *inst, d, false);
            case IR::Opcode::FPOrdLessThanEqual32:
            case IR::Opcode::FPUnordLessThanEqual32:
                return FloatCompare(GuardOp::FLe, *inst, d, false);
            case IR::Opcode::FPOrdGreaterThan32:
            case IR::Opcode::FPUnordGreaterThan32:
                return FloatCompare(GuardOp::FLt, *inst, d, true);
            case IR::Opcode::FPOrdGreaterThanEqual32:
            case IR::Opcode::FPUnordGreaterThanEqual32:
                return FloatCompare(GuardOp::FLe, *inst, d, true);
            case IR::Opcode::FPIsNan32:
            case IR::Opcode::FPIsInf32:
                return Strict(GuardOp::FIsNanOrInf, Float(inst->Arg(0), d));
            default:
                return NodeUnknown;
            }
        });
    }

    u8 Word(const IR::Value& value, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() != IR::Type::U32) {
                return NodeUnknown;
            }
            return Leaf(GuardOp::ImmU32, value.U32());
        }
        const IR::Inst* const inst = value.Inst();
        return Memo(inst, ViewWord, depth, [&](u32 d) -> u8 {
            switch (inst->GetOpcode()) {
            case IR::Opcode::GetUserData:
                return Leaf(GuardOp::Flatbuf, static_cast<u32>(inst->Arg(0).ScalarReg()));
            case IR::Opcode::ReadConst:
                if (const u32 offset = inst->Flags<u32>(); offset != 0) {
                    return Leaf(GuardOp::Flatbuf, offset);
                }
                return NodeUnknown;
            case IR::Opcode::ReadFirstLane:
                return Word(inst->Arg(0), d);
            case IR::Opcode::BitCastU32F32:
                return Strict(GuardOp::AsU32, Float(inst->Arg(0), d));
            case IR::Opcode::SelectU32: {
                const u8 cond = Bool(inst->Arg(0), d);
                const u8 lhs = Word(inst->Arg(1), d);
                const u8 rhs = Word(inst->Arg(2), d);
                return Select(cond, lhs, rhs);
            }
            case IR::Opcode::BitwiseAnd32:
                return WordBinary(GuardOp::BitAnd, *inst, d);
            case IR::Opcode::BitwiseOr32:
                return WordBinary(GuardOp::BitOr, *inst, d);
            case IR::Opcode::BitwiseXor32:
                return WordBinary(GuardOp::BitXor, *inst, d);
            case IR::Opcode::ShiftRightLogical32:
                return WordBinary(GuardOp::Shr, *inst, d);
            case IR::Opcode::BitFieldUExtract: {
                const u8 base = Word(inst->Arg(0), d);
                const u8 offset = Word(inst->Arg(1), d);
                const u8 count = Word(inst->Arg(2), d);
                return Strict(GuardOp::UExtract, base, offset, count);
            }
            default:
                return NodeUnknown;
            }
        });
    }

    u8 Float(const IR::Value& value, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() != IR::Type::F32) {
                return NodeUnknown;
            }
            return Leaf(GuardOp::ImmF32, std::bit_cast<u32>(value.F32()));
        }
        const IR::Inst* const inst = value.Inst();
        return Memo(inst, ViewFloat, depth, [&](u32 d) -> u8 {
            switch (inst->GetOpcode()) {
            case IR::Opcode::BitCastF32U32:
                return Strict(GuardOp::AsF32, Word(inst->Arg(0), d));
            case IR::Opcode::SelectF32: {
                const u8 cond = Bool(inst->Arg(0), d);
                const u8 lhs = Float(inst->Arg(1), d);
                const u8 rhs = Float(inst->Arg(2), d);
                return Select(cond, lhs, rhs);
            }
            case IR::Opcode::FPAdd32:
                return FloatBinary(GuardOp::FAdd, *inst, d);
            case IR::Opcode::FPSub32:
                return FloatBinary(GuardOp::FSub, *inst, d);
            case IR::Opcode::FPMin32:
                return FloatBinary(GuardOp::FMin, *inst, d);
            case IR::Opcode::FPMax32:
                return FloatBinary(GuardOp::FMax, *inst, d);
            case IR::Opcode::FPNeg32:
                return Strict(GuardOp::FNeg, Float(inst->Arg(0), d));
            case IR::Opcode::FPAbs32:
                return Strict(GuardOp::FAbs, Float(inst->Arg(0), d));
            case IR::Opcode::FPMedTri32:
                return FloatTernary(GuardOp::FMed3, *inst, d);
            case IR::Opcode::FPClamp32:
                return FloatTernary(GuardOp::FClamp, *inst, d);
            case IR::Opcode::FPSaturate32: {
                const u8 x = Float(inst->Arg(0), d);
                const u8 zero = Leaf(GuardOp::ImmF32, std::bit_cast<u32>(0.0f));
                const u8 one = Leaf(GuardOp::ImmF32, std::bit_cast<u32>(1.0f));
                return Strict(GuardOp::FClamp, x, zero, one);
            }
            default:
                return NodeUnknown;
            }
        });
    }

    u8 Lanes(const IR::Value& value, u32 depth) {
        if (value.IsImmediate()) {
            if (value.Type() != IR::Type::U64) {
                return NodeUnknown;
            }
            if (value.U64() == 0) {
                return NodeFalse;
            }
            return value.U64() == std::numeric_limits<u64>::max() ? NodeTrue : NodeUnknown;
        }
        const IR::Inst* const inst = value.Inst();
        return Memo(inst, ViewLanes, depth, [&](u32 d) -> u8 {
            switch (inst->GetOpcode()) {
            case IR::Opcode::Ballot:
                return Bool(inst->Arg(0), d);
            case IR::Opcode::BitwiseAnd64: {
                const u8 lhs = Lanes(inst->Arg(0), d);
                const u8 rhs = Lanes(inst->Arg(1), d);
                return And(lhs, rhs);
            }
            case IR::Opcode::BitwiseOr64: {
                const u8 lhs = Lanes(inst->Arg(0), d);
                const u8 rhs = Lanes(inst->Arg(1), d);
                return Or(lhs, rhs);
            }
            case IR::Opcode::BitwiseXor64: {
                const u8 lhs = Lanes(inst->Arg(0), d);
                const u8 rhs = Lanes(inst->Arg(1), d);
                return Xor(lhs, rhs);
            }
            case IR::Opcode::BitwiseNot64:
                return Not(Lanes(inst->Arg(0), d));
            case IR::Opcode::SelectU64: {
                const u8 cond = Bool(inst->Arg(0), d);
                const u8 lhs = Lanes(inst->Arg(1), d);
                const u8 rhs = Lanes(inst->Arg(2), d);
                return Select(cond, lhs, rhs);
            }
            case IR::Opcode::PackUint2x32:
                return PackedLanes(inst->Arg(0), d);
            default:
                return NodeUnknown;
            }
        });
    }

    u8 PackedLanes(const IR::Value& value, u32 depth) {
        const IR::Inst* const vec = value.TryInst();
        if (!vec) {
            return NodeUnknown;
        }
        if (vec->GetOpcode() == IR::Opcode::UnpackUint2x32) {
            return Lanes(vec->Arg(0), depth);
        }
        if (vec->GetOpcode() != IR::Opcode::CompositeConstructU32x2) {
            return NodeUnknown;
        }
        const IR::Inst* const lo = vec->Arg(0).TryInst();
        const IR::Inst* const hi = vec->Arg(1).TryInst();
        if (!lo || !hi || lo->GetOpcode() != IR::Opcode::CompositeExtractU32x2 ||
            hi->GetOpcode() != IR::Opcode::CompositeExtractU32x2 || lo->Arg(0) != hi->Arg(0) ||
            lo->Arg(1) != IR::Value{0U} || hi->Arg(1) != IR::Value{1U}) {
            return NodeUnknown;
        }
        const IR::Inst* const unpack = lo->Arg(0).TryInst();
        if (!unpack || unpack->GetOpcode() != IR::Opcode::UnpackUint2x32) {
            return NodeUnknown;
        }
        return Lanes(unpack->Arg(0), depth);
    }

    ResourceGuards& table;
    std::array<bool, ResourceGuards::MaxNodes> has_leaf{};
    std::unordered_map<const IR::Inst*, std::array<u8, NumViews>> memo;
};

class GuardCollector {
public:
    explicit GuardCollector(GuardBuilder& builder_) : builder{builder_} {}

    u16 BlockTerm(IR::Block* block) {
        boost::container::small_vector<IR::Block*, 32> chain;
        u16 term{};
        for (IR::Block* current = block; current;) {
            if (const auto it = terms.find(current); it != terms.end()) {
                term = it->second;
                break;
            }
            chain.push_back(current);
            IR::Block* const idom = current->immediate_dominator;
            current = idom != current ? idom : nullptr;
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            term |= EdgeTerm(*it);
            terms.emplace(*it, term);
        }
        return term;
    }

private:
    u16 EdgeTerm(IR::Block* block) {
        const IR::Block* const idom = block->immediate_dominator;
        if (!idom || idom == block || block->ImmPredecessors().size() != 1) {
            return 0;
        }
        const IR::Block* const pred = block->ImmPredecessors().front();
        const Gcn::Block* const cfg = pred->cfg_block;
        if (pred->branch_cond.IsEmpty() || pred->ImmSuccessors().size() != 2 || !cfg ||
            !cfg->branch_true || !cfg->branch_false) {
            return 0;
        }
        const IR::Block* const on_true = cfg->branch_true->ir_block;
        const IR::Block* const on_false = cfg->branch_false->ir_block;
        if (on_true == on_false || (block != on_true && block != on_false)) {
            return 0;
        }
        return builder.Condition(pred->branch_cond, block == on_true);
    }

    GuardBuilder& builder;
    std::unordered_map<const IR::Block*, u16> terms;
};

template <typename T>
class GuardGroups {
public:
    u32 AddUse(const SharpFetch<T>& key, u16 term) {
        const auto it = std::ranges::find(entries, key, &Entry::key);
        const u32 index = static_cast<u32>(std::distance(entries.begin(), it));
        if (it == entries.end()) {
            entries.push_back({.key = key});
        }
        const bool is_guardable =
            key.summary != SharpFetch<T>::Summary::Invalid && key.load_mask != 0;
        AddTerm(entries[index], is_guardable ? term : u16{0});
        return index;
    }

    void AddTermToAll(u16 term) {
        for (Entry& entry : entries) {
            AddTerm(entry, term);
        }
    }

    void Emit(ResourceGuards& table) {
        for (Entry& entry : entries) {
            if (entry.is_live || entry.num_terms == 0 ||
                table.num_groups == ResourceGuards::MaxGroups) {
                continue;
            }
            entry.group = table.num_groups;
            auto& group = table.groups[table.num_groups++];
            group.terms = entry.terms;
            group.num_terms = entry.num_terms;
        }
    }

    u8 GroupOf(u32 index) const {
        return index < entries.size() ? entries[index].group : NO_GUARD;
    }

private:
    struct Entry {
        SharpFetch<T> key{};
        std::array<u16, ResourceGuards::MaxTerms> terms{};
        u8 num_terms{};
        u8 group{NO_GUARD};
        bool is_live{};
    };

    static void AddTerm(Entry& entry, u16 term) {
        if (entry.is_live) {
            return;
        }
        if (term == 0) {
            entry.is_live = true;
            return;
        }
        for (u32 i = 0; i < entry.num_terms; ++i) {
            if ((entry.terms[i] & term) == entry.terms[i]) {
                return;
            }
        }
        u32 kept{};
        for (u32 i = 0; i < entry.num_terms; ++i) {
            if ((entry.terms[i] & term) != term) {
                entry.terms[kept++] = entry.terms[i];
            }
        }
        entry.num_terms = static_cast<u8>(kept);
        if (entry.num_terms < ResourceGuards::MaxTerms) {
            entry.terms[entry.num_terms++] = term;
            return;
        }
        u32 closest{};
        for (u32 i = 1; i < entry.num_terms; ++i) {
            if (std::popcount(static_cast<u32>(entry.terms[i] & term)) >
                std::popcount(static_cast<u32>(entry.terms[closest] & term))) {
                closest = i;
            }
        }
        entry.terms[closest] &= term;
        entry.is_live = entry.terms[closest] == 0;
    }

    boost::container::small_vector<Entry, 16> entries;
};

using ResourceGuardList = boost::container::small_vector<std::array<u8, 2>, 32>;

ResourceGuardList FindResourceGuards(IR::Program& program, const ResourceDiscoveryList& resources) {
    Info& info = program.info;
    ResourceGuardList guards(resources.size(), {NO_GUARD, NO_GUARD});
    info.resource_guards = {};
    if (!info.skip_resource_guards) {
        IR::ComputeDominators(program.post_order_blocks);
        GuardBuilder builder{info.resource_guards};
        GuardCollector collector{builder};
        GuardGroups<AmdGpu::Image> images;
        GuardGroups<AmdGpu::Sampler> samplers;
        constexpr u32 NoUse = std::numeric_limits<u32>::max();
        boost::container::small_vector<std::array<u32, 2>, 32> uses(resources.size(),
                                                                    {NoUse, NoUse});
        boost::container::small_vector<u16, 4> lod_terms;
        for (size_t i = 0; i < resources.size(); ++i) {
            const IR::Inst& inst = *resources[i].user;
            if (!IsImageInstruction(inst)) {
                continue;
            }
            const u16 term = collector.BlockTerm(inst.GetParent());
            const bool is_storage =
                inst.GetOpcode() == IR::Opcode::ImageWrite || IsImageAtomicInstruction(inst);
            uses[i][0] =
                images.AddUse(ConstructSharpFetch<AmdGpu::Image>(resources[i].sharps[0], false),
                              is_storage ? u16{0} : term);
            if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
                uses[i][1] = samplers.AddUse(
                    ConstructSharpFetch<AmdGpu::Sampler>(resources[i].sharps[1], false), term);
            } else if (inst.GetOpcode() == IR::Opcode::ImageQueryLod) {
                lod_terms.push_back(term);
            }
        }
        for (const u16 term : lod_terms) {
            samplers.AddTermToAll(term);
        }
        images.Emit(info.resource_guards);
        samplers.Emit(info.resource_guards);
        for (size_t i = 0; i < resources.size(); ++i) {
            guards[i] = {images.GroupOf(uses[i][0]), samplers.GroupOf(uses[i][1])};
        }
        for (IR::Block* const block : program.blocks) {
            block->immediate_dominator = nullptr;
        }
    }
    if (info.key_info) {
        info.dead_resource_guards = info.key_info->resource_guards == info.resource_guards
                                        ? info.key_info->dead_resource_guards
                                        : 0;
    } else {
        info.RefreshResourceGuards();
    }
    return guards;
}

} // Anonymous namespace

void ResourcePatchingPass(IR::Program& program, const ResourceDiscoveryList& resources,
                          const Profile& profile) {
    Shader::Info& info = program.info;
    const ResourceGuardList guards = FindResourceGuards(program, resources);

    // Iterate over discovered resources and patch them after finding the sharp.
    // Pass 1: Track resource sharps
    Descriptors descriptors{info};
    for (size_t i = 0; i < resources.size(); ++i) {
        const auto& usage = resources[i];
        IR::Inst& inst = *usage.user;
        if (IsBufferInstruction(inst)) {
            PatchBufferSharp(usage, info, descriptors, profile);
        } else if (IsImageInstruction(inst)) {
            PatchImageSharp(usage, guards[i], info, descriptors, profile);
        }
    }

    // Pass 2: Patch instruction args
    for (const auto& usage : resources) {
        IR::Inst& inst = *usage.user;
        if (IsBufferInstruction(inst)) {
            PatchBufferArgs(inst, info);
        } else if (IsImageInstruction(inst)) {
            PatchImageArgs(inst, info);
        } else if (IsDataRingInstruction(inst)) {
            PatchGlobalDataShareAccess(inst, info, descriptors, profile);
        }
    }
}

} // namespace Shader::Optimization

namespace Shader {
namespace {

enum class GuardKind : u8 {
    Unknown,
    Bool,
    Word,
    Float,
};

struct GuardValue {
    GuardKind kind{};
    bool float_op{};
    u32 bits{};
};

GuardValue BoolValue(bool value) {
    return {.kind = GuardKind::Bool, .bits = value ? 1U : 0U};
}

GuardValue WordValue(u32 value) {
    return {.kind = GuardKind::Word, .bits = value};
}

GuardValue FloatValue(u32 bits, bool float_op) {
    return {.kind = GuardKind::Float, .float_op = float_op, .bits = bits};
}

bool IsNormalOrZero(const GuardValue& value) {
    if (value.kind != GuardKind::Float) {
        return false;
    }
    const u32 exponent = (value.bits >> 23) & 0xFF;
    return exponent != 0xFF && (exponent != 0 || (value.bits & 0x7FFFFF) == 0);
}

double AsDouble(const GuardValue& value) {
    return static_cast<double>(std::bit_cast<float>(value.bits));
}

GuardValue FloatResult(double result) {
    if (!(std::abs(result) <= static_cast<double>(std::numeric_limits<float>::max()))) {
        return {};
    }
    const float rounded = static_cast<float>(result);
    if (static_cast<double>(rounded) != result) {
        return {};
    }
    const GuardValue value = FloatValue(std::bit_cast<u32>(rounded), true);
    return IsNormalOrZero(value) ? value : GuardValue{};
}

GuardValue FloatSum(double x, double y) {
    const double sum = x + y;
    const double y_part = sum - x;
    const double error = (x - (sum - y_part)) + (y - y_part);
    return error == 0.0 ? FloatResult(sum) : GuardValue{};
}

GuardValue Extract(u32 base, u32 offset, u32 count) {
    if (offset > 32 || count > 32 || offset + count > 32) {
        return {};
    }
    if (count == 0) {
        return WordValue(0);
    }
    const u32 shifted = base >> offset;
    return WordValue(count == 32 ? shifted : shifted & ((1U << count) - 1));
}

GuardValue WordOp(GuardOp op, u32 x, u32 y) {
    switch (op) {
    case GuardOp::BitAnd:
        return WordValue(x & y);
    case GuardOp::BitOr:
        return WordValue(x | y);
    case GuardOp::BitXor:
        return WordValue(x ^ y);
    case GuardOp::Shr:
        return y < 32 ? WordValue(x >> y) : GuardValue{};
    case GuardOp::IEq:
        return BoolValue(x == y);
    case GuardOp::INe:
        return BoolValue(x != y);
    case GuardOp::SLt:
        return BoolValue(static_cast<s32>(x) < static_cast<s32>(y));
    case GuardOp::SLe:
        return BoolValue(static_cast<s32>(x) <= static_cast<s32>(y));
    case GuardOp::ULt:
        return BoolValue(x < y);
    case GuardOp::ULe:
        return BoolValue(x <= y);
    default:
        return {};
    }
}

GuardValue FloatOp(GuardOp op, const GuardValue& a, const GuardValue& b, const GuardValue& c,
                   bool nearest) {
    switch (op) {
    case GuardOp::FNeg:
        return FloatValue(a.bits ^ 0x80000000U, a.float_op);
    case GuardOp::FAbs:
        return FloatValue(a.bits & 0x7FFFFFFFU, true);
    case GuardOp::FAdd:
        return nearest ? FloatSum(AsDouble(a), AsDouble(b)) : GuardValue{};
    case GuardOp::FSub:
        return nearest ? FloatSum(AsDouble(a), -AsDouble(b)) : GuardValue{};
    case GuardOp::FMin:
        return FloatValue(AsDouble(a) < AsDouble(b) ? a.bits : b.bits, true);
    case GuardOp::FMax:
        return FloatValue(AsDouble(a) > AsDouble(b) ? a.bits : b.bits, true);
    case GuardOp::FMed3: {
        const double x = AsDouble(a);
        const double y = AsDouble(b);
        const double z = AsDouble(c);
        const double median = std::max(std::min(x, y), std::min(std::max(x, y), z));
        return FloatValue(median == x ? a.bits : (median == y ? b.bits : c.bits), true);
    }
    case GuardOp::FClamp: {
        const double x = AsDouble(a);
        const double lo = AsDouble(b);
        const double hi = AsDouble(c);
        if (lo > hi) {
            return {};
        }
        return FloatValue(x < lo ? b.bits : (x > hi ? c.bits : a.bits), true);
    }
    case GuardOp::FEq:
        return BoolValue(AsDouble(a) == AsDouble(b));
    case GuardOp::FNe:
        return BoolValue(AsDouble(a) != AsDouble(b));
    case GuardOp::FLt:
        return BoolValue(AsDouble(a) < AsDouble(b));
    case GuardOp::FLe:
        return BoolValue(AsDouble(a) <= AsDouble(b));
    case GuardOp::FIsNanOrInf:
        return BoolValue(false);
    default:
        return {};
    }
}

u32 FloatOperandCount(GuardOp op) {
    switch (op) {
    case GuardOp::FNeg:
    case GuardOp::FAbs:
    case GuardOp::FIsNanOrInf:
        return 1;
    case GuardOp::FAdd:
    case GuardOp::FSub:
    case GuardOp::FMin:
    case GuardOp::FMax:
    case GuardOp::FEq:
    case GuardOp::FNe:
    case GuardOp::FLt:
    case GuardOp::FLe:
        return 2;
    case GuardOp::FMed3:
    case GuardOp::FClamp:
        return 3;
    default:
        return 0;
    }
}

GuardValue EvaluateNode(const GuardNode& node, std::span<const GuardValue> values,
                        std::span<const u32> flatbuf, bool nearest) {
    const GuardValue& a = values[node.a];
    const GuardValue& b = values[node.b];
    const GuardValue& c = values[node.c];
    const auto is_bool = [](const GuardValue& value, bool expected) {
        return value.kind == GuardKind::Bool && (value.bits != 0) == expected;
    };
    const auto are_words = [](std::initializer_list<const GuardValue*> operands) {
        return std::ranges::all_of(
            operands, [](const GuardValue* operand) { return operand->kind == GuardKind::Word; });
    };
    switch (node.op) {
    case GuardOp::False:
        return BoolValue(false);
    case GuardOp::True:
        return BoolValue(true);
    case GuardOp::Flatbuf:
        return node.imm < flatbuf.size() ? WordValue(flatbuf[node.imm]) : GuardValue{};
    case GuardOp::ImmU32:
        return WordValue(node.imm);
    case GuardOp::ImmF32:
        return FloatValue(node.imm, false);
    case GuardOp::AsU32:
        if (a.kind != GuardKind::Float || (a.float_op && (a.bits & 0x7FFFFFFFU) == 0)) {
            return {};
        }
        return WordValue(a.bits);
    case GuardOp::AsF32:
        return a.kind == GuardKind::Word ? FloatValue(a.bits, false) : GuardValue{};
    case GuardOp::Not:
        return a.kind == GuardKind::Bool ? BoolValue(a.bits == 0) : GuardValue{};
    case GuardOp::And:
        if (is_bool(a, false) || is_bool(b, false)) {
            return BoolValue(false);
        }
        return is_bool(a, true) && is_bool(b, true) ? BoolValue(true) : GuardValue{};
    case GuardOp::Or:
        if (is_bool(a, true) || is_bool(b, true)) {
            return BoolValue(true);
        }
        return is_bool(a, false) && is_bool(b, false) ? BoolValue(false) : GuardValue{};
    case GuardOp::Xor:
        if (a.kind != GuardKind::Bool || b.kind != GuardKind::Bool) {
            return {};
        }
        return BoolValue(a.bits != b.bits);
    case GuardOp::Select:
        if (a.kind == GuardKind::Bool) {
            return a.bits != 0 ? b : c;
        }
        if (b.kind != GuardKind::Unknown && b.kind == c.kind && b.bits == c.bits) {
            return {.kind = b.kind, .float_op = b.float_op || c.float_op, .bits = b.bits};
        }
        return {};
    case GuardOp::UExtract:
        return are_words({&a, &b, &c}) ? Extract(a.bits, b.bits, c.bits) : GuardValue{};
    default:
        break;
    }
    if (const u32 count = FloatOperandCount(node.op); count != 0) {
        const std::array operands{&a, &b, &c};
        for (u32 i = 0; i < count; ++i) {
            if (!IsNormalOrZero(*operands[i])) {
                return {};
            }
        }
        return FloatOp(node.op, a, b, c, nearest);
    }
    return are_words({&a, &b}) ? WordOp(node.op, a.bits, b.bits) : GuardValue{};
}

} // Anonymous namespace

u32 ResourceGuards::EvaluateDead(std::span<const u32> flatbuf) const {
    if (num_groups == 0) {
        return 0;
    }
    std::array<GuardValue, MaxNodes> values{};
    const bool nearest = std::fegetround() == FE_TONEAREST;
    for (u32 i = 0; i < num_nodes; ++i) {
        values[i] = EvaluateNode(nodes[i], values, flatbuf, nearest);
    }
    u32 false_conds{};
    for (u32 i = 0; i < num_conds; ++i) {
        const GuardValue& value = values[conds[i]];
        if (value.kind == GuardKind::Bool && value.bits == 0) {
            false_conds |= 1U << i;
        }
    }
    if (false_conds == 0) {
        return 0;
    }
    u32 dead{};
    for (u32 i = 0; i < num_groups; ++i) {
        const Group& group = groups[i];
        bool is_dead = group.num_terms != 0;
        for (u32 t = 0; is_dead && t < group.num_terms; ++t) {
            is_dead = (group.terms[t] & false_conds) != 0;
        }
        dead |= static_cast<u32>(is_dead) << i;
    }
    return dead;
}

} // namespace Shader
