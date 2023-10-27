#pragma once

#include <memory>

template <class T>
class singleton {
public:
    static T* instance() {
        if (!m_instance) {
            m_instance = std::make_unique<T>();
        }
        return m_instance.get();
    }

protected:
    singleton();
    ~singleton();

private:
    static inline std::unique_ptr<T> m_instance{};
};
