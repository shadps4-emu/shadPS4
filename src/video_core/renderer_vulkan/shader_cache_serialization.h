// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cereal/types/map.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/utility.hpp>
#include "shader_recompiler/info.h"

namespace cereal {

// boost::small_vector
template<class Archive, class T, std::size_t N, class Alloc>
void save(Archive& ar, boost::container::small_vector<T, N, Alloc> const& smallVector) {
    ar(static_cast<std::uint32_t>(smallVector.size()));
    for (auto const& element : smallVector)
        ar(element);
}

template<class Archive, class T, std::size_t N, class Alloc>
void load(Archive& ar, boost::container::small_vector<T, N, Alloc>& smallVector) {
    std::uint32_t elementCount;
    ar(elementCount);
    smallVector.resize(elementCount);
    for (auto& element : smallVector)
        ar(element);
}

// Shader::Info::UserDataMask
template<class Archive>
void serialize(Archive& ar, Shader::Info::UserDataMask& mask) {
    ar(mask.mask);
}

// Shader::CopyShaderData
template<class Archive>
void serialize(Archive& ar, Shader::CopyShaderData& data) {
    ar(
        data.attr_map,
        data.num_attrs,
        data.output_vertices);
}

// AmdGPU::Buffer
template<class Archive>
void serialize(Archive& ar, AmdGpu::Buffer& buffer) {
    ar(cereal::binary_data(reinterpret_cast<uint8_t*>(&buffer), sizeof(buffer)));
    // is base_adress cacheable?
}

// Shader::BufferResource
template<class Archive>
void serialize(Archive& ar, Shader::BufferResource& buffer)
{
    ar(
        buffer.sharp_idx,
        buffer.used_types,
        buffer.inline_cbuf,
        buffer.buffer_type,
        buffer.instance_attrib,
        buffer.is_written,
        buffer.is_formatted);
}

// Shader::ImageResource
template<class Archive>
void serialize(Archive& ar, Shader::ImageResource& image)
{
    ar(
        image.sharp_idx,
        image.is_depth,
        image.is_atomic,
        image.is_array,
        image.is_written,
        image.is_r128);
}

// AmdGpu::Sampler
template<class Archive>
void serialize(Archive& ar, AmdGpu::Sampler& sampler) {
    ar(cereal::binary_data(reinterpret_cast<u8*>(&sampler), sizeof(sampler)));
}

// Shader::SamplerResource
template<class Archive>
void serialize(Archive& ar, Shader::SamplerResource& sampler) {
    ar(sampler.sampler);
    ar(static_cast<u32>(sampler.associated_image),
       static_cast<u32>(sampler.disable_aniso));
}

}