#include <fcntl.h>

#include "common/assert.h"
#include "core/libraries/kernel/posix_error.h"

// Convert linux/unix errno to FreeBSD errno
// They differ in higher errno numbers, which may throw Orbis off quite a bit

s32 posix2bsd(s32 id) {
    switch (id) {
    default:
        UNREACHABLE_MSG("Unknown POSIX errno");
    }
}