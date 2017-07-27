typedef struct PROC_THREAD_NATIVE_ATTRIBUTE{
    DWORD    NativeAttributeId;
    DWORD    DataSize;
    PVOID    DataPointer;
    DWORD    OutLengthPointer;
}*PPROC_THREAD_NATIVE_ATTRIBUTE;

typedef struct PROC_THREAD_NATIVE_ATTRIBUTE_LIST{
    DWORD    Length;
    PROC_THREAD_NATIVE_ATTRIBUTE Items[8]; 
}*PPROC_THREAD_NATIVE_ATTRIBUTE_LIST;

typedef struct _RTL_DRIVE_LETTER_CURDIR {
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;


typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG MaximumLength;
    ULONG Length;
    ULONG Flags;
    ULONG DebugFlags;
    PVOID ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StdInputHandle;
    HANDLE StdOutputHandle;
    HANDLE StdErrorHandle;
    UNICODE_STRING CurrentDirectoryPath;
    HANDLE CurrentDirectoryHandle;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PVOID Environment;
    ULONG StartingPositionLeft;
    ULONG StartingPositionTop;
    ULONG Width;
    ULONG Height;
    ULONG CharWidth;
    ULONG CharHeight;
    ULONG ConsoleTextAttributes;
    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopName;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

NTSTATUS __stdcall NtCreateUserProcess(
    PHANDLE hProcess, 
    PHANDLE hThread, 
    ACCESS_MASK ProcessDesiredAccess,
    ACCESS_MASK ThreadDesiredAccess,
    POBJECT_ATTRIBUTES lpProcessObjectAttributes, 
    POBJECT_ATTRIBUTES lpThreadObjectAttributes, 
    ULONG dwCreateProcessFlags, 
    ULONG dwCreateThreadFlags,
    PRTL_USER_PROCESS_PARAMETERS lpProcessParameters, 
    PVOID lpArg10, 
    PPROC_THREAD_NATIVE_ATTRIBUTE_LIST lpAttributeList);
