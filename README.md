# NtWriteVirtualMemory Dumper

This C++ DLL hooks `NtWriteVirtualMemory` and dumps the `Buffer` parameter to a unique `.bin` file on the desktop each time the function is called.

## Features

- **Trampoline Hooking**: Hooks the `NtWriteVirtualMemory` function using trampoline technique.
- **Parameter Dumping**: Captures the `Buffer` parameter of `NtWriteVirtualMemory` and writes it to a `.bin` file.
- **Unique File Naming**: Generates a unique filename for each dump to avoid overwriting existing files.

## Usage

### Requirements

- Visual Studio (or any C++ compiler that supports Windows DLL development)

### Building

To build the DLL, follow these steps:

1. Clone the repository: `git clone https://github.com/felipeczpaz/NtWriteVirtualMemory-Dumper.git`
2. Compile dllmain.cpp: Use any C++ compiler capable of building Windows DLLs.
4. Copy the DLL: Once compiled, copy NtWriteVirtualMemoryDumper.dll to the desired directory.

### Installation

1. Inject the DLL in the target process using any LoadLibrary or ManualMap injector
2. The DLL will hook NtWriteVirtualMemory.

### Output

- Each time NtWriteVirtualMemory is called, the Buffer parameter will be dumped to a unique .bin file on the desktop.

### License

This project is licensed under the MIT License - see the LICENSE file for details.
