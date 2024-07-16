#include <windows.h>
#include <shlobj_core.h>
#include <iostream>

typedef NTSTATUS(NTAPI* _NtWriteVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToWrite, PULONG NumberOfBytesWritten);
_NtWriteVirtualMemory OriginalNtWriteVirtualMemory;

DWORD g_FileID = 0;

void WriteBytesToFile(LPBYTE Buffer, DWORD BufferSize) {
    TCHAR szFolderPath[MAX_PATH];
    if (SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, szFolderPath) != S_OK)
        return;

    TCHAR szFileName[MAX_PATH];
    swprintf_s(szFileName, MAX_PATH, TEXT("%s\\output_%d.bin"), szFolderPath, g_FileID);

    HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    DWORD BytesWritten = 0;
    BOOL Result = WriteFile(hFile, Buffer, BufferSize, &BytesWritten, NULL);
    if (Result == FALSE) {
        CloseHandle(hFile);
        return;
    }

    CloseHandle(hFile);
    g_FileID++;
}

bool Hook(BYTE* src, BYTE* dst, uintptr_t len)
{
    if (len < 5) return false;

    DWORD dwOldProtect;
    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwOldProtect);

    uintptr_t relativeAddress = dst - src - 5;

    *src = 0xE9;
    *(uintptr_t*)(src + 1) = relativeAddress;

    VirtualProtect(src, len, dwOldProtect, &dwOldProtect);
    return true;
}

BYTE* TrampHook(BYTE* src, BYTE* dst, uintptr_t len)
{
    if (len < 5) return NULL;

    BYTE* gateway = (BYTE*)VirtualAlloc(NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy(gateway, src, len);

    uintptr_t gatewayRelativeAddress = src - gateway - 5;

    *(gateway + len) = 0xE9;
    *(uintptr_t*)((uintptr_t)gateway + len + 1) = gatewayRelativeAddress;

    Hook(src, dst, len);
    return gateway;
}

void Unhook(BYTE* src, BYTE* gateway, uintptr_t len)
{
    DWORD dwOldProtect;
    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwOldProtect);

    memcpy(src, gateway, len);

    VirtualProtect(src, len, dwOldProtect, &dwOldProtect);

    VirtualFree(gateway, len, MEM_RELEASE);
}

NTSTATUS NTAPI hkNtWriteVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToWrite, PULONG NumberOfBytesWritten)
{
    NTSTATUS Status = OriginalNtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);

    MessageBoxW(NULL, L"Hello from NtWriteVirtualMemory!", L"NtWriteVirtualMemory", MB_OK);
    WriteBytesToFile((LPBYTE)Buffer, NumberOfBytesToWrite);

    return Status;
}

void StartHook()
{
    _NtWriteVirtualMemory NtWriteVirtualMemory = (_NtWriteVirtualMemory)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWriteVirtualMemory");
    OriginalNtWriteVirtualMemory = (_NtWriteVirtualMemory)TrampHook((BYTE*)NtWriteVirtualMemory, (BYTE*)hkNtWriteVirtualMemory, 5);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        StartHook();
    }
    return TRUE;
}
