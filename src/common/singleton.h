// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

namespace Common {

template <class T>
class Singleton {
public:
    static T* Instance() {
        if (!m_instance) {
            m_instance = std::make_unique<T>();
        }
        return m_instance.get();
    }

protected:
    Singleton();
    ~Singleton();

private:
    static inline std::unique_ptr<T> m_instance{};
};

} // namespace Common
