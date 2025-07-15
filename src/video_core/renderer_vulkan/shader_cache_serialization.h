// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>
#include "common/serialization.h"
#include "shader_recompiler/info.h"

#include "shader_recompiler/info.h"

namespace cereal {

// Shader::Info::UserDataMask
template <class Archive>
void serialize(Archive& ar, Shader::Info::UserDataMask& mask) {
    ar(mask.mask);
}

// Shader::CopyShaderData
template <class Archive>
void serialize(Archive& ar, Shader::CopyShaderData& data) {
    ar(data.attr_map, data.num_attrs, data.output_vertices);
}

// AmdGPU::Buffer
template <class Archive>
void serialize(Archive& ar, AmdGpu::Buffer& buffer) {
    ar(cereal::binary_data(reinterpret_cast<uint8_t*>(&buffer), sizeof(buffer)));
    // is base_adress cacheable?
}

// Shader::BufferResource
template <class Archive>
void serialize(Archive& ar, Shader::BufferResource& buffer) {
    ar(buffer.sharp_idx, buffer.used_types, buffer.inline_cbuf, buffer.buffer_type,
       buffer.instance_attrib, buffer.is_written, buffer.is_formatted);
}

// Shader::ImageResource
template <class Archive>
void serialize(Archive& ar, Shader::ImageResource& image) {
    ar(image.sharp_idx, image.is_depth, image.is_atomic, image.is_array, image.is_written,
       image.is_r128);
}

// AmdGpu::Sampler
template <class Archive>
void serialize(Archive& ar, AmdGpu::Sampler& sampler) {
    ar(cereal::binary_data(reinterpret_cast<u8*>(&sampler), sizeof(sampler)));
}

// Shader::SamplerResource
template <class Archive>
void serialize(Archive& ar, Shader::SamplerResource& sampler) {
    ar(sampler.sampler);
    ar(static_cast<u32>(sampler.associated_image), static_cast<u32>(sampler.disable_aniso));
}

// Shader::FMaskResource
template <class Archive>
void serialize(Archive& ar, Shader::FMaskResource& fmask) {
    cereal::binary_data(reinterpret_cast<uint8_t*>(&fmask), sizeof(fmask));
}

// Shader::Info::Interpolation
template <class Archive>
void serialize(Archive& ar, Shader::Info::Interpolation& interpolation) {
    ar(interpolation.primary, interpolation.auxiliary);
}
} // namespace cereal