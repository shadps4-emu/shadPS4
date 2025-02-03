// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32

#include <windows.h>
#include "common/types.h"

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation = 1,
    FileFullDirectoryInformation = 2,
    FileBothDirectoryInformation = 3,
    FileBasicInformation = 4,
    FileStandardInformation = 5,
    FileInternalInformation = 6,
    FileEaInformation = 7,
    FileAccessInformation = 8,
    FileNameInformation = 9,
    FileRenameInformation = 10,
    FileLinkInformation = 11,
    FileNamesInformation = 12,
    FileDispositionInformation = 13,
    FilePositionInformation = 14,
    FileFullEaInformation = 15,
    FileModeInformation = 16,
    FileAlignmentInformation = 17,
    FileAllInformation = 18,
    FileAllocationInformation = 19,
    FileEndOfFileInformation = 20,
    FileAlternateNameInformation = 21,
    FileStreamInformation = 22,
    FilePipeInformation = 23,
    FilePipeLocalInformation = 24,
    FilePipeRemoteInformation = 25,
    FileMailslotQueryInformation = 26,
    FileMailslotSetInformation = 27,
    FileCompressionInformation = 28,
    FileObjectIdInformation = 29,
    FileCompletionInformation = 30,
    FileMoveClusterInformation = 31,
    FileQuotaInformation = 32,
    FileReparsePointInformation = 33,
    FileNetworkOpenInformation = 34,
    FileAttributeTagInformation = 35,
    FileTrackingInformation = 36,
    FileIdBothDirectoryInformation = 37,
    FileIdFullDirectoryInformation = 38,
    FileValidDataLengthInformation = 39,
    FileShortNameInformation = 40,
    FileIoCompletionNotificationInformation = 41,
    FileIoStatusBlockRangeInformation = 42,
    FileIoPriorityHintInformation = 43,
    FileSfioReserveInformation = 44,
    FileSfioVolumeInformation = 45,
    FileHardLinkInformation = 46,
    FileProcessIdsUsingFileInformation = 47,
    FileNormalizedNameInformation = 48,
    FileNetworkPhysicalNameInformation = 49,
    FileIdGlobalTxDirectoryInformation = 50,
    FileIsRemoteDeviceInformation = 51,
    FileUnusedInformation = 52,
    FileNumaNodeInformation = 53,
    FileStandardLinkInformation = 54,
    FileRemoteProtocolInformation = 55,
    FileRenameInformationBypassAccessCheck = 56,
    FileLinkInformationBypassAccessCheck = 57,
    FileVolumeNameInformation = 58,
    FileIdInformation = 59,
    FileIdExtdDirectoryInformation = 60,
    FileReplaceCompletionInformation = 61,
    FileHardLinkFullIdInformation = 62,
    FileIdExtdBothDirectoryInformation = 63,
    FileDispositionInformationEx = 64,
    FileRenameInformationEx = 65,
    FileRenameInformationExBypassAccessCheck = 66,
    FileDesiredStorageClassInformation = 67,
    FileStatInformation = 68,
    FileMemoryPartitionInformation = 69,
    FileStatLxInformation = 70,
    FileCaseSensitiveInformation = 71,
    FileLinkInformationEx = 72,
    FileLinkInformationExBypassAccessCheck = 73,
    FileStorageReserveIdInformation = 74,
    FileCaseSensitiveInformationForceAccessCheck = 75,
    FileKnownFolderInformation = 76,
    FileStatBasicInformation = 77,
    FileId64ExtdDirectoryInformation = 78,
    FileId64ExtdBothDirectoryInformation = 79,
    FileIdAllExtdDirectoryInformation = 80,
    FileIdAllExtdBothDirectoryInformation = 81,
    FileStreamReservationInformation,
    FileMupProviderInfo,
    FileMaximumInformation
} FILE_INFORMATION_CLASS,
    *PFILE_INFORMATION_CLASS;

typedef struct _IO_STATUS_BLOCK {
    union {
        u32 Status;
        PVOID Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _FILE_DISPOSITION_INFORMATION {
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PCUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;       // PSECURITY_DESCRIPTOR;
    PVOID SecurityQualityOfService; // PSECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef const OBJECT_ATTRIBUTES* PCOBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _INITIAL_TEB {
    struct {
        PVOID OldStackBase;
        PVOID OldStackLimit;
    } OldInitialTeb;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID StackAllocationBase;
} INITIAL_TEB, *PINITIAL_TEB;

typedef struct _PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID EntryInProgress;
    BOOLEAN ShutdownInProgress;
    HANDLE ShutdownThreadId;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _CURDIR {
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct RTL_DRIVE_LETTER_CURDIR {
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG AllocationSize;
    ULONG Size;
    ULONG Flags;
    ULONG DebugFlags;
    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PWSTR Environment;
    ULONG dwX;
    ULONG dwY;
    ULONG dwXSize;
    ULONG dwYSize;
    ULONG dwXCountChars;
    ULONG dwYCountChars;
    ULONG dwFillAttribute;
    ULONG dwFlags;
    ULONG wShowWindow;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING Desktop;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
    ULONG_PTR EnvironmentSize;
    ULONG_PTR EnvironmentVersion;
    PVOID PackageDependencyData;
    ULONG ProcessGroupId;
    ULONG LoaderThreads;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct tagRTL_BITMAP {
    ULONG SizeOfBitMap;
    PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct {
    UINT next;
    UINT id;
    ULONGLONG addr;
    ULONGLONG size;
    UINT args[4];
} CROSS_PROCESS_WORK_ENTRY;

typedef union {
    struct {
        UINT first;
        UINT counter;
    };
    volatile LONGLONG hdr;
} CROSS_PROCESS_WORK_HDR;

typedef struct {
    CROSS_PROCESS_WORK_HDR free_list;
    CROSS_PROCESS_WORK_HDR work_list;
    ULONGLONG unknown[4];
    CROSS_PROCESS_WORK_ENTRY entries[1];
} CROSS_PROCESS_WORK_LIST;

typedef struct _CHPEV2_PROCESS_INFO {
    ULONG Wow64ExecuteFlags;                       /* 000 */
    USHORT NativeMachineType;                      /* 004 */
    USHORT EmulatedMachineType;                    /* 006 */
    HANDLE SectionHandle;                          /* 008 */
    CROSS_PROCESS_WORK_LIST* CrossProcessWorkList; /* 010 */
    void* unknown;                                 /* 018 */
} CHPEV2_PROCESS_INFO, *PCHPEV2_PROCESS_INFO;

typedef u64(__stdcall* KERNEL_CALLBACK_PROC)(void*, ULONG);

typedef struct _PEB {                 /* win32/win64 */
    BOOLEAN InheritedAddressSpace;    /* 000/000 */
    BOOLEAN ReadImageFileExecOptions; /* 001/001 */
    BOOLEAN BeingDebugged;            /* 002/002 */
    UCHAR ImageUsedLargePages : 1;    /* 003/003 */
    UCHAR IsProtectedProcess : 1;
    UCHAR IsImageDynamicallyRelocated : 1;
    UCHAR SkipPatchingUser32Forwarders : 1;
    UCHAR IsPackagedProcess : 1;
    UCHAR IsAppContainer : 1;
    UCHAR IsProtectedProcessLight : 1;
    UCHAR IsLongPathAwareProcess : 1;
    HANDLE Mutant;                                  /* 004/008 */
    HMODULE ImageBaseAddress;                       /* 008/010 */
    PPEB_LDR_DATA LdrData;                          /* 00c/018 */
    RTL_USER_PROCESS_PARAMETERS* ProcessParameters; /* 010/020 */
    PVOID SubSystemData;                            /* 014/028 */
    HANDLE ProcessHeap;                             /* 018/030 */
    PRTL_CRITICAL_SECTION FastPebLock;              /* 01c/038 */
    PVOID AtlThunkSListPtr;                         /* 020/040 */
    PVOID IFEOKey;                                  /* 024/048 */
    ULONG ProcessInJob : 1;                         /* 028/050 */
    ULONG ProcessInitializing : 1;
    ULONG ProcessUsingVEH : 1;
    ULONG ProcessUsingVCH : 1;
    ULONG ProcessUsingFTH : 1;
    ULONG ProcessPreviouslyThrottled : 1;
    ULONG ProcessCurrentlyThrottled : 1;
    ULONG ProcessImagesHotPatched : 1;
    ULONG ReservedBits0 : 24;
    KERNEL_CALLBACK_PROC* KernelCallbackTable; /* 02c/058 */
    ULONG Reserved;                            /* 030/060 */
    ULONG AtlThunkSListPtr32;                  /* 034/064 */
    PVOID ApiSetMap;                           /* 038/068 */
    ULONG TlsExpansionCounter;                 /* 03c/070 */
    PRTL_BITMAP TlsBitmap;                     /* 040/078 */
    ULONG TlsBitmapBits[2];                    /* 044/080 */
    PVOID ReadOnlySharedMemoryBase;            /* 04c/088 */
    PVOID SharedData;                          /* 050/090 */
    PVOID* ReadOnlyStaticServerData;           /* 054/098 */
    PVOID AnsiCodePageData;                    /* 058/0a0 */
    PVOID OemCodePageData;                     /* 05c/0a8 */
    PVOID UnicodeCaseTableData;                /* 060/0b0 */
    ULONG NumberOfProcessors;                  /* 064/0b8 */
    ULONG NtGlobalFlag;                        /* 068/0bc */
    LARGE_INTEGER CriticalSectionTimeout;      /* 070/0c0 */
    SIZE_T HeapSegmentReserve;                 /* 078/0c8 */
    SIZE_T HeapSegmentCommit;                  /* 07c/0d0 */
    SIZE_T HeapDeCommitTotalFreeThreshold;     /* 080/0d8 */
    SIZE_T HeapDeCommitFreeBlockThreshold;     /* 084/0e0 */
    ULONG NumberOfHeaps;                       /* 088/0e8 */
    ULONG MaximumNumberOfHeaps;                /* 08c/0ec */
    PVOID* ProcessHeaps;                       /* 090/0f0 */
    PVOID GdiSharedHandleTable;                /* 094/0f8 */
    PVOID ProcessStarterHelper;                /* 098/100 */
    PVOID GdiDCAttributeList;                  /* 09c/108 */
    PVOID LoaderLock;                          /* 0a0/110 */
    ULONG OSMajorVersion;                      /* 0a4/118 */
    ULONG OSMinorVersion;                      /* 0a8/11c */
    ULONG OSBuildNumber;                       /* 0ac/120 */
    ULONG OSPlatformId;                        /* 0b0/124 */
    ULONG ImageSubSystem;                      /* 0b4/128 */
    ULONG ImageSubSystemMajorVersion;          /* 0b8/12c */
    ULONG ImageSubSystemMinorVersion;          /* 0bc/130 */
    KAFFINITY ActiveProcessAffinityMask;       /* 0c0/138 */
#ifdef _WIN64
    ULONG GdiHandleBuffer[60]; /*    /140 */
#else
    ULONG GdiHandleBuffer[34]; /* 0c4/    */
#endif
    PVOID PostProcessInitRoutine;      /* 14c/230 */
    PRTL_BITMAP TlsExpansionBitmap;    /* 150/238 */
    ULONG TlsExpansionBitmapBits[32];  /* 154/240 */
    ULONG SessionId;                   /* 1d4/2c0 */
    ULARGE_INTEGER AppCompatFlags;     /* 1d8/2c8 */
    ULARGE_INTEGER AppCompatFlagsUser; /* 1e0/2d0 */
    PVOID ShimData;                    /* 1e8/2d8 */
    PVOID AppCompatInfo;               /* 1ec/2e0 */
    UNICODE_STRING CSDVersion;         /* 1f0/2e8 */
    PVOID ActivationContextData;       /* 1f8/2f8 */
    PVOID ProcessAssemblyStorageMap;   /* 1fc/300 */
    PVOID SystemDefaultActivationData; /* 200/308 */
    PVOID SystemAssemblyStorageMap;    /* 204/310 */
    SIZE_T MinimumStackCommit;         /* 208/318 */
    PVOID* FlsCallback;                /* 20c/320 */
    LIST_ENTRY FlsListHead;            /* 210/328 */
    union {
        PRTL_BITMAP FlsBitmap; /* 218/338 */
#ifdef _WIN64
        CHPEV2_PROCESS_INFO* ChpeV2ProcessInfo; /*    /338 */
#endif
    };
    ULONG FlsBitmapBits[4];       /* 21c/340 */
    ULONG FlsHighIndex;           /* 22c/350 */
    PVOID WerRegistrationData;    /* 230/358 */
    PVOID WerShipAssertPtr;       /* 234/360 */
    PVOID EcCodeBitMap;           /* 238/368 */
    PVOID pImageHeaderHash;       /* 23c/370 */
    ULONG HeapTracingEnabled : 1; /* 240/378 */
    ULONG CritSecTracingEnabled : 1;
    ULONG LibLoaderTracingEnabled : 1;
    ULONG SpareTracingBits : 29;
    ULONGLONG CsrServerReadOnlySharedMemoryBase;  /* 248/380 */
    ULONG TppWorkerpListLock;                     /* 250/388 */
    LIST_ENTRY TppWorkerpList;                    /* 254/390 */
    PVOID WaitOnAddressHashTable[0x80];           /* 25c/3a0 */
    PVOID TelemetryCoverageHeader;                /* 45c/7a0 */
    ULONG CloudFileFlags;                         /* 460/7a8 */
    ULONG CloudFileDiagFlags;                     /* 464/7ac */
    CHAR PlaceholderCompatibilityMode;            /* 468/7b0 */
    CHAR PlaceholderCompatibilityModeReserved[7]; /* 469/7b1 */
    PVOID LeapSecondData;                         /* 470/7b8 */
    ULONG LeapSecondFlags;                        /* 474/7c0 */
    ULONG NtGlobalFlag2;                          /* 478/7c4 */
} PEB, *PPEB;

typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME {
    struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME* Previous;
    struct _ACTIVATION_CONTEXT* ActivationContext;
    ULONG Flags;
} RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

typedef struct _ACTIVATION_CONTEXT_STACK {
    RTL_ACTIVATION_CONTEXT_STACK_FRAME* ActiveFrame;
    LIST_ENTRY FrameListCache;
    ULONG Flags;
    ULONG NextCookieSequenceNumber;
    ULONG_PTR StackId;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

typedef struct _GDI_TEB_BATCH {
    ULONG Offset;
    HANDLE HDC;
    ULONG Buffer[0x136];
} GDI_TEB_BATCH;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT {
    ULONG Flags;
    const char* FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME {
    ULONG Flags;
    struct _TEB_ACTIVE_FRAME* Previous;
    TEB_ACTIVE_FRAME_CONTEXT* Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

typedef struct _TEB {                             /* win32/win64 */
    NT_TIB Tib;                                   /* 000/0000 */
    PVOID EnvironmentPointer;                     /* 01c/0038 */
    CLIENT_ID ClientId;                           /* 020/0040 */
    PVOID ActiveRpcHandle;                        /* 028/0050 */
    PVOID ThreadLocalStoragePointer;              /* 02c/0058 */
    PPEB Peb;                                     /* 030/0060 */
    ULONG LastErrorValue;                         /* 034/0068 */
    ULONG CountOfOwnedCriticalSections;           /* 038/006c */
    PVOID CsrClientThread;                        /* 03c/0070 */
    PVOID Win32ThreadInfo;                        /* 040/0078 */
    ULONG User32Reserved[26];                     /* 044/0080 */
    ULONG UserReserved[5];                        /* 0ac/00e8 */
    PVOID WOW32Reserved;                          /* 0c0/0100 */
    ULONG CurrentLocale;                          /* 0c4/0108 */
    ULONG FpSoftwareStatusRegister;               /* 0c8/010c */
    PVOID ReservedForDebuggerInstrumentation[16]; /* 0cc/0110 */
#ifdef _WIN64
    PVOID SystemReserved1[30]; /*    /0190 */
#else
    PVOID SystemReserved1[26]; /* 10c/     */
#endif
    char PlaceholderCompatibilityMode;                       /* 174/0280 */
    BOOLEAN PlaceholderHydrationAlwaysExplicit;              /* 175/0281 */
    char PlaceholderReserved[10];                            /* 176/0282 */
    DWORD ProxiedProcessId;                                  /* 180/028c */
    ACTIVATION_CONTEXT_STACK ActivationContextStack;         /* 184/0290 */
    UCHAR WorkingOnBehalfOfTicket[8];                        /* 19c/02b8 */
    LONG ExceptionCode;                                      /* 1a4/02c0 */
    ACTIVATION_CONTEXT_STACK* ActivationContextStackPointer; /* 1a8/02c8 */
    ULONG_PTR InstrumentationCallbackSp;                     /* 1ac/02d0 */
    ULONG_PTR InstrumentationCallbackPreviousPc;             /* 1b0/02d8 */
    ULONG_PTR InstrumentationCallbackPreviousSp;             /* 1b4/02e0 */
#ifdef _WIN64
    ULONG TxFsContext;                       /*    /02e8 */
    BOOLEAN InstrumentationCallbackDisabled; /*    /02ec */
    BOOLEAN UnalignedLoadStoreExceptions;    /*    /02ed */
#else
    BOOLEAN InstrumentationCallbackDisabled; /* 1b8/     */
    BYTE SpareBytes1[23];                    /* 1b9/     */
    ULONG TxFsContext;                       /* 1d0/     */
#endif
    GDI_TEB_BATCH GdiTebBatch;          /* 1d4/02f0 */
    CLIENT_ID RealClientId;             /* 6b4/07d8 */
    HANDLE GdiCachedProcessHandle;      /* 6bc/07e8 */
    ULONG GdiClientPID;                 /* 6c0/07f0 */
    ULONG GdiClientTID;                 /* 6c4/07f4 */
    PVOID GdiThreadLocaleInfo;          /* 6c8/07f8 */
    ULONG_PTR Win32ClientInfo[62];      /* 6cc/0800 */
    PVOID glDispatchTable[233];         /* 7c4/09f0 */
    PVOID glReserved1[29];              /* b68/1138 */
    PVOID glReserved2;                  /* bdc/1220 */
    PVOID glSectionInfo;                /* be0/1228 */
    PVOID glSection;                    /* be4/1230 */
    PVOID glTable;                      /* be8/1238 */
    PVOID glCurrentRC;                  /* bec/1240 */
    PVOID glContext;                    /* bf0/1248 */
    ULONG LastStatusValue;              /* bf4/1250 */
    UNICODE_STRING StaticUnicodeString; /* bf8/1258 */
    WCHAR StaticUnicodeBuffer[261];     /* c00/1268 */
    PVOID DeallocationStack;            /* e0c/1478 */
    PVOID TlsSlots[64];                 /* e10/1480 */
    LIST_ENTRY TlsLinks;                /* f10/1680 */
    PVOID Vdm;                          /* f18/1690 */
    PVOID ReservedForNtRpc;             /* f1c/1698 */
    PVOID DbgSsReserved[2];             /* f20/16a0 */
    ULONG HardErrorMode;                /* f28/16b0 */
#ifdef _WIN64
    PVOID Instrumentation[11]; /*    /16b8 */
#else
    PVOID Instrumentation[9]; /* f2c/ */
#endif
    GUID ActivityId;                   /* f50/1710 */
    PVOID SubProcessTag;               /* f60/1720 */
    PVOID PerflibData;                 /* f64/1728 */
    PVOID EtwTraceData;                /* f68/1730 */
    PVOID WinSockData;                 /* f6c/1738 */
    ULONG GdiBatchCount;               /* f70/1740 */
    ULONG IdealProcessorValue;         /* f74/1744 */
    ULONG GuaranteedStackBytes;        /* f78/1748 */
    PVOID ReservedForPerf;             /* f7c/1750 */
    PVOID ReservedForOle;              /* f80/1758 */
    ULONG WaitingOnLoaderLock;         /* f84/1760 */
    PVOID SavedPriorityState;          /* f88/1768 */
    ULONG_PTR ReservedForCodeCoverage; /* f8c/1770 */
    PVOID ThreadPoolData;              /* f90/1778 */
    PVOID* TlsExpansionSlots;          /* f94/1780 */
#ifdef _WIN64
    union {
        PVOID DeallocationBStore; /*    /1788 */
        PVOID* ChpeV2CpuAreaInfo; /*    /1788 */
    } DUMMYUNIONNAME;
    PVOID BStoreLimit; /*    /1790 */
#endif
    ULONG MuiGeneration;            /* f98/1798 */
    ULONG IsImpersonating;          /* f9c/179c */
    PVOID NlsCache;                 /* fa0/17a0 */
    PVOID ShimData;                 /* fa4/17a8 */
    ULONG HeapVirtualAffinity;      /* fa8/17b0 */
    PVOID CurrentTransactionHandle; /* fac/17b8 */
    TEB_ACTIVE_FRAME* ActiveFrame;  /* fb0/17c0 */
    PVOID* FlsSlots;                /* fb4/17c8 */
    PVOID PreferredLanguages;       /* fb8/17d0 */
    PVOID UserPrefLanguages;        /* fbc/17d8 */
    PVOID MergedPrefLanguages;      /* fc0/17e0 */
    ULONG MuiImpersonation;         /* fc4/17e8 */
    USHORT CrossTebFlags;           /* fc8/17ec */
    USHORT SameTebFlags;            /* fca/17ee */
    PVOID TxnScopeEnterCallback;    /* fcc/17f0 */
    PVOID TxnScopeExitCallback;     /* fd0/17f8 */
    PVOID TxnScopeContext;          /* fd4/1800 */
    ULONG LockCount;                /* fd8/1808 */
    LONG WowTebOffset;              /* fdc/180c */
    PVOID ResourceRetValue;         /* fe0/1810 */
    PVOID ReservedForWdf;           /* fe4/1818 */
    ULONGLONG ReservedForCrt;       /* fe8/1820 */
    GUID EffectiveContainerId;      /* ff0/1828 */
} TEB, *PTEB;
static_assert(offsetof(TEB, DeallocationStack) ==
              0x1478); /* The only member we care about at the moment */

typedef enum _QUEUE_USER_APC_FLAGS {
    QueueUserApcFlagsNone,
    QueueUserApcFlagsSpecialUserApc,
    QueueUserApcFlagsMaxValue
} QUEUE_USER_APC_FLAGS;

typedef union _USER_APC_OPTION {
    ULONG_PTR UserApcFlags;
    HANDLE MemoryReserveHandle;
} USER_APC_OPTION, *PUSER_APC_OPTION;

using PPS_APC_ROUTINE = void (*)(PVOID ApcArgument1, PVOID ApcArgument2, PVOID ApcArgument3,
                                 PCONTEXT Context);

typedef u64(__stdcall* NtClose_t)(HANDLE Handle);

typedef u64(__stdcall* NtSetInformationFile_t)(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
                                               PVOID FileInformation, ULONG Length,
                                               FILE_INFORMATION_CLASS FileInformationClass);

typedef u64(__stdcall* NtCreateThread_t)(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess,
                                         PCOBJECT_ATTRIBUTES ObjectAttributes, HANDLE ProcessHandle,
                                         PCLIENT_ID ClientId, PCONTEXT ThreadContext,
                                         PINITIAL_TEB InitialTeb, BOOLEAN CreateSuspended);

typedef u64(__stdcall* NtTerminateThread_t)(HANDLE ThreadHandle, u64 ExitStatus);

typedef u64(__stdcall* NtQueueApcThreadEx_t)(HANDLE ThreadHandle,
                                             USER_APC_OPTION UserApcReserveHandle,
                                             PPS_APC_ROUTINE ApcRoutine, PVOID ApcArgument1,
                                             PVOID ApcArgument2, PVOID ApcArgument3);

extern NtClose_t NtClose;
extern NtSetInformationFile_t NtSetInformationFile;
extern NtCreateThread_t NtCreateThread;
extern NtTerminateThread_t NtTerminateThread;
extern NtQueueApcThreadEx_t NtQueueApcThreadEx;

namespace Common::NtApi {
void Initialize();
}

#endif
