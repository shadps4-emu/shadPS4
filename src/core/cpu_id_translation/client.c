// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdatomic.h>
#include <stdint.h>
#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"

static atomic_uintptr_t guest_begin;
static atomic_uintptr_t guest_end;
static app_pc main_start;

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
    DR_ASSERT_MSG(active != NULL && range != NULL,
                  "The emulator must be built with ENABLE_CPU_ID_TRANSLATION=ON");
    DR_ASSERT(drwrap_wrap(active, TranslationActive, NULL));
    DR_ASSERT(drwrap_wrap(range, SetGuestRange, NULL));
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
