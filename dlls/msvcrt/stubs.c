/*
 * Minimal stubs for features not implemented by wibo.
 * Implements DllMain and small NTDLL helpers.
 */

#include <windows.h>
#include <winternl.h>

#define STATUS_SUCCESS                   ((NTSTATUS) 0x00000000)
#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS) 0xC0000002)
#define STATUS_INVALID_PARAMETER         ((NTSTATUS) 0xC000000D)
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS) 0xC0000023)

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

NTSYSAPI BOOLEAN WINAPI RtlTimeToSecondsSince1970(const LARGE_INTEGER *time, LPDWORD result)
{
    const unsigned long long epoch_difference = 11644473600ULL; /* seconds */
    unsigned long long ticks = ((unsigned long long)(unsigned long)time->LowPart) |
                               ((unsigned long long)(unsigned long)time->HighPart << 32);

    if (!result || ticks < epoch_difference * 10000000ULL)
        return FALSE;

    ticks -= epoch_difference * 10000000ULL;
    *result = (ULONG)(ticks / 10000000ULL);
    return TRUE;
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
            io->Status = STATUS_SUCCESS;
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
            io->Status = STATUS_SUCCESS;
            io->Information = sizeof(FILE_STANDARD_INFORMATION);
        }
        return STATUS_SUCCESS;
    default:
        (void)handle;
        (void)buffer;
        (void)length;
        if (io)
        {
            io->Status = STATUS_NOT_IMPLEMENTED;
            io->Information = 0;
        }
        return STATUS_NOT_IMPLEMENTED;
    }
}

NTSYSAPI NTSTATUS WINAPI LdrAddRefDll(ULONG flags, HMODULE module)
{
    (void)flags;
    (void)module;
    return STATUS_SUCCESS;
}

/* Stubs for missing functions */
int __cdecl _stdio_common_vfprintf(void)
{
    return 0;
}

intptr_t __cdecl _wfindfirst32i64(const wchar_t *filespec, void *fileinfo)
{
    (void)filespec;
    (void)fileinfo;
    return -1;
}

int __cdecl _wfindnext32i64(intptr_t handle, void *fileinfo)
{
    (void)handle;
    (void)fileinfo;
    return -1;
}
