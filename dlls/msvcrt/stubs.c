/*
 * Minimal stubs for features not implemented by wibo.
 * Implements DllMain and small NTDLL helpers.
 */

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <winternl.h>

static inline LARGE_INTEGER filetime_to_large(const FILETIME *ft)
{
    LARGE_INTEGER li;
    li.LowPart = ft->dwLowDateTime;
    li.HighPart = (LONG)ft->dwHighDateTime;
    return li;
}

static inline NTSTATUS status_from_win32_error(DWORD error)
{
    switch (error)
    {
    case ERROR_SUCCESS:
        return STATUS_SUCCESS;
    case ERROR_INVALID_HANDLE:
        return STATUS_INVALID_HANDLE;
    case ERROR_ACCESS_DENIED:
        return STATUS_ACCESS_DENIED;
    case ERROR_FILE_NOT_FOUND:
        return STATUS_OBJECT_NAME_NOT_FOUND;
    case ERROR_PATH_NOT_FOUND:
        return STATUS_OBJECT_PATH_NOT_FOUND;
    case ERROR_NOT_ENOUGH_MEMORY:
        return STATUS_INSUFFICIENT_RESOURCES;
    case ERROR_INVALID_PARAMETER:
        return STATUS_INVALID_PARAMETER;
    case ERROR_MORE_DATA:
    case ERROR_BUFFER_OVERFLOW:
        return STATUS_BUFFER_OVERFLOW;
    case ERROR_NOT_SUPPORTED:
        return STATUS_NOT_SUPPORTED;
    case ERROR_SHARING_VIOLATION:
        return STATUS_SHARING_VIOLATION;
    case ERROR_PRIVILEGE_NOT_HELD:
        return STATUS_PRIVILEGE_NOT_HELD;
    case ERROR_INVALID_FUNCTION:
        return STATUS_INVALID_DEVICE_REQUEST;
    case ERROR_CALL_NOT_IMPLEMENTED:
        return STATUS_NOT_IMPLEMENTED;
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

static inline BOOL should_fallback_to_legacy_file_info(DWORD error)
{
    return error == ERROR_CALL_NOT_IMPLEMENTED || error == ERROR_INVALID_FUNCTION || error == ERROR_NOT_SUPPORTED;
}

static NTSTATUS query_basic_information(HANDLE handle, FILE_BASIC_INFORMATION *info)
{
    FILE_BASIC_INFO basic;

    if (GetFileInformationByHandleEx(handle, FileBasicInfo, &basic, sizeof(basic)))
    {
        info->CreationTime = basic.CreationTime;
        info->LastAccessTime = basic.LastAccessTime;
        info->LastWriteTime = basic.LastWriteTime;
        info->ChangeTime = basic.ChangeTime;
        info->FileAttributes = basic.FileAttributes;
        return STATUS_SUCCESS;
    }

    DWORD error = GetLastError();
    if (!should_fallback_to_legacy_file_info(error))
        return status_from_win32_error(error);

    {
        BY_HANDLE_FILE_INFORMATION bhfi;
        if (!GetFileInformationByHandle(handle, &bhfi))
            return status_from_win32_error(GetLastError());

        info->CreationTime = filetime_to_large(&bhfi.ftCreationTime);
        info->LastAccessTime = filetime_to_large(&bhfi.ftLastAccessTime);
        info->LastWriteTime = filetime_to_large(&bhfi.ftLastWriteTime);
        info->ChangeTime = info->LastWriteTime;
        info->FileAttributes = bhfi.dwFileAttributes;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS query_standard_information(HANDLE handle, FILE_STANDARD_INFORMATION *info)
{
    FILE_STANDARD_INFO standard_info;

    if (GetFileInformationByHandleEx(handle, FileStandardInfo, &standard_info, sizeof(standard_info)))
    {
        info->AllocationSize = standard_info.AllocationSize;
        info->EndOfFile = standard_info.EndOfFile;
        info->NumberOfLinks = standard_info.NumberOfLinks;
        info->DeletePending = standard_info.DeletePending;
        info->Directory = standard_info.Directory;
        return STATUS_SUCCESS;
    }

    DWORD error = GetLastError();
    if (!should_fallback_to_legacy_file_info(error))
        return status_from_win32_error(error);

    {
        BY_HANDLE_FILE_INFORMATION bhfi;
        if (!GetFileInformationByHandle(handle, &bhfi))
            return status_from_win32_error(GetLastError());

        info->AllocationSize.LowPart = bhfi.nFileSizeLow;
        info->AllocationSize.HighPart = (LONG)bhfi.nFileSizeHigh;
        info->EndOfFile = info->AllocationSize;
        info->NumberOfLinks = bhfi.nNumberOfLinks;
        info->DeletePending = FALSE;
        info->Directory = (bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS query_attribute_tag_information(HANDLE handle, FILE_ATTRIBUTE_TAG_INFORMATION *info)
{
    FILE_ATTRIBUTE_TAG_INFO tag_info;

    if (GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &tag_info, sizeof(tag_info)))
    {
        info->FileAttributes = tag_info.FileAttributes;
        info->ReparseTag = tag_info.ReparseTag;
        return STATUS_SUCCESS;
    }

    DWORD error = GetLastError();
    if (!should_fallback_to_legacy_file_info(error))
        return status_from_win32_error(error);

    {
        BY_HANDLE_FILE_INFORMATION bhfi;
        if (!GetFileInformationByHandle(handle, &bhfi))
            return status_from_win32_error(GetLastError());

        info->FileAttributes = bhfi.dwFileAttributes;
        info->ReparseTag = 0;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS query_position_information(HANDLE handle, FILE_POSITION_INFORMATION *info)
{
    LARGE_INTEGER current;
    LARGE_INTEGER zero;

    zero.QuadPart = 0;

    if (!SetFilePointerEx(handle, zero, &current, FILE_CURRENT))
        return status_from_win32_error(GetLastError());

    info->CurrentByteOffset = current;
    return STATUS_SUCCESS;
}

static NTSTATUS query_name_information(HANDLE handle, FILE_NAME_INFORMATION *info, ULONG length, ULONG *written)
{
    FILE_NAME_INFO *win_info = (FILE_NAME_INFO *)info;

    *written = 0;

    if (!GetFileInformationByHandleEx(handle, FileNameInfo, win_info, length))
    {
        DWORD error = GetLastError();
        NTSTATUS status = status_from_win32_error(error);

        if (status == STATUS_BUFFER_OVERFLOW && length >= sizeof(ULONG))
            *written = sizeof(ULONG) + win_info->FileNameLength;
        return status;
    }

    if (length >= sizeof(ULONG))
        *written = sizeof(ULONG) + win_info->FileNameLength;
    else
        *written = 0;

    return STATUS_SUCCESS;
}

void NTAPI RtlInitializeBitMap(PRTL_BITMAP bitmap, ULONG *buffer, ULONG size)
{
    if (!bitmap)
        return;

    bitmap->SizeOfBitMap = size;
    bitmap->Buffer = buffer;
}

void NTAPI RtlSetBits(PRTL_BITMAP bitmap, ULONG start, ULONG count)
{
    ULONG i;

    if (!bitmap || !bitmap->Buffer || !count)
        return;

    if (start >= bitmap->SizeOfBitMap || count > bitmap->SizeOfBitMap - start)
        return;

    for (i = 0; i < count; ++i)
    {
        ULONG bit = start + i;
        bitmap->Buffer[bit / (sizeof(*bitmap->Buffer) * 8)] |= 1u << (bit % (sizeof(*bitmap->Buffer) * 8));
    }
}

BOOLEAN NTAPI RtlAreBitsSet(const RTL_BITMAP *bitmap, ULONG start, ULONG count)
{
    ULONG i;

    if (!bitmap || !bitmap->Buffer || !count)
        return FALSE;

    if (start >= bitmap->SizeOfBitMap || count > bitmap->SizeOfBitMap - start)
        return FALSE;

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

    if (!bitmap || !bitmap->Buffer || !count)
        return FALSE;

    if (start >= bitmap->SizeOfBitMap || count > bitmap->SizeOfBitMap - start)
        return FALSE;

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
    if (!time)
        return STATUS_INVALID_PARAMETER;

    GetSystemTimeAsFileTime((FILETIME *)time);
    return STATUS_SUCCESS;
}

NTSYSAPI BOOLEAN WINAPI RtlTimeToSecondsSince1970(const LARGE_INTEGER *time, LPDWORD result)
{
    const unsigned long long epoch_difference = 11644473600ULL; /* seconds */
    const unsigned long long ticks_per_second = 10000000ULL;
    const unsigned long long epoch_ticks = epoch_difference * ticks_per_second;
    unsigned long long ticks;

    if (!time || !result)
        return FALSE;

    ticks = ((unsigned long long)(unsigned long)time->LowPart) |
            ((unsigned long long)(unsigned long)time->HighPart << 32);

    if (ticks < epoch_ticks)
        return FALSE;

    ticks -= epoch_ticks;
    if (ticks / ticks_per_second > 0xFFFFFFFFULL)
        return FALSE;

    *result = (ULONG)(ticks / ticks_per_second);
    return TRUE;
}

NTSTATUS NTAPI NtQueryInformationFile(HANDLE handle, IO_STATUS_BLOCK *io, void *buffer,
                                      ULONG length, FILE_INFORMATION_CLASS information_class)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (!io)
        return STATUS_INVALID_PARAMETER;

    io->Status = STATUS_INVALID_PARAMETER;
    io->Information = 0;

    if (!handle || !buffer)
        return STATUS_INVALID_PARAMETER;

    switch (information_class)
    {
    case FileBasicInformation:
        if (length < sizeof(FILE_BASIC_INFORMATION))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        status = query_basic_information(handle, (FILE_BASIC_INFORMATION *)buffer);
        if (status == STATUS_SUCCESS)
            io->Information = sizeof(FILE_BASIC_INFORMATION);
        break;
    case FileStandardInformation:
        if (length < sizeof(FILE_STANDARD_INFORMATION))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        status = query_standard_information(handle, (FILE_STANDARD_INFORMATION *)buffer);
        if (status == STATUS_SUCCESS)
            io->Information = sizeof(FILE_STANDARD_INFORMATION);
        break;
    case FilePositionInformation:
        if (length < sizeof(FILE_POSITION_INFORMATION))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        status = query_position_information(handle, (FILE_POSITION_INFORMATION *)buffer);
        if (status == STATUS_SUCCESS)
            io->Information = sizeof(FILE_POSITION_INFORMATION);
        break;
    case FileNameInformation:
        if (length < sizeof(ULONG))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        status = query_name_information(handle, (FILE_NAME_INFORMATION *)buffer, length, &io->Information);
        break;
    case FileAttributeTagInformation:
        if (length < sizeof(FILE_ATTRIBUTE_TAG_INFORMATION))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        status = query_attribute_tag_information(handle, (FILE_ATTRIBUTE_TAG_INFORMATION *)buffer);
        if (status == STATUS_SUCCESS)
            io->Information = sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
        break;
    default:
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }

    io->Status = status;
    return status;
}

NTSYSAPI NTSTATUS WINAPI LdrAddRefDll(ULONG flags, HMODULE module)
{
    (void)flags;
    (void)module;
    return STATUS_SUCCESS;
}
