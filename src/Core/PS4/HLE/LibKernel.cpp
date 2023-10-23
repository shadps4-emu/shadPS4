#include "LibKernel.h"

#include <Util/log.h>
#include <debug.h>
#include <io.h>
#include <sys/types.h>
#include <windows.h>
#include "Emulator/Util/singleton.h"
#include "../Loader/Elf.h"
#include "Kernel/Objects/physical_memory.h"
#include "Kernel/ThreadManagement.h"
#include "Kernel/cpu_management.h"
#include "Kernel/event_queues.h"
#include "Kernel/memory_management.h"
#include "Libs.h"
#include "Emulator/HLE/Libraries/LibKernel/FileSystem/file_system.h"
#include "Emulator/HLE/Libraries/LibKernel/FileSystem/posix_file_system.h"

namespace HLE::Libs::LibKernel {

static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC;  // dummy return
  
int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len) {
    BREAKPOINT();
    return 0;
}

static PS4_SYSV_ABI void stack_chk_fail() { BREAKPOINT(); }
u64 PS4_SYSV_ABI sceKernelReadTsc() {
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return c.QuadPart;
}
PS4_SYSV_ABI void sched_yield() { BREAKPOINT(); }
PS4_SYSV_ABI void sysconf() { BREAKPOINT(); }
PS4_SYSV_ABI void open() { BREAKPOINT(); }

PS4_SYSV_ABI void ioctl() { BREAKPOINT(); }
PS4_SYSV_ABI void sigprocmask() { BREAKPOINT(); }
PS4_SYSV_ABI void nanosleep() { BREAKPOINT(); }
PS4_SYSV_ABI void _write() { BREAKPOINT(); }
PS4_SYSV_ABI void sigaction() { BREAKPOINT(); }

PS4_SYSV_ABI void _exit() { BREAKPOINT(); }

namespace POSIX {
PS4_SYSV_ABI int pthread_cond_broadcast(ThreadManagement::ScePthreadCond* cond) {
    // posix call the difference is that there is a different behaviour when it doesn't return 0 or SCE_OK
    int result = ThreadManagement::scePthreadCondBroadcast(cond);
    if (result != 0) {
        BREAKPOINT();
    }
    return result;
}
PS4_SYSV_ABI void pthread_rwlock_wrlock() { BREAKPOINT(); }
PS4_SYSV_ABI void pthread_cond_destroy() { BREAKPOINT(); }

PS4_SYSV_ABI void pthread_rwlock_rdlock() { BREAKPOINT(); }

PS4_SYSV_ABI int pthread_setspecific(ThreadManagement::ScePthreadKey key, /* const*/ void* value) {
    // posix call the difference is that there is a different behaviour when it doesn't return 0 or SCE_OK
    int result = ThreadManagement::scePthreadSetspecific(key, value);
    if (result != 0) {
        BREAKPOINT();
    }
    return result;
}
PS4_SYSV_ABI void pthread_equal() { BREAKPOINT(); }
PS4_SYSV_ABI int pthread_mutex_unlock(ThreadManagement::ScePthreadMutex* mutex) {
    // posix call the difference is that there is a different behaviour when it doesn't return 0 or SCE_OK
    int result = ThreadManagement::scePthreadMutexUnlock(mutex);
    if (result != 0) {
        BREAKPOINT();
    }
    return result;
}

PS4_SYSV_ABI void pthread_cond_timedwait() { BREAKPOINT(); }
PS4_SYSV_ABI void pthread_rwlock_unlock() { BREAKPOINT(); }
PS4_SYSV_ABI void pthread_detach() { BREAKPOINT(); }
PS4_SYSV_ABI void pthread_mutexattr_init() { BREAKPOINT(); }
int PS4_SYSV_ABI pthread_mutex_lock(ThreadManagement::ScePthreadMutex* mutex) {
    // posix call the difference is that there is a different behaviour when it doesn't return 0 or SCE_OK
    int result = ThreadManagement::scePthreadMutexLock(mutex);
    if (result != 0) {
        BREAKPOINT();
    }
    return result;
}
PS4_SYSV_ABI void pthread_mutex_destroy() { BREAKPOINT(); }
PS4_SYSV_ABI void pthread_mutex_trylock() { BREAKPOINT(); }
PS4_SYSV_ABI void pthread_join() { BREAKPOINT(); }
PS4_SYSV_ABI void* pthread_getspecific(ThreadManagement::ScePthreadKey key) {
    return HLE::Libs::LibKernel::ThreadManagement::scePthreadGetspecific(key);
}
PS4_SYSV_ABI void pthread_mutexattr_destroy() { BREAKPOINT(); }
PS4_SYSV_ABI void pthread_self() { BREAKPOINT(); }
PS4_SYSV_ABI void pthread_mutex_init() { BREAKPOINT(); }
PS4_SYSV_ABI int _pthread_once(ThreadManagement::ScePthreadOnce* once_control, void (*init_routine)(void)) {
    // posix call the difference is that there is a different behaviour when it doesn't return 0 or SCE_OK
    HLE::Libs::LibKernel::ThreadManagement::ScePthreadOnce o1nce_control = 0;
    *once_control = new HLE::Libs::LibKernel::ThreadManagement::PtheadOnceInternal{};
    (*once_control)->pthreadOnce = 0;
    int result = pthread_once(&(*once_control)->pthreadOnce, init_routine);
    if (result != 0) {
        BREAKPOINT();
    }
    return result;
}
PS4_SYSV_ABI void pthread_cond_wait() { BREAKPOINT(); }
PS4_SYSV_ABI void pthread_mutexattr_settype() { BREAKPOINT(); }
int PS4_SYSV_ABI pthread_key_create(ThreadManagement::ScePthreadKey* key, ThreadManagement::PthreadKeyDestructor destructor) {
    int result = HLE::Libs::LibKernel::ThreadManagement::scePthreadKeyCreate(key, destructor);
    if (result != 0) {
        BREAKPOINT();
    }
    return result;
}
PS4_SYSV_ABI void pthread_cond_signal() { BREAKPOINT(); }

PS4_SYSV_ABI void _open() { BREAKPOINT(); }
PS4_SYSV_ABI void _fcntl() { BREAKPOINT(); }
PS4_SYSV_ABI void _readv() { BREAKPOINT(); }
PS4_SYSV_ABI void _read() { BREAKPOINT(); }

};  // namespace POSIX

PS4_SYSV_ABI void poll() { BREAKPOINT(); }

PS4_SYSV_ABI void munmap() { BREAKPOINT(); }

PS4_SYSV_ABI void sceKernelUsleep(unsigned int microseconds) {
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
    // BREAKPOINT();
}

#define PROT_READ 0x1
#define PROT_WRITE 0x2

int PS4_SYSV_ABI sceKernelMmap(void* addr, u64 len, int prot, int flags, int fd, off_t offset, void** res) {
    DWORD flProtect;
    if (prot & PROT_WRITE) {
        flProtect = PAGE_READWRITE;
    }

    off_t end = len + offset;
    HANDLE mmap_fd, h;
    if (fd == -1)
        mmap_fd = INVALID_HANDLE_VALUE;
    else
        mmap_fd = (HANDLE)_get_osfhandle(fd);
    h = CreateFileMapping(mmap_fd, NULL, flProtect, 0, end, NULL);
    int k = GetLastError();
    if (NULL == h) return -1;

    DWORD dwDesiredAccess;
    if (prot & PROT_WRITE)
        dwDesiredAccess = FILE_MAP_WRITE;
    else
        dwDesiredAccess = FILE_MAP_READ;
    void* ret = MapViewOfFile(h, dwDesiredAccess, 0, offset, len);
    if (ret == NULL) {
        CloseHandle(h);
        ret = nullptr;
    }
    *res = ret;
    return 0;
}

PS4_SYSV_ABI void* mmap(void* addr, u64 len, int prot, int flags, int fd, u64 offset) {
    void* ptr;
    // posix call the difference is that there is a different behaviour when it doesn't return 0 or SCE_OK
    int result = sceKernelMmap(addr, len, prot, flags, fd, offset, &ptr);
    if (result != 0) {
        BREAKPOINT();
    }
    return ptr;
}

PS4_SYSV_ABI void close() { BREAKPOINT(); }

PS4_SYSV_ABI void madvise() { BREAKPOINT(); }
struct iovec {
    void* iov_base; /* Base	address. */
    size_t iov_len; /* Length. */
};

PS4_SYSV_ABI size_t _writev(int fd, const struct iovec* iov, int iovcn) {
    size_t total_written = 0;
    for (int i = 0; i < iovcn; i++) {
        total_written += ::fwrite(iov[i].iov_base, 1, iov[i].iov_len, stdout);
    }
    return total_written;
}
PS4_SYSV_ABI void lseek() { BREAKPOINT(); }
PS4_SYSV_ABI int* __error() { return _errno(); }

PS4_SYSV_ABI void raise() { BREAKPOINT(); }

PS4_SYSV_ABI void _ioctl() { BREAKPOINT(); }
PS4_SYSV_ABI void fstat() { BREAKPOINT(); }

void LibKernel_Register(SymbolsResolver* sym) {
    // obj
    LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &HLE::Libs::LibKernel::g_stack_chk_guard);
    // memory
    LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelAllocateDirectMemory);
    LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelGetDirectMemorySize);
    LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelMapDirectMemory);
    LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
    LIB_FUNCTION("cQke9UuBQOk", "libkernel", 1, "libkernel", 1, 1, sceKernelMunmap);
    // equeue
    LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, EventQueues::sceKernelCreateEqueue);
    LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, EventQueues::sceKernelWaitEqueue);
    // misc
    LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, CPUManagement::sceKernelIsNeoMode);
    LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
    // time
    LIB_FUNCTION("-2IRUCO--PM", "libkernel", 1, "libkernel", 1, 1, sceKernelReadTsc);

    // Pthreads
    LIB_FUNCTION("F8bUHwAG284", "libkernel", 1, "libkernel", 1, 1, ThreadManagement::scePthreadMutexattrInit);
    LIB_FUNCTION("iMp8QpE+XO4", "libkernel", 1, "libkernel", 1, 1, ThreadManagement::scePthreadMutexattrSettype);
    LIB_FUNCTION("1FGvU0i9saQ", "libkernel", 1, "libkernel", 1, 1, ThreadManagement::scePthreadMutexattrSetprotocol);
    LIB_FUNCTION("9UK1vLZQft4", "libkernel", 1, "libkernel", 1, 1, ThreadManagement::scePthreadMutexLock);
    LIB_FUNCTION("tn3VlD0hG60", "libkernel", 1, "libkernel", 1, 1, ThreadManagement::scePthreadMutexUnlock);
    LIB_FUNCTION("m5-2bsNfv7s", "libkernel", 1, "libkernel", 1, 1, ThreadManagement::scePthreadCondattrInit);
    LIB_FUNCTION("2Tb92quprl0", "libkernel", 1, "libkernel", 1, 1, ThreadManagement::scePthreadCondInit);
    LIB_FUNCTION("JGgj7Uvrl+A", "libkernel", 1, "libkernel", 1, 1, ThreadManagement::scePthreadCondBroadcast);

    LIB_FUNCTION("6XG4B33N09g", "libkernel", 1, "libkernel", 1, 1, sched_yield);
    LIB_FUNCTION("mkawd0NA9ts", "libkernel", 1, "libkernel", 1, 1, sysconf);

    LIB_FUNCTION("wuCroIGjt2g", "libkernel", 1, "libkernel", 1, 1, open);
    LIB_FUNCTION("PfccT7qURYE", "libkernel", 1, "libkernel", 1, 1, ioctl);
    LIB_FUNCTION("aPcyptbOiZs", "libkernel", 1, "libkernel", 1, 1, sigprocmask);
    LIB_FUNCTION("mqQMh1zPPT8", "libkernel", 1, "libkernel", 1, 1, fstat);
    LIB_FUNCTION("yS8U2TGCe1A", "libkernel", 1, "libkernel", 1, 1, nanosleep);
    LIB_FUNCTION("FxVZqBAA7ks", "libkernel", 1, "libkernel", 1, 1, _write);
    LIB_FUNCTION("KiJEPEWRyUY", "libkernel", 1, "libkernel", 1, 1, sigaction);
    LIB_FUNCTION("6Z83sYWFlA8", "libkernel", 1, "libkernel", 1, 1, _exit);
    LIB_FUNCTION("mkx2fVhNMsg", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_cond_broadcast);
    LIB_FUNCTION("sIlRvQqsN2Y", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_rwlock_wrlock);
    LIB_FUNCTION("RXXqi4CtF8w", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_cond_destroy);
    LIB_FUNCTION("iGjsr1WAtI0", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_rwlock_rdlock);
    LIB_FUNCTION("WrOLvHU0yQM", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_setspecific);
    LIB_FUNCTION("7Xl257M4VNI", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_equal);
    LIB_FUNCTION("2Z+PpY6CaJg", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_mutex_unlock);
    LIB_FUNCTION("27bAgiJmOh0", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_cond_timedwait);
    LIB_FUNCTION("ku7D4q1Y9PI", "libkernel", 1, "libkernel", 1, 1, poll);
    LIB_FUNCTION("EgmLo6EWgso", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_rwlock_unlock);
    LIB_FUNCTION("+U1R4WtXvoc", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_detach);
    LIB_FUNCTION("dQHWEsJtoE4", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_mutexattr_init);
    LIB_FUNCTION("7H0iTOciTLo", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_mutex_lock);
    LIB_FUNCTION("UqDGjXA5yUM", "libkernel", 1, "libkernel", 1, 1, munmap);
    LIB_FUNCTION("ltCfaGr2JGE", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_mutex_destroy);
    LIB_FUNCTION("1jfXLRVzisc", "libkernel", 1, "libkernel", 1, 1, sceKernelUsleep);
    LIB_FUNCTION("K-jXhbt2gn4", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_mutex_trylock);
    LIB_FUNCTION("BPE9s9vQQXo", "libkernel", 1, "libkernel", 1, 1, mmap);
    LIB_FUNCTION("h9CcP3J0oVM", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_join);
    LIB_FUNCTION("0-KXaS70xy4", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_getspecific);
    LIB_FUNCTION("HF7lK46xzjY", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_mutexattr_destroy);
    LIB_FUNCTION("EotR8a3ASf4", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_self);
    LIB_FUNCTION("bY-PO6JhzhQ", "libkernel", 1, "libkernel", 1, 1, close);
    LIB_FUNCTION("mDmgMOGVUqg", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_mutexattr_settype);
    LIB_FUNCTION("mqULNdimTn0", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_key_create);
    LIB_FUNCTION("ttHNfU+qDBU", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_mutex_init);
    LIB_FUNCTION("Jahsnh4KKkg", "libkernel", 1, "libkernel", 1, 1, madvise);
    LIB_FUNCTION("YSHRBRLn2pI", "libkernel", 1, "libkernel", 1, 1, _writev);
    LIB_FUNCTION("Oy6IpwgtYOk", "libkernel", 1, "libkernel", 1, 1, lseek);
    LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", 1, 1, __error);
    LIB_FUNCTION("Z4QosVuAsA0", "libkernel", 1, "libkernel", 1, 1, POSIX::_pthread_once);
    LIB_FUNCTION("Op8TBGY5KHg", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_cond_wait);
    LIB_FUNCTION("0t0-MxQNwK4", "libkernel", 1, "libkernel", 1, 1, raise);
    LIB_FUNCTION("2MOy+rUfuhQ", "libkernel", 1, "libkernel", 1, 1, POSIX::pthread_cond_signal);
    LIB_FUNCTION("wW+k21cmbwQ", "libkernel", 1, "libkernel", 1, 1, _ioctl);
    LIB_FUNCTION("6c3rCVE-fTU", "libkernel", 1, "libkernel", 1, 1, POSIX::_open);
    LIB_FUNCTION("t0fXUzq61Z4", "libkernel", 1, "libkernel", 1, 1, POSIX::_fcntl);
    LIB_FUNCTION("+WRlkKjZvag", "libkernel", 1, "libkernel", 1, 1, POSIX::_readv);
    LIB_FUNCTION("DRuBt2pvICk", "libkernel", 1, "libkernel", 1, 1, POSIX::_read);

    // fs
    LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", 1, 1, Emulator::HLE::Libraries::LibKernel::FileSystem::sceKernelOpen);
    LIB_FUNCTION("wuCroIGjt2g", "libScePosix", 1, "libkernel", 1, 1, Emulator::HLE::Libraries::LibKernel::FileSystem::POSIX::open);

}

};  // namespace HLE::Libs::LibKernel