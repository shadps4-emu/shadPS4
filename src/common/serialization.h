// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/small_vector.hpp>
#include <cereal/archives/binary.hpp>

#include <common/types.h>

namespace cereal {

// boost::small_vector
template <class Archive, class T, std::size_t N, class Alloc>
void save(Archive& ar, boost::container::small_vector<T, N, Alloc> const& smallVector) {
    ar(make_size_tag(static_cast<u32>(smallVector.size())));
    for (auto const& element : smallVector)
        ar(element);
}

template <class Archive, class T, std::size_t N, class Alloc>
void load(Archive& ar, boost::container::small_vector<T, N, Alloc>& smallVector) {
    u32 elementCount;
    ar(make_size_tag(elementCount));
    smallVector.resize(elementCount);
    for (auto& element : smallVector)
        ar(element);
}

} // namespace cereal