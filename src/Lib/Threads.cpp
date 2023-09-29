#include "Threads.h"

#include <sstream>
#include <chrono>

static std::thread::id g_main_thread;
static int g_main_thread_int;
static std::atomic<int> g_thread_counter = 0;

void Lib::InitThreads() {
    g_main_thread = std::this_thread::get_id();
    g_main_thread_int = Lib::Thread::GetThreadIdUnique();
}

Lib::Thread::Thread(thread_func_t func, void* arg) {
    m_thread = new ThreadStructInternal(func, arg);
    while (!m_thread->started) {
        SleepThreadMicro(1000);
    }
}

Lib::Thread::~Thread() { delete m_thread; }

void Lib::Thread::JoinThread() { m_thread->m_thread.join(); }

void Lib::Thread::DetachThread() { m_thread->m_thread.detach(); }

std::string Lib::Thread::GetId() const {
    std::stringstream ss;
    ss << m_thread->m_thread.get_id();
    return ss.str().c_str();
}

int Lib::Thread::GetUniqueId() const { return m_thread->unique_id; }

void Lib::Thread::SleepThread(u32 millis) { std::this_thread::sleep_for(std::chrono::milliseconds(millis)); }

void Lib::Thread::SleepThreadMicro(u32 micros) { std::this_thread::sleep_for(std::chrono::microseconds(micros)); }

void Lib::Thread::SleepThreadNano(u64 nanos) { std::this_thread::sleep_for(std::chrono::nanoseconds(nanos)); }

bool Lib::Thread::IsMainThread() { return g_main_thread == std::this_thread::get_id(); }

std::string Lib::Thread::GetThreadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str().c_str();
}

int Lib::Thread::GetThreadIdUnique() {
    static thread_local int tid = ++g_thread_counter;
    return tid;
}

Lib::Mutex::Mutex() { m_mutex = new MutexStructInternal(); }

Lib::Mutex::~Mutex() { delete m_mutex; }

void Lib::Mutex::LockMutex() {
    if (m_mutex->m_owner == std::this_thread::get_id()) {
        return;
    }

    m_mutex->m_cs.lock();
    m_mutex->m_owner = std::this_thread::get_id();
}

void Lib::Mutex::UnlockMutex() {
    m_mutex->m_owner = std::thread::id();
    m_mutex->m_cs.unlock();
}

bool Lib::Mutex::TryLockMutex() {
    if (m_mutex->m_owner == std::this_thread::get_id()) {
        return false;
    }

    if (m_mutex->m_cs.try_lock()) {
        m_mutex->m_owner = std::this_thread::get_id();
        return true;
    }
    return false;
}

Lib::ConditionVariable::ConditionVariable() { m_cond_var = new ConditionVariableStructInternal(); }

Lib::ConditionVariable::~ConditionVariable() { delete m_cond_var; }

void Lib::ConditionVariable::WaitCondVar(Mutex* mutex) {
    std::unique_lock<std::mutex> lock(mutex->m_mutex->m_cs);
    m_cond_var->m_cv.wait(lock);
}

bool Lib::ConditionVariable::WaitCondVarFor(Mutex* mutex, u32 micros) {
    std::unique_lock<std::mutex> lock(mutex->m_mutex->m_cs);
    return m_cond_var->m_cv.wait_for(lock, std::chrono::microseconds(micros)) == std::cv_status::no_timeout;
}

void Lib::ConditionVariable::SignalCondVar() { m_cond_var->m_cv.notify_one(); }

void Lib::ConditionVariable::SignalCondVarAll() { m_cond_var->m_cv.notify_all(); }
