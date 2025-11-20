// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>

#include "aio.h"
#include "common/assert.h"
#include "common/debug.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/equeue.h"
#include "core/libraries/kernel/orbis_error.h"
#include "core/libraries/libs.h"
#include "file_system.h"

namespace Libraries::Kernel {

#define MAX_QUEUE 512

static s32* id_state;
static s32 id_index;

s32 PS4_SYSV_ABI sceKernelAioInitializeImpl(void* p, s32 size) {

    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioDeleteRequest(OrbisKernelAioSubmitId id, s32* ret) {
    if (ret == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    id_state[id] = ORBIS_KERNEL_AIO_STATE_ABORTED;
    *ret = 0;
    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioDeleteRequests(OrbisKernelAioSubmitId id[], s32 num, s32 ret[]) {
    if (ret == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    for (s32 i = 0; i < num; i++) {
        id_state[id[i]] = ORBIS_KERNEL_AIO_STATE_ABORTED;
        ret[i] = 0;
    }

    return 0;
}
s32 PS4_SYSV_ABI sceKernelAioPollRequest(OrbisKernelAioSubmitId id, s32* state) {
    if (state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    *state = id_state[id];
    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioPollRequests(OrbisKernelAioSubmitId id[], s32 num, s32 state[]) {
    if (state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    for (s32 i = 0; i < num; i++) {
        state[i] = id_state[id[i]];
    }

    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioCancelRequest(OrbisKernelAioSubmitId id, s32* state) {
    if (state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    if (id) {
        id_state[id] = ORBIS_KERNEL_AIO_STATE_ABORTED;
        *state = ORBIS_KERNEL_AIO_STATE_ABORTED;
    } else {
        *state = ORBIS_KERNEL_AIO_STATE_PROCESSING;
    }
    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioCancelRequests(OrbisKernelAioSubmitId id[], s32 num, s32 state[]) {
    if (state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    for (s32 i = 0; i < num; i++) {
        if (id[i]) {
            id_state[id[i]] = ORBIS_KERNEL_AIO_STATE_ABORTED;
            state[i] = ORBIS_KERNEL_AIO_STATE_ABORTED;
        } else {
            state[i] = ORBIS_KERNEL_AIO_STATE_PROCESSING;
        }
    }

    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioWaitRequest(OrbisKernelAioSubmitId id, s32* state, u32* usec) {
    if (state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    u32 timer = 0;

    s32 timeout = 0;

    while (id_state[id] == ORBIS_KERNEL_AIO_STATE_PROCESSING) {
        sceKernelUsleep(10);

        timer += 10;
        if (*usec) {
            if (timer > *usec) {
                timeout = 1;
                break;
            }
        }
    }

    *state = id_state[id];

    if (timeout)
        return ORBIS_KERNEL_ERROR_ETIMEDOUT;
    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioWaitRequests(OrbisKernelAioSubmitId id[], s32 num, s32 state[],
                                          u32 mode, u32* usec) {
    if (state == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    u32 timer = 0;
    s32 timeout = 0;
    s32 completion = 0;

    for (s32 i = 0; i < num; i++) {
        if (!completion && !timeout) {
            while (id_state[id[i]] == ORBIS_KERNEL_AIO_STATE_PROCESSING) {
                sceKernelUsleep(10);
                timer += 10;

                if (*usec) {
                    if (timer > *usec) {
                        timeout = 1;
                        break;
                    }
                }
            }
        }

        if (mode == 0x02) {
            if (id_state[id[i]] == ORBIS_KERNEL_AIO_STATE_COMPLETED)
                completion = 1;
        }

        state[i] = id_state[id[i]];
    }

    if (timeout)
        return ORBIS_KERNEL_ERROR_ETIMEDOUT;

    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioSubmitReadCommands(OrbisKernelAioRWRequest req[], s32 size, s32 prio,
                                                OrbisKernelAioSubmitId* id) {
    if (req == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    if (id == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    id_state[id_index] = ORBIS_KERNEL_AIO_STATE_PROCESSING;

    for (s32 i = 0; i < size; i++) {

        s64 ret = sceKernelPread(req[i].fd, req[i].buf, req[i].nbyte, req[i].offset);

        if (ret < 0) {
            req[i].result->state = ORBIS_KERNEL_AIO_STATE_ABORTED;
            req[i].result->returnValue = ret;

        } else {
            req[i].result->state = ORBIS_KERNEL_AIO_STATE_COMPLETED;
            req[i].result->returnValue = ret;
        }
    }

    id_state[id_index] = ORBIS_KERNEL_AIO_STATE_COMPLETED;

    *id = id_index;

    id_index = (id_index + 1) % MAX_QUEUE;

    if (!id_index)
        id_index++;

    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioSubmitReadCommandsMultiple(OrbisKernelAioRWRequest req[], s32 size,
                                                        s32 prio, OrbisKernelAioSubmitId id[]) {
    if (req == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    if (id == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    for (s32 i = 0; i < size; i++) {
        id_state[id_index] = ORBIS_KERNEL_AIO_STATE_PROCESSING;

        s64 ret = sceKernelPread(req[i].fd, req[i].buf, req[i].nbyte, req[i].offset);

        if (ret < 0) {
            req[i].result->state = ORBIS_KERNEL_AIO_STATE_ABORTED;
            req[i].result->returnValue = ret;

            id_state[id_index] = ORBIS_KERNEL_AIO_STATE_ABORTED;

        } else {
            req[i].result->state = ORBIS_KERNEL_AIO_STATE_COMPLETED;
            req[i].result->returnValue = ret;

            id_state[id_index] = ORBIS_KERNEL_AIO_STATE_COMPLETED;
        }

        id[i] = id_index;

        id_index = (id_index + 1) % MAX_QUEUE;

        if (!id_index)
            id_index++;
    }

    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioSubmitWriteCommands(OrbisKernelAioRWRequest req[], s32 size, s32 prio,
                                                 OrbisKernelAioSubmitId* id) {
    if (req == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    if (id == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    for (s32 i = 0; i < size; i++) {
        id_state[id_index] = ORBIS_KERNEL_AIO_STATE_PROCESSING;

        s64 ret = sceKernelPwrite(req[i].fd, req[i].buf, req[i].nbyte, req[i].offset);

        if (ret < 0) {
            req[i].result->state = ORBIS_KERNEL_AIO_STATE_ABORTED;
            req[i].result->returnValue = ret;

            id_state[id_index] = ORBIS_KERNEL_AIO_STATE_ABORTED;

        } else {
            req[i].result->state = ORBIS_KERNEL_AIO_STATE_COMPLETED;
            req[i].result->returnValue = ret;

            id_state[id_index] = ORBIS_KERNEL_AIO_STATE_COMPLETED;
        }
    }

    *id = id_index;

    id_index = (id_index + 1) % MAX_QUEUE;

    // skip id_index equals 0 , because sceKernelAioCancelRequest will submit id
    // equal to 0
    if (!id_index)
        id_index++;

    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioSubmitWriteCommandsMultiple(OrbisKernelAioRWRequest req[], s32 size,
                                                         s32 prio, OrbisKernelAioSubmitId id[]) {
    if (req == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    if (id == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    for (s32 i = 0; i < size; i++) {
        id_state[id_index] = ORBIS_KERNEL_AIO_STATE_PROCESSING;
        s64 ret = sceKernelPwrite(req[i].fd, req[i].buf, req[i].nbyte, req[i].offset);

        if (ret < 0) {
            req[i].result->state = ORBIS_KERNEL_AIO_STATE_ABORTED;
            req[i].result->returnValue = ret;

            id_state[id_index] = ORBIS_KERNEL_AIO_STATE_ABORTED;

        } else {
            req[i].result->state = ORBIS_KERNEL_AIO_STATE_COMPLETED;
            req[i].result->returnValue = ret;
            id_state[id_index] = ORBIS_KERNEL_AIO_STATE_COMPLETED;
        }

        id[i] = id_index;
        id_index = (id_index + 1) % MAX_QUEUE;

        if (!id_index)
            id_index++;
    }
    return 0;
}

s32 PS4_SYSV_ABI sceKernelAioSetParam() {
    LOG_ERROR(Kernel, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceKernelAioInitializeParam() {
    LOG_ERROR(Kernel, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterAio(Core::Loader::SymbolsResolver* sym) {
    id_index = 1;
    id_state = (int*)malloc(sizeof(int) * MAX_QUEUE);
    memset(id_state, 0, sizeof(sizeof(int) * MAX_QUEUE));

    LIB_FUNCTION("fR521KIGgb8", "libkernel", 1, "libkernel", sceKernelAioCancelRequest);
    LIB_FUNCTION("3Lca1XBrQdY", "libkernel", 1, "libkernel", sceKernelAioCancelRequests);
    LIB_FUNCTION("5TgME6AYty4", "libkernel", 1, "libkernel", sceKernelAioDeleteRequest);
    LIB_FUNCTION("Ft3EtsZzAoY", "libkernel", 1, "libkernel", sceKernelAioDeleteRequests);
    LIB_FUNCTION("vYU8P9Td2Zo", "libkernel", 1, "libkernel", sceKernelAioInitializeImpl);
    LIB_FUNCTION("nu4a0-arQis", "libkernel", 1, "libkernel", sceKernelAioInitializeParam);
    LIB_FUNCTION("2pOuoWoCxdk", "libkernel", 1, "libkernel", sceKernelAioPollRequest);
    LIB_FUNCTION("o7O4z3jwKzo", "libkernel", 1, "libkernel", sceKernelAioPollRequests);
    LIB_FUNCTION("9WK-vhNXimw", "libkernel", 1, "libkernel", sceKernelAioSetParam);
    LIB_FUNCTION("HgX7+AORI58", "libkernel", 1, "libkernel", sceKernelAioSubmitReadCommands);
    LIB_FUNCTION("lXT0m3P-vs4", "libkernel", 1, "libkernel",
                 sceKernelAioSubmitReadCommandsMultiple);
    LIB_FUNCTION("XQ8C8y+de+E", "libkernel", 1, "libkernel", sceKernelAioSubmitWriteCommands);
    LIB_FUNCTION("xT3Cpz0yh6Y", "libkernel", 1, "libkernel",
                 sceKernelAioSubmitWriteCommandsMultiple);
    LIB_FUNCTION("KOF-oJbQVvc", "libkernel", 1, "libkernel", sceKernelAioWaitRequest);
    LIB_FUNCTION("lgK+oIWkJyA", "libkernel", 1, "libkernel", sceKernelAioWaitRequests);
}

} // namespace Libraries::Kernel