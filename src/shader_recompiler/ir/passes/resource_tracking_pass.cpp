// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reinterpret.h"
#include "video_core/amdgpu/resource.h"

namespace Shader::Optimization {
namespace {

using SharpLocation = u32;

bool IsBufferAtomic(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::BufferAtomicIAdd32:
    case IR::Opcode::BufferAtomicIAdd64:
    case IR::Opcode::BufferAtomicISub32:
    case IR::Opcode::BufferAtomicSMin32:
    case IR::Opcode::BufferAtomicSMin64:
    case IR::Opcode::BufferAtomicUMin32:
    case IR::Opcode::BufferAtomicUMin64:
    case IR::Opcode::BufferAtomicFMin32:
    case IR::Opcode::BufferAtomicSMax32:
    case IR::Opcode::BufferAtomicSMax64:
    case IR::Opcode::BufferAtomicUMax32:
    case IR::Opcode::BufferAtomicUMax64:
    case IR::Opcode::BufferAtomicFMax32:
    case IR::Opcode::BufferAtomicInc32:
    case IR::Opcode::BufferAtomicDec32:
    case IR::Opcode::BufferAtomicAnd32:
    case IR::Opcode::BufferAtomicOr32:
    case IR::Opcode::BufferAtomicXor32:
    case IR::Opcode::BufferAtomicSwap32:
    case IR::Opcode::BufferAtomicCmpSwap32:
        return true;
    default:
        return false;
    }
}

bool IsBufferStore(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::StoreBufferU8:
    case IR::Opcode::StoreBufferU16:
    case IR::Opcode::StoreBufferU32:
    case IR::Opcode::StoreBufferU32x2:
    case IR::Opcode::StoreBufferU32x3:
    case IR::Opcode::StoreBufferU32x4:
    case IR::Opcode::StoreBufferU64:
    case IR::Opcode::StoreBufferF32:
    case IR::Opcode::StoreBufferF32x2:
    case IR::Opcode::StoreBufferF32x3:
    case IR::Opcode::StoreBufferF32x4:
    case IR::Opcode::StoreBufferFormatF32:
        return true;
    default:
        return IsBufferAtomic(inst);
    }
}

bool IsBufferInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadBufferU8:
    case IR::Opcode::LoadBufferU16:
    case IR::Opcode::LoadBufferU32:
    case IR::Opcode::LoadBufferU32x2:
    case IR::Opcode::LoadBufferU32x3:
    case IR::Opcode::LoadBufferU32x4:
    case IR::Opcode::LoadBufferU64:
    case IR::Opcode::LoadBufferF32:
    case IR::Opcode::LoadBufferF32x2:
    case IR::Opcode::LoadBufferF32x3:
    case IR::Opcode::LoadBufferF32x4:
    case IR::Opcode::LoadBufferFormatF32:
    case IR::Opcode::ReadConstBuffer:
        return true;
    default:
        return IsBufferStore(inst);
    }
}

bool IsDataRingInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::DataAppend:
    case IR::Opcode::DataConsume:
        return true;
    case IR::Opcode::LoadSharedU16:
    case IR::Opcode::LoadSharedU32:
    case IR::Opcode::LoadSharedU64:
    case IR::Opcode::WriteSharedU16:
    case IR::Opcode::WriteSharedU32:
    case IR::Opcode::WriteSharedU64:
    case IR::Opcode::SharedAtomicIAdd32:
    case IR::Opcode::SharedAtomicIAdd64:
    case IR::Opcode::SharedAtomicUMin32:
    case IR::Opcode::SharedAtomicUMin64:
    case IR::Opcode::SharedAtomicSMin32:
    case IR::Opcode::SharedAtomicSMin64:
    case IR::Opcode::SharedAtomicUMax32:
    case IR::Opcode::SharedAtomicUMax64:
    case IR::Opcode::SharedAtomicSMax32:
    case IR::Opcode::SharedAtomicSMax64:
    case IR::Opcode::SharedAtomicAnd32:
    case IR::Opcode::SharedAtomicAnd64:
    case IR::Opcode::SharedAtomicOr32:
    case IR::Opcode::SharedAtomicOr64:
    case IR::Opcode::SharedAtomicXor32:
    case IR::Opcode::SharedAtomicXor64:
    case IR::Opcode::SharedAtomicISub32:
    case IR::Opcode::SharedAtomicISub64:
    case IR::Opcode::SharedAtomicInc32:
    case IR::Opcode::SharedAtomicInc64:
    case IR::Opcode::SharedAtomicDec32:
    case IR::Opcode::SharedAtomicDec64:
        return inst.Flags<bool>(); // is_gds
    default:
        return false;
    }
}

IR::Type BufferDataType(const IR::Inst& inst, AmdGpu::NumberFormat num_format) {
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

bool IsImageAtomicInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::ImageAtomicIAdd32:
    case IR::Opcode::ImageAtomicSMin32:
    case IR::Opcode::ImageAtomicUMin32:
    case IR::Opcode::ImageAtomicSMax32:
    case IR::Opcode::ImageAtomicUMax32:
    case IR::Opcode::ImageAtomicFMax32:
    case IR::Opcode::ImageAtomicFMin32:
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

bool IsImageInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::ImageRead:
    case IR::Opcode::ImageWrite:
    case IR::Opcode::ImageQueryDimensions:
    case IR::Opcode::ImageQueryLod:
    case IR::Opcode::ImageSampleRaw:
        return true;
    default:
        return IsImageAtomicInstruction(inst);
    }
}

class Descriptors {
public:
    explicit Descriptors(Info& info_)
        : info{info_}, buffer_resources{info_.buffers}, image_resources{info_.images},
          sampler_resources{info_.samplers}, fmask_resources(info_.fmasks) {}

    u32 Add(const BufferResource& desc) {
        const u32 index{Add(buffer_resources, desc, [&desc](const auto& existing) {
            return desc.sharp_idx == existing.sharp_idx &&
                   desc.inline_cbuf == existing.inline_cbuf &&
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
            return desc.sharp_idx == existing.sharp_idx && desc.is_array == existing.is_array;
        })};
        auto& image = image_resources[index];
        image.is_atomic |= desc.is_atomic;
        image.is_written |= desc.is_written;
        return index;
    }

    u32 Add(const SamplerResource& desc) {
        const u32 index{Add(sampler_resources, desc, [this, &desc](const auto& existing) {
            return desc.sampler == existing.sampler;
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
    if (prod0->GetOpcode() != IR::Opcode::IEqual32 ||
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

SharpLocation AttemptTrackSharp(const IR::Inst* inst, auto& visited_insts) {
    // Search until we find a potential sharp source.
    const auto pred = [&visited_insts](const IR::Inst* inst) -> std::optional<const IR::Inst*> {
        if (std::ranges::find(visited_insts, inst) != visited_insts.end()) {
            return std::nullopt;
        }
        if (inst->GetOpcode() == IR::Opcode::GetUserData ||
            inst->GetOpcode() == IR::Opcode::ReadConst) {
            return inst;
        }
        return std::nullopt;
    };
    const auto result = IR::BreadthFirstSearch(inst, pred);
    ASSERT_MSG(result, "Unable to track sharp source");
    inst = result.value();
    visited_insts.emplace_back(inst);
    if (inst->GetOpcode() == IR::Opcode::GetUserData) {
        return static_cast<u32>(inst->Arg(0).ScalarReg());
    } else {
        ASSERT_MSG(inst->GetOpcode() == IR::Opcode::ReadConst,
                   "Sharp load not from constant memory");
        return inst->Flags<u32>();
    }
}

/// Tracks a sharp with validation of the chosen data type.
template <typename DataType>
std::pair<SharpLocation, DataType> TrackSharp(const IR::Inst* inst, const Info& info) {
    boost::container::small_vector<const IR::Inst*, 4> visited_insts{};
    while (true) {
        const auto prev_size = visited_insts.size();
        const auto sharp = AttemptTrackSharp(inst, visited_insts);
        if (const auto data = info.ReadUdSharp<DataType>(sharp); data.Valid()) {
            return std::make_pair(sharp, data);
        }
        if (prev_size == visited_insts.size()) {
            // No change in visited instructions, we've run out of paths.
            UNREACHABLE_MSG("Unable to find valid sharp.");
        }
    }
}

/// Tracks a sharp without data validation.
SharpLocation TrackSharp(const IR::Inst* inst, const Info& info) {
    boost::container::static_vector<const IR::Inst*, 1> visited_insts{};
    return AttemptTrackSharp(inst, visited_insts);
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
        .buffer_type = BufferType::Guest,
    });
}

void PatchBufferSharp(IR::Block& block, IR::Inst& inst, Info& info, Descriptors& descriptors) {
    s32 binding{};
    AmdGpu::Buffer buffer;
    if (binding = TryHandleInlineCbuf(inst, info, descriptors, buffer); binding == -1) {
        IR::Inst* handle = inst.Arg(0).InstRecursive();
        IR::Inst* producer = handle->Arg(0).InstRecursive();
        SharpLocation sharp;
        std::tie(sharp, buffer) = TrackSharp<AmdGpu::Buffer>(producer, info);
        binding = descriptors.Add(BufferResource{
            .sharp_idx = sharp,
            .used_types = BufferDataType(inst, buffer.GetNumberFmt()),
            .buffer_type = BufferType::Guest,
            .is_written = IsBufferStore(inst),
            .is_formatted = inst.GetOpcode() == IR::Opcode::LoadBufferFormatF32 ||
                            inst.GetOpcode() == IR::Opcode::StoreBufferFormatF32,
        });
    }

    // Replace handle with binding index in buffer resource list.
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    inst.SetArg(0, ir.Imm32(binding));
}

void PatchImageSharp(IR::Block& block, IR::Inst& inst, Info& info, Descriptors& descriptors) {
    const auto pred = [](const IR::Inst* inst) -> std::optional<const IR::Inst*> {
        const auto opcode = inst->GetOpcode();
        if (opcode == IR::Opcode::ReadConst || // IMAGE_LOAD (image only)
            opcode == IR::Opcode::GetUserData) {
            return inst;
        }
        return std::nullopt;
    };
    const auto result = IR::BreadthFirstSearch(&inst, pred);
    ASSERT_MSG(result, "Unable to find image sharp source");
    const IR::Inst* tsharp_handle = result.value();

    // Read image sharp.
    const auto tsharp = TrackSharp(tsharp_handle, info);
    const auto inst_info = inst.Flags<IR::TextureInstInfo>();
    const bool is_atomic = IsImageAtomicInstruction(inst);
    const bool is_written = inst.GetOpcode() == IR::Opcode::ImageWrite || is_atomic;
    const ImageResource image_res = {
        .sharp_idx = tsharp,
        .is_depth = bool(inst_info.is_depth),
        .is_atomic = is_atomic,
        .is_array = bool(inst_info.is_array),
        .is_written = is_written,
        .is_r128 = bool(inst_info.is_r128),
    };
    auto image = image_res.GetSharp(info);
    ASSERT(image.GetType() != AmdGpu::ImageType::Invalid);

    // Patch image instruction if image is FMask.
    if (image.IsFmask()) {
        ASSERT_MSG(!is_written, "FMask storage instructions are not supported");

        IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
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

    u32 image_binding = descriptors.Add(image_res);

    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};

    if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
        // Read sampler sharp.
        const auto sampler_binding = [&] -> u32 {
            const auto sampler = inst.Arg(5).InstRecursive();
            ASSERT(sampler && sampler->GetOpcode() == IR::Opcode::CompositeConstructU32x4);
            const auto handle = sampler->Arg(0);
            // Inline sampler resource.
            if (handle.IsImmediate()) {
                LOG_DEBUG(Render_Vulkan, "Inline sampler detected");
                const auto [s1, s2, s3, s4] =
                    std::tuple{sampler->Arg(0), sampler->Arg(1), sampler->Arg(2), sampler->Arg(3)};
                ASSERT(s1.IsImmediate() && s2.IsImmediate() && s3.IsImmediate() &&
                       s4.IsImmediate());
                const auto inline_sampler = AmdGpu::Sampler{
                    .raw0 = u64(s2.U32()) << 32 | u64(s1.U32()),
                    .raw1 = u64(s4.U32()) << 32 | u64(s3.U32()),
                };
                const auto binding = descriptors.Add(SamplerResource{inline_sampler});
                return binding;
            } else {
                // Normal sampler resource.
                const auto ssharp_handle = handle.InstRecursive();
                const auto& [ssharp_ud, disable_aniso] = TryDisableAnisoLod0(ssharp_handle);
                const auto ssharp = TrackSharp(ssharp_ud, info);
                const auto binding =
                    descriptors.Add(SamplerResource{ssharp, image_binding, disable_aniso});
                return binding;
            }
        }();
        // Patch image and sampler handle.
        inst.SetArg(0, ir.Imm32(image_binding | sampler_binding << 16));
    } else {
        // Patch image handle.
        inst.SetArg(0, ir.Imm32(image_binding));
    }
}

void PatchGlobalDataShareAccess(IR::Block& block, IR::Inst& inst, Info& info,
                                Descriptors& descriptors) {
    const u32 binding = descriptors.Add(BufferResource{
        .used_types = IR::Type::U32,
        .inline_cbuf = AmdGpu::Buffer::Null(),
        .buffer_type = BufferType::GdsBuffer,
        .is_written = true,
    });

    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};

    // For data append/consume operations attempt to deduce the GDS address.
    if (inst.GetOpcode() == IR::Opcode::DataAppend || inst.GetOpcode() == IR::Opcode::DataConsume) {
        const auto pred = [](const IR::Inst* inst) -> std::optional<const IR::Inst*> {
            if (inst->GetOpcode() == IR::Opcode::GetUserData) {
                return inst;
            }
            return std::nullopt;
        };

        u32 gds_addr = 0;
        const IR::Value& gds_offset = inst.Arg(0);
        if (gds_offset.IsImmediate()) {
            // Nothing to do, offset is known.
            gds_addr = gds_offset.U32() & 0xFFFF;
        } else {
            const auto result = IR::BreadthFirstSearch(&inst, pred);
            ASSERT_MSG(result, "Unable to track M0 source");

            // M0 must be set by some user data register.
            const IR::Inst* prod = gds_offset.InstRecursive();
            const u32 ud_reg = u32(result.value()->Arg(0).ScalarReg());
            u32 m0_val = info.user_data[ud_reg] >> 16;
            if (prod->GetOpcode() == IR::Opcode::IAdd32) {
                m0_val += prod->Arg(1).U32();
            }
            gds_addr = m0_val & 0xFFFF;
        }

        // Patch instruction.
        inst.SetArg(0, ir.Imm32(gds_addr >> 2));
        inst.SetArg(1, ir.Imm32(binding));
    } else {
        // Convert shared memory opcode to storage buffer atomic to GDS buffer.
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
            inst.ReplaceUsesWith(
                ir.BufferAtomicIAdd(handle, address_qwords, IR::U64{inst.Arg(1)}, {}));
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
        case IR::Opcode::LoadSharedU16:
            inst.ReplaceUsesWith(ir.LoadBufferU16(handle, address_words, {}));
            break;
        case IR::Opcode::LoadSharedU32:
            inst.ReplaceUsesWith(ir.LoadBufferU32(1, handle, address_dwords, {}));
            break;
        case IR::Opcode::LoadSharedU64:
            inst.ReplaceUsesWith(ir.LoadBufferU64(handle, address_qwords, {}));
            break;
        case IR::Opcode::WriteSharedU16:
            ir.StoreBufferU16(handle, address_words, IR::U16{inst.Arg(1)}, {});
            inst.Invalidate();
            break;
        case IR::Opcode::WriteSharedU32:
            ir.StoreBufferU32(1, handle, address_dwords, inst.Arg(1), {});
            inst.Invalidate();
            break;
        case IR::Opcode::WriteSharedU64:
            ir.StoreBufferU64(handle, address_qwords, IR::U64{inst.Arg(1)}, {});
            inst.Invalidate();
            break;
        default:
            UNREACHABLE();
        }
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

    // If address calculation is of the form "index * const_stride + offset" with offset constant
    // and both const_stride and offset are divisible with the element size, apply shift directly.
    if (inst_info.index_enable && !inst_info.offset_enable && !buffer.swizzle_enable &&
        !buffer.add_tid_enable && (stride & mask) == 0 && (inst_offset & mask) == 0) {
        // buffer_offset = index * (const_stride >> shift) + (inst_offset >> shift)
        const IR::U32 index = IR::U32{inst.Arg(1)};
        return ir.IAdd(ir.IMul(index, ir.Imm32(stride >> shift)), ir.Imm32(inst_offset >> shift));
    }

    // index = (inst_idxen ? vgpr_index : 0) + (const_add_tid_enable ? thread_id[5:0] : 0)
    IR::U32 index = ir.Imm32(0U);
    if (inst_info.index_enable) {
        const IR::U32 vgpr_index{inst_info.offset_enable
                                     ? IR::U32{ir.CompositeExtract(inst.Arg(1), 0)}
                                     : IR::U32{inst.Arg(1)}};
        index = ir.IAdd(index, vgpr_index);
    }
    if (buffer.add_tid_enable) {
        ASSERT_MSG(info.l_stage == LogicalStage::Compute,
                   "Thread ID buffer addressing is not supported outside of compute.");
        const IR::U32 thread_id{ir.LaneId()};
        index = ir.IAdd(index, thread_id);
    }
    // offset = (inst_offen ? vgpr_offset : 0) + inst_offset
    IR::U32 offset = ir.Imm32(inst_offset);
    if (inst_info.offset_enable) {
        const IR::U32 vgpr_offset = inst_info.index_enable
                                        ? IR::U32{ir.CompositeExtract(inst.Arg(1), 1)}
                                        : IR::U32{inst.Arg(1)};
        offset = ir.IAdd(offset, vgpr_offset);
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

void PatchBufferArgs(IR::Block& block, IR::Inst& inst, Info& info) {
    const auto handle = inst.Arg(0);
    const auto buffer_res = info.buffers[handle.U32()];
    const auto buffer = buffer_res.GetSharp(info);

    // Address of constant buffer reads can be calculated at IR emission time.
    if (inst.GetOpcode() == IR::Opcode::ReadConstBuffer) {
        return;
    }

    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    inst.SetArg(1, CalculateBufferAddress(ir, inst, info, buffer, buffer.stride));
}

IR::Value FixCubeCoords(IR::IREmitter& ir, const AmdGpu::Image& image, const IR::Value& x,
                        const IR::Value& y, const IR::Value& face) {
    if (!image.IsCube()) {
        return ir.CompositeConstruct(x, y, face);
    }
    // AMD cube math results in coordinates in the range [1.0, 2.0]. We need
    // to convert this to the range [0.0, 1.0] to get correct results.
    const auto fixed_x = ir.FPSub(IR::F32{x}, ir.Imm32(1.f));
    const auto fixed_y = ir.FPSub(IR::F32{y}, ir.Imm32(1.f));
    return ir.CompositeConstruct(fixed_x, fixed_y, face);
}

void PatchImageSampleArgs(IR::Block& block, IR::Inst& inst, Info& info,
                          const ImageResource& image_res, const AmdGpu::Image& image) {
    const auto handle = inst.Arg(0);
    const auto sampler_res = info.samplers[(handle.U32() >> 16) & 0xFFFF];
    auto sampler = sampler_res.GetSharp(info);

    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const auto inst_info = inst.Flags<IR::TextureInstInfo>();
    const auto view_type = image.GetViewType(image_res.is_array);

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

        switch (view_type) {
        case AmdGpu::ImageType::Color1D:
        case AmdGpu::ImageType::Color1DArray:
            return read(0);
        case AmdGpu::ImageType::Color2D:
        case AmdGpu::ImageType::Color2DMsaa:
        case AmdGpu::ImageType::Color2DArray:
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

    const auto unnormalized = sampler.force_unnormalized || inst_info.is_unnormalized;
    // Query dimensions of image if needed for normalization.
    // We can't use the image sharp because it could be bound to a different image later.
    const auto dimensions =
        unnormalized ? ir.ImageQueryDimension(handle, ir.Imm32(0u), ir.Imm1(false), inst_info)
                     : IR::Value{};
    const auto get_coord = [&](u32 coord_idx, u32 dim_idx) -> IR::Value {
        const auto coord = get_addr_reg(coord_idx);
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
            [[fallthrough]];
        case AmdGpu::ImageType::Color2D: // x, y
            addr_reg = addr_reg + 2;
            return ir.CompositeConstruct(get_coord(addr_reg - 2, 0), get_coord(addr_reg - 1, 1));
        case AmdGpu::ImageType::Color2DMsaa: // x, y, frag
            [[fallthrough]];
        case AmdGpu::ImageType::Color2DArray: // x, y, slice
            addr_reg = addr_reg + 3;
            // Note we can use FixCubeCoords with fallthrough cases since it checks for image type.
            return FixCubeCoords(ir, image, get_coord(addr_reg - 3, 0), get_coord(addr_reg - 2, 1),
                                 get_addr_reg(addr_reg - 1));
        case AmdGpu::ImageType::Color3D: // x, y, z
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

    const auto converted = ApplyReadNumberConversionVec4(ir, texel, image.GetNumberConversion());
    inst.ReplaceUsesWith(converted);
}

void PatchImageArgs(IR::Block& block, IR::Inst& inst, Info& info) {
    // Nothing to patch for dimension query.
    if (inst.GetOpcode() == IR::Opcode::ImageQueryDimensions) {
        return;
    }

    const auto handle = inst.Arg(0);
    const auto image_res = info.images[handle.U32() & 0xFFFF];
    auto image = image_res.GetSharp(info);

    // Sample instructions must be handled separately using address register data.
    if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
        PatchImageSampleArgs(block, inst, info, image_res, image);
        return;
    }

    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const auto inst_info = inst.Flags<IR::TextureInstInfo>();
    const auto view_type = image.GetViewType(image_res.is_array);

    // Now that we know the image type, adjust texture coordinate vector.
    IR::Inst* body = inst.Arg(1).InstRecursive();
    const auto [coords, arg] = [&] -> std::pair<IR::Value, IR::Value> {
        switch (view_type) {
        case AmdGpu::ImageType::Color1D: // x, [lod]
            return {body->Arg(0), body->Arg(1)};
        case AmdGpu::ImageType::Color1DArray: // x, slice, [lod]
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
        case AmdGpu::ImageType::Color3D: // x, y, z, [lod]
            return {ir.CompositeConstruct(body->Arg(0), body->Arg(1), body->Arg(2)), body->Arg(3)};
        default:
            UNREACHABLE_MSG("Unknown image type {}", view_type);
        }
    }();

    const auto has_ms = view_type == AmdGpu::ImageType::Color2DMsaa ||
                        view_type == AmdGpu::ImageType::Color2DMsaaArray;
    ASSERT(!inst_info.has_lod || !has_ms);
    const auto lod = inst_info.has_lod ? IR::U32{arg} : IR::U32{};
    const auto ms = has_ms ? IR::U32{arg} : IR::U32{};

    const auto is_storage = image_res.is_written;
    if (inst.GetOpcode() == IR::Opcode::ImageRead) {
        auto texel = ir.ImageRead(handle, coords, lod, ms, inst_info);
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

void ResourceTrackingPass(IR::Program& program) {
    // Iterate resource instructions and patch them after finding the sharp.
    auto& info = program.info;

    // Pass 1: Track resource sharps
    Descriptors descriptors{info};
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (IsBufferInstruction(inst)) {
                PatchBufferSharp(*block, inst, info, descriptors);
            } else if (IsImageInstruction(inst)) {
                PatchImageSharp(*block, inst, info, descriptors);
            }
        }
    }

    // Pass 2: Patch instruction args
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (IsBufferInstruction(inst)) {
                PatchBufferArgs(*block, inst, info);
            } else if (IsImageInstruction(inst)) {
                PatchImageArgs(*block, inst, info);
            } else if (IsDataRingInstruction(inst)) {
                PatchGlobalDataShareAccess(*block, inst, info, descriptors);
            }
        }
    }
}

} // namespace Shader::Optimization
