// SPDX-FileCopyrightText: Copyright (C) 2001-2024 Free Software Foundation, Inc.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
    DW_EH_PE_omit = 0xFF
};

/// Information encoded in the EH frame header.
struct EHHeaderInfo {
    uintptr_t eh_frame_ptr;
    size_t fde_count;
    uintptr_t table;
    u8 table_enc;
};

bool DecodeEHHdr(uintptr_t ehHdrStart, uintptr_t ehHdrEnd, EHHeaderInfo& ehHdrInfo);

} // namespace Dwarf
