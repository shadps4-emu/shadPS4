// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include <boost/align/aligned_allocator.hpp>
#include <boost/align/aligned_delete.hpp>
#include <boost/container/map.hpp>
#include <boost/container/set.hpp>
#include <boost/container/small_vector.hpp>

#include <memory>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/types.h"
#include "xbyak/xbyak.h"

namespace Shader {

class FlattenedUserDataBuffer {
public:
    template <typename T>
    T ReadUdSharp(u32 sharp_idx) const noexcept {
        return *reinterpret_cast<const T*>(&buf[sharp_idx]);
    }

    size_t num_dwords() const {
        return buf.size();
    }

    size_t size_bytes() const {
        return buf.size() * sizeof(u32);
    }

    u32* data() {
        return buf.data();
    }

    const u32* data() const {
        return buf.data();
    }

    void resize(size_t new_size_dw) {
        buf.resize(new_size_dw);
    }

private:
    std::vector<u32> buf;
};

typedef void(__attribute__((sysv_abi)) * PFN_SrtWalker)(const u32* /*user_data*/,
                                                        u32* /*flat_dst*/);

// Utility for copying a simple relocatable function from a Xbyak code generator to manage memory
// separately
class SmallCodeArray {
public:
    SmallCodeArray() : bufsize(0), codebuf(nullptr) {}
    SmallCodeArray& operator=(SmallCodeArray&& other) = default;
    SmallCodeArray(SmallCodeArray&& other) = default;

    SmallCodeArray& operator=(const SmallCodeArray& other) {
        *this = SmallCodeArray(reinterpret_cast<u8*>(codebuf.get()), bufsize);
        return *this;
    }
    SmallCodeArray(const SmallCodeArray& other) {
        *this = other;
    };

    SmallCodeArray(const u8* code, size_t codesize) : SmallCodeArray() {
        size_t pagesize = Xbyak::inner::getPageSize();
        bufsize = Common::AlignUp(codesize, pagesize);
        if (bufsize > 0) {
            auto fn = reinterpret_cast<u8*>(boost::alignment::aligned_alloc(pagesize, bufsize));
            ASSERT(fn);
            codebuf = aligned_unique_ptr(fn);
            memcpy(codebuf.get(), code, codesize);
            Xbyak::CodeArray::protect(codebuf.get(), bufsize, Xbyak::CodeArray::PROTECT_RE);
        }
    }

    ~SmallCodeArray() {
        if (bufsize > 0) {
            Xbyak::CodeArray::protect(codebuf.get(), bufsize, Xbyak::CodeArray::PROTECT_RW);
        }
    }

    template <class F>
    F getCode() const {
        return reinterpret_cast<F>(codebuf.get());
    }

private:
    using aligned_unique_ptr = std::unique_ptr<u8, boost::alignment::aligned_delete>;

    size_t bufsize;
    aligned_unique_ptr codebuf;
};

struct PersistentSrtInfo {
    PersistentSrtInfo() : flattened_bufsize_dw(/*NumUserDataRegs*/ 16) {}

    // Special case when fetch shader uses step rates.
    struct SrtSharpReservation {
        u32 sgpr_base;
        u32 dword_offset;
        u32 num_dwords;
    };

    SmallCodeArray walker;
    boost::container::small_vector<SrtSharpReservation, 2> srt_reservations;
    u32 flattened_bufsize_dw;

    // Special case for fetch shaders because we don't generate IR to read from step rate buffers,
    // so we won't see usage with GetUserData/ReadConst.
    // Reserve space in the flattened buffer for a sharp ahead of time
    u32 reserve_sharp(u32 sgpr_base, u32 dword_offset, u32 num_dwords) {
        u32 rv = flattened_bufsize_dw;
        srt_reservations.emplace_back(sgpr_base, dword_offset, num_dwords);
        flattened_bufsize_dw += num_dwords;
        return rv;
    }
};

} // namespace Shader