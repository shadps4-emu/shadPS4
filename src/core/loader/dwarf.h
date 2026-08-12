// SPDX-FileCopyrightText: Copyright (C) 2001-2024 Free Software Foundation, Inc.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector> // Windows static guest red-zone protection

#include "common/types.h"

namespace Dwarf {

enum {
    DW_EH_PE_ptr = 0x00,
    DW_EH_PE_uleb128 = 0x01,
    DW_EH_PE_udata2 = 0x02,
    DW_EH_PE_udata4 = 0x03,
    DW_EH_PE_udata8 = 0x04,
    DW_EH_PE_signed = 0x08,
    DW_EH_PE_sleb128 = 0x09,
    DW_EH_PE_sdata2 = 0x0A,
    DW_EH_PE_sdata4 = 0x0B,
    DW_EH_PE_sdata8 = 0x0C,
    DW_EH_PE_absptr = 0x00,
    DW_EH_PE_pcrel = 0x10,
    DW_EH_PE_textrel = 0x20,
    DW_EH_PE_datarel = 0x30,
    DW_EH_PE_funcrel = 0x40,
    DW_EH_PE_aligned = 0x50,
    DW_EH_PE_indirect = 0x80,
    DW_EH_PE_omit = 0xFF,
    // Windows static guest red-zone protection
    DW_EH_PE_format_mask = 0x0F,
    DW_EH_PE_application_mask = 0x70,
};

/// Information encoded in the EH frame header.
struct EHHeaderInfo {
    uintptr_t eh_frame_ptr{};    // Windows static guest red-zone protection
    size_t fde_count{};          // Windows static guest red-zone protection
    uintptr_t table{};           // Windows static guest red-zone protection
    uintptr_t datarel_base{};    // Windows static guest red-zone protection
    u8 table_enc{DW_EH_PE_omit}; // Windows static guest red-zone protection
};

bool DecodeEHHdr(uintptr_t ehHdrStart, uintptr_t ehHdrEnd, EHHeaderInfo& ehHdrInfo);
// Windows static guest red-zone protection
bool DecodeEHHdrTable(const EHHeaderInfo& ehHdrInfo, uintptr_t ehHdrEnd,
                      std::vector<uintptr_t>& functionStarts);

} // namespace Dwarf
