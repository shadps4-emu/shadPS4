// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include <boost/container/map.hpp>
#include <boost/container/set.hpp>
#include <boost/container/small_vector.hpp>

#include <memory>
#include <unordered_map>
#include "common/alignment.h"
#include "common/assert.h"
#include "common/types.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/value.h"
#include "xbyak/xbyak.h"

namespace Shader {

// Refactor FlatSharpBuffer so we only rerun walker once per draw. Stuff it in
// runtime_info?

struct Info;

struct FlatSharpBuffer {
    FlatSharpBuffer(const Info& info);

    template <typename T>
    T ReadUdSharp(u32 sharp_idx) const noexcept {
        return *reinterpret_cast<const T*>(&buf[sharp_idx]);
    }

    std::vector<u32> buf;
};

typedef void (*PFN_SrtWalker)(const u32* /*user_data*/, u32* /*flat_dst*/);

// Utility for copying a simple relocatable function from a Xbyak code generator to manage memory
// separately
class SmallCodeArray {
public:
    SmallCodeArray() {}
    SmallCodeArray& operator=(SmallCodeArray&& other) = default;
    SmallCodeArray(SmallCodeArray&& other) = default;

    SmallCodeArray& operator=(const SmallCodeArray& other) {
        *this = SmallCodeArray(codebuf.get(), bufsize);
        return *this;
    }
    SmallCodeArray(const SmallCodeArray& other) {
        *this = other;
    };

    SmallCodeArray(const u8* code, size_t codesize) {
        size_t pagesize = Xbyak::inner::getPageSize();
        bufsize = Common::AlignUp(codesize, pagesize);
        auto fn = new (std::align_val_t(pagesize)) u8[bufsize];
        ASSERT(fn);
        codebuf = std::unique_ptr<u8[]>(fn);
        memcpy(codebuf.get(), code, codesize);
        Xbyak::CodeArray::protect(codebuf.get(), bufsize, Xbyak::CodeArray::PROTECT_RE);
    }

    ~SmallCodeArray() {
        Xbyak::CodeArray::protect(codebuf.get(), bufsize, Xbyak::CodeArray::PROTECT_RW);
    }

    template <class F>
    F getCode() const {
        return reinterpret_cast<F>(codebuf.get());
    }

private:
    size_t bufsize;
    std::unique_ptr<u8[]> codebuf;
};

// Only put the Inst corresponding to the LO dword (the sgpr base) of the pointer in the node
// -> children map (as a key)
struct SrtInfo {
    // map offset to inst
    using PtrUserList = boost::container::map<u32, const IR::Inst*>;

    std::unordered_map<const IR::Inst*, u32> srt_node_to_flat_off_dw;
    // keys are GetUserData or ReadConst instructions that are used as pointers
    std::unordered_map<const IR::Inst*, PtrUserList> pointer_uses;
    // GetUserData instructions corresponding to sgpr_base of SRT roots
    boost::container::map<IR::ScalarReg, const IR::Inst*> srt_roots;

    // Special case when fetch shader uses step rates.
    // Need to reserve space for those V#s, and find them in SrtWalker function
    struct FetchShaderReservation {
        u32 sgpr_base;
        u32 dword_offset;
    };
    boost::container::small_vector<FetchShaderReservation, 2> fetch_reservations;

    u32 flattened_bufsize_dw{0};

    // Special case for fetch shaders because we don't generate IR to read from step rate buffers,
    // so we won't see usage with GetUserData/ReadConst.
    // Reserve space in the flattened sharp buffer for a V#, and return dword offset into
    // flattened sharp buffer, to be filled during SRT walk
    u32 reserve_fetch_sharp(u32 sgpr_base, u32 dword_offset) {
        u32 rv = 16 /*NumUserDataRegs*/ + 4 * fetch_reservations.size();
        fetch_reservations.emplace_back(sgpr_base, dword_offset);
        return rv;
    }

    u32 GetReadFlatOffsetDw(const IR::Inst* read) const {
        ASSERT(srt_node_to_flat_off_dw.contains(read));
        return srt_node_to_flat_off_dw.find(read)->second;
    }

    const PtrUserList* GetUsesAsPointer(const IR::Inst* inst) const {
        auto it = pointer_uses.find(inst);
        if (it != pointer_uses.end()) {
            return &it->second;
        }
        return nullptr;
    }

    bool IsEmpty() {
        return fetch_reservations.empty() && srt_roots.empty();
    }
};

} // namespace Shader