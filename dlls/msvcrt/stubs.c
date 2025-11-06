/*
 * Minimal stubs for features not required by the emulator.
 * Implements DllMain, signal/exception placeholders, and small NTDLL helpers.
 */

#include <stddef.h>

#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef NTAPI
#define NTAPI __stdcall
#endif
#ifndef CDECL
#define CDECL __cdecl
#endif

typedef void *LPVOID;
typedef void *HINSTANCE;
typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned long ULONG;
typedef long LONG;
typedef unsigned long ULONG_PTR;
typedef unsigned char BOOLEAN;
typedef long NTSTATUS;
typedef void *HANDLE;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct
{
    unsigned long LowPart;
    LONG HighPart;
} LARGE_INTEGER;

typedef struct
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

typedef struct
{
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD dwVolumeSerialNumber;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD nNumberOfLinks;
    DWORD nFileIndexHigh;
    DWORD nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION;

typedef struct _IO_STATUS_BLOCK
{
    union
    {
        NTSTATUS Status;
        LPVOID Pointer;
    } u;
    ULONG_PTR Information;
} IO_STATUS_BLOCK;

typedef enum
{
    FileDirectoryInformation        = 1,
    FileBasicInformation            = 4,
    FileStandardInformation         = 5
} FILE_INFORMATION_CLASS;

typedef struct
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION;

typedef struct
{
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION;

typedef struct _RTL_BITMAP
{
    ULONG SizeOfBitMap;
    ULONG *Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

#define STATUS_SUCCESS             ((NTSTATUS)0x00000000L)
#define STATUS_INVALID_PARAMETER   ((NTSTATUS)0xC000000DL)
#define STATUS_BUFFER_TOO_SMALL    ((NTSTATUS)0xC0000023L)
#define STATUS_NOT_IMPLEMENTED     ((NTSTATUS)0xC0000002L)

static inline unsigned long long filetime_to_ticks(const FILETIME *ft)
{
    return ((unsigned long long)ft->dwHighDateTime << 32) | (unsigned long long)ft->dwLowDateTime;
}

static inline LARGE_INTEGER filetime_to_large(const FILETIME *ft)
{
    LARGE_INTEGER li;
    li.LowPart = ft->dwLowDateTime;
    li.HighPart = (LONG)ft->dwHighDateTime;
    return li;
}

/* kernel32 functions we call */
void WINAPI GetSystemTimeAsFileTime(FILETIME *time);
BOOL WINAPI GetFileInformationByHandle(HANDLE handle, void *info);

/* Exception initialisation stubs */
void msvcrt_init_exception(void *module)
{
    (void)module;
}

void msvcrt_init_signals(void)
{
}

void msvcrt_free_signals(void)
{
}

int CDECL raise(int sig)
{
    (void)sig;
    return 0;
}

int CDECL fegetround(void)
{
    return 0;
}

int CDECL fetestexcept(int excepts)
{
    (void)excepts;
    return 0;
}

int CDECL _dsign(double value)
{
    union
    {
        double f;
        unsigned long long i;
    } u = { value };
    return (int)(u.i >> 63);
}

int CDECL _fdsign(float value)
{
    union
    {
        float f;
        unsigned int i;
    } u = { value };
    return (int)(u.i >> 31);
}

extern BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved);

BOOL WINAPI _DllMainCRTStartup(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return DllMain(instance, reason, reserved);
}

void NTAPI RtlInitializeBitMap(PRTL_BITMAP bitmap, ULONG *buffer, ULONG size)
{
    bitmap->SizeOfBitMap = size;
    bitmap->Buffer = buffer;
}

void NTAPI RtlSetBits(PRTL_BITMAP bitmap, ULONG start, ULONG count)
{
    ULONG i;
    for (i = 0; i < count; ++i)
    {
        ULONG bit = start + i;
        bitmap->Buffer[bit / (sizeof(*bitmap->Buffer) * 8)] |= 1u << (bit % (sizeof(*bitmap->Buffer) * 8));
    }
}

BOOLEAN NTAPI RtlAreBitsSet(const RTL_BITMAP *bitmap, ULONG start, ULONG count)
{
    ULONG i;
    for (i = 0; i < count; ++i)
    {
        ULONG bit = start + i;
        if (!(bitmap->Buffer[bit / (sizeof(*bitmap->Buffer) * 8)] & (1u << (bit % (sizeof(*bitmap->Buffer) * 8)))))
            return FALSE;
    }
    return TRUE;
}

BOOLEAN NTAPI RtlAreBitsClear(const RTL_BITMAP *bitmap, ULONG start, ULONG count)
{
    ULONG i;
    for (i = 0; i < count; ++i)
    {
        ULONG bit = start + i;
        if (bitmap->Buffer[bit / (sizeof(*bitmap->Buffer) * 8)] & (1u << (bit % (sizeof(*bitmap->Buffer) * 8))))
            return FALSE;
    }
    return TRUE;
}

NTSTATUS NTAPI NtQuerySystemTime(LARGE_INTEGER *time)
{
    GetSystemTimeAsFileTime((FILETIME *)time);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlTimeToSecondsSince1970(const LARGE_INTEGER *time, ULONG *result)
{
    const unsigned long long epoch_difference = 11644473600ULL; /* seconds */
    unsigned long long ticks = ((unsigned long long)(unsigned long)time->LowPart) |
                               ((unsigned long long)(unsigned long)time->HighPart << 32);

    if (!result || ticks < epoch_difference * 10000000ULL)
        return STATUS_INVALID_PARAMETER;

    ticks -= epoch_difference * 10000000ULL;
    *result = (ULONG)(ticks / 10000000ULL);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtQueryInformationFile(HANDLE handle, IO_STATUS_BLOCK *io, void *buffer,
                                      ULONG length, FILE_INFORMATION_CLASS information_class)
{
    switch (information_class)
    {
    case FileBasicInformation:
        if (length < sizeof(FILE_BASIC_INFORMATION))
            return STATUS_BUFFER_TOO_SMALL;
        if (!handle || !io || !buffer)
            return STATUS_INVALID_PARAMETER;
        {
            FILE_BASIC_INFORMATION *info = buffer;
            FILETIME creation, access, write;
            FILE_BASIC_INFORMATION tmp = {0};
            BY_HANDLE_FILE_INFORMATION bhfi;
            if (!GetFileInformationByHandle(handle, &bhfi))
                return STATUS_NOT_IMPLEMENTED;
            creation.dwLowDateTime = bhfi.ftCreationTime.dwLowDateTime;
            creation.dwHighDateTime = bhfi.ftCreationTime.dwHighDateTime;
            access.dwLowDateTime   = bhfi.ftLastAccessTime.dwLowDateTime;
            access.dwHighDateTime  = bhfi.ftLastAccessTime.dwHighDateTime;
            write.dwLowDateTime    = bhfi.ftLastWriteTime.dwLowDateTime;
            write.dwHighDateTime   = bhfi.ftLastWriteTime.dwHighDateTime;
            tmp.CreationTime  = filetime_to_large(&creation);
            tmp.LastAccessTime= filetime_to_large(&access);
            tmp.LastWriteTime = filetime_to_large(&write);
            tmp.ChangeTime    = tmp.LastWriteTime;
            tmp.FileAttributes = bhfi.dwFileAttributes;
            *info = tmp;
            io->u.Status = STATUS_SUCCESS;
            io->Information = sizeof(FILE_BASIC_INFORMATION);
        }
        return STATUS_SUCCESS;
    case FileStandardInformation:
        if (length < sizeof(FILE_STANDARD_INFORMATION))
            return STATUS_BUFFER_TOO_SMALL;
        if (!handle || !io || !buffer)
            return STATUS_INVALID_PARAMETER;
        {
            FILE_STANDARD_INFORMATION *info = buffer;
            BY_HANDLE_FILE_INFORMATION bhfi;
            if (!GetFileInformationByHandle(handle, &bhfi))
                return STATUS_NOT_IMPLEMENTED;
            info->AllocationSize.LowPart = bhfi.nFileSizeLow;
            info->AllocationSize.HighPart = (LONG)bhfi.nFileSizeHigh;
            info->EndOfFile.LowPart = bhfi.nFileSizeLow;
            info->EndOfFile.HighPart = (LONG)bhfi.nFileSizeHigh;
            info->NumberOfLinks = bhfi.nNumberOfLinks;
            info->DeletePending = FALSE;
            info->Directory = (bhfi.dwFileAttributes & 0x10) != 0;
            io->u.Status = STATUS_SUCCESS;
            io->Information = sizeof(FILE_STANDARD_INFORMATION);
        }
        return STATUS_SUCCESS;
    default:
        (void)handle;
        (void)buffer;
        (void)length;
        if (io)
        {
            io->u.Status = STATUS_NOT_IMPLEMENTED;
            io->Information = 0;
        }
        return STATUS_NOT_IMPLEMENTED;
    }
}

NTSTATUS NTAPI LdrAddRefDll(ULONG flags, LPVOID module)
{
    (void)flags;
    (void)module;
    return STATUS_SUCCESS;
}

void CDECL throw_bad_alloc(void)
{
    /* No exception support; pretend nothing happened. */
}

#ifdef _alloca
#undef _alloca
#endif

__asm__(".globl _alloca\n"
        "_alloca:\n"
        "    jmp __alloca\n");
