# NtWriteVirtualMemory-Dumper
A C++ DLL that hooks `NtWriteVirtualMemory` using trampoline technique to capture the `Buffer` parameter and save it to a unique `.bin` file on the desktop every time the function is called. This tool can be used for debugging or forensic purposes to monitor memory write operations and capture data being written to a process's virtual memory space.
