// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstring>
#include <gtest/gtest.h>

#include "shader_recompiler/info.h"
#include "shader_recompiler/params.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/recompiler.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/resource.h"

using namespace Shader;

namespace {

// s_mov_b32 vcc_hi, #0
// v_mov_b32 v2, s12
// image_sample_lz v[4:7], v[0:2], s[0:7], s[8:11] dmask:0xf da
// ds_write_b32 v0, v4
// s_endpgm
constexpr std::array<u32, 8> SampleLayerFromUserData{
    0xBEEB03FF, 0x00000000, 0x7E04020C, 0xF09C4F00,
    0x00400400, 0xD8340000, 0x00000400, 0xBF810000,
};
constexpr u32 LayerUserDataReg = 12;

bool HasFlatbuf(AmdGpu::ImageType image_type) {
    std::array<u32, ShaderParams::NumShaderUserData> user_data{};
    auto image = AmdGpu::Image::Null(false);
    image.type = u64(image_type);
    image.last_array = 1;
    std::memcpy(user_data.data(), &image, sizeof(image));
    user_data[LayerUserDataReg] = 1;

    const ShaderParams params{
        .user_data = user_data,
        .code = SampleLayerFromUserData,
        .hash = 0,
    };
    Info info{HwStage::Compute, SwStage::Compute, params};

    RuntimeInfo runtime_info{};
    runtime_info.Initialize(HwStage::Compute, SwStage::Compute);
    runtime_info.props.num_user_data = LayerUserDataReg + 1;
    runtime_info.hw.cs.workgroup_size = {1, 1, 1};

    Profile profile{};
    profile.supported_spirv = 0x00010600;
    profile.subgroup_size = 32;

    Pools pools{};
    const auto program = TranslateProgram(params.code, pools, info, runtime_info, profile);
    return program.info.has_flatbuf;
}

} // namespace

// The pipeline cache compiles a permutation per view type but binds the resources of the first
// one, so the resources a shader needs must not depend on the view type.
TEST(ImageAddressLiveness, FlatbufDoesNotDependOnViewType) {
    EXPECT_EQ(HasFlatbuf(AmdGpu::ImageType::Color2D),
              HasFlatbuf(AmdGpu::ImageType::Color2DArray));
}
