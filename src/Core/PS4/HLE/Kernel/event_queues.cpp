#include "event_queues.h"

#include <Core/PS4/HLE/ErrorCodes.h>
#include <Core/PS4/HLE/Libs.h>
#include <Util/log.h>

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
};  // namespace HLE::Libs::LibKernel::EventQueues