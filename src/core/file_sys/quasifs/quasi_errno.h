// INAA License @marecl 2025

#pragma once

#include <cstdint>

#define QUASI_EPERM 1    /* Operation not permitted */
#define QUASI_ENOENT 2   /* No such file or directory */
#define QUASI_ESRCH 3    /* No such process */
#define QUASI_EINTR 4    /* Interrupted system call */
#define QUASI_EIO 5      /* I/O error */
#define QUASI_ENXIO 6    /* No such device or address */
#define QUASI_E2BIG 7    /* Argument list too long */
#define QUASI_ENOEXEC 8  /* Exec format error */
#define QUASI_EBADF 9    /* Bad file number */
#define QUASI_ECHILD 10  /* No child processes */
#define QUASI_EAGAIN 11  /* Try again */
#define QUASI_ENOMEM 12  /* Out of memory */
#define QUASI_EACCES 13  /* Permission denied */
#define QUASI_EFAULT 14  /* Bad address */
#define QUASI_ENOTBLK 15 /* Block device required */
#define QUASI_EBUSY 16   /* Device or resource busy */
#define QUASI_EEXIST 17  /* File exists */
#define QUASI_EXDEV 18   /* Cross-device link */
#define QUASI_ENODEV 19  /* No such device */
#define QUASI_ENOTDIR 20 /* Not a directory */
#define QUASI_EISDIR 21  /* Is a directory */
#define QUASI_EINVAL 22  /* Invalid argument */
#define QUASI_ENFILE 23  /* File table overflow */
#define QUASI_EMFILE 24  /* Too many open files */
#define QUASI_ENOTTY 25  /* Not a typewriter */
#define QUASI_ETXTBSY 26 /* Text file busy */
#define QUASI_EFBIG 27   /* File too large */
#define QUASI_ENOSPC 28  /* No space left on device */
#define QUASI_ESPIPE 29  /* Illegal seek */
#define QUASI_EROFS 30   /* Read-only file system */
#define QUASI_EMLINK 31  /* Too many links */
#define QUASI_EPIPE 32   /* Broken pipe */
#define QUASI_EDOM 33    /* Math argument out of domain of func */
#define QUASI_ERANGE 34  /* Math result not representable */

#define QUASI_EDEADLK 35      /* Resource deadlock would occur */
#define QUASI_ENAMETOOLONG 36 /* File name too long */
#define QUASI_ENOLCK 37       /* No record locks available */

/*
 * This error code is special: arch syscall entry code will return
 * -ENOSYS if users try to call a syscall that doesn't exist.  To keep
 * failures of syscalls that really do exist distinguishable from
 * failures due to attempts to use a nonexistent syscall, syscall
 * implementations should refrain from returning -ENOSYS.
 */
#define QUASI_ENOSYS 38 /* Invalid system call number */

#define QUASI_ENOTEMPTY 39       /* Directory not empty */
#define QUASI_ELOOP 40           /* Too many symbolic links encountered */
#define QUASI_EWOULDBLOCK EAGAIN /* Operation would block */
#define QUASI_ENOMSG 42          /* No message of desired type */
#define QUASI_EIDRM 43           /* Identifier removed */
#define QUASI_ECHRNG 44          /* Channel number out of range */
#define QUASI_EL2NSYNC 45        /* Level 2 not synchronized */
#define QUASI_EL3HLT 46          /* Level 3 halted */
#define QUASI_EL3RST 47          /* Level 3 reset */
#define QUASI_ELNRNG 48          /* Link number out of range */
#define QUASI_EUNATCH 49         /* Protocol driver not attached */
#define QUASI_ENOCSI 50          /* No CSI structure available */
#define QUASI_EL2HLT 51          /* Level 2 halted */
#define QUASI_EBADE 52           /* Invalid exchange */
#define QUASI_EBADR 53           /* Invalid request descriptor */
#define QUASI_EXFULL 54          /* Exchange full */
#define QUASI_ENOANO 55          /* No anode */
#define QUASI_EBADRQC 56         /* Invalid request code */
#define QUASI_EBADSLT 57         /* Invalid slot */

#define QUASI_EDEADLOCK QUASI_EDEADLK

#define QUASI_EBFONT 59          /* Bad font file format */
#define QUASI_ENOSTR 60          /* Device not a stream */
#define QUASI_ENODATA 61         /* No data available */
#define QUASI_ETIME 62           /* Timer expired */
#define QUASI_ENOSR 63           /* Out of streams resources */
#define QUASI_ENONET 64          /* Machine is not on the network */
#define QUASI_ENOPKG 65          /* Package not installed */
#define QUASI_EREMOTE 66         /* Object is remote */
#define QUASI_ENOLINK 67         /* Link has been severed */
#define QUASI_EADV 68            /* Advertise error */
#define QUASI_ESRMNT 69          /* Srmount error */
#define QUASI_ECOMM 70           /* Communication error on send */
#define QUASI_EPROTO 71          /* Protocol error */
#define QUASI_EMULTIHOP 72       /* Multihop attempted */
#define QUASI_EDOTDOT 73         /* RFS specific error */
#define QUASI_EBADMSG 74         /* Not a data message */
#define QUASI_EOVERFLOW 75       /* Value too large for defined data type */
#define QUASI_ENOTUNIQ 76        /* Name not unique on network */
#define QUASI_EBADFD 77          /* File descriptor in bad state */
#define QUASI_EREMCHG 78         /* Remote address changed */
#define QUASI_ELIBACC 79         /* Can not access a needed shared library */
#define QUASI_ELIBBAD 80         /* Accessing a corrupted shared library */
#define QUASI_ELIBSCN 81         /* .lib section in a.out corrupted */
#define QUASI_ELIBMAX 82         /* Attempting to link in too many shared libraries */
#define QUASI_ELIBEXEC 83        /* Cannot exec a shared library directly */
#define QUASI_EILSEQ 84          /* Illegal byte sequence */
#define QUASI_ERESTART 85        /* Interrupted system call should be restarted */
#define QUASI_ESTRPIPE 86        /* Streams pipe error */
#define QUASI_EUSERS 87          /* Too many users */
#define QUASI_ENOTSOCK 88        /* Socket operation on non-socket */
#define QUASI_EDESTADDRREQ 89    /* Destination address required */
#define QUASI_EMSGSIZE 90        /* Message too long */
#define QUASI_EPROTOTYPE 91      /* Protocol wrong type for socket */
#define QUASI_ENOPROTOOPT 92     /* Protocol not available */
#define QUASI_EPROTONOSUPPORT 93 /* Protocol not supported */
#define QUASI_ESOCKTNOSUPPORT 94 /* Socket type not supported */
#define QUASI_EOPNOTSUPP 95      /* Operation not supported on transport endpoint */
#define QUASI_EPFNOSUPPORT 96    /* Protocol family not supported */
#define QUASI_EAFNOSUPPORT 97    /* Address family not supported by protocol */
#define QUASI_EADDRINUSE 98      /* Address already in use */
#define QUASI_EADDRNOTAVAIL 99   /* Cannot assign requested address */
#define QUASI_ENETDOWN 100       /* Network is down */
#define QUASI_ENETUNREACH 101    /* Network is unreachable */
#define QUASI_ENETRESET 102      /* Network dropped connection because of reset */
#define QUASI_ECONNABORTED 103   /* Software caused connection abort */
#define QUASI_ECONNRESET 104     /* Connection reset by peer */
#define QUASI_ENOBUFS 105        /* No buffer space available */
#define QUASI_EISCONN 106        /* Transport endpoint is already connected */
#define QUASI_ENOTCONN 107       /* Transport endpoint is not connected */
#define QUASI_ESHUTDOWN 108      /* Cannot send after transport endpoint shutdown */
#define QUASI_ETOOMANYREFS 109   /* Too many references: cannot splice */
#define QUASI_ETIMEDOUT 110      /* Connection timed out */
#define QUASI_ECONNREFUSED 111   /* Connection refused */
#define QUASI_EHOSTDOWN 112      /* Host is down */
#define QUASI_EHOSTUNREACH 113   /* No route to host */
#define QUASI_EALREADY 114       /* Operation already in progress */
#define QUASI_EINPROGRESS 115    /* Operation now in progress */
#define QUASI_ESTALE 116         /* Stale file handle */
#define QUASI_EUCLEAN 117        /* Structure needs cleaning */
#define QUASI_ENOTNAM 118        /* Not a XENIX named type file */
#define QUASI_ENAVAIL 119        /* No XENIX semaphores available */
#define QUASI_EISNAM 120         /* Is a named type file */
#define QUASI_EREMOTEIO 121      /* Remote I/O error */
#define QUASI_EDQUOT 122         /* Quota exceeded */

#define QUASI_ENOMEDIUM 123    /* No medium found */
#define QUASI_EMEDIUMTYPE 124  /* Wrong medium type */
#define QUASI_ECANCELED 125    /* Operation Canceled */
#define QUASI_ENOKEY 126       /* Required key not available */
#define QUASI_EKEYEXPIRED 127  /* Key has expired */
#define QUASI_EKEYREVOKED 128  /* Key has been revoked */
#define QUASI_EKEYREJECTED 129 /* Key was rejected by service */

/* for robust mutexes */
#define QUASI_EOWNERDEAD 130      /* Owner died */
#define QUASI_ENOTRECOVERABLE 131 /* State not recoverable */

#define QUASI_ERFKILL 132 /* Operation not possible due to RF-kill */

#define QUASI_EHWPOISON 133 /* Memory page has hardware error */