//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

// Credits to https://github.com/psucien/tlg-emu-tools/

#include "common/types.h"
#include "gcn/si_ci_vi_merged_pm4_it_opcodes.h"

namespace Core::Devtools::Gcn {
const char* GetOpCodeName(u32 op) {
    switch (op) {
    case IT_NOP:
        return "IT_NOP";
    case IT_SET_BASE:
        return "IT_SET_BASE";
    case IT_INDEX_BUFFER_SIZE:
        return "IT_INDEX_BUFFER_SIZE";
    case IT_SET_PREDICATION:
        return "IT_SET_PREDICATION";
    case IT_COND_EXEC:
        return "IT_COND_EXEC";
    case IT_INDEX_BASE:
        return "IT_INDEX_BASE";
    case IT_INDEX_TYPE:
        return "IT_INDEX_TYPE";
    case IT_NUM_INSTANCES:
        return "IT_NUM_INSTANCES";
    case IT_STRMOUT_BUFFER_UPDATE:
        return "IT_STRMOUT_BUFFER_UPDATE";
    case IT_WRITE_DATA:
        return "IT_WRITE_DATA";
    case IT_MEM_SEMAPHORE:
        return "IT_MEM_SEMAPHORE";
    case IT_WAIT_REG_MEM:
        return "IT_WAIT_REG_MEM";
    case IT_INDIRECT_BUFFER:
        return "IT_INDIRECT_BUFFER";
    case IT_PFP_SYNC_ME:
        return "IT_PFP_SYNC_ME";
    case IT_EVENT_WRITE:
        return "IT_EVENT_WRITE";
    case IT_EVENT_WRITE_EOP:
        return "IT_EVENT_WRITE_EOP";
    case IT_EVENT_WRITE_EOS:
        return "IT_EVENT_WRITE_EOS";
    case IT_DMA_DATA__CI__VI:
        return "IT_DMA_DATA";
    case IT_ACQUIRE_MEM__CI__VI:
        return "IT_ACQUIRE_MEM";
    case IT_REWIND__CI__VI:
        return "IT_REWIND";
    case IT_SET_CONFIG_REG:
        return "IT_SET_CONFIG_REG";
    case IT_SET_CONTEXT_REG:
        return "IT_SET_CONTEXT_REG";
    case IT_SET_SH_REG:
        return "IT_SET_SH_REG";
    case IT_SET_UCONFIG_REG__CI__VI:
        return "IT_SET_UCONFIG_REG";
    case IT_INCREMENT_DE_COUNTER:
        return "IT_INCREMENT_DE_COUNTER";
    case IT_WAIT_ON_CE_COUNTER:
        return "IT_WAIT_ON_CE_COUNTER";
    case IT_DISPATCH_DIRECT:
        return "IT_DISPATCH_DIRECT";
    case IT_DISPATCH_INDIRECT:
        return "IT_DISPATCH_INDIRECT";
    case IT_OCCLUSION_QUERY:
        return "IT_OCCLUSION_QUERY";
    case IT_REG_RMW:
        return "IT_REG_RMW";
    case IT_PRED_EXEC:
        return "IT_PRED_EXEC";
    case IT_DRAW_INDIRECT:
        return "IT_DRAW_INDIRECT";
    case IT_DRAW_INDEX_INDIRECT:
        return "IT_DRAW_INDEX_INDIRECT";
    case IT_DRAW_INDEX_2:
        return "IT_DRAW_INDEX_2";
    case IT_DRAW_INDEX_OFFSET_2:
        return "IT_DRAW_INDEX_OFFSET_2";
    case IT_CONTEXT_CONTROL:
        return "IT_CONTEXT_CONTROL";
    case IT_DRAW_INDIRECT_MULTI:
        return "IT_DRAW_INDIRECT_MULTI";
    case IT_DRAW_INDEX_AUTO:
        return "IT_DRAW_INDEX_AUTO";
    case IT_DRAW_INDEX_MULTI_AUTO:
        return "IT_DRAW_INDEX_MULTI_AUTO";
    case IT_COPY_DATA:
        return "IT_COPY_DATA";
    case IT_CP_DMA:
        return "IT_CP_DMA";
    case IT_SURFACE_SYNC:
        return "IT_SURFACE_SYNC";
    case IT_COND_WRITE:
        return "IT_COND_WRITE";
    case IT_RELEASE_MEM__CI__VI:
        return "IT_RELEASE_MEM";
    case IT_WRITE_CONST_RAM:
        return "IT_WRITE_CONST_RAM"; // used in CCB
    case IT_WAIT_ON_DE_COUNTER_DIFF:
        return "IT_WAIT_ON_DE_COUNTER_DIFF"; // used in CCB
    case IT_DUMP_CONST_RAM:
        return "IT_DUMP_CONST_RAM"; // used in CCB
    case IT_INCREMENT_CE_COUNTER:
        return "IT_INCREMENT_CE_COUNTER"; // used in CCB
    case IT_CLEAR_STATE:
        return "IT_CLEAR_STATE";
    case 0xFF:
        return "<STUB (TMP)>";
    default:
        break;
    }

    return "<UNK>";
}
} // namespace Core::Devtools::Gcn