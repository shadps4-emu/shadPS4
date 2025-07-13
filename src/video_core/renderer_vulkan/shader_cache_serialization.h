// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "shader_recompiler/info.h"

namespace cereal {

// boost::small_vector
template<class Archive, class T, std::size_t N, class Alloc>
void save(Archive& ar, boost::container::small_vector<T, N, Alloc> const& v) {
    ar(static_cast<std::uint32_t>(v.size()));
    for (auto const& e : v)
        ar(e);
}

template<class Archive, class T, std::size_t N, class Alloc>
void load(Archive& ar, boost::container::small_vector<T, N, Alloc>& v) {
    std::uint32_t n;
    ar(n);
    v.resize(n);
    for (auto& e : v)
        ar(e);
}

// Shader::ImageResource
template<class Archive>
void serialize(Archive& ar, Shader::ImageResource& img)
{
    ar(img.sharp_idx,
       img.is_depth,
       img.is_atomic,
       img.is_array,
       img.is_written,
       img.is_r128);
}
}