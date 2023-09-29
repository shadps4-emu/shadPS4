#pragma once

#include <atomic>
#include <condition_variable>
#include <string>
#include <thread>
#include <functional>

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
    ThreadStructInternal(thread_func_t f, void* a) : func(f), arg(a), m_thread(&ThreadStructInternal::Run, this) {}

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
    MutexStructInternal() = default;
    ~MutexStructInternal() = default;
    std::mutex m_cs{};
    std::atomic<std::thread::id> m_owner{};
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
    ConditionVariableStructInternal() = default;
    ~ConditionVariableStructInternal() = default;
    std::condition_variable m_cv{};
};

class LockMutexGuard {
  public:
    explicit LockMutexGuard(Mutex& m) : m_mutex(m) { m_mutex.LockMutex(); }

    ~LockMutexGuard() { m_mutex.UnlockMutex(); }

  private:
    Mutex& m_mutex;

  public:
    LockMutexGuard(const LockMutexGuard&) = delete;
    LockMutexGuard& operator=(const LockMutexGuard&) = delete;
    LockMutexGuard(LockMutexGuard&&) noexcept = delete;
    LockMutexGuard& operator=(LockMutexGuard&&) noexcept = delete;
};
}  // namespace Lib