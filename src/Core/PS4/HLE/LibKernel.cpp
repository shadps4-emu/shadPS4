#include "../Loader/Elf.h"
#include "LibKernel.h"
#include "Libs.h"
#include <debug.h>
#include <Util/log.h>
#include "Kernel/memory_management.h"
#include "../../../Util/Singleton.h"
#include "Kernel/Objects/physical_memory.h"
#include "Kernel/cpu_management.h"
#include "Kernel/event_queues.h"

namespace HLE::Libs::LibKernel {

    static u64 g_stack_chk_guard = 0xDEADBEEF54321ABC; //dummy return

    int32_t PS4_SYSV_ABI sceKernelReleaseDirectMemory(off_t start, size_t len) { 
        BREAKPOINT();
        return 0;
    }


    static PS4_SYSV_ABI void stack_chk_fail() { BREAKPOINT();
    }
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
    PS4_SYSV_ABI void pthread_cond_broadcast() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_rwlock_wrlock() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_cond_destroy() { BREAKPOINT(); }


    PS4_SYSV_ABI void pthread_rwlock_rdlock() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_setspecific() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_equal() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_mutex_unlock() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_cond_timedwait() { BREAKPOINT(); }
    PS4_SYSV_ABI void poll() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_rwlock_unlock() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_detach() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_mutexattr_init() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_mutex_lock() { BREAKPOINT(); }
    PS4_SYSV_ABI void munmap() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_mutex_destroy() { BREAKPOINT(); }
    PS4_SYSV_ABI void sceKernelUsleep() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_mutex_trylock() { BREAKPOINT(); }
    PS4_SYSV_ABI void mmap() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_join() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_getspecific() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_mutexattr_destroy() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_self() { BREAKPOINT(); }
    PS4_SYSV_ABI void close() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_mutexattr_settype() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_key_create() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_mutex_init() { BREAKPOINT(); }
    PS4_SYSV_ABI void madvise() { BREAKPOINT(); }
    PS4_SYSV_ABI void _writev() { BREAKPOINT(); }
    PS4_SYSV_ABI void lseek() { BREAKPOINT(); }
    PS4_SYSV_ABI void __error() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_once() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_cond_wait() { BREAKPOINT(); }
    PS4_SYSV_ABI void raise() { BREAKPOINT(); }
    PS4_SYSV_ABI void pthread_cond_signal() { BREAKPOINT(); }

    void LibKernel_Register(SymbolsResolver* sym) { 
        //obj
        LIB_OBJ("f7uOxY9mM1U", "libkernel", 1, "libkernel", 1, 1, &HLE::Libs::LibKernel::g_stack_chk_guard);
        //memory
        LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelAllocateDirectMemory); 
        LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelGetDirectMemorySize);
        LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel", 1, 1, MemoryManagement::sceKernelMapDirectMemory);
        LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel", 1, 1, sceKernelReleaseDirectMemory);
        //equeue
        LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel", 1, 1, EventQueues::sceKernelCreateEqueue);
        LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", 1, 1, EventQueues::sceKernelWaitEqueue);
        //misc
        LIB_FUNCTION("WslcK1FQcGI", "libkernel", 1, "libkernel", 1, 1, CPUManagement::sceKernelIsNeoMode);
        LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", 1, 1, stack_chk_fail);
        //time
        LIB_FUNCTION("-2IRUCO--PM", "libkernel", 1, "libkernel", 1, 1, sceKernelReadTsc);

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
        LIB_FUNCTION("mkx2fVhNMsg", "libkernel", 1, "libkernel", 1, 1, pthread_cond_broadcast);
        LIB_FUNCTION("sIlRvQqsN2Y", "libkernel", 1, "libkernel", 1, 1, pthread_rwlock_wrlock);
        LIB_FUNCTION("RXXqi4CtF8w", "libkernel", 1, "libkernel", 1, 1, pthread_cond_destroy);
        LIB_FUNCTION("iGjsr1WAtI0", "libkernel", 1, "libkernel", 1, 1, pthread_rwlock_rdlock);
        LIB_FUNCTION("WrOLvHU0yQM", "libkernel", 1, "libkernel", 1, 1, pthread_setspecific);
        LIB_FUNCTION("7Xl257M4VNI", "libkernel", 1, "libkernel", 1, 1, pthread_equal);
        LIB_FUNCTION("2Z+PpY6CaJg", "libkernel", 1, "libkernel", 1, 1, pthread_mutex_unlock);
        LIB_FUNCTION("27bAgiJmOh0", "libkernel", 1, "libkernel", 1, 1, pthread_cond_timedwait);
        LIB_FUNCTION("ku7D4q1Y9PI", "libkernel", 1, "libkernel", 1, 1, poll);
        LIB_FUNCTION("EgmLo6EWgso", "libkernel", 1, "libkernel", 1, 1, pthread_rwlock_unlock);
        LIB_FUNCTION("+U1R4WtXvoc", "libkernel", 1, "libkernel", 1, 1, pthread_detach);
        LIB_FUNCTION("dQHWEsJtoE4", "libkernel", 1, "libkernel", 1, 1, pthread_mutexattr_init);
        LIB_FUNCTION("7H0iTOciTLo", "libkernel", 1, "libkernel", 1, 1, pthread_mutex_lock);
        LIB_FUNCTION("UqDGjXA5yUM", "libkernel", 1, "libkernel", 1, 1, munmap);
        LIB_FUNCTION("ltCfaGr2JGE", "libkernel", 1, "libkernel", 1, 1, pthread_mutex_destroy);
        LIB_FUNCTION("1jfXLRVzisc", "libkernel", 1, "libkernel", 1, 1, sceKernelUsleep);
        LIB_FUNCTION("K-jXhbt2gn4", "libkernel", 1, "libkernel", 1, 1, pthread_mutex_trylock);
        LIB_FUNCTION("BPE9s9vQQXo", "libkernel", 1, "libkernel", 1, 1, mmap);
        LIB_FUNCTION("h9CcP3J0oVM", "libkernel", 1, "libkernel", 1, 1, pthread_join);
        LIB_FUNCTION("0-KXaS70xy4", "libkernel", 1, "libkernel", 1, 1, pthread_getspecific);
        LIB_FUNCTION("HF7lK46xzjY", "libkernel", 1, "libkernel", 1, 1, pthread_mutexattr_destroy);
        LIB_FUNCTION("EotR8a3ASf4", "libkernel", 1, "libkernel", 1, 1, pthread_self);
        LIB_FUNCTION("bY-PO6JhzhQ", "libkernel", 1, "libkernel", 1, 1, close);
        LIB_FUNCTION("mDmgMOGVUqg", "libkernel", 1, "libkernel", 1, 1, pthread_mutexattr_settype);
        LIB_FUNCTION("mqULNdimTn0", "libkernel", 1, "libkernel", 1, 1, pthread_key_create);
        LIB_FUNCTION("ttHNfU+qDBU", "libkernel", 1, "libkernel", 1, 1, pthread_mutex_init);
        LIB_FUNCTION("Jahsnh4KKkg", "libkernel", 1, "libkernel", 1, 1, madvise);
        LIB_FUNCTION("YSHRBRLn2pI", "libkernel", 1, "libkernel", 1, 1, _writev);
        LIB_FUNCTION("Oy6IpwgtYOk", "libkernel", 1, "libkernel", 1, 1, lseek);
        LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", 1, 1, __error);
        LIB_FUNCTION("Z4QosVuAsA0", "libkernel", 1, "libkernel", 1, 1, pthread_once);
        LIB_FUNCTION("Op8TBGY5KHg", "libkernel", 1, "libkernel", 1, 1, pthread_cond_wait);
        LIB_FUNCTION("0t0-MxQNwK4", "libkernel", 1, "libkernel", 1, 1, raise);
        LIB_FUNCTION("2MOy+rUfuhQ", "libkernel", 1, "libkernel", 1, 1, pthread_cond_signal);
    }

}; 