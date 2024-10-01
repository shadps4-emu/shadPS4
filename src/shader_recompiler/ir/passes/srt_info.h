// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
// #include <boost/container/flat_map.hpp>
// #include <boost/container/flat_set.hpp>
#include <boost/container/map.hpp>
#include <boost/container/set.hpp>
#include <boost/container/small_vector.hpp>

#include <memory>
#include <unordered_map>
#include "common/assert.h"
#include "common/bit_field.h"
#include "common/types.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/value.h"

namespace Shader {

// TODO: GVN for readconst instructions to dedup them before SRT pass?
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

struct SrtNode {
    const IR::Inst* inst; // Readconst inst
    u32 flattened_sharp_off_dw;
    u32 flattened_cbuf_off_dw;
    // How the readconst is used
    union {
        u32 raw;
        BitField<0, 1, u32> pointer_lo;
        // Ignore pointer_hi. Use LO for tracking, which corresponds to lower SGPR
        BitField<1, 1, u32> pointer_hi;
        // Unused for now, assumed that readconst is sharp if not pointer
        BitField<2, 1, u32> sharp;
        // Arbitrary use, e.g. in ALU
        BitField<3, 1, u32> cbuffer;

    } use_kind;

    SrtNode(const IR::Inst* inst_)
        : inst(inst_), flattened_sharp_off_dw(-1U), flattened_cbuf_off_dw(-1U), use_kind(0) {}
};

// Only put the Inst corresponding to the LO dword (the sgpr base) of the pointer in the node
// -> children map (as a key)
struct SrtInfo {
    using PtrUserList = boost::container::small_vector<const IR::Inst*, 8>;

    std::unordered_map<const IR::Inst*, std::unique_ptr<SrtNode>> srt_nodes;
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

    u32 flattened_sharp_bufsize_dw{0};
    u32 flattened_cbuf_bufsize_dw{0};

    SrtNode* getOrInsertNode(const IR::Inst* inst) {
        auto it = srt_nodes.find(inst);
        if (it != srt_nodes.end()) {
            return it->second.get();
        }

        auto ref = srt_nodes.insert(std::make_pair(inst, std::make_unique<SrtNode>(inst)));
        return ref.first->second.get();
    }

    SrtNode* getNode(const IR::Inst* inst) const {
        ASSERT(srt_nodes.contains(inst));
        return srt_nodes.find(inst)->second.get();
    }

    // Special case for fetch shaders because we don't generate IR to read from step rate buffers,
    // so we won't see usage with GetUserData/ReadConst.
    // Reserve space in the flattened sharp buffer for a V#, and return dword offset into
    // flattened sharp buffer, to be filled during SRT walk
    u32 reserve_fetch_sharp(u32 sgpr_base, u32 dword_offset) {
        u32 rv = 16 /*NumUserDataRegs*/ + 4 * fetch_reservations.size();
        fetch_reservations.emplace_back(sgpr_base, dword_offset);
        return rv;
    }
};

} // namespace Shader