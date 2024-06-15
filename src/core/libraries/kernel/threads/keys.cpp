// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/threads/threads.h"
#include "core/libraries/libs.h"

namespace Libraries::Kernel {

extern PThreadCxt* g_pthread_cxt;

bool PthreadKeys::CreateKey(int* key, PthreadKeyDestructor destructor) {
    std::scoped_lock lk{m_mutex};

    for (int index = 0; index < 256; index++) {
        if (!m_keys[index].used) {
            *key = index;
            m_keys[index].used = true;
            m_keys[index].destructor = destructor;
            m_keys[index].specific_values.clear();
            return true;
        }
    }

    return false;
}

bool PthreadKeys::GetKey(int key, int thread_id, void** data) {
    std::scoped_lock lk{m_mutex};

    if (key < 0 || key >= 256 || !m_keys[key].used) {
        return false;
    }

    for (auto& v : m_keys[key].specific_values) {
        if (v.thread_id == thread_id) {
            *data = v.data;
            return true;
        }
    }

    *data = nullptr;

    return true;
}
bool PthreadKeys::SetKey(int key, int thread_id, void* data) {
    std::scoped_lock lk{m_mutex};

    if (key < 0 || key >= 256 || !m_keys[key].used) {
        return false;
    }

    for (auto& v : m_keys[key].specific_values) {
        if (v.thread_id == thread_id) {
            v.data = data;
            return true;
        }
    }

    Map keymap = {thread_id, data};
    m_keys[key].specific_values.push_back(keymap);

    return true;
}

int PS4_SYSV_ABI scePthreadKeyCreate(OrbisPthreadKey* key, PthreadKeyDestructor destructor) {
    if (key == nullptr) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    if (!g_pthread_cxt->getPthreadKeys()->CreateKey(key, destructor)) {
        return ORBIS_KERNEL_ERROR_EAGAIN;
    }

    return ORBIS_OK;
}

void* PS4_SYSV_ABI scePthreadGetspecific(OrbisPthreadKey key) {
    auto id = std::this_thread::get_id();
    int thread_id = *(unsigned*)&id;

    void* value = nullptr;
    if (!g_pthread_cxt->getPthreadKeys()->GetKey(key, thread_id, &value)) {
        return nullptr;
    }

    return value;
}

int PS4_SYSV_ABI scePthreadSetspecific(OrbisPthreadKey key, /* const*/ void* value) {
    auto id = std::this_thread::get_id();
    int thread_id = *(unsigned*)&id;

    if (!g_pthread_cxt->getPthreadKeys()->SetKey(key, thread_id, value)) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    return ORBIS_OK;
}

} // namespace Libraries::Kernel
