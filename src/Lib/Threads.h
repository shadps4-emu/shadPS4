#pragma once

#include "windows.h"
#include <synchapi.h>
#include <atomic>
#include <condition_variable>
#include <string>
#include <thread>

#include "../types.h"


namespace Lib {
using thread_func_t = void (*)(void*);

void InitThreads();

struct ThreadStructInternal;
struct MutexStructInternal;
struct ConditionVariableStructInternal;

class Thread {
  public:
    Thread(thread_func_t func, void* arg);
    virtual ~Thread();

    void JoinThread();
    void DetachThread();

    std::string GetId() const;
    int GetUniqueId() const;

    static void SleepThread(u32 millis);
    static void SleepThreadMicro(u32 micros);
    static void SleepThreadNano(u64 nanos);
    static bool IsMainThread();

    // Get current thread id (reusable id)
    static std::string GetThreadId();

    // Get current thread id (unique id)
    static int GetThreadIdUnique();

  public:
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;

  private:
    ThreadStructInternal* m_thread;
};

struct ThreadStructInternal {
    ThreadStructInternal(thread_func_t f, void* a) : func(f), arg(a), m_thread(&Run, this) {}

    static void Run(ThreadStructInternal* t) {
        t->unique_id = Lib::Thread::GetThreadIdUnique();
        t->started = true;
        t->func(t->arg);
    }

    thread_func_t func;
    void* arg;
    std::atomic_bool started = false;
    int unique_id = 0;
    std::thread m_thread;
};

class Mutex {
  public:
    Mutex();
    virtual ~Mutex();

    void LockMutex();
    void UnlockMutex();
    bool TryLockMutex();

    friend class ConditionVariable;

  public:
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex(Mutex&&) = delete;
    Mutex& operator=(Mutex&&) = delete;

  private:
    MutexStructInternal* m_mutex;
};

struct MutexStructInternal {
    MutexStructInternal() { InitializeCriticalSectionAndSpinCount(&m_cs, 4000); }
    ~MutexStructInternal() { DeleteCriticalSection(&m_cs); }
    CRITICAL_SECTION m_cs{};
};
class ConditionVariable {
  public:
    ConditionVariable();
    virtual ~ConditionVariable();

    void WaitCondVar(Mutex* mutex);
    bool WaitCondVarFor(Mutex* mutex, u32 micros);
    void SignalCondVar();
    void SignalCondVarAll();

  private:
    ConditionVariableStructInternal* m_cond_var;
};

struct ConditionVariableStructInternal {
    ConditionVariableStructInternal() { InitializeConditionVariable(&m_cv); }
    ~ConditionVariableStructInternal() = default;
    CONDITION_VARIABLE m_cv{};
};

class LockMutexGuard {
  public:
    explicit LockMutexGuard(Mutex& m) : m_mutex(m) { m_mutex.LockMutex(); }

    ~LockMutexGuard() { m_mutex.UnlockMutex(); }

  private:
    Mutex& m_mutex;
};
}  // namespace Lib