// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstring>
#include <type_traits>

#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/condition.h"
#include "shader_recompiler/ir/patch.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::IR {

class IREmitter {
public:
    explicit IREmitter() = default;
    explicit IREmitter(Block& block_) : block{&block_}, insertion_point{block->end()} {}
    explicit IREmitter(Block& block_, Block::iterator insertion_point_)
        : block{&block_}, insertion_point{insertion_point_} {}

    Block* block;

    [[nodiscard]] U1 Imm1(bool value) const;
    [[nodiscard]] U8 Imm8(u8 value) const;
    [[nodiscard]] U16 Imm16(u16 value) const;
    [[nodiscard]] U32 Imm32(u32 value) const;
    [[nodiscard]] U32 Imm32(s32 value) const;
    [[nodiscard]] F32 Imm32(f32 value) const;
    [[nodiscard]] U64 Imm64(u64 value) const;
    [[nodiscard]] U64 Imm64(s64 value) const;
    [[nodiscard]] F64 Imm64(f64 value) const;

    template <typename Dest, typename Source>
    [[nodiscard]] Dest BitCast(const Source& value);

    U1 ConditionRef(const U1& value);
    void Reference(const Value& value);

    void PhiMove(IR::Inst& phi, const Value& value);

    void Prologue();
    void Epilogue();
    void Discard();
    void Discard(const U1& cond);
    void DebugPrint(const char* fmt, boost::container::small_vector<Value, 5> args);

    void Barrier();
    void WorkgroupMemoryBarrier();
    void DeviceMemoryBarrier();

    [[nodiscard]] U32 GetUserData(IR::ScalarReg reg);
    [[nodiscard]] U1 GetThreadBitScalarReg(IR::ScalarReg reg);
    void SetThreadBitScalarReg(IR::ScalarReg reg, const U1& value);

    template <typename T = U32>
    [[nodiscard]] T GetScalarReg(IR::ScalarReg reg);
    template <typename T = U32>
    [[nodiscard]] T GetVectorReg(IR::VectorReg reg);
    void SetScalarReg(IR::ScalarReg reg, const U32F32& value);
    void SetVectorReg(IR::VectorReg reg, const U32F32& value);

    [[nodiscard]] U1 GetGotoVariable(u32 id);
    void SetGotoVariable(u32 id, const U1& value);

    [[nodiscard]] U1 GetScc();
    [[nodiscard]] U1 GetExec();
    [[nodiscard]] U1 GetVcc();
    [[nodiscard]] U32 GetVccLo();
    [[nodiscard]] U32 GetVccHi();
    [[nodiscard]] U32 GetM0();
    void SetScc(const U1& value);
    void SetExec(const U1& value);
    void SetVcc(const U1& value);
    void SetSccLo(const U32& value);
    void SetVccLo(const U32& value);
    void SetVccHi(const U32& value);
    void SetM0(const U32& value);

    [[nodiscard]] U1 Condition(IR::Condition cond);

    [[nodiscard]] F32 GetAttribute(Attribute attribute, u32 comp = 0, u32 index = 0);
    [[nodiscard]] U32 GetAttributeU32(Attribute attribute, u32 comp = 0);
    void SetAttribute(Attribute attribute, const F32& value, u32 comp = 0);

    [[nodiscard]] F32 GetTessGenericAttribute(const U32& vertex_index, const U32& attr_index,
                                              const U32& comp_index);
    void SetTcsGenericAttribute(const F32& value, const U32& attr_index, const U32& comp_index);

    [[nodiscard]] F32 ReadTcsGenericOuputAttribute(const U32& vertex_index, const U32& attr_index,
                                                   const U32& comp_index);

    [[nodiscard]] F32 GetPatch(Patch patch);
    void SetPatch(Patch patch, const F32& value);

    [[nodiscard]] Value LoadShared(int bit_size, bool is_signed, const U32& offset,
                                   bool is_gds = false);
    void WriteShared(int bit_size, const Value& value, const U32& offset, bool is_gds = false);

    [[nodiscard]] U32U64 SharedAtomicIAdd(const U32& address, const U32U64& data, bool is_gds);
    [[nodiscard]] U32U64 SharedAtomicISub(const U32& address, const U32U64& data, bool is_gds);
    [[nodiscard]] U32U64 SharedAtomicIMin(const U32& address, const U32U64& data, bool is_signed,
                                          bool is_gds);
    [[nodiscard]] U32U64 SharedAtomicIMax(const U32& address, const U32U64& data, bool is_signed,
                                          bool is_gds);
    [[nodiscard]] U32U64 SharedAtomicAnd(const U32& address, const U32U64& data, bool is_gds);
    [[nodiscard]] U32U64 SharedAtomicOr(const U32& address, const U32U64& data, bool is_gds);
    [[nodiscard]] U32U64 SharedAtomicXor(const U32& address, const U32U64& data, bool is_gds);

    template <typename T = U32>
    [[nodiscard]] T SharedAtomicInc(const U32& address, bool is_gds);
    template <typename T = U32>
    [[nodiscard]] T SharedAtomicDec(const U32& address, bool is_gds);

    [[nodiscard]] U32 ReadConst(const Value& base, const U32& offset);
    [[nodiscard]] U32 ReadConstBuffer(const Value& handle, const U32& index);

    [[nodiscard]] U8 LoadBufferU8(const Value& handle, const Value& address, BufferInstInfo info);
    [[nodiscard]] U16 LoadBufferU16(const Value& handle, const Value& address, BufferInstInfo info);
    [[nodiscard]] Value LoadBufferU32(int num_dwords, const Value& handle, const Value& address,
                                      BufferInstInfo info);
    [[nodiscard]] U64 LoadBufferU64(const Value& handle, const Value& address, BufferInstInfo info);
    [[nodiscard]] Value LoadBufferF32(int num_dwords, const Value& handle, const Value& address,
                                      BufferInstInfo info);
    [[nodiscard]] Value LoadBufferFormat(const Value& handle, const Value& address,
                                         BufferInstInfo info);
    void StoreBufferU8(const Value& handle, const Value& address, const U8& data,
                       BufferInstInfo info);
    void StoreBufferU16(const Value& handle, const Value& address, const U16& data,
                        BufferInstInfo info);
    void StoreBufferU32(int num_dwords, const Value& handle, const Value& address,
                        const Value& data, BufferInstInfo info);
    void StoreBufferU64(const Value& handle, const Value& address, const U64& data,
                        BufferInstInfo info);
    void StoreBufferF32(int num_dwords, const Value& handle, const Value& address,
                        const Value& data, BufferInstInfo info);
    void StoreBufferFormat(const Value& handle, const Value& address, const Value& data,
                           BufferInstInfo info);

    [[nodiscard]] Value BufferAtomicIAdd(const Value& handle, const Value& address,
                                         const Value& value, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicISub(const Value& handle, const Value& address,
                                         const Value& value, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicIMin(const Value& handle, const Value& address,
                                         const Value& value, bool is_signed, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicFMin(const Value& handle, const Value& address,
                                         const Value& value, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicIMax(const Value& handle, const Value& address,
                                         const Value& value, bool is_signed, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicFMax(const Value& handle, const Value& address,
                                         const Value& value, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicInc(const Value& handle, const Value& address,
                                        BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicDec(const Value& handle, const Value& address,
                                        BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicAnd(const Value& handle, const Value& address,
                                        const Value& value, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicOr(const Value& handle, const Value& address,
                                       const Value& value, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicXor(const Value& handle, const Value& address,
                                        const Value& value, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicSwap(const Value& handle, const Value& address,
                                         const Value& value, BufferInstInfo info);
    [[nodiscard]] Value BufferAtomicCmpSwap(const Value& handle, const Value& address,
                                            const Value& value, const Value& cmp_value,
                                            BufferInstInfo info);

    [[nodiscard]] U32 DataAppend(const U32& counter);
    [[nodiscard]] U32 DataConsume(const U32& counter);
    [[nodiscard]] U32 LaneId();
    [[nodiscard]] U32 WarpId();
    [[nodiscard]] U32 QuadShuffle(const U32& value, const U32& index);
    [[nodiscard]] U32 ReadFirstLane(const U32& value);
    [[nodiscard]] U32 ReadLane(const U32& value, const U32& lane);
    [[nodiscard]] U32 WriteLane(const U32& value, const U32& write_value, const U32& lane);
    [[nodiscard]] Value Ballot(const U1& bit);
    [[nodiscard]] U32 BallotFindLsb(const Value& mask);

    [[nodiscard]] Value CompositeConstruct(const Value& e1, const Value& e2);
    [[nodiscard]] Value CompositeConstruct(const Value& e1, const Value& e2, const Value& e3);
    [[nodiscard]] Value CompositeConstruct(const Value& e1, const Value& e2, const Value& e3,
                                           const Value& e4);
    [[nodiscard]] Value CompositeConstruct(std::span<const Value> values);

    [[nodiscard]] Value CompositeExtract(const Value& vector, size_t element);
    [[nodiscard]] Value CompositeInsert(const Value& vector, const Value& object, size_t element);

    [[nodiscard]] Value CompositeShuffle(const Value& vector1, const Value& vector2, size_t comp0,
                                         size_t comp1);
    [[nodiscard]] Value CompositeShuffle(const Value& vector1, const Value& vector2, size_t comp0,
                                         size_t comp1, size_t comp2);
    [[nodiscard]] Value CompositeShuffle(const Value& vector1, const Value& vector2, size_t comp0,
                                         size_t comp1, size_t comp2, size_t comp3);

    [[nodiscard]] Value Select(const U1& condition, const Value& true_value,
                               const Value& false_value);

    [[nodiscard]] U64 PackUint2x32(const Value& vector);
    [[nodiscard]] Value UnpackUint2x32(const U64& value);
    [[nodiscard]] F64 PackDouble2x32(const Value& vector);
    [[nodiscard]] Value UnpackDouble2x32(const F64& value);

    [[nodiscard]] U32 Pack2x16(AmdGpu::NumberFormat number_format, const Value& vector);
    [[nodiscard]] Value Unpack2x16(AmdGpu::NumberFormat number_format, const U32& value);

    [[nodiscard]] U32 Pack4x8(AmdGpu::NumberFormat number_format, const Value& vector);
    [[nodiscard]] Value Unpack4x8(AmdGpu::NumberFormat number_format, const U32& value);

    [[nodiscard]] U32 Pack10_11_11(AmdGpu::NumberFormat number_format, const Value& vector);
    [[nodiscard]] Value Unpack10_11_11(AmdGpu::NumberFormat number_format, const U32& value);

    [[nodiscard]] U32 Pack2_10_10_10(AmdGpu::NumberFormat number_format, const Value& vector);
    [[nodiscard]] Value Unpack2_10_10_10(AmdGpu::NumberFormat number_format, const U32& value);

    [[nodiscard]] F32F64 FPAdd(const F32F64& a, const F32F64& b);
    [[nodiscard]] F32F64 FPSub(const F32F64& a, const F32F64& b);
    [[nodiscard]] F32F64 FPMul(const F32F64& a, const F32F64& b);
    [[nodiscard]] F32F64 FPDiv(const F32F64& a, const F32F64& b);
    [[nodiscard]] F32F64 FPFma(const F32F64& a, const F32F64& b, const F32F64& c);

    [[nodiscard]] F32F64 FPAbs(const F32F64& value);
    [[nodiscard]] F32F64 FPNeg(const F32F64& value);
    [[nodiscard]] F32F64 FPAbsNeg(const F32F64& value, bool abs, bool neg);

    [[nodiscard]] F32 FPCos(const F32& value);
    [[nodiscard]] F32 FPSin(const F32& value);
    [[nodiscard]] F32 FPExp2(const F32& value);
    [[nodiscard]] F32 FPLog2(const F32& value);
    [[nodiscard]] F32 FPLdexp(const F32& value, const U32& exp);
    [[nodiscard]] F32F64 FPRecip(const F32F64& value);
    [[nodiscard]] F32F64 FPRecipSqrt(const F32F64& value);
    [[nodiscard]] F32 FPSqrt(const F32& value);
    [[nodiscard]] F32F64 FPSaturate(const F32F64& value);
    [[nodiscard]] F32F64 FPClamp(const F32F64& value, const F32F64& min_value,
                                 const F32F64& max_value);
    [[nodiscard]] F32F64 FPRoundEven(const F32F64& value);
    [[nodiscard]] F32F64 FPFloor(const F32F64& value);
    [[nodiscard]] F32F64 FPCeil(const F32F64& value);
    [[nodiscard]] F32F64 FPTrunc(const F32F64& value);
    [[nodiscard]] F32F64 FPFract(const F32F64& value);
    [[nodiscard]] F32F64 FPFrexpSig(const F32F64& value);
    [[nodiscard]] U32 FPFrexpExp(const F32F64& value);

    [[nodiscard]] U1 FPEqual(const F32F64& lhs, const F32F64& rhs, bool ordered = true);
    [[nodiscard]] U1 FPNotEqual(const F32F64& lhs, const F32F64& rhs, bool ordered = true);
    [[nodiscard]] U1 FPLessThanEqual(const F32F64& lhs, const F32F64& rhs, bool ordered = true);
    [[nodiscard]] U1 FPGreaterThanEqual(const F32F64& lhs, const F32F64& rhs, bool ordered = true);
    [[nodiscard]] U1 FPLessThan(const F32F64& lhs, const F32F64& rhs, bool ordered = true);
    [[nodiscard]] U1 FPGreaterThan(const F32F64& lhs, const F32F64& rhs, bool ordered = true);
    [[nodiscard]] U1 FPIsNan(const F32F64& value);
    [[nodiscard]] U1 FPIsInf(const F32F64& value);
    [[nodiscard]] U1 FPCmpClass32(const F32& value, const U32& op);
    [[nodiscard]] U1 FPOrdered(const F32F64& lhs, const F32F64& rhs);
    [[nodiscard]] U1 FPUnordered(const F32F64& lhs, const F32F64& rhs);
    [[nodiscard]] F32F64 FPMax(const F32F64& lhs, const F32F64& rhs, bool is_legacy = false);
    [[nodiscard]] F32F64 FPMin(const F32F64& lhs, const F32F64& rhs, bool is_legacy = false);
    [[nodiscard]] F32F64 FPMinTri(const F32F64& a, const F32F64& b, const F32F64& c);
    [[nodiscard]] F32F64 FPMaxTri(const F32F64& a, const F32F64& b, const F32F64& c);
    [[nodiscard]] F32F64 FPMedTri(const F32F64& a, const F32F64& b, const F32F64& c);

    [[nodiscard]] U32U64 IAdd(const U32U64& a, const U32U64& b);
    [[nodiscard]] Value IAddCarry(const U32& a, const U32& b);
    [[nodiscard]] U32U64 ISub(const U32U64& a, const U32U64& b);
    [[nodiscard]] U32 IMulHi(const U32& a, const U32& b, bool is_signed = false);
    [[nodiscard]] U32U64 IMul(const U32U64& a, const U32U64& b);
    [[nodiscard]] U32 IDiv(const U32& a, const U32& b, bool is_signed = false);
    [[nodiscard]] U32 IMod(const U32& a, const U32& b, bool is_signed = false);
    [[nodiscard]] U32U64 INeg(const U32U64& value);
    [[nodiscard]] U32 IAbs(const U32& value);
    [[nodiscard]] U32U64 ShiftLeftLogical(const U32U64& base, const U32& shift);
    [[nodiscard]] U32U64 ShiftRightLogical(const U32U64& base, const U32& shift);
    [[nodiscard]] U32U64 ShiftRightArithmetic(const U32U64& base, const U32& shift);
    [[nodiscard]] U32U64 BitwiseAnd(const U32U64& a, const U32U64& b);
    [[nodiscard]] U32U64 BitwiseOr(const U32U64& a, const U32U64& b);
    [[nodiscard]] U32 BitwiseXor(const U32& a, const U32& b);
    [[nodiscard]] U32 BitFieldInsert(const U32& base, const U32& insert, const U32& offset,
                                     const U32& count);
    [[nodiscard]] U32 BitFieldExtract(const U32& base, const U32& offset, const U32& count,
                                      bool is_signed = false);
    [[nodiscard]] U32 BitReverse(const U32& value);
    [[nodiscard]] U32 BitCount(const U32U64& value);
    [[nodiscard]] U32 BitwiseNot(const U32& value);

    [[nodiscard]] U32 FindSMsb(const U32& value);
    [[nodiscard]] U32 FindUMsb(const U32U64& value);
    [[nodiscard]] U32 FindILsb(const U32U64& value);
    [[nodiscard]] U32 SMin(const U32& a, const U32& b);
    [[nodiscard]] U32 UMin(const U32& a, const U32& b);
    [[nodiscard]] U32 IMin(const U32& a, const U32& b, bool is_signed);
    [[nodiscard]] U32 SMax(const U32& a, const U32& b);
    [[nodiscard]] U32 UMax(const U32& a, const U32& b);
    [[nodiscard]] U32 IMax(const U32& a, const U32& b, bool is_signed);
    [[nodiscard]] U32 SMinTri(const U32& a, const U32& b, const U32& c);
    [[nodiscard]] U32 UMinTri(const U32& a, const U32& b, const U32& c);
    [[nodiscard]] U32 IMinTri(const U32& a, const U32& b, const U32& c, bool is_signed);
    [[nodiscard]] U32 SMaxTri(const U32& a, const U32& b, const U32& c);
    [[nodiscard]] U32 UMaxTri(const U32& a, const U32& b, const U32& c);
    [[nodiscard]] U32 IMaxTri(const U32& a, const U32& b, const U32& c, bool is_signed);
    [[nodiscard]] U32 SMedTri(const U32& a, const U32& b, const U32& c);
    [[nodiscard]] U32 UMedTri(const U32& a, const U32& b, const U32& c);
    [[nodiscard]] U32 IMedTri(const U32& a, const U32& b, const U32& c, bool is_signed);
    [[nodiscard]] U32 SClamp(const U32& value, const U32& min, const U32& max);
    [[nodiscard]] U32 UClamp(const U32& value, const U32& min, const U32& max);

    [[nodiscard]] U1 ILessThan(const U32U64& lhs, const U32U64& rhs, bool is_signed);
    [[nodiscard]] U1 IEqual(const U32U64& lhs, const U32U64& rhs);
    [[nodiscard]] U1 ILessThanEqual(const U32U64& lhs, const U32U64& rhs, bool is_signed);
    [[nodiscard]] U1 IGreaterThan(const U32U64& lhs, const U32U64& rhs, bool is_signed);
    [[nodiscard]] U1 INotEqual(const U32U64& lhs, const U32U64& rhs);
    [[nodiscard]] U1 IGreaterThanEqual(const U32U64& lhs, const U32U64& rhs, bool is_signed);

    [[nodiscard]] U1 LogicalOr(const U1& a, const U1& b);
    [[nodiscard]] U1 LogicalAnd(const U1& a, const U1& b);
    [[nodiscard]] U1 LogicalXor(const U1& a, const U1& b);
    [[nodiscard]] U1 LogicalNot(const U1& value);

    [[nodiscard]] U32U64 ConvertFToS(size_t bitsize, const F32F64& value);
    [[nodiscard]] U32U64 ConvertFToU(size_t bitsize, const F32F64& value);
    [[nodiscard]] U32U64 ConvertFToI(size_t bitsize, bool is_signed, const F32F64& value);
    [[nodiscard]] F32F64 ConvertSToF(size_t dest_bitsize, size_t src_bitsize, const Value& value);
    [[nodiscard]] F32F64 ConvertUToF(size_t dest_bitsize, size_t src_bitsize, const Value& value);
    [[nodiscard]] F32F64 ConvertIToF(size_t dest_bitsize, size_t src_bitsize, bool is_signed,
                                     const Value& value);

    [[nodiscard]] U8U16U32U64 UConvert(size_t result_bitsize, const U8U16U32U64& value);
    [[nodiscard]] U8U16U32U64 SConvert(size_t result_bitsize, const U8U16U32U64& value);
    [[nodiscard]] F16F32F64 FPConvert(size_t result_bitsize, const F16F32F64& value);

    [[nodiscard]] Value ImageAtomicIAdd(const Value& handle, const Value& coords,
                                        const Value& value, TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicSMin(const Value& handle, const Value& coords,
                                        const Value& value, TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicUMin(const Value& handle, const Value& coords,
                                        const Value& value, TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicIMin(const Value& handle, const Value& coords,
                                        const Value& value, bool is_signed, TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicSMax(const Value& handle, const Value& coords,
                                        const Value& value, TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicUMax(const Value& handle, const Value& coords,
                                        const Value& value, TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicFMax(const Value& handle, const Value& coords,
                                        const Value& value, TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicFMin(const Value& handle, const Value& coords,
                                        const Value& value, TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicIMax(const Value& handle, const Value& coords,
                                        const Value& value, bool is_signed, TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicInc(const Value& handle, const Value& coords, const Value& value,
                                       TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicDec(const Value& handle, const Value& coords, const Value& value,
                                       TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicAnd(const Value& handle, const Value& coords, const Value& value,
                                       TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicOr(const Value& handle, const Value& coords, const Value& value,
                                      TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicXor(const Value& handle, const Value& coords, const Value& value,
                                       TextureInstInfo info);
    [[nodiscard]] Value ImageAtomicExchange(const Value& handle, const Value& coords,
                                            const Value& value, TextureInstInfo info);

    [[nodiscard]] Value ImageSampleRaw(const Value& image_handle, const Value& sampler_handle,
                                       const Value& address1, const Value& address2,
                                       const Value& address3, const Value& address4,
                                       TextureInstInfo info);

    [[nodiscard]] Value ImageSampleImplicitLod(const Value& handle, const Value& body,
                                               const F32& bias, const Value& offset,
                                               TextureInstInfo info);

    [[nodiscard]] Value ImageSampleExplicitLod(const Value& handle, const Value& body,
                                               const F32& lod, const Value& offset,
                                               TextureInstInfo info);

    [[nodiscard]] Value ImageSampleDrefImplicitLod(const Value& handle, const Value& body,
                                                   const F32& dref, const F32& bias,
                                                   const Value& offset, TextureInstInfo info);

    [[nodiscard]] Value ImageSampleDrefExplicitLod(const Value& handle, const Value& body,
                                                   const F32& dref, const F32& lod,
                                                   const Value& offset, TextureInstInfo info);

    [[nodiscard]] Value ImageQueryDimension(const Value& handle, const U32& lod,
                                            const U1& skip_mips, TextureInstInfo info);

    [[nodiscard]] Value ImageQueryLod(const Value& handle, const Value& coords,
                                      TextureInstInfo info);
    [[nodiscard]] Value ImageGather(const Value& handle, const Value& coords, const Value& offset,
                                    TextureInstInfo info);
    [[nodiscard]] Value ImageGatherDref(const Value& handle, const Value& coords,
                                        const Value& offset, const F32& dref, TextureInstInfo info);
    [[nodiscard]] Value ImageGradient(const Value& handle, const Value& coords,
                                      const Value& derivatives_dx, const Value& derivatives_dy,
                                      const Value& offset, const F32& lod_clamp,
                                      TextureInstInfo info);
    [[nodiscard]] Value ImageRead(const Value& handle, const Value& coords, const U32& lod,
                                  const U32& multisampling, TextureInstInfo info);
    void ImageWrite(const Value& handle, const Value& coords, const U32& lod,
                    const U32& multisampling, const Value& color, TextureInstInfo info);

    [[nodiscard]] F32 CubeFaceIndex(const Value& cube_coords);

    void EmitVertex();
    void EmitPrimitive();

private:
    IR::Block::iterator insertion_point;

    template <typename T = Value, typename... Args>
    T Inst(Opcode op, Args... args) {
        auto it{block->PrependNewInst(insertion_point, op, {Value{args}...})};
        it->SetParent(block);
        return T{Value{&*it}};
    }

    template <typename T>
        requires(sizeof(T) <= sizeof(u32) && std::is_trivially_copyable_v<T>)
    struct Flags {
        Flags() = default;
        Flags(T proxy_) : proxy{proxy_} {}

        T proxy;
    };

    template <typename T = Value, typename FlagType, typename... Args>
    T Inst(Opcode op, Flags<FlagType> flags, Args... args) {
        u32 raw_flags{};
        std::memcpy(&raw_flags, &flags.proxy, sizeof(flags.proxy));
        auto it{block->PrependNewInst(insertion_point, op, {Value{args}...}, raw_flags)};
        it->SetParent(block);
        return T{Value{&*it}};
    }
};

} // namespace Shader::IR
