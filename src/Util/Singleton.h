#pragma once

#include <cstdlib>
#include <new>

template <class T>
class Singleton
{
public:
	static T* Instance()
	{
		if (!m_instance)
		{
			m_instance = static_cast<T*>(std::malloc(sizeof(T)));
			new (m_instance) T;
		}

		return m_instance;
	}

protected:
	Singleton();
	~Singleton();

private:
	static inline T* m_instance = nullptr;
};