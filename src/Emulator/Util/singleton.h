#pragma once

#include <cstdlib>
#include <new>

template <class T>
class singleton {
  public:
    static T* instance() {
        if (!m_instance) {
            m_instance = static_cast<T*>(std::malloc(sizeof(T)));
            new (m_instance) T;
        }

        return m_instance;
    }

  protected:
    singleton();
    ~singleton();

  private:
    static inline T* m_instance = nullptr;
};