// INAA License @marecl 2025

#pragma once

// open flags
#define QUASI_O_RDONLY 0x0000
#define QUASI_O_WRONLY 0x0001
#define QUASI_O_RDWR 0x0002
#define QUASI_O_NONBLOCK 0x0004
#define QUASI_O_APPEND 0x0008
#define QUASI_O_FSYNC 0x0080
#define QUASI_O_SYNC 0x0080
#define QUASI_O_CREAT 0x0200
#define QUASI_O_TRUNC 0x0400
#define QUASI_O_EXCL 0x0800
#define QUASI_O_DSYNC 0x1000
#define QUASI_O_DIRECT 0x00010000
#define QUASI_O_DIRECTORY 0x00020000

#define __QUASI_O_ALLFLAGS_AT_ONCE                                                                 \
    (QUASI_O_RDONLY | QUASI_O_WRONLY | QUASI_O_RDWR | QUASI_O_NONBLOCK | QUASI_O_APPEND |          \
     QUASI_O_FSYNC | QUASI_O_SYNC | QUASI_O_CREAT | QUASI_O_TRUNC | QUASI_O_EXCL | QUASI_O_DSYNC | \
     QUASI_O_DIRECT | QUASI_O_DIRECTORY)

// not implemented
#define QUASI_O_CLOEXEC 0x100000
