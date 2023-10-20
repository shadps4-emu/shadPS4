#include "file_system.h"
#include <debug.h>
#include <Util/log.h>

namespace Emulator::HLE::Libraries::LibKernel::FileSystem {
constexpr bool log_file_fs = true;  // disable it to disable logging

int PS4_SYSV_ABI sceKernelOpen(const char* path, int flags, u16 mode) { 
	LOG_INFO_IF(log_file_fs, "sceKernelOpen path = {} flags = {} mode = {}\n", path, log_hex_full(flags), log_hex_full(mode));
	return 0; 
}

}  // namespace Emulator::HLE::Libraries::LibKernel::FileSystem