/*
*************************************************************
*                                                           *
*   Flowhooks Software - Open Source License                *
*                                                           *
*  This software is licensed under the GNU Affero General   *
*  Public License v3. You may use, modify, and distribute   *
*  this code under the terms of the AGPLv3.                 *
*                                                           *
*  This program is distributed in the hope that it will be  *
*  useful, but WITHOUT ANY WARRANTY; without even the       *
*  implied warranty of MERCHANTABILITY or FITNESS FOR A     *
*  PARTICULAR PURPOSE. See the GNU AGPLv3 for more details. *
*                                                           *
*  Author: Felipe Cezar Paz (git@felipecezar.com)           *
*  File: dllmain.cpp                                        *
*  Description: Hook for NtWriteVirtualMemory               *
*                                                           *
*************************************************************
*/

#include <windows.h>
#include <shlobj_core.h>
#include <iostream>

typedef NTSTATUS(NTAPI* NtWriteVirtualMemoryFunc)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToWrite, PULONG NumberOfBytesWritten);
NtWriteVirtualMemoryFunc OriginalNtWriteVirtualMemory;

DWORD g_FileCounter = 0;

void WriteBytesToFile(LPBYTE buffer, DWORD bufferSize) {
    TCHAR desktopFolderPath[MAX_PATH];
    if (SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopFolderPath) != S_OK)
        return;

    TCHAR outputFileName[MAX_PATH];
    swprintf_s(outputFileName, MAX_PATH, TEXT("%s\\output_%d.bin"), desktopFolderPath, g_FileCounter);

    HANDLE fileHandle = CreateFile(outputFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return;

    DWORD bytesWritten = 0;
    BOOL writeResult = WriteFile(fileHandle, buffer, bufferSize, &bytesWritten, NULL);
    if (!writeResult) {
        CloseHandle(fileHandle);
        return;
    }

    CloseHandle(fileHandle);
    g_FileCounter++;
}

bool HookFunction(BYTE* source, BYTE* destination, uintptr_t length) {
    if (length < 5) return false;

    DWORD oldProtect;
    VirtualProtect(source, length, PAGE_EXECUTE_READWRITE, &oldProtect);

    uintptr_t relativeAddress = destination - source - 5;

    *source = 0xE9; // JMP instruction
    *(uintptr_t*)(source + 1) = relativeAddress;

    VirtualProtect(source, length, oldProtect, &oldProtect);
    return true;
}

BYTE* CreateTrampolineHook(BYTE* source, BYTE* destination, uintptr_t length) {
    if (length < 5) return nullptr;

    BYTE* gateway = (BYTE*)VirtualAlloc(NULL, length, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!gateway) return nullptr;

    memcpy(gateway, source, length);

    uintptr_t gatewayRelativeAddress = source - gateway - 5;
    *(gateway + length) = 0xE9; // JMP instruction
    *(uintptr_t*)((uintptr_t)gateway + length + 1) = gatewayRelativeAddress;

    HookFunction(source, destination, length);
    return gateway;
}

void RemoveHook(BYTE* source, BYTE* gateway, uintptr_t length) {
    DWORD oldProtect;
    VirtualProtect(source, length, PAGE_EXECUTE_READWRITE, &oldProtect);

    memcpy(source, gateway, length);

    VirtualProtect(source, length, oldProtect, &oldProtect);
    VirtualFree(gateway, 0, MEM_RELEASE); // Free the allocated memory
}

NTSTATUS NTAPI HookedNtWriteVirtualMemory(HANDLE processHandle, PVOID baseAddress, PVOID buffer, ULONG numberOfBytesToWrite, PULONG numberOfBytesWritten) {
    NTSTATUS status = OriginalNtWriteVirtualMemory(processHandle, baseAddress, buffer, numberOfBytesToWrite, numberOfBytesWritten);

    MessageBoxW(NULL, L"Hello from NtWriteVirtualMemory!", L"NtWriteVirtualMemory", MB_OK);
    WriteBytesToFile((LPBYTE)buffer, numberOfBytesToWrite);

    return status;
}

void InitializeHook() {
    NtWriteVirtualMemoryFunc ntWriteVirtualMemory = (NtWriteVirtualMemoryFunc)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWriteVirtualMemory");
    OriginalNtWriteVirtualMemory = (NtWriteVirtualMemoryFunc)CreateTrampolineHook((BYTE*)ntWriteVirtualMemory, (BYTE*)HookedNtWriteVirtualMemory, 5);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        InitializeHook();
    }
    return TRUE;
}
