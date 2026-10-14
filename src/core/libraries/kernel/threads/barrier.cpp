// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <new>

#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/posix_error.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

static s32 BarrierInit(PthreadBarrierT* barrier, u32 count) {
    if (barrier == nullptr || count == 0) {
        return POSIX_EINVAL;
    }
    auto* value = new (std::nothrow) PthreadBarrier{};
    if (value == nullptr) {
        return POSIX_ENOMEM;
    }
    try {
        value->state = std::make_shared<PthreadBarrier::State>();
    } catch (const std::bad_alloc&) {
        delete value;
        return POSIX_ENOMEM;
    }
    value->state->count = count;
    *barrier = value;
    return 0;
}

s32 PS4_SYSV_ABI posix_pthread_barrier_init(PthreadBarrierT* barrier, const PthreadBarrierAttrT*,
                                            u32 count) {
    return BarrierInit(barrier, count);
}

s32 PS4_SYSV_ABI scePthreadBarrierInit(PthreadBarrierT* barrier, const PthreadBarrierAttrT*,
                                       u32 count, const char*) {
    return BarrierInit(barrier, count);
}

s32 PS4_SYSV_ABI posix_pthread_barrier_wait(PthreadBarrierT* barrier) {
    if (barrier == nullptr || *barrier == nullptr) {
        return POSIX_EINVAL;
    }
    const auto value = (*barrier)->state;
    std::unique_lock lock{value->mutex};
    if (++value->arrived == value->count) {
        value->arrived = 0;
        ++value->generation;
        value->cv.notify_all();
        return PthreadBarrierSerialThread;
    }
    const u64 generation = value->generation;
    value->cv.wait(lock, [&] { return value->generation != generation; });
    return 0;
}

s32 PS4_SYSV_ABI scePthreadBarrierWait(PthreadBarrierT* barrier) {
    const s32 ret = posix_pthread_barrier_wait(barrier);
    return ret > 0 ? ErrnoToSceKernelError(ret) : ret;
}

s32 PS4_SYSV_ABI posix_pthread_barrier_destroy(PthreadBarrierT* barrier) {
    if (barrier == nullptr || *barrier == nullptr) {
        return POSIX_EINVAL;
    }
    auto* handle = *barrier;
    const auto value = handle->state;
    std::unique_lock lock{value->mutex};
    if (value->arrived != 0) {
        return POSIX_EBUSY;
    }
    *barrier = nullptr;
    delete handle;
    return 0;
}

s32 PS4_SYSV_ABI posix_pthread_barrierattr_init(PthreadBarrierAttrT* attr) {
    if (attr == nullptr) {
        return POSIX_EINVAL;
    }
    auto* value = new (std::nothrow) PthreadBarrierAttr{};
    if (value == nullptr) {
        return POSIX_ENOMEM;
    }
    *attr = value;
    return 0;
}

s32 PS4_SYSV_ABI posix_pthread_barrierattr_destroy(PthreadBarrierAttrT* attr) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    delete *attr;
    return 0;
}

s32 PS4_SYSV_ABI posix_pthread_barrierattr_getpshared(const PthreadBarrierAttrT* attr,
                                                      s32* pshared) {
    if (attr == nullptr || *attr == nullptr) {
        return POSIX_EINVAL;
    }
    *pshared = (*attr)->pshared;
    return 0;
}

s32 PS4_SYSV_ABI posix_pthread_barrierattr_setpshared(PthreadBarrierAttrT* attr, s32 pshared) {
    if (attr == nullptr || *attr == nullptr || pshared != 0) {
        return POSIX_EINVAL;
    }
    (*attr)->pshared = pshared;
    return 0;
}

void RegisterBarrier(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ZsXLFtd2jqQ", "libkernel", 1, "libkernel", posix_pthread_barrier_init);
    LIB_FUNCTION("CawZgCYqXWk", "libkernel", 1, "libkernel", posix_pthread_barrier_wait);
    LIB_FUNCTION("+Pqub9HZCPo", "libkernel", 1, "libkernel", posix_pthread_barrier_destroy);
    LIB_FUNCTION("4nqCnLJSvck", "libkernel", 1, "libkernel", posix_pthread_barrierattr_init);
    LIB_FUNCTION("AsCQCYTbe80", "libkernel", 1, "libkernel", posix_pthread_barrierattr_destroy);
    LIB_FUNCTION("a5JZMyjFV68", "libkernel", 1, "libkernel", posix_pthread_barrierattr_getpshared);
    LIB_FUNCTION("jqrGJJxFhmU", "libkernel", 1, "libkernel", posix_pthread_barrierattr_setpshared);

    LIB_FUNCTION("ZsXLFtd2jqQ", "libScePosix", 1, "libkernel", posix_pthread_barrier_init);
    LIB_FUNCTION("CawZgCYqXWk", "libScePosix", 1, "libkernel", posix_pthread_barrier_wait);
    LIB_FUNCTION("+Pqub9HZCPo", "libScePosix", 1, "libkernel", posix_pthread_barrier_destroy);
    LIB_FUNCTION("4nqCnLJSvck", "libScePosix", 1, "libkernel", posix_pthread_barrierattr_init);
    LIB_FUNCTION("AsCQCYTbe80", "libScePosix", 1, "libkernel", posix_pthread_barrierattr_destroy);
    LIB_FUNCTION("a5JZMyjFV68", "libScePosix", 1, "libkernel",
                 posix_pthread_barrierattr_getpshared);
    LIB_FUNCTION("jqrGJJxFhmU", "libScePosix", 1, "libkernel",
                 posix_pthread_barrierattr_setpshared);

    LIB_FUNCTION("5dgOEPsEGqw", "libkernel", 1, "libkernel", ORBIS(scePthreadBarrierInit));
    LIB_FUNCTION("t9vVyTglqHQ", "libkernel", 1, "libkernel", scePthreadBarrierWait);
    LIB_FUNCTION("HudB2Jv2MPY", "libkernel", 1, "libkernel", ORBIS(posix_pthread_barrier_destroy));
    LIB_FUNCTION("SDkV9xhINKI", "libkernel", 1, "libkernel", ORBIS(posix_pthread_barrierattr_init));
    LIB_FUNCTION("oT-j4DqJHY8", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_barrierattr_destroy));
    LIB_FUNCTION("SkutDtgqJ9g", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_barrierattr_getpshared));
    LIB_FUNCTION("NpfpcLf5PYM", "libkernel", 1, "libkernel",
                 ORBIS(posix_pthread_barrierattr_setpshared));
}

} // namespace Libraries::Kernel
