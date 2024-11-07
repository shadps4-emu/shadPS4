// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <boost/container/small_vector.hpp>
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "video_core/amdgpu/resource.h"

namespace Shader::Optimization {
namespace {

using SharpLocation = u32;

bool IsBufferAtomic(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::BufferAtomicIAdd32:
    case IR::Opcode::BufferAtomicSMin32:
    case IR::Opcode::BufferAtomicUMin32:
    case IR::Opcode::BufferAtomicSMax32:
    case IR::Opcode::BufferAtomicUMax32:
    case IR::Opcode::BufferAtomicInc32:
    case IR::Opcode::BufferAtomicDec32:
    case IR::Opcode::BufferAtomicAnd32:
    case IR::Opcode::BufferAtomicOr32:
    case IR::Opcode::BufferAtomicXor32:
    case IR::Opcode::BufferAtomicSwap32:
        return true;
    default:
        return false;
    }
}

bool IsBufferStore(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::StoreBufferU32:
    case IR::Opcode::StoreBufferU32x2:
    case IR::Opcode::StoreBufferU32x3:
    case IR::Opcode::StoreBufferU32x4:
        return true;
    default:
        return IsBufferAtomic(inst);
    }
}

bool IsBufferInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadBufferU32:
    case IR::Opcode::LoadBufferU32x2:
    case IR::Opcode::LoadBufferU32x3:
    case IR::Opcode::LoadBufferU32x4:
    case IR::Opcode::ReadConstBuffer:
        return true;
    default:
        return IsBufferStore(inst);
    }
}

bool IsDataRingInstruction(const IR::Inst& inst) {
    return inst.GetOpcode() == IR::Opcode::DataAppend ||
           inst.GetOpcode() == IR::Opcode::DataConsume;
}

bool IsTextureBufferInstruction(const IR::Inst& inst) {
    return inst.GetOpcode() == IR::Opcode::LoadBufferFormatF32 ||
           inst.GetOpcode() == IR::Opcode::StoreBufferFormatF32;
}

bool UseFP16(AmdGpu::DataFormat data_format, AmdGpu::NumberFormat num_format) {
    switch (num_format) {
    case AmdGpu::NumberFormat::Float:
        switch (data_format) {
        case AmdGpu::DataFormat::Format16:
        case AmdGpu::DataFormat::Format16_16:
        case AmdGpu::DataFormat::Format16_16_16_16:
            return true;
        default:
            return false;
        }
    case AmdGpu::NumberFormat::Unorm:
    case AmdGpu::NumberFormat::Snorm:
    case AmdGpu::NumberFormat::Uscaled:
    case AmdGpu::NumberFormat::Sscaled:
    case AmdGpu::NumberFormat::Uint:
    case AmdGpu::NumberFormat::Sint:
    case AmdGpu::NumberFormat::SnormNz:
    default:
        return false;
    }
}

IR::Type BufferDataType(const IR::Inst& inst, AmdGpu::NumberFormat num_format) {
    return IR::Type::U32;
}

bool IsImageAtomicInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::ImageAtomicIAdd32:
    case IR::Opcode::ImageAtomicSMin32:
    case IR::Opcode::ImageAtomicUMin32:
    case IR::Opcode::ImageAtomicSMax32:
    case IR::Opcode::ImageAtomicUMax32:
    case IR::Opcode::ImageAtomicInc32:
    case IR::Opcode::ImageAtomicDec32:
    case IR::Opcode::ImageAtomicAnd32:
    case IR::Opcode::ImageAtomicOr32:
    case IR::Opcode::ImageAtomicXor32:
    case IR::Opcode::ImageAtomicExchange32:
        return true;
    default:
        return false;
    }
}

bool IsImageStorageInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::ImageWrite:
    case IR::Opcode::ImageRead:
        return true;
    default:
        return IsImageAtomicInstruction(inst);
    }
}

bool IsImageInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::ImageFetch:
    case IR::Opcode::ImageQueryDimensions:
    case IR::Opcode::ImageQueryLod:
    case IR::Opcode::ImageSampleRaw:
        return true;
    default:
        return IsImageStorageInstruction(inst);
    }
}

class Descriptors {
public:
    explicit Descriptors(Info& info_)
        : info{info_}, buffer_resources{info_.buffers},
          texture_buffer_resources{info_.texture_buffers}, image_resources{info_.images},
          sampler_resources{info_.samplers}, fmask_resources(info_.fmasks) {}

    u32 Add(const BufferResource& desc) {
        const u32 index{Add(buffer_resources, desc, [&desc](const auto& existing) {
            // Only one GDS binding can exist.
            if (desc.is_gds_buffer && existing.is_gds_buffer) {
                return true;
            }
            return desc.sharp_idx == existing.sharp_idx && desc.inline_cbuf == existing.inline_cbuf;
        })};
        auto& buffer = buffer_resources[index];
        buffer.used_types |= desc.used_types;
        buffer.is_written |= desc.is_written;
        return index;
    }

    u32 Add(const TextureBufferResource& desc) {
        const u32 index{Add(texture_buffer_resources, desc, [&desc](const auto& existing) {
            return desc.sharp_idx == existing.sharp_idx;
        })};
        auto& buffer = texture_buffer_resources[index];
        buffer.is_written |= desc.is_written;
        return index;
    }

    u32 Add(const ImageResource& desc) {
        const u32 index{Add(image_resources, desc, [&desc](const auto& existing) {
            return desc.sharp_idx == existing.sharp_idx;
        })};
        auto& image = image_resources[index];
        image.is_storage |= desc.is_storage;
        return index;
    }

    u32 Add(const SamplerResource& desc) {
        const u32 index{Add(sampler_resources, desc, [this, &desc](const auto& existing) {
            return desc.sharp_idx == existing.sharp_idx;
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
    TextureBufferResourceList& texture_buffer_resources;
    ImageResourceList& image_resources;
    SamplerResourceList& sampler_resources;
    FMaskResourceList& fmask_resources;
};

} // Anonymous namespace

std::pair<const IR::Inst*, bool> TryDisableAnisoLod0(const IR::Inst* inst) {
    std::pair not_found{inst, false};

    // Assuming S# is in UD s[12:15] and T# is in s[4:11]
    // The next pattern:
    //  s_bfe_u32     s0, s7,  $0x0008000c
    //  s_and_b32     s1, s12, $0xfffff1ff
    //  s_cmp_eq_u32  s0, 0
    //  s_cselect_b32 s0, s1, s12
    // is used to disable anisotropy in the sampler if the sampled texture doesn't have mips

    if (inst->GetOpcode() != IR::Opcode::SelectU32) {
        return not_found;
    }

    // Select should be based on zero check
    const auto* prod0 = inst->Arg(0).InstRecursive();
    if (prod0->GetOpcode() != IR::Opcode::IEqual ||
        !(prod0->Arg(1).IsImmediate() && prod0->Arg(1).U32() == 0u)) {
        return not_found;
    }

    // The bits range is for lods (note that constants are changed after constant propagation pass)
    const auto* prod0_arg0 = prod0->Arg(0).InstRecursive();
    if (prod0_arg0->GetOpcode() != IR::Opcode::BitFieldUExtract ||
        !(prod0_arg0->Arg(1).IsIdentity() && prod0_arg0->Arg(1).U32() == 12) ||
        !(prod0_arg0->Arg(2).IsIdentity() && prod0_arg0->Arg(2).U32() == 8)) {
        return not_found;
    }

    // Make sure mask is masking out anisotropy
    const auto* prod1 = inst->Arg(1).InstRecursive();
    if (prod1->GetOpcode() != IR::Opcode::BitwiseAnd32 || prod1->Arg(1).U32() != 0xfffff1ff) {
        return not_found;
    }

    // We're working on the first dword of s#
    const auto* prod2 = inst->Arg(2).InstRecursive();
    if (prod2->GetOpcode() != IR::Opcode::GetUserData &&
        prod2->GetOpcode() != IR::Opcode::ReadConst) {
        return not_found;
    }

    return {prod2, true};
}

SharpLocation TrackSharp(const IR::Inst* inst, const Shader::Info& info) {
    // Search until we find a potential sharp source.
    const auto pred = [](const IR::Inst* inst) -> std::optional<const IR::Inst*> {
        if (inst->GetOpcode() == IR::Opcode::GetUserData ||
            inst->GetOpcode() == IR::Opcode::ReadConst) {
            return inst;
        }
        return std::nullopt;
    };
    const auto result = IR::BreadthFirstSearch(inst, pred);
    ASSERT_MSG(result, "Unable to track sharp source");
    inst = result.value();
    if (inst->GetOpcode() == IR::Opcode::GetUserData) {
        return static_cast<u32>(inst->Arg(0).ScalarReg());
    } else {
        ASSERT_MSG(inst->GetOpcode() == IR::Opcode::ReadConst,
                   "Sharp load not from constant memory");
        return inst->Flags<u32>();
    }
}

s32 TryHandleInlineCbuf(IR::Inst& inst, Info& info, Descriptors& descriptors,
                        AmdGpu::Buffer& cbuf) {

    // Assuming V# is in UD s[32:35]
    // The next pattern:
    // s_getpc_b64     s[32:33]
    // s_add_u32       s32, <const>, s32
    // s_addc_u32      s33, 0, s33
    // s_mov_b32       s35, <const>
    // s_movk_i32      s34, <const>
    // buffer_load_format_xyz v[8:10], v1, s[32:35], 0 ...
    // is used to define an inline constant buffer

    IR::Inst* handle = inst.Arg(0).InstRecursive();
    if (!handle->AreAllArgsImmediates()) {
        return -1;
    }
    // We have found this pattern. Build the sharp.
    std::array<u64, 2> buffer;
    buffer[0] = info.pgm_base + (handle->Arg(0).U32() | u64(handle->Arg(1).U32()) << 32);
    buffer[1] = handle->Arg(2).U32() | u64(handle->Arg(3).U32()) << 32;
    cbuf = std::bit_cast<AmdGpu::Buffer>(buffer);
    // Assign a binding to this sharp.
    return descriptors.Add(BufferResource{
        .sharp_idx = std::numeric_limits<u32>::max(),
        .used_types = BufferDataType(inst, cbuf.GetNumberFmt()),
        .inline_cbuf = cbuf,
    });
}

void PatchBufferInstruction(IR::Block& block, IR::Inst& inst, Info& info,
                            Descriptors& descriptors) {
    s32 binding{};
    AmdGpu::Buffer buffer;
    if (binding = TryHandleInlineCbuf(inst, info, descriptors, buffer); binding == -1) {
        IR::Inst* handle = inst.Arg(0).InstRecursive();
        IR::Inst* producer = handle->Arg(0).InstRecursive();
        const auto sharp = TrackSharp(producer, info);
        buffer = info.ReadUdSharp<AmdGpu::Buffer>(sharp);
        binding = descriptors.Add(BufferResource{
            .sharp_idx = sharp,
            .used_types = BufferDataType(inst, buffer.GetNumberFmt()),
            .is_written = IsBufferStore(inst),
        });
    }

    // Update buffer descriptor format.
    const auto inst_info = inst.Flags<IR::BufferInstInfo>();

    // Replace handle with binding index in buffer resource list.
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    inst.SetArg(0, ir.Imm32(binding));
    ASSERT(!buffer.add_tid_enable);

    // Address of constant buffer reads can be calculated at IR emittion time.
    if (inst.GetOpcode() == IR::Opcode::ReadConstBuffer) {
        return;
    }

    const IR::U32 index_stride = ir.Imm32(buffer.index_stride);
    const IR::U32 element_size = ir.Imm32(buffer.element_size);

    // Compute address of the buffer using the stride.
    IR::U32 address = ir.Imm32(inst_info.inst_offset.Value());
    if (inst_info.index_enable) {
        const IR::U32 index = inst_info.offset_enable ? IR::U32{ir.CompositeExtract(inst.Arg(1), 0)}
                                                      : IR::U32{inst.Arg(1)};
        if (buffer.swizzle_enable) {
            const IR::U32 stride_index_stride =
                ir.Imm32(static_cast<u32>(buffer.stride * buffer.index_stride));
            const IR::U32 index_msb = ir.IDiv(index, index_stride);
            const IR::U32 index_lsb = ir.IMod(index, index_stride);
            address = ir.IAdd(address, ir.IAdd(ir.IMul(index_msb, stride_index_stride),
                                               ir.IMul(index_lsb, element_size)));
        } else {
            address = ir.IAdd(address, ir.IMul(index, ir.Imm32(buffer.GetStride())));
        }
    }
    if (inst_info.offset_enable) {
        const IR::U32 offset = inst_info.index_enable ? IR::U32{ir.CompositeExtract(inst.Arg(1), 1)}
                                                      : IR::U32{inst.Arg(1)};
        if (buffer.swizzle_enable) {
            const IR::U32 element_size_index_stride =
                ir.Imm32(buffer.element_size * buffer.index_stride);
            const IR::U32 offset_msb = ir.IDiv(offset, element_size);
            const IR::U32 offset_lsb = ir.IMod(offset, element_size);
            address = ir.IAdd(address,
                              ir.IAdd(ir.IMul(offset_msb, element_size_index_stride), offset_lsb));
        } else {
            address = ir.IAdd(address, offset);
        }
    }
    inst.SetArg(1, address);
}

void PatchTextureBufferInstruction(IR::Block& block, IR::Inst& inst, Info& info,
                                   Descriptors& descriptors) {
    const IR::Inst* handle = inst.Arg(0).InstRecursive();
    const IR::Inst* producer = handle->Arg(0).InstRecursive();
    const auto sharp = TrackSharp(producer, info);
    const auto buffer = info.ReadUdSharp<AmdGpu::Buffer>(sharp);
    const s32 binding = descriptors.Add(TextureBufferResource{
        .sharp_idx = sharp,
        .nfmt = buffer.GetNumberFmt(),
        .is_written = inst.GetOpcode() == IR::Opcode::StoreBufferFormatF32,
    });

    // Replace handle with binding index in texture buffer resource list.
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    inst.SetArg(0, ir.Imm32(binding));
    ASSERT(!buffer.swizzle_enable && !buffer.add_tid_enable);
}

IR::Value PatchCubeCoord(IR::IREmitter& ir, const IR::Value& s, const IR::Value& t,
                         const IR::Value& z, bool is_storage, bool is_array) {
    // When cubemap is written with imageStore it is treated like 2DArray.
    if (is_storage) {
        return ir.CompositeConstruct(s, t, z);
    }

    ASSERT(s.Type() == IR::Type::F32); // in case of fetched image need to adjust the code below

    // We need to fix x and y coordinate,
    // because the s and t coordinate will be scaled and plus 1.5 by v_madak_f32.
    // We already force the scale value to be 1.0 when handling v_cubema_f32,
    // here we subtract 1.5 to recover the original value.
    const IR::Value x = ir.FPSub(IR::F32{s}, ir.Imm32(1.5f));
    const IR::Value y = ir.FPSub(IR::F32{t}, ir.Imm32(1.5f));
    if (is_array) {
        const IR::U32 array_index = ir.ConvertFToU(32, IR::F32{z});
        const IR::U32 face_id = ir.BitwiseAnd(array_index, ir.Imm32(7u));
        const IR::U32 slice_id = ir.ShiftRightLogical(array_index, ir.Imm32(3u));
        return ir.CompositeConstruct(x, y, ir.ConvertIToF(32, 32, false, face_id),
                                     ir.ConvertIToF(32, 32, false, slice_id));
    } else {
        return ir.CompositeConstruct(x, y, z);
    }
}

void PatchImageSampleInstruction(IR::Block& block, IR::Inst& inst, Info& info,
                                 Descriptors& descriptors, const IR::Inst* producer,
                                 const u32 image_binding, const AmdGpu::Image& image) {
    // Read sampler sharp. This doesn't exist for IMAGE_LOAD/IMAGE_STORE instructions
    const u32 sampler_binding = [&] {
        ASSERT(producer->GetOpcode() == IR::Opcode::CompositeConstructU32x2);
        const IR::Value& handle = producer->Arg(1);
        // Inline sampler resource.
        if (handle.IsImmediate()) {
            LOG_WARNING(Render_Vulkan, "Inline sampler detected");
            return descriptors.Add(SamplerResource{
                .sharp_idx = std::numeric_limits<u32>::max(),
                .inline_sampler = AmdGpu::Sampler{.raw0 = handle.U32()},
            });
        }
        // Normal sampler resource.
        const auto ssharp_handle = handle.InstRecursive();
        const auto& [ssharp_ud, disable_aniso] = TryDisableAnisoLod0(ssharp_handle);
        const auto ssharp = TrackSharp(ssharp_ud, info);
        return descriptors.Add(SamplerResource{
            .sharp_idx = ssharp,
            .associated_image = image_binding,
            .disable_aniso = disable_aniso,
        });
    }();

    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};

    const auto inst_info = inst.Flags<IR::TextureInstInfo>();
    const IR::U32 handle = ir.Imm32(image_binding | sampler_binding << 16);

    IR::Inst* body1 = inst.Arg(1).InstRecursive();
    IR::Inst* body2 = inst.Arg(2).InstRecursive();
    IR::Inst* body3 = inst.Arg(3).InstRecursive();
    IR::F32 body4 = IR::F32{inst.Arg(4)};
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
        if (const IR::Inst* offset_inst = arg.TryInstRecursive()) {
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

        switch (image.GetType()) {
        case AmdGpu::ImageType::Color1D:
        case AmdGpu::ImageType::Color1DArray:
            return read(0);
        case AmdGpu::ImageType::Color2D:
        case AmdGpu::ImageType::Color2DArray:
        case AmdGpu::ImageType::Color2DMsaa:
            return ir.CompositeConstruct(read(0), read(8));
        case AmdGpu::ImageType::Color3D:
        case AmdGpu::ImageType::Cube:
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
        switch (image.GetType()) {
        case AmdGpu::ImageType::Color1D:
        case AmdGpu::ImageType::Color1DArray:
            // du/dx, du/dy
            addr_reg = addr_reg + 2;
            return {get_addr_reg(addr_reg - 2), get_addr_reg(addr_reg - 1)};
        case AmdGpu::ImageType::Color2D:
        case AmdGpu::ImageType::Color2DArray:
        case AmdGpu::ImageType::Color2DMsaa:
            // (du/dx, dv/dx), (du/dy, dv/dy)
            addr_reg = addr_reg + 4;
            return {ir.CompositeConstruct(get_addr_reg(addr_reg - 4), get_addr_reg(addr_reg - 3)),
                    ir.CompositeConstruct(get_addr_reg(addr_reg - 2), get_addr_reg(addr_reg - 1))};
        case AmdGpu::ImageType::Color3D:
        case AmdGpu::ImageType::Cube:
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

    // Now we can load body components as noted in Table 8.9 Image Opcodes with Sampler
    const IR::Value coords = [&] -> IR::Value {
        switch (image.GetType()) {
        case AmdGpu::ImageType::Color1D: // x
            addr_reg = addr_reg + 1;
            return get_addr_reg(addr_reg - 1);
        case AmdGpu::ImageType::Color1DArray: // x, slice
            [[fallthrough]];
        case AmdGpu::ImageType::Color2D: // x, y
            addr_reg = addr_reg + 2;
            return ir.CompositeConstruct(get_addr_reg(addr_reg - 2), get_addr_reg(addr_reg - 1));
        case AmdGpu::ImageType::Color2DArray: // x, y, slice
            [[fallthrough]];
        case AmdGpu::ImageType::Color2DMsaa: // x, y, frag
            [[fallthrough]];
        case AmdGpu::ImageType::Color3D: // x, y, z
            addr_reg = addr_reg + 3;
            return ir.CompositeConstruct(get_addr_reg(addr_reg - 3), get_addr_reg(addr_reg - 2),
                                         get_addr_reg(addr_reg - 1));
        case AmdGpu::ImageType::Cube: // x, y, face
            addr_reg = addr_reg + 3;
            return PatchCubeCoord(ir, get_addr_reg(addr_reg - 3), get_addr_reg(addr_reg - 2),
                                  get_addr_reg(addr_reg - 1), false, inst_info.is_array);
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

    auto new_inst = [&] -> IR::Value {
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
    inst.ReplaceUsesWith(new_inst);
}

void PatchImageInstruction(IR::Block& block, IR::Inst& inst, Info& info, Descriptors& descriptors) {
    const auto pred = [](const IR::Inst* inst) -> std::optional<const IR::Inst*> {
        const auto opcode = inst->GetOpcode();
        if (opcode == IR::Opcode::CompositeConstructU32x2 || // IMAGE_SAMPLE (image+sampler)
            opcode == IR::Opcode::ReadConst ||               // IMAGE_LOAD (image only)
            opcode == IR::Opcode::GetUserData) {
            return inst;
        }
        return std::nullopt;
    };
    const auto result = IR::BreadthFirstSearch(&inst, pred);
    ASSERT_MSG(result, "Unable to find image sharp source");
    const IR::Inst* producer = result.value();
    const bool has_sampler = producer->GetOpcode() == IR::Opcode::CompositeConstructU32x2;
    const auto tsharp_handle = has_sampler ? producer->Arg(0).InstRecursive() : producer;

    // Read image sharp.
    const auto tsharp = TrackSharp(tsharp_handle, info);
    const auto inst_info = inst.Flags<IR::TextureInstInfo>();
    auto image = info.ReadUdSharp<AmdGpu::Image>(tsharp);
    if (!image.Valid()) {
        LOG_ERROR(Render_Vulkan, "Shader compiled with unbound image!");
        image = AmdGpu::Image::Null();
    }
    ASSERT(image.GetType() != AmdGpu::ImageType::Invalid);
    const bool is_storage = IsImageStorageInstruction(inst);

    // Patch image instruction if image is FMask.
    if (image.IsFmask()) {
        ASSERT_MSG(!is_storage, "FMask storage instructions are not supported");

        IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
        switch (inst.GetOpcode()) {
        case IR::Opcode::ImageFetch:
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
            IR::Value dims = ir.CompositeConstruct(ir.Imm32(static_cast<u32>(image.width)), // x
                                                   ir.Imm32(static_cast<u32>(image.width)), // y
                                                   ir.Imm32(1), ir.Imm32(1)); // depth, mip
            inst.ReplaceUsesWith(dims);

            // Track FMask resource to do specialization.
            descriptors.Add(FMaskResource{
                .sharp_idx = tsharp,
            });
            return;
        }
        default:
            UNREACHABLE_MSG("Can't patch fmask instruction {}", inst.GetOpcode());
        }
    }

    const auto type = image.IsPartialCubemap() ? AmdGpu::ImageType::Color2DArray : image.GetType();
    u32 image_binding = descriptors.Add(ImageResource{
        .sharp_idx = tsharp,
        .type = type,
        .nfmt = image.GetNumberFmt(),
        .is_storage = is_storage,
        .is_depth = bool(inst_info.is_depth),
        .is_atomic = IsImageAtomicInstruction(inst),
        .is_array = bool(inst_info.is_array),
    });

    // Sample instructions must be resolved into a new instruction using address register data.
    if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
        PatchImageSampleInstruction(block, inst, info, descriptors, producer, image_binding, image);
        return;
    }

    // Patch image handle
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    inst.SetArg(0, ir.Imm32(image_binding));

    // No need to patch coordinates if we are just querying.
    if (inst.GetOpcode() == IR::Opcode::ImageQueryDimensions) {
        return;
    }

    // Now that we know the image type, adjust texture coordinate vector.
    IR::Inst* body = inst.Arg(1).InstRecursive();
    const auto [coords, arg] = [&] -> std::pair<IR::Value, IR::Value> {
        switch (image.GetType()) {
        case AmdGpu::ImageType::Color1D: // x
            return {body->Arg(0), body->Arg(1)};
        case AmdGpu::ImageType::Color1DArray: // x, slice
            [[fallthrough]];
        case AmdGpu::ImageType::Color2D: // x, y, [lod]
            [[fallthrough]];
        case AmdGpu::ImageType::Color2DMsaa: // x, y. (sample is passed on different argument)
            return {ir.CompositeConstruct(body->Arg(0), body->Arg(1)), body->Arg(2)};
        case AmdGpu::ImageType::Color2DArray: // x, y, slice, [lod]
            [[fallthrough]];
        case AmdGpu::ImageType::Color2DMsaaArray: // x, y, slice. (sample is passed on different
                                                  // argument)
            [[fallthrough]];
        case AmdGpu::ImageType::Color3D: // x, y, z
            return {ir.CompositeConstruct(body->Arg(0), body->Arg(1), body->Arg(2)), body->Arg(3)};
        case AmdGpu::ImageType::Cube: // x, y, face
            return {PatchCubeCoord(ir, body->Arg(0), body->Arg(1), body->Arg(2), is_storage,
                                   inst_info.is_array),
                    body->Arg(3)};
        default:
            UNREACHABLE_MSG("Unknown image type {}", image.GetType());
        }
    }();
    inst.SetArg(1, coords);

    if (inst_info.has_lod) {
        ASSERT(inst.GetOpcode() == IR::Opcode::ImageFetch);
        ASSERT(image.GetType() == AmdGpu::ImageType::Color2D ||
               image.GetType() == AmdGpu::ImageType::Color2DArray);
        inst.SetArg(3, arg);
    } else if (image.GetType() == AmdGpu::ImageType::Color2DMsaa ||
               image.GetType() == AmdGpu::ImageType::Color2DMsaaArray) {
        inst.SetArg(4, arg);
    }
}

void PatchDataRingInstruction(IR::Block& block, IR::Inst& inst, Info& info,
                              Descriptors& descriptors) {
    // Insert gds binding in the shader if it doesn't exist already.
    // The buffer is used for append/consume counters.
    constexpr static AmdGpu::Buffer GdsSharp{.base_address = 1};
    const u32 binding = descriptors.Add(BufferResource{
        .used_types = IR::Type::U32,
        .inline_cbuf = GdsSharp,
        .is_gds_buffer = true,
        .is_written = true,
    });

    const auto pred = [](const IR::Inst* inst) -> std::optional<const IR::Inst*> {
        if (inst->GetOpcode() == IR::Opcode::GetUserData) {
            return inst;
        }
        return std::nullopt;
    };

    // Attempt to deduce the GDS address of counter at compile time.
    const u32 gds_addr = [&] {
        const IR::Value& gds_offset = inst.Arg(0);
        if (gds_offset.IsImmediate()) {
            // Nothing to do, offset is known.
            return gds_offset.U32() & 0xFFFF;
        }
        const auto result = IR::BreadthFirstSearch(&inst, pred);
        ASSERT_MSG(result, "Unable to track M0 source");

        // M0 must be set by some user data register.
        const IR::Inst* prod = gds_offset.InstRecursive();
        const u32 ud_reg = u32(result.value()->Arg(0).ScalarReg());
        u32 m0_val = info.user_data[ud_reg] >> 16;
        if (prod->GetOpcode() == IR::Opcode::IAdd32) {
            m0_val += prod->Arg(1).U32();
        }
        return m0_val & 0xFFFF;
    }();

    // Patch instruction.
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    inst.SetArg(0, ir.Imm32(gds_addr >> 2));
    inst.SetArg(1, ir.Imm32(binding));
}

void ResourceTrackingPass(IR::Program& program) {
    // Iterate resource instructions and patch them after finding the sharp.
    auto& info = program.info;

    Descriptors descriptors{info};
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (IsBufferInstruction(inst)) {
                PatchBufferInstruction(*block, inst, info, descriptors);
                continue;
            }
            if (IsTextureBufferInstruction(inst)) {
                PatchTextureBufferInstruction(*block, inst, info, descriptors);
                continue;
            }
            if (IsImageInstruction(inst)) {
                PatchImageInstruction(*block, inst, info, descriptors);
                continue;
            }
            if (IsDataRingInstruction(inst)) {
                PatchDataRingInstruction(*block, inst, info, descriptors);
            }
        }
    }
}

} // namespace Shader::Optimization
