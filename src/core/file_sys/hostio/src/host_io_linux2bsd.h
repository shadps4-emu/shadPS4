// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <errno.h>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/posix_error.h"

// Convert linux/unix errno to FreeBSD errno
// They differ in higher errno numbers, which may throw Orbis off quite a bit

s32 unix2bsd(s32 id) {
    switch (id) {
    default:
        LOG_CRITICAL(Kernel_Fs, "Can't resolve POSIX errno: {}", id);
        return POSIX_EINVAL;
    case 0:
        return 0;
    case EPERM:
        return POSIX_EPERM;
    case ENOENT:
        return POSIX_ENOENT;
    case ESRCH:
        return POSIX_ESRCH;
    case EINTR:
        return POSIX_EINTR;
    case EIO:
        return POSIX_EIO;
    case ENXIO:
        return POSIX_ENXIO;
    case E2BIG:
        return POSIX_E2BIG;
    case ENOEXEC:
        return POSIX_ENOEXEC;
    case EBADF:
        return POSIX_EBADF;
    case ECHILD:
        return POSIX_ECHILD;
    case EDEADLK:
        return POSIX_EDEADLK;
    case ENOMEM:
        return POSIX_ENOMEM;
    case EACCES:
        return POSIX_EACCES;
    case EFAULT:
        return POSIX_EFAULT;

#ifndef _WIN32
    case ENOTBLK:
        return POSIX_ENOTBLK;
#endif

    case EBUSY:
        return POSIX_EBUSY;
    case EEXIST:
        return POSIX_EEXIST;
    case EXDEV:
        return POSIX_EXDEV;
    case ENODEV:
        return POSIX_ENODEV;
    case ENOTDIR:
        return POSIX_ENOTDIR;
    case EISDIR:
        return POSIX_EISDIR;
    case EINVAL:
        return POSIX_EINVAL;
    case ENFILE:
        return POSIX_ENFILE;
    case EMFILE:
        return POSIX_EMFILE;
    case ENOTTY:
        return POSIX_ENOTTY;
    case ETXTBSY:
        return POSIX_ETXTBSY;
    case EFBIG:
        return POSIX_EFBIG;
    case ENOSPC:
        return POSIX_ENOSPC;
    case ESPIPE:
        return POSIX_ESPIPE;
    case EROFS:
        return POSIX_EROFS;
    case EMLINK:
        return POSIX_EMLINK;
    case EPIPE:
        return POSIX_EPIPE;
    case EDOM:
        return POSIX_EDOM;
    case ERANGE:
        return POSIX_ERANGE;
    case EAGAIN:
        return POSIX_EAGAIN;
    // same as POSIX_EAGAIN
    // case EWOULDBLOCK:
    //     return POSIX_EWOULDBLOCK;
    case EINPROGRESS:
        return POSIX_EINPROGRESS;
    case EALREADY:
        return POSIX_EALREADY;
    case ENOTSOCK:
        return POSIX_ENOTSOCK;
    case EDESTADDRREQ:
        return POSIX_EDESTADDRREQ;
    case EMSGSIZE:
        return POSIX_EMSGSIZE;
    case EPROTOTYPE:
        return POSIX_EPROTOTYPE;
    case ENOPROTOOPT:
        return POSIX_ENOPROTOOPT;
    case EPROTONOSUPPORT:
        return POSIX_EPROTONOSUPPORT;

#if !defined(_WIN32)
    case ESOCKTNOSUPPORT:
        return POSIX_ESOCKTNOSUPPORT;
#endif

    case EOPNOTSUPP:
        return POSIX_EOPNOTSUPP;
        // same as POSIX_EOPNOTSUPP
        // case ENOTSUP:
        //     return POSIX_ENOTSUP;

#if !defined(_WIN32)
    case EPFNOSUPPORT:
        return POSIX_EPFNOSUPPORT;
#endif

    case EAFNOSUPPORT:
        return POSIX_EAFNOSUPPORT;
    case EADDRINUSE:
        return POSIX_EADDRINUSE;
    case EADDRNOTAVAIL:
        return POSIX_EADDRNOTAVAIL;
    case ENETDOWN:
        return POSIX_ENETDOWN;
    case ENETUNREACH:
        return POSIX_ENETUNREACH;
    case ENETRESET:
        return POSIX_ENETRESET;
    case ECONNABORTED:
        return POSIX_ECONNABORTED;
    case ECONNRESET:
        return POSIX_ECONNRESET;
    case ENOBUFS:
        return POSIX_ENOBUFS;
    case EISCONN:
        return POSIX_EISCONN;
    case ENOTCONN:
        return POSIX_ENOTCONN;

#if !defined(_WIN32)
    case ESHUTDOWN:
        return POSIX_ESHUTDOWN;
    case ETOOMANYREFS:
        return POSIX_ETOOMANYREFS;
#endif

    case ETIMEDOUT:
        return POSIX_ETIMEDOUT;
    case ECONNREFUSED:
        return POSIX_ECONNREFUSED;
    case ELOOP:
        return POSIX_ELOOP;
    case ENAMETOOLONG:
        return POSIX_ENAMETOOLONG;

#if !defined(_WIN32)
    case EHOSTDOWN:
        return POSIX_EHOSTDOWN;
#endif

    case EHOSTUNREACH:
        return POSIX_EHOSTUNREACH;
    case ENOTEMPTY:
        return POSIX_ENOTEMPTY;

#if !(defined(_WIN32) || defined(__linux__))
    case EPROCLIM:
        return POSIX_EPROCLIM;
#endif

#if !defined(_WIN32)
    case EUSERS:
        return POSIX_EUSERS;
    case EDQUOT:
        return POSIX_EDQUOT;
    case ESTALE:
        return POSIX_ESTALE;
    case EREMOTE:
        return POSIX_EREMOTE;
#endif

#if !(defined(_WIN32) || defined(__linux__))
    case EBADRPC:
        return POSIX_EBADRPC;
    case ERPCMISMATCH:
        return POSIX_ERPCMISMATCH;
    case EPROGUNAVAIL:
        return POSIX_EPROGUNAVAIL;
    case EPROGMISMATCH:
        return POSIX_EPROGMISMATCH;
    case EPROCUNAVAIL:
        return POSIX_EPROCUNAVAIL;
#endif

    case ENOLCK:
        return POSIX_ENOLCK;
    case ENOSYS:
        return POSIX_ENOSYS;

#if !(defined(_WIN32) || defined(__linux__))
    case EFTYPE:
        return POSIX_EFTYPE;
    case EAUTH:
        return POSIX_EAUTH;
    case ENEEDAUTH:
        return POSIX_ENEEDAUTH;
#endif

    case EIDRM:
        return POSIX_EIDRM;
    case ENOMSG:
        return POSIX_ENOMSG;
    case EOVERFLOW:
        return POSIX_EOVERFLOW;
    case ECANCELED:
        return POSIX_ECANCELED;
    case EILSEQ:
        return POSIX_EILSEQ;

#if !(defined(_WIN32) || defined(__linux__))
    case ENOATTR:
        return POSIX_ENOATTR;
#ifndef __APPLE_CC__
    case EDOOFUS:
        return POSIX_EDOOFUS;
#endif
#endif // !(defined(_WIN32) || defined(__linux__))

    case EBADMSG:
        return POSIX_EBADMSG;

#ifndef _WIN32
    case EMULTIHOP:
        return POSIX_EMULTIHOP;
#endif

    case ENOLINK:
        return POSIX_ENOLINK;
    case EPROTO:
        return POSIX_EPROTO;

#if !(defined(_WIN32) || defined(__linux__))
    case ENOTCAPABLE:
        return POSIX_ENOTCAPABLE;
#ifndef __APPLE_CC__
    case ECAPMODE:
        return POSIX_ECAPMODE;
    case ENOBLK:
        return POSIX_ENOBLK;
    case EICV:
        return POSIX_EICV;
    case ENOPLAYGOENT:
        return POSIX_ENOPLAYGOENT;
    case EREVOKE:
        return POSIX_EREVOKE;
    case ESDKVERSION:
        return POSIX_ESDKVERSION;
    case ESTART:
        return POSIX_ESTART;
    case ESTOP:
        return POSIX_ESTOP;
    case EINVALID2MB:
        return POSIX_EINVALID2MB;
#endif          // !__APPLE_CC__
    case ELAST: // same as EINVALID2MB on macOS
        return POSIX_ELAST;
#ifndef __APPLE_CC__
    case EADHOC:
        return POSIX_EADHOC;
    case EINACTIVEDISABLED:
        return POSIX_EINACTIVEDISABLED;
    case ENETNODATA:
        return POSIX_ENETNODATA;
    case ENETDESC:
        return POSIX_ENETDESC;
    case ENETDESCTIMEDOUT:
        return POSIX_ENETDESCTIMEDOUT;
    case ENETINTR:
        return POSIX_ENETINTR;
    case ERETURN:
        return POSIX_ERETURN;
    case EFPOS:
        return POSIX_EFPOS;
#endif // !__APPLE_CC__
#endif

    case ENODATA:
        return POSIX_ENODATA;
    case ENOSR:
        return POSIX_ENOSR;
    case ENOSTR:
        return POSIX_ENOSTR;
    case ENOTRECOVERABLE:
        return POSIX_ENOTRECOVERABLE;
#if !(defined(__linux__) || defined(__APPLE_CC__))
    case EOTHER:
        return POSIX_EOTHER;
#endif
    case EOWNERDEAD:
        return POSIX_EOWNERDEAD;
    case ETIME:
        return POSIX_ETIME;
    }
    UNREACHABLE_MSG("Something went horribly wrong");
}
