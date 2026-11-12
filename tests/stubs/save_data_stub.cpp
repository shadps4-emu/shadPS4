// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/system/userservice.h"
#include "imgui/imgui_layer.h"
#include "imgui/imgui_texture.h"

namespace ImGui {

void Layer::AddLayer(Layer* /*layer*/) {}
void Layer::RemoveLayer(Layer* /*layer*/) {}

RefCountedTexture::RefCountedTexture() : inner(nullptr) {}
RefCountedTexture::RefCountedTexture(Core::TextureManager::Inner* in) : inner(in) {}
RefCountedTexture::RefCountedTexture(const RefCountedTexture& other) : inner(other.inner) {}
RefCountedTexture::RefCountedTexture(RefCountedTexture&& other) noexcept : inner(other.inner) {
    other.inner = nullptr;
}
RefCountedTexture& RefCountedTexture::operator=(const RefCountedTexture& other) = default;
RefCountedTexture& RefCountedTexture::operator=(RefCountedTexture&& other) noexcept {
    inner = other.inner;
    other.inner = nullptr;
    return *this;
}
RefCountedTexture::~RefCountedTexture() = default;

RefCountedTexture RefCountedTexture::DecodePngTexture(std::vector<u8> /*data*/) {
    return RefCountedTexture{};
}

RefCountedTexture RefCountedTexture::DecodePngFile(std::filesystem::path /*path*/) {
    return RefCountedTexture{};
}

RefCountedTexture::Image RefCountedTexture::GetTexture() const {
    return Image{0, 0, 0};
}

RefCountedTexture::operator bool() const {
    return inner != nullptr;
}

} // namespace ImGui

namespace Libraries::UserService {

void AddUserServiceEvent(const OrbisUserServiceEvent /*e*/) {}

} // namespace Libraries::UserService

namespace Libraries::Kernel {

thread_local Pthread* g_curthread{};

struct StubThreadRegistry {
    std::mutex mtx;
    std::unordered_map<PthreadT, std::thread> threads;
    uintptr_t next_id = 0x1000;
};

StubThreadRegistry& GetPthreadState() {
    static auto* state = new StubThreadRegistry();
    return *state;
}

int PS4_SYSV_ABI posix_pthread_attr_init(PthreadAttrT* attr) {
    if (attr != nullptr) {
        *attr = reinterpret_cast<PthreadAttrT>(0x1);
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_attr_destroy(PthreadAttrT* attr) {
    if (attr != nullptr) {
        *attr = nullptr;
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_create(PthreadT* thread, const PthreadAttrT* /*attr*/,
                                      PthreadEntryFunc start_routine, void* arg) {
    auto& state = GetPthreadState();
    std::thread thr([start_routine, arg]() { start_routine(arg); });
    PthreadT handle = nullptr;
    {
        std::scoped_lock lk(state.mtx);
        handle = reinterpret_cast<PthreadT>(state.next_id);
        state.next_id += 16;
        state.threads.emplace(handle, std::move(thr));
    }
    if (thread != nullptr) {
        *thread = handle;
    }
    return 0;
}

int PS4_SYSV_ABI posix_pthread_join(PthreadT pthread, void** /*thread_return*/) {
    auto& state = GetPthreadState();
    std::thread thr;
    {
        std::scoped_lock lk(state.mtx);
        auto it = state.threads.find(pthread);
        if (it != state.threads.end()) {
            thr = std::move(it->second);
            state.threads.erase(it);
        }
    }
    if (thr.joinable()) {
        thr.join();
    }
    return 0;
}

} // namespace Libraries::Kernel
