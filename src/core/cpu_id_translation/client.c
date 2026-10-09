// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <xmmintrin.h>
#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"

static atomic_uintptr_t guest_begin;
static atomic_uintptr_t guest_end;
static app_pc main_start;
static const uint8_t* reciprocal_indices[2];
static const uint32_t* reciprocal_values[2];
static atomic_bool reciprocal_ready;

static void SetReciprocalTables(void* context, void** user_data) {
    reciprocal_indices[0] = drwrap_get_arg(context, 0);
    reciprocal_indices[1] = drwrap_get_arg(context, 1);
    reciprocal_values[0] = drwrap_get_arg(context, 2);
    reciprocal_values[1] = drwrap_get_arg(context, 3);
    atomic_store_explicit(&reciprocal_ready, true, memory_order_release);
}

static void ExecuteReciprocal(uintptr_t operation, uintptr_t registers, uintptr_t source_low,
                              uintptr_t source_high) {
    void* context = dr_get_current_drcontext();
    dr_mcontext_t state = {.size = sizeof(state), .flags = DR_MC_ALL};
    DR_ASSERT(dr_get_mcontext(context, &state));
    const uintptr_t words[2] = {source_low, source_high};
    opnd_t source;
    memcpy(&source, words, sizeof(source));
    const bool scalar = (operation & 1) != 0;
    const bool vex = (operation & 2) != 0;
    const unsigned rsqrt = (operation >> 2) & 1;
    const unsigned width = operation >> 3;
    const unsigned destination = registers & 255;
    const unsigned merge = registers >> 8;
    dr_zmm_t input = {0};
    if (opnd_is_memory_reference(source)) {
        const app_pc address = opnd_compute_address(source, &state);
        if ((!scalar && !vex && ((uintptr_t)address & 15) != 0) ||
            !dr_safe_read(address, scalar ? 4 : width, &input, NULL)) {
            dr_write_saved_reg(context, SPILL_SLOT_3, 0);
            return;
        }
    } else {
        DR_ASSERT(reg_get_value_ex(opnd_get_reg(source), &state, (byte*)&input));
    }
    dr_zmm_t result = state.simd[scalar && vex ? merge : destination];
    if (vex) {
        memset((byte*)&result + width, 0, sizeof(result) - width);
    }
    for (unsigned lane = 0; lane < (scalar ? 1 : width / 4); ++lane) {
        float value;
        memcpy(&value, &input.u32[lane], sizeof(value));
        const __m128 packed = _mm_set_ss(value);
        const float native = _mm_cvtss_f32(rsqrt ? _mm_rsqrt_ss(packed) : _mm_rcp_ss(packed));
        memcpy(&result.u32[lane], &native, sizeof(native));
        if ((input.u32[lane] & 0x7fffffff) <= 0x7f800000) {
            const unsigned index = reciprocal_indices[rsqrt][input.u32[lane] >> 11];
            result.u32[lane] ^= reciprocal_values[rsqrt][index];
        }
    }
    state.simd[destination] = result;
    DR_ASSERT(dr_set_mcontext(context, &state));
    dr_write_saved_reg(context, SPILL_SLOT_3, 1);
}

static bool TranslateReciprocal(void* context, instrlist_t* block, instr_t* instruction) {
    if (!atomic_load_explicit(&reciprocal_ready, memory_order_acquire)) {
        return false;
    }
    bool scalar, vex, rsqrt;
    switch (instr_get_opcode(instruction)) {
    case OP_rcpss:
        scalar = true;
        vex = false;
        rsqrt = false;
        break;
    case OP_vrcpss:
        scalar = true;
        vex = true;
        rsqrt = false;
        break;
    case OP_rsqrtss:
        scalar = true;
        vex = false;
        rsqrt = true;
        break;
    case OP_vrsqrtss:
        scalar = true;
        vex = true;
        rsqrt = true;
        break;
    case OP_rcpps:
        scalar = false;
        vex = false;
        rsqrt = false;
        break;
    case OP_vrcpps:
        scalar = false;
        vex = true;
        rsqrt = false;
        break;
    case OP_rsqrtps:
        scalar = false;
        vex = false;
        rsqrt = true;
        break;
    case OP_vrsqrtps:
        scalar = false;
        vex = true;
        rsqrt = true;
        break;
    default:
        return false;
    }
    const opnd_t destination = instr_get_dst(instruction, 0);
    const reg_id_t reg = opnd_get_reg(destination);
    const unsigned dst = reg - (reg_is_ymm(reg) ? DR_REG_YMM0 : DR_REG_XMM0);
    const unsigned width = reg_is_ymm(reg) ? 32 : 16;
    const opnd_t source = instr_get_src(instruction, scalar && vex ? 1 : 0);
    const unsigned merge =
        scalar && vex ? opnd_get_reg(instr_get_src(instruction, 0)) - DR_REG_XMM0 : dst;
    uintptr_t words[2] = {0};
    _Static_assert(sizeof(source) <= sizeof(words), "Unexpected DynamoRIO operand size");
    memcpy(words, &source, sizeof(source));
    dr_insert_clean_call_ex(context, block, instruction, ExecuteReciprocal,
                            DR_CLEANCALL_READS_APP_CONTEXT | DR_CLEANCALL_WRITES_APP_CONTEXT, 4,
                            OPND_CREATE_INTPTR(scalar | (vex << 1) | (rsqrt << 2) | (width << 3)),
                            OPND_CREATE_INTPTR(dst | (merge << 8)), OPND_CREATE_INTPTR(words[0]),
                            OPND_CREATE_INTPTR(words[1]));
    if (!opnd_is_memory_reference(source)) {
        instrlist_remove(block, instruction);
        instr_destroy(context, instruction);
        return true;
    }
    // A failed read must fault at the original guest instruction and address.
    instr_t* original = INSTR_CREATE_label(context);
    instr_t* done = INSTR_CREATE_label(context);
    dr_save_reg(context, block, instruction, DR_REG_XAX, SPILL_SLOT_1);
    dr_save_arith_flags_to_xax(context, block, instruction);
    dr_save_reg(context, block, instruction, DR_REG_XAX, SPILL_SLOT_2);
    dr_restore_reg(context, block, instruction, DR_REG_XAX, SPILL_SLOT_3);
    instrlist_meta_preinsert(
        block, instruction,
        INSTR_CREATE_test(context, opnd_create_reg(DR_REG_XAX), opnd_create_reg(DR_REG_XAX)));
    instrlist_meta_preinsert(block, instruction,
                             INSTR_CREATE_jcc(context, OP_jz, opnd_create_instr(original)));
    dr_restore_reg(context, block, instruction, DR_REG_XAX, SPILL_SLOT_2);
    dr_restore_arith_flags_from_xax(context, block, instruction);
    dr_restore_reg(context, block, instruction, DR_REG_XAX, SPILL_SLOT_1);
    instrlist_meta_preinsert(block, instruction,
                             INSTR_CREATE_jmp(context, opnd_create_instr(done)));
    instrlist_meta_preinsert(block, instruction, original);
    dr_restore_reg(context, block, instruction, DR_REG_XAX, SPILL_SLOT_2);
    dr_restore_arith_flags_from_xax(context, block, instruction);
    dr_restore_reg(context, block, instruction, DR_REG_XAX, SPILL_SLOT_1);
    instrlist_meta_postinsert(block, instruction, done);
    return true;
}

static void SetGuestRange(void* context, void** user_data) {
    const uintptr_t begin = (uintptr_t)drwrap_get_arg(context, 0);
    const uintptr_t end = (uintptr_t)drwrap_get_arg(context, 1);
    atomic_store_explicit(&guest_begin, begin, memory_order_relaxed);
    atomic_store_explicit(&guest_end, end, memory_order_release);
}

static void TranslationActive(void* context, void** user_data) {
    DR_ASSERT(drwrap_skip_call(context, (void*)1, 0));
}

static void ModuleLoaded(void* context, const module_data_t* module, bool loaded) {
    if (module->start != main_start) {
        return;
    }
    app_pc active = (app_pc)dr_get_proc_address(module->handle, "ShadCpuIdTranslationActive");
    app_pc range = (app_pc)dr_get_proc_address(module->handle, "ShadCpuIdTranslationRange");
    app_pc tables = (app_pc)dr_get_proc_address(module->handle, "ShadReciprocalTranslationTables");
    DR_ASSERT_MSG(active != NULL && range != NULL && tables != NULL,
                  "The emulator must be built with ENABLE_CPU_ID_TRANSLATION=ON");
    DR_ASSERT(drwrap_wrap(active, TranslationActive, NULL));
    DR_ASSERT(drwrap_wrap(range, SetGuestRange, NULL));
    DR_ASSERT(drwrap_wrap(tables, SetReciprocalTables, NULL));
}

static void Shutdown(void) {
    drwrap_exit();
    drmgr_exit();
}

static dr_emit_flags_t TranslateBlock(void* context, void* tag, instrlist_t* block, bool for_trace,
                                      bool translating) {
    const uintptr_t end = atomic_load_explicit(&guest_end, memory_order_acquire);
    const uintptr_t begin = atomic_load_explicit(&guest_begin, memory_order_relaxed);
    for (instr_t* instruction = instrlist_first_app(block); instruction != NULL;) {
        instr_t* next = instr_get_next_app(instruction);
        const app_pc pc = instr_get_app_pc(instruction);
        const int opcode = instr_get_opcode(instruction);
        if ((uintptr_t)pc >= begin && (uintptr_t)pc < end &&
            TranslateReciprocal(context, block, instruction)) {
            instruction = next;
            continue;
        }
        if ((uintptr_t)pc >= begin && (uintptr_t)pc < end &&
            (opcode == OP_cpuid || opcode == OP_rdtscp || opcode == OP_rdpid)) {
            instr_t* trap = INSTR_CREATE_ud2(context);
            instr_set_translation(trap, pc);
            instrlist_replace(block, instruction, trap);
            instr_destroy(context, instruction);
        }
        instruction = next;
    }
    return DR_EMIT_STORE_TRANSLATIONS;
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char* argv[]) {
    dr_set_client_name("shadPS4 CPU identity translation", "https://github.com/Chreece/shadPS4");
    DR_ASSERT(drmgr_init());
    DR_ASSERT(drwrap_init());
    module_data_t* main_module = dr_get_main_module();
    main_start = main_module->start;
    dr_free_module_data(main_module);
    DR_ASSERT(drmgr_register_exit_event(Shutdown));
    DR_ASSERT(drmgr_register_module_load_event(ModuleLoaded));
    DR_ASSERT(drmgr_register_bb_app2app_event(TranslateBlock, NULL));
}
