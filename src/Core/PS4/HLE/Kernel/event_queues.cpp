#include "event_queues.h"

#include <Core/PS4/HLE/ErrorCodes.h>
#include <Core/PS4/HLE/Libs.h>
#include <Util/log.h>
#include <debug.h>

namespace HLE::Libs::LibKernel::EventQueues {
constexpr bool log_file_equeues = true;  // disable it to disable logging

int PS4_SYSV_ABI sceKernelCreateEqueue(SceKernelEqueue* eq, const char* name) {
    PRINT_FUNCTION_NAME();

    if (eq == nullptr) {
        LOG_TRACE_IF(log_file_equeues, "sceKernelCreateEqueue returned SCE_KERNEL_ERROR_EINVAL eq invalid\n");
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (name == nullptr) {
        LOG_TRACE_IF(log_file_equeues, "sceKernelCreateEqueue returned SCE_KERNEL_ERROR_EFAULT name invalid\n");
        return SCE_KERNEL_ERROR_EFAULT;
    }
    if (name == NULL) {
        LOG_TRACE_IF(log_file_equeues, "sceKernelCreateEqueue returned SCE_KERNEL_ERROR_EINVAL name is null\n");
        return SCE_KERNEL_ERROR_EINVAL;
    }

    if (strlen(name) > 31) {  // max is 32 including null terminator
        LOG_TRACE_IF(log_file_equeues, "sceKernelCreateEqueue returned SCE_KERNEL_ERROR_ENAMETOOLONG name size exceeds 32 bytes\n");
        return SCE_KERNEL_ERROR_ENAMETOOLONG;
    }
    *eq = new Kernel::Objects::EqueueInternal;

    (*eq)->setName(std::string(name));

    LOG_INFO_IF(log_file_equeues, "sceKernelCreateEqueue created with name \"{}\"\n", name);
    return SCE_OK;
}

int PS4_SYSV_ABI sceKernelWaitEqueue(SceKernelEqueue eq, HLE::Kernel::Objects::SceKernelEvent* ev, int num, int* out, SceKernelUseconds* timo) {
    PRINT_FUNCTION_NAME();

    if (eq == nullptr) {
        return SCE_KERNEL_ERROR_EBADF;
    }

    if (ev == nullptr) {
        return SCE_KERNEL_ERROR_EFAULT;
    }

    if (num < 1) {
        return SCE_KERNEL_ERROR_EINVAL;
    }
    if (timo == nullptr) {  // wait until an event arrives without timing out
        *out = eq->waitForEvents(ev, num, 0);
    }
    if (timo != nullptr) {
        if (*timo == 0) {//only events that have already arrived at the time of this function call can be received
            BREAKPOINT();
        } else {  // wait until an event arrives with timing out
            BREAKPOINT();
        }
    }
    return SCE_OK;
}

};  // namespace HLE::Libs::LibKernel::EventQueues