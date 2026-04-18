// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <mutex>

template <typename T, size_t N, int INVALID_OBJECT_ID_ERROR, int OBJECT_NOT_FOUND_ERROR,
          int MAX_OBJECTS_ERROR>
struct ObjectManager {
    s32 GetObject(int objectId, T** out) {
        std::scoped_lock lk{mutex};
        if (objectId < 1 || objectId > N) {
            return INVALID_OBJECT_ID_ERROR;
        }
        auto obj = objects[objectId - 1];
        if (!obj) {
            return OBJECT_NOT_FOUND_ERROR;
        }
        *out = obj;
        return ORBIS_OK;
    }

    template <typename... Args>
    s32 CreateObject(Args&&... args) {
        std::scoped_lock lk{mutex};

        if (auto slot = std::ranges::find(objects, nullptr); slot != objects.end()) {
            *slot = new T{args...};

            return std::ranges::distance(objects.begin(), slot) + 1;
        }

        return MAX_OBJECTS_ERROR;
    }

    s32 DeleteObject(int objectId) {
        std::scoped_lock lk{mutex};

        if (objectId < 1 || objectId > N) {
            return INVALID_OBJECT_ID_ERROR;
        }
        auto obj = objects[objectId - 1];
        if (!obj) {
            return OBJECT_NOT_FOUND_ERROR;
        }

        delete obj;
        objects[objectId - 1] = nullptr;

        return ORBIS_OK;
    }

private:
    std::mutex mutex;
    std::array<T*, N> objects = {nullptr};
};