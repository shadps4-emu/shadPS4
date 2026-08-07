// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <spirv/unified1/spirv.hpp11>

#include "gcn_test_runner.hpp"
#include "shader_recompiler/frontend/opcodes.h"
#include "translator.hpp"

namespace {

using Shader::Gcn::InstEncoding;
using Shader::Gcn::OpcodeDS;

constexpr u64 EncodeDs(OpcodeDS opcode, u8 address, u8 data0 = 0, u8 data1 = 0, u8 dest = 0,
                       u8 offset0 = 0, u8 offset1 = 0) {
    return u64{offset0} | (u64{offset1} << 8) | (u64{std::to_underlying(opcode)} << 18) |
           (u64{std::to_underlying(InstEncoding::DS)} << 0) | (u64{address} << 32) |
           (u64{data0} << 40) | (u64{data1} << 48) | (u64{dest} << 56);
}

struct SpirvInstruction {
    spv::Op opcode;
    std::span<const u32> operands;
};

std::vector<SpirvInstruction> DecodeSpirv(std::span<const u32> spirv) {
    constexpr size_t HeaderWords = 5;
    EXPECT_GE(spirv.size(), HeaderWords);
    std::vector<SpirvInstruction> instructions;
    for (size_t offset = HeaderWords; offset < spirv.size();) {
        const u32 word_count = spirv[offset] >> 16;
        const auto opcode = static_cast<spv::Op>(spirv[offset] & 0xffff);
        EXPECT_GT(word_count, 0u);
        EXPECT_LE(offset + word_count, spirv.size());
        if (word_count == 0 || offset + word_count > spirv.size()) {
            break;
        }
        instructions.push_back({opcode, spirv.subspan(offset + 1, word_count - 1)});
        offset += word_count;
    }
    return instructions;
}

void ExpectGuardedWorkgroupAccess(std::span<const u32> spirv, spv::Op memory_opcode,
                                  bool returns_zero) {
    const auto instructions = DecodeSpirv(spirv);

    std::unordered_set<u32> workgroup_variables;
    std::unordered_set<u32> zero_constants;
    std::unordered_set<u32> bounds_conditions;
    for (const auto& inst : instructions) {
        if (inst.opcode == spv::Op::OpVariable && inst.operands.size() >= 3 &&
            inst.operands[2] == std::to_underlying(spv::StorageClass::Workgroup)) {
            workgroup_variables.insert(inst.operands[1]);
        } else if (inst.opcode == spv::Op::OpConstant && inst.operands.size() >= 3 &&
                   std::ranges::all_of(inst.operands.subspan(2),
                                       [](u32 word) { return word == 0; })) {
            zero_constants.insert(inst.operands[1]);
        } else if (inst.opcode == spv::Op::OpULessThan && inst.operands.size() >= 4) {
            bounds_conditions.insert(inst.operands[1]);
        }
    }
    ASSERT_FALSE(workgroup_variables.empty());
    ASSERT_FALSE(bounds_conditions.empty());

    std::unordered_set<u32> guarded_blocks;
    std::unordered_set<u32> merge_blocks;
    for (const auto& inst : instructions) {
        if (inst.opcode == spv::Op::OpBranchConditional && inst.operands.size() >= 3 &&
            bounds_conditions.contains(inst.operands[0])) {
            guarded_blocks.insert(inst.operands[1]);
            merge_blocks.insert(inst.operands[2]);
        }
    }
    ASSERT_FALSE(guarded_blocks.empty());

    u32 current_block{};
    std::unordered_set<u32> guarded_pointers;
    bool found_memory_operation = false;
    bool found_zero_phi = false;
    for (const auto& inst : instructions) {
        if (inst.opcode == spv::Op::OpLabel && !inst.operands.empty()) {
            current_block = inst.operands[0];
            continue;
        }
        if (inst.opcode == spv::Op::OpAccessChain && inst.operands.size() >= 3 &&
            workgroup_variables.contains(inst.operands[2])) {
            EXPECT_TRUE(guarded_blocks.contains(current_block))
                << "Workgroup OpAccessChain was emitted outside the in-bounds block";
            guarded_pointers.insert(inst.operands[1]);
            continue;
        }

        const auto uses_guarded_pointer = [&] {
            if (memory_opcode == spv::Op::OpStore) {
                return inst.opcode == memory_opcode && !inst.operands.empty() &&
                       guarded_pointers.contains(inst.operands[0]);
            }
            return inst.opcode == memory_opcode && inst.operands.size() >= 3 &&
                   guarded_pointers.contains(inst.operands[2]);
        }();
        if (uses_guarded_pointer) {
            EXPECT_TRUE(guarded_blocks.contains(current_block));
            found_memory_operation = true;
        }

        if (returns_zero && inst.opcode == spv::Op::OpPhi && merge_blocks.contains(current_block) &&
            inst.operands.size() >= 6) {
            for (size_t operand = 2; operand + 1 < inst.operands.size(); operand += 2) {
                if (zero_constants.contains(inst.operands[operand]) &&
                    !guarded_blocks.contains(inst.operands[operand + 1])) {
                    found_zero_phi = true;
                }
            }
        }
    }

    EXPECT_FALSE(guarded_pointers.empty());
    EXPECT_TRUE(found_memory_operation);
    EXPECT_EQ(found_zero_phi, returns_zero);
}

void ExpectBoundsLimit(std::span<const u32> spirv, u32 expected_limit) {
    const auto instructions = DecodeSpirv(spirv);
    std::unordered_map<u32, u32> constants;
    for (const auto& inst : instructions) {
        if (inst.opcode == spv::Op::OpConstant && inst.operands.size() == 3) {
            constants.emplace(inst.operands[1], inst.operands[2]);
        }
    }
    const auto comparison = std::ranges::find_if(instructions, [](const auto& inst) {
        return inst.opcode == spv::Op::OpULessThan && inst.operands.size() >= 4;
    });
    ASSERT_NE(comparison, instructions.end());
    ASSERT_TRUE(constants.contains(comparison->operands[3]));
    EXPECT_EQ(constants.at(comparison->operands[3]), expected_limit);
}

TEST(LdsBoundsCodegen, LoadB32ReturnsZeroWithoutCreatingAnOutOfBoundsPointer) {
    constexpr u32 LdsSize = 16;
    const auto spirv = TranslateToSpirv(EncodeDs(OpcodeDS::DS_READ_B32, 0, 0, 0, 0), LdsSize);
    ExpectGuardedWorkgroupAccess(spirv, spv::Op::OpLoad, true);
    ExpectBoundsLimit(spirv, LdsSize - sizeof(u32) + 1);
}

TEST(LdsBoundsCodegen, LoadB64ReturnsZeroWithoutCreatingAnOutOfBoundsPointer) {
    constexpr u32 LdsSize = 16;
    const auto spirv = TranslateToSpirv(EncodeDs(OpcodeDS::DS_READ_B64, 0, 0, 0, 0), LdsSize);
    ExpectGuardedWorkgroupAccess(spirv, spv::Op::OpLoad, true);
    ExpectBoundsLimit(spirv, LdsSize - sizeof(u64) + 1);
}

TEST(LdsBoundsCodegen, WriteB32IsDiscardedWithoutCreatingAnOutOfBoundsPointer) {
    constexpr u32 LdsSize = 16;
    const auto spirv = TranslateToSpirv(EncodeDs(OpcodeDS::DS_WRITE_B32, 0, 1), LdsSize);
    ExpectGuardedWorkgroupAccess(spirv, spv::Op::OpStore, false);
}

TEST(LdsBoundsCodegen, ReturningAtomicIsDiscardedAndReturnsZero) {
    constexpr u32 LdsSize = 16;
    const auto spirv = TranslateToSpirv(EncodeDs(OpcodeDS::DS_ADD_RTN_U32, 0, 1, 0, 0), LdsSize);
    ExpectGuardedWorkgroupAccess(spirv, spv::Op::OpAtomicIAdd, true);
}

TEST(LdsBoundsRuntime, ReadB32AtAllocationEndReturnsZero) {
#ifdef __APPLE__
    GTEST_SKIP() << "The Vulkan GCN runner is unavailable under this checkout's MoltenVK setup";
#endif
    constexpr u32 LdsSize = 16;
    const auto runner = gcn_test::Runner::instance();
    if (!runner) {
        GTEST_SKIP() << runner.error().message;
    }
    const auto spirv = TranslateToSpirv(EncodeDs(OpcodeDS::DS_READ_B32, 0, 0, 0, 0), LdsSize);
    const auto result = (*runner)->run<u32>(spirv, LdsSize);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(*result, 0u);
}

TEST(LdsBoundsRuntime, ReadB64CrossingAllocationEndReturnsZero) {
#ifdef __APPLE__
    GTEST_SKIP() << "The Vulkan GCN runner is unavailable under this checkout's MoltenVK setup";
#endif
    constexpr u32 LdsSize = 16;
    const auto runner = gcn_test::Runner::instance();
    if (!runner) {
        GTEST_SKIP() << runner.error().message;
    }
    const auto spirv = TranslateToSpirv(EncodeDs(OpcodeDS::DS_READ_B64, 0, 0, 0, 0), LdsSize);
    const auto result = (*runner)->run<u32>(spirv, LdsSize - sizeof(u32));
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(*result, 0u);
}

TEST(LdsBoundsRuntime, OutOfRangeWriteDoesNotChangeValidLds) {
#ifdef __APPLE__
    GTEST_SKIP() << "The Vulkan GCN runner is unavailable under this checkout's MoltenVK setup";
#endif
    constexpr u32 LdsSize = 16;
    constexpr u32 Sentinel = 0x12345678;
    const auto runner = gcn_test::Runner::instance();
    if (!runner) {
        GTEST_SKIP() << runner.error().message;
    }
    const std::array instructions{
        EncodeDs(OpcodeDS::DS_WRITE_B32, 0, 1),
        EncodeDs(OpcodeDS::DS_WRITE_B32, 2, 3),
        EncodeDs(OpcodeDS::DS_READ_B32, 0, 0, 0, 0),
    };
    const auto spirv = TranslateToSpirv(instructions, LdsSize);
    const auto result = (*runner)->run<u32>(spirv, std::array{0u, Sentinel, LdsSize, 0xdeadbeefu});
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(*result, Sentinel);
}

TEST(LdsBoundsRuntime, OutOfRangeAtomicDoesNotChangeValidLds) {
#ifdef __APPLE__
    GTEST_SKIP() << "The Vulkan GCN runner is unavailable under this checkout's MoltenVK setup";
#endif
    constexpr u32 LdsSize = 16;
    constexpr u32 Sentinel = 7;
    const auto runner = gcn_test::Runner::instance();
    if (!runner) {
        GTEST_SKIP() << runner.error().message;
    }
    const std::array instructions{
        EncodeDs(OpcodeDS::DS_WRITE_B32, 0, 1),
        EncodeDs(OpcodeDS::DS_ADD_U32, 2, 3),
        EncodeDs(OpcodeDS::DS_READ_B32, 0, 0, 0, 0),
    };
    const auto spirv = TranslateToSpirv(instructions, LdsSize);
    const auto result = (*runner)->run<u32>(spirv, std::array{0u, Sentinel, LdsSize, 5u});
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(*result, Sentinel);
}

TEST(LdsBoundsRuntime, ReturningOutOfRangeAtomicReturnsZero) {
#ifdef __APPLE__
    GTEST_SKIP() << "The Vulkan GCN runner is unavailable under this checkout's MoltenVK setup";
#endif
    constexpr u32 LdsSize = 16;
    const auto runner = gcn_test::Runner::instance();
    if (!runner) {
        GTEST_SKIP() << runner.error().message;
    }
    const auto spirv = TranslateToSpirv(EncodeDs(OpcodeDS::DS_ADD_RTN_U32, 0, 1, 0, 0), LdsSize);
    const auto result = (*runner)->run<u32>(spirv, std::array{LdsSize, 5u});
    ASSERT_TRUE(result.has_value()) << result.error().message;
    EXPECT_EQ(*result, 0u);
}

} // namespace
