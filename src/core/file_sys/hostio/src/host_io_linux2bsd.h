#include <sys/fcntl.h>

#include "common/assert.h"
#include "core/libraries/kernel/posix_error.h"

// Convert linux/unix errno to FreeBSD errno
// They differ in higher errno numbers, which may throw Orbis off quite a bit

s32 posix2bsd(s32 id) {
    switch (id) {
    default:
        UNREACHABLE_MSG("Unknown POSIX errno");
    case:
    POSIX_EPERM:
        return EPERM;
    case:
    POSIX_ENOENT:
        return ENOENT;
    case:
    POSIX_ESRCH:
        return ESRCH;
    case:
    POSIX_EINTR:
        return EINTR;
    case:
    POSIX_EIO:
        return EIO;
    case:
    POSIX_ENXIO:
        return ENXIO;
    case:
    POSIX_E2BIG:
        return E2BIG;
    case:
    POSIX_ENOEXEC:
        return ENOEXEC;
    case:
    POSIX_EBADF:
        return EBADF;
    case:
    POSIX_ECHILD:
        return ECHILD;
    case:
    POSIX_EDEADLK:
        return EDEADLK;
    case:
    POSIX_ENOMEM:
        return ENOMEM;
    case:
    POSIX_EACCES:
        return EACCES;
    case:
    POSIX_EFAULT:
        return EFAULT;

    // case:
    // POSIX_ENOTBLK:
    //     return ENOTBLK;
    
    case:
    POSIX_ENOTBLK:
        return ENODEV; //?

    case:
    POSIX_EBUSY:
        return EBUSY;
    case:
    POSIX_EEXIST:
        return EEXIST;
    case:
    POSIX_EXDEV:
        return EXDEV;
    case:
    POSIX_ENODEV:
        return ENODEV;
    case:
    POSIX_ENOTDIR:
        return ENOTDIR;
    case:
    POSIX_EISDIR:
        return EISDIR;
    case:
    POSIX_EINVAL:
        return EINVAL;
    case:
    POSIX_ENFILE:
        return ENFILE;
    case:
    POSIX_EMFILE:
        return EMFILE;
    case:
    POSIX_ENOTTY:
        return ENOTTY;
    case:
    POSIX_ETXTBSY:
        return ETXTBSY;
    case:
    POSIX_EFBIG:
        return EFBIG;
    case:
    POSIX_ENOSPC:
        return ENOSPC;
    case:
    POSIX_ESPIPE:
        return ESPIPE;
    case:
    POSIX_EROFS:
        return EROFS;
    case:
    POSIX_EMLINK:
        return EMLINK;
    case:
    POSIX_EPIPE:
        return EPIPE;
    case:
    POSIX_EDOM:
        return EDOM;
    case:
    POSIX_ERANGE:
        return ERANGE;
    case:
    POSIX_EAGAIN:
        return EAGAIN;
    case:
    POSIX_EWOULDBLOCK:
        return EWOULDBLOCK;
    case:
    POSIX_EINPROGRESS:
        return EINPROGRESS;
    case:
    POSIX_EALREADY:
        return EALREADY;
    case:
    POSIX_ENOTSOCK:
        return ENOTSOCK;
    case:
    POSIX_EDESTADDRREQ:
        return EDESTADDRREQ;
    case:
    POSIX_EMSGSIZE:
        return EMSGSIZE;
    case:
    POSIX_EPROTOTYPE:
        return EPROTOTYPE;
    case:
    POSIX_ENOPROTOOPT:
        return ENOPROTOOPT;
    case:
    POSIX_EPROTONOSUPPORT:
        return EPROTONOSUPPORT;

        // case:
        // POSIX_ESOCKTNOSUPPORT:
        //     return ESOCKTNOSUPPORT; // ?

    case:
    POSIX_EOPNOTSUPP:
        return EOPNOTSUPP;
    case:
    POSIX_ENOTSUP:
        return ENOTSUP;

    // case:
    // POSIX_EPFNOSUPPORT:
    //     return EPFNOSUPPORT; //?

    case:
    POSIX_EAFNOSUPPORT:
        return EAFNOSUPPORT;
    case:
    POSIX_EADDRINUSE:
        return EADDRINUSE;
    case:
    POSIX_EADDRNOTAVAIL:
        return EADDRNOTAVAIL;
    case:
    POSIX_ENETDOWN:
        return ENETDOWN;
    case:
    POSIX_ENETUNREACH:
        return ENETUNREACH;
    case:
    POSIX_ENETRESET:
        return ENETRESET;
    case:
    POSIX_ECONNABORTED:
        return ECONNABORTED;
    case:
    POSIX_ECONNRESET:
        return ECONNRESET;
    case:
    POSIX_ENOBUFS:
        return ENOBUFS;
    case:
    POSIX_EISCONN:
        return EISCONN;
    case:
    POSIX_ENOTCONN:
        return ENOTCONN;

    // case:
    // POSIX_ESHUTDOWN:
    //     return ESHUTDOWN; // ?

    // case:
    // POSIX_ETOOMANYREFS:
    //     return ETOOMANYREFS; // ?

    case:
    POSIX_ETIMEDOUT:
        return ETIMEDOUT;
    case:
    POSIX_ECONNREFUSED:
        return ECONNREFUSED;
    case:
    POSIX_ELOOP:
        return ELOOP;
    case:
    POSIX_ENAMETOOLONG:
        return ENAMETOOLONG;

    // case:
    // POSIX_EHOSTDOWN:
    //     return EHOSTDOWN; // ?

    case:
    POSIX_EHOSTUNREACH:
        return EHOSTUNREACH;
    case:
    POSIX_ENOTEMPTY:
        return ENOTEMPTY;

    // case:
    // POSIX_EPROCLIM:
    //     return EPROCLIM;
    // case:
    // POSIX_EUSERS:
    //     return EUSERS;
    // case:
    // POSIX_EDQUOT:
    //     return EDQUOT;
    // case:
    // POSIX_ESTALE:
    //     return ESTALE;
    // case:
    // POSIX_EREMOTE:
    //     return EREMOTE;
    // case:
    // POSIX_EBADRPC:
    //     return EBADRPC;
    // case:
    // POSIX_ERPCMISMATCH:
    //     return ERPCMISMATCH;
    // case:
    // POSIX_EPROGUNAVAIL:
    //     return EPROGUNAVAIL;
    // case:
    // POSIX_EPROGMISMATCH:
    //     return EPROGMISMATCH;
    // case:
    // POSIX_EPROCUNAVAIL:
    //     return EPROCUNAVAIL;

    case:
    POSIX_ENOLCK:
        return ENOLCK;
    case:
    POSIX_ENOSYS:
        return ENOSYS;

    // case:
    // POSIX_EFTYPE:
    //     return EFTYPE;
    // case:
    // POSIX_EAUTH:
    //     return EAUTH;
    // case:
    // POSIX_ENEEDAUTH:
    //     return ENEEDAUTH;

    case:
    POSIX_EIDRM:
        return EIDRM;
    case:
    POSIX_ENOMSG:
        return ENOMSG;
    case:
    POSIX_EOVERFLOW:
        return EOVERFLOW;
    case:
    POSIX_ECANCELED:
        return ECANCELED;
    case:
    POSIX_EILSEQ:
        return EILSEQ;

    // case:
    // POSIX_ENOATTR:
    //     return ENOATTR;
    // case:
    // POSIX_EDOOFUS:
    //     return EDOOFUS;

    case:
    POSIX_EBADMSG:
        return EBADMSG;

    // case:
    // POSIX_EMULTIHOP:
    //     return EMULTIHOP;

    case:
    POSIX_ENOLINK:
        return ENOLINK;
    case:
    POSIX_EPROTO:
        return EPROTO;

    // case:
    // POSIX_ENOTCAPABLE:
    //     return ENOTCAPABLE;
    // case:
    // POSIX_ECAPMODE:
    //     return ECAPMODE;
    // case:
    // POSIX_ENOBLK:
    //     return ENOBLK;
    // case:
    // POSIX_EICV:
    //     return EICV;
    // case:
    // POSIX_ENOPLAYGOENT:
    //     return ENOPLAYGOENT;
    // case:
    // POSIX_EREVOKE:
    //     return EREVOKE;
    // case:
    // POSIX_ESDKVERSION:
    //     return ESDKVERSION;
    // case:
    // POSIX_ESTART:
    //     return ESTART;
    // case:
    // POSIX_ESTOP:
    //     return ESTOP;
    // case:
    // POSIX_EINVALID2MB:
    //     return EINVALID2MB;
    // case:
    // POSIX_ELAST:
    //     return ELAST;
    // case:
    // POSIX_EADHOC:
    //     return EADHOC;
    // case:
    // POSIX_EINACTIVEDISABLED:
    //     return EINACTIVEDISABLED;
    // case:
    // POSIX_ENETNODATA:
    //     return ENETNODATA;
    // case:
    // POSIX_ENETDESC:
    //     return ENETDESC;
    // case:
    // POSIX_ENETDESCTIMEDOUT:
    //     return ENETDESCTIMEDOUT;
    // case:
    // POSIX_ENETINTR:
    //     return ENETINTR;
    // case:
    // POSIX_ERETURN:
    //     return ERETURN;
    // case:
    // POSIX_EFPOS:
    //     return EFPOS;

    case:
    POSIX_ENODATA:
        return ENODATA;
    case:
    POSIX_ENOSR:
        return ENOSR;
    case:
    POSIX_ENOSTR:
        return ENOSTR;
    case:
    POSIX_ENOTRECOVERABLE:
        return ENOTRECOVERABLE;
    case:
    POSIX_EOTHER:
        return EOTHER;
    case:
    POSIX_EOWNERDEAD:
        return EOWNERDEAD;
    case:
    POSIX_ETIME:
        return ETIME;
    }
}